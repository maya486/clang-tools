// fixed_rewriter.cpp
#include "clang/AST/ASTContext.h"
#include "clang/AST/ParentMapContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"

#include <algorithm>
#include <string>
#include <vector>

using namespace clang::tooling;
using namespace llvm;
using namespace clang;
using namespace clang::ast_matchers;

DeclarationMatcher FunctionMatcher = functionDecl(isDefinition()).bind("funcDecl");

static bool isIntFloatSizedTwoFieldUnion(const RecordDecl *RD, ASTContext &Ctx,
                                         const FieldDecl *&out_int_field,
                                         const FieldDecl *&out_float_field) {
    llvm::outs() << "[Debug] Checking Union\n";
    if (!RD || !RD->isUnion() || !RD->isCompleteDefinition()) {
        llvm::outs() << "[Debug] Union is not complete\n";
        return false;
    }

    const FieldDecl *int_field = nullptr;
    const FieldDecl *float_field = nullptr;
    uint64_t int_width = 0, float_width = 0;
    int num_fields = 0;
    llvm::outs() << "[Debug] Union a\n";

    for (const FieldDecl *FD : RD->fields()) {
        ++num_fields;
        QualType QT = FD->getType();
        QualType canon = Ctx.getCanonicalType(QT.getUnqualifiedType());
        if (canon->isFloatingType()) {
            float_field = FD;
            float_width = Ctx.getTypeSize(canon);
        } else if (canon->isIntegerType()) {
            int_field = FD;
            int_width = Ctx.getTypeSize(canon);
        } else {
            llvm::outs() << "[Debug] Union has invalid type\n";
            return false;
        }
    }

    llvm::outs() << "[Debug] Union b\n";

    if (num_fields != 2 || int_field == nullptr || float_field == nullptr) {
        llvm::outs() << "[Debug] Union does not have 2 fields, 1 int, 1 float\n";
        return false;
    }

    if (int_width != float_width) {
        llvm::outs() << "[Debug] Union does not have int and float field same width\n";
        return false;
    }

    llvm::outs() << "[Debug] Union c\n";
    out_int_field = int_field;
    out_float_field = float_field;
    return true;
}

struct AccessRec {
    const FieldDecl *field;
    SourceLocation loc;
    bool isWrite;
    const MemberExpr *expr;
};

// For each int float union variable, collects all reads/writes
class MemberAccessVisitor : public RecursiveASTVisitor<MemberAccessVisitor> {
  public:
    explicit MemberAccessVisitor(ASTContext &Ctx) : Ctx(Ctx) {}

    bool VisitMemberExpr(MemberExpr *ME) {
        const FieldDecl *FD = dyn_cast<FieldDecl>(ME->getMemberDecl());
        if (!FD)
            return true;

        llvm::outs() << "[Debug] Visiting MemberExpr: " << FD->getNameAsString() << "\n";

        // find parent RecordDecl (works for anonymous unions)
        const RecordDecl *parentRD = nullptr;
        for (auto &P : Ctx.getParents(*FD)) {
            if (auto RD = P.get<RecordDecl>()) {
                parentRD = RD;
                break;
            }
        }
        if (!parentRD || !parentRD->isUnion())
            return true;

        llvm::outs() << "[Debug] Parent is union: ";
        if (parentRD->getIdentifier())
            llvm::outs() << parentRD->getName() << "\n";
        else
            llvm::outs() << "(anonymous)\n";

        const FieldDecl *int_field = nullptr, *float_field = nullptr;
        if (!isIntFloatSizedTwoFieldUnion(parentRD, Ctx, int_field, float_field)) {
            llvm::outs() << "[Debug] Union failed int/float sized check\n";
            return true;
        }

        llvm::outs() << "[Debug] Union passed int/float sized check\n";

        const Expr *base = ME->getBase()->IgnoreParenImpCasts();
        const VarDecl *ownerVar = nullptr;
        if (const DeclRefExpr *DRE = dyn_cast<DeclRefExpr>(base)) { // handles union.member
            if (const VarDecl *VD = dyn_cast<VarDecl>(DRE->getDecl()))
                ownerVar = VD;
        } else if (const UnaryOperator *UO =
                       dyn_cast<UnaryOperator>(base)) { // handles union_ptr->member
            if (UO->getOpcode() == UO_Deref) {
                const Expr *op = UO->getSubExpr()->IgnoreParenImpCasts();
                if (const DeclRefExpr *DRE2 = dyn_cast<DeclRefExpr>(op))
                    if (const VarDecl *VD = dyn_cast<VarDecl>(DRE2->getDecl()))
                        ownerVar = VD;
            }
        }
        if (!ownerVar)
            return true;

        llvm::outs() << "[Debug] Owner variable: " << ownerVar->getNameAsString() << "\n";

        bool isWrite = false;
        auto parents = Ctx.getParents(*ME);
        if (!parents.empty()) {
            const Stmt *P = parents[0].get<Stmt>();
            if (const BinaryOperator *BO = dyn_cast_or_null<BinaryOperator>(P)) {
                if ((BO->isAssignmentOp() || BO->isCompoundAssignmentOp()) &&
                    BO->getLHS() == ME) // if on LHS is write
                    isWrite = true;
            } else if (const UnaryOperator *UO = dyn_cast_or_null<UnaryOperator>(P)) {
                if (UO->isIncrementDecrementOp()) // if is -- or ++ is also a write
                    isWrite = true;
            }
        }

        llvm::outs() << "[Debug] Access type: " << (isWrite ? "write" : "read") << "\n";

        AccessRec ar = {FD, ME->getExprLoc(), isWrite, ME};
        accesses[ownerVar].push_back(ar);
        return true;
    }

  public:
    llvm::DenseMap<const VarDecl *, std::vector<AccessRec>> accesses;

  private:
    ASTContext &Ctx;
};

template <typename T> const T *findEnclosingStmt(const Decl *D, ASTContext &Ctx) {
    for (DynTypedNode parentNode : Ctx.getParents(*D)) {
        if (const Stmt *stmtParent = parentNode.get<Stmt>()) {
            const T *result = nullptr;
            const Stmt *current = stmtParent;
            while (current) {
                if ((result = dyn_cast<T>(current)))
                    return result;

                auto grandparents = Ctx.getParents(*current);
                if (grandparents.empty())
                    break;
                current = grandparents[0].get<Stmt>();
            }
        }
    }
    return nullptr;
}
template <typename T> const T *findEnclosingStmt(const Expr *E, ASTContext &Ctx) {
    const Stmt *stmtParent = nullptr;
    for (DynTypedNode parentNode : Ctx.getParents(*E)) {
        stmtParent = parentNode.get<Stmt>();
        while (stmtParent) {
            if (const T *result = dyn_cast<T>(stmtParent))
                return result;

            // climb up
            auto grandparents = Ctx.getParents(*stmtParent);
            if (grandparents.empty())
                break;
            stmtParent = grandparents[0].get<Stmt>();
        }
    }
    return nullptr;
}

std::string typeToStr(QualType QT, ASTContext &Ctx) {
    uint64_t size = Ctx.getTypeSize(QT); // in bits
    if (QT->isFloatingType()) {
        return (size == 32) ? "f32" : "f64";
    } else if (QT->isIntegerType()) {
        return (QT->isUnsignedIntegerType() ? "u" : "i") + std::to_string(size);
    }
    return "";
}

std::string generateFuncName(QualType srcType, QualType dstType, ASTContext &Ctx) {
    std::string srcStr = typeToStr(srcType, Ctx);
    std::string dstStr = typeToStr(dstType, Ctx);
    if (srcStr.empty() || dstStr.empty())
        return "";
    return "tenjin_" + srcStr + "_to_" + dstStr;
}

std::string generateUnionConversionFunction(QualType srcType, QualType dstType, ASTContext &Ctx) {
    std::string funcName = generateFuncName(srcType, dstType, Ctx);
    if (funcName.empty())
        return "";

    uint64_t srcSize = Ctx.getTypeSize(srcType);
    uint64_t dstSize = Ctx.getTypeSize(dstType);
    if (srcSize != dstSize)
        return "";

    std::string srcC, dstC;
    if (srcType->isFloatingType())
        srcC = (srcSize == 32) ? "float" : "double";
    else
        srcC = (srcType->isUnsignedIntegerType() ? "uint" : "int") + std::to_string(srcSize) + "_t";

    if (dstType->isFloatingType())
        dstC = (dstSize == 32) ? "float" : "double";
    else
        dstC = (dstType->isUnsignedIntegerType() ? "uint" : "int") + std::to_string(dstSize) + "_t";

    std::string code;
    code += dstC + " " + funcName + "(" + srcC + " x) {\n";
    code += "    " + dstC + " y;\n";
    code += "    memcpy(&y, &x, sizeof y);\n";
    code += "    return y;\n";
    code += "}\n";

    return code;
}

class FunctionAccessAnalyzer : public MatchFinder::MatchCallback {
  public:
    explicit FunctionAccessAnalyzer(Rewriter &R) : TheRewriter(R) {}

    void run(const MatchFinder::MatchResult &Result) override {
        const FunctionDecl *FD = Result.Nodes.getNodeAs<FunctionDecl>("funcDecl");
        if (!FD || !FD->hasBody())
            return;
        ASTContext &Ctx = *Result.Context;
        Stmt *Body = FD->getBody();

        MemberAccessVisitor V(Ctx);

        llvm::outs() << "[Debug] Traversing Function Body for union accesses\n";
        V.TraverseStmt(Body);
        llvm::outs() << "[Debug] Done traversing Function Body for union accesses\n";

        llvm::outs() << "[Debug] Function: " << FD->getNameAsString() << "\n";
        llvm::outs() << "[Debug] Collected MemberExpr accesses:\n";

        for (auto &pair : V.accesses) {
            const VarDecl *VD = pair.first;
            auto seq = pair.second; // copy because we may sort
            if (seq.empty())
                continue;

            llvm::outs() << "  Variable: " << VD->getNameAsString() << " (" << seq.size()
                         << " accesses)\n";

            for (const auto &a : seq) {
                PresumedLoc ploc = Ctx.getSourceManager().getPresumedLoc(a.loc);
                std::string locStr =
                    ploc.isValid()
                        ? (std::string(ploc.getFilename()) + ":" + std::to_string(ploc.getLine()))
                        : "<unknown>";
                llvm::outs() << "    Field: " << a.field->getNameAsString() << " | "
                             << (a.isWrite ? "WRITE" : "READ") << " | at " << locStr << "\n";
            }

            // Sort accesses by source location (file offset) to obtain program order
            std::sort(seq.begin(), seq.end(), [&Ctx](const AccessRec &A, const AccessRec &B) {
                const SourceManager &SM = Ctx.getSourceManager();
                if (A.loc.isInvalid() || B.loc.isInvalid())
                    return false;
                return SM.getFileOffset(A.loc) < SM.getFileOffset(B.loc);
            });

            // Count per-field reads/writes
            unsigned writes_f = 0, reads_f = 0;
            unsigned writes_i = 0, reads_i = 0;
            const FieldDecl *floatField = nullptr, *intField = nullptr;
            for (const auto &a : seq) {
                QualType ft = a.field->getType();
                if (ft->isFloatingType())
                    floatField = a.field;
                else if (ft->isIntegerType())
                    intField = a.field;

                if (a.field->getType()->isFloatingType()) {
                    if (a.isWrite)
                        ++writes_f;
                    else
                        ++reads_f;
                } else if (a.field->getType()->isIntegerType()) {
                    if (a.isWrite)
                        ++writes_i;
                    else
                        ++reads_i;
                }
            }

            llvm::outs() << "[Debug] counts: writes_f=" << writes_f << " reads_f=" << reads_f
                         << " writes_i=" << writes_i << " reads_i=" << reads_i << "\n";

            bool tryFloatToInt = (reads_i == 1 && writes_i == 0 && (writes_f + reads_f > 0));
            bool tryIntToFloat = (reads_f == 1 && writes_f == 0 && (writes_i + reads_i > 0));

            if (!tryFloatToInt && !tryIntToFloat) {
                llvm::outs() << "[Debug] Not a supported access pattern; skipping\n";
                continue;
            }

            const FieldDecl *srcField = nullptr;
            const FieldDecl *dstField = nullptr;
            bool srcIsFloat = false;
            if (tryFloatToInt) {
                srcField = floatField;
                dstField = intField;
                srcIsFloat = true;
            } else if (tryIntToFloat) {
                srcField = intField;
                dstField = floatField;
                srcIsFloat = false;
            } else {
                continue;
            }

            // Ensure no writes to the dstField
            bool anyDstWrites = false;
            for (const auto &a : seq)
                if (a.field == dstField && a.isWrite)
                    anyDstWrites = true;
            if (anyDstWrites) {
                llvm::outs() << "[Debug] There are writes to the destination field; skipping\n";
                continue;
            }

            // Ensure all accesses are inside same CompoundStmt
            const CompoundStmt *enclosingCompound = nullptr;
            bool sameCompound = true;
            for (const auto &a : seq) {
                const CompoundStmt *C = findEnclosingStmt<CompoundStmt>(a.expr, Ctx);
                if (!C) {
                    sameCompound = false;
                    break;
                }
                if (!enclosingCompound)
                    enclosingCompound = C;
                else if (enclosingCompound != C) {
                    sameCompound = false;
                    break;
                }
            }
            if (!sameCompound || !enclosingCompound) {
                llvm::outs() << "[Debug] Not all accesses in same CompoundStmt; skipping\n";
                continue;
            }

            // Find the single dst read access and index
            AccessRec dstReadRec;
            bool foundDstRead = false;
            size_t dstIndex = 0;
            for (size_t i = 0; i < seq.size(); ++i) {
                if (seq[i].field == dstField && !seq[i].isWrite) {
                    dstReadRec = seq[i];
                    foundDstRead = true;
                    dstIndex = i;
                    break;
                }
            }
            if (!foundDstRead) {
                llvm::outs() << "[Debug] Couldn't find destination read; skipping\n";
                continue;
            }

            // Ensure there is a source write before dst read
            size_t firstSrcWriteIndex = SIZE_MAX;
            for (size_t i = 0; i < dstIndex; ++i) {
                if (seq[i].field == srcField && seq[i].isWrite) {
                    firstSrcWriteIndex = i;
                    break;
                }
            }
            if (firstSrcWriteIndex == SIZE_MAX) {
                llvm::outs() << "[Debug] No source write before destination read; skipping\n";
                continue;
            }

            // Good — we will:
            // - replace the first source write statement with "T tmp = <rhs>;"
            // - remove union var declaration
            // - replace remaining src memberexprs with tmp name
            // - replace dst read with func(tmp)
            // Gather edits and apply in descending offset order.

            SourceManager &SM = Ctx.getSourceManager();
            LangOptions LO = Ctx.getLangOpts();

            // Type strings
            QualType srcType = srcField->getType();
            QualType dstType = dstField->getType();
            uint64_t srcSize = Ctx.getTypeSize(srcType);
            std::string srcC;
            if (srcType->isFloatingType())
                srcC = (srcSize == 32) ? "float" : "double";
            else
                srcC = (srcType->isUnsignedIntegerType() ? "uint" : "int") +
                       std::to_string(srcSize) + "_t";

            std::string funcName = generateFuncName(srcType, dstType, Ctx);
            std::string funcCode = generateUnionConversionFunction(srcType, dstType, Ctx);
            if (funcName.empty() || funcCode.empty()) {
                llvm::outs() << "[Debug] Could not generate conversion function; skipping\n";
                continue;
            }

            // Prepare tmp name
            std::string tmpName = "__tenjin_tmp_" + VD->getNameAsString();

            // Collect edits
            struct Edit {
                unsigned offset;
                SourceLocation start;
                SourceLocation end;
                std::string text;
            };
            std::vector<Edit> edits;

            // 1) Remove union declaration (replace with empty)
            const DeclStmt *declStmt = findEnclosingStmt<DeclStmt>(VD, Ctx);
            if (!declStmt) {
                llvm::outs() << "[Debug] Couldn't find enclosing DeclStmt; skipping\n";
                continue;
            }

            // Get the start and end of the statement
            SourceLocation start = declStmt->getBeginLoc();
            SourceLocation end = Lexer::getLocForEndOfToken(declStmt->getEndLoc(), 0, SM, LO);

            if (start.isValid() && end.isValid()) {
                unsigned off = SM.getFileOffset(start);
                edits.push_back(
                    {off, start, end, ""}); // remove entire statement including semicolon
            } else {
                llvm::outs() << "[Debug] Invalid DeclStmt source range; skipping\n";
                continue;
            }
            // SourceRange unionRange = VD->getSourceRange();
            // SourceLocation unionEnd = Lexer::getLocForEndOfToken(unionRange.getEnd(), 0, SM, LO);
            // Token afterUnion;
            // if (!Lexer::getRawToken(unionEnd, afterUnion, SM, LO, true) &&
            // afterUnion.is(tok::semi)) {
            // unionEnd = afterUnion.getLocation(); // extend to semicolon
            //}

            // SourceLocation unionStart = unionRange.getBegin();
            // if (unionStart.isValid() && unionEnd.isValid()) {
            // unsigned off = SM.getFileOffset(unionStart);
            // edits.push_back({off, unionStart, unionEnd, std::string("")});
            //} else {
            // llvm::outs() << "[Debug] Invalid union source range; skipping\n";
            // continue;
            //}

            // 2) Replace the first source write assignment with an initialized tmp decl
            const AccessRec &firstSrcWrite = seq[firstSrcWriteIndex];
            const BinaryOperator *assignStmt =
                findEnclosingStmt<BinaryOperator>(firstSrcWrite.expr, Ctx);

            // Fallback: if ExprStmt not found, just use the BinaryOperator itself
            // if (!assignStmt)
            // assignStmt = findEnclosingStmt<BinaryOperator>(firstSrcWrite.expr, Ctx);

            if (!assignStmt) {
                llvm::outs() << "[Debug] Couldn't find assignment statement for first source "
                                "write; skipping\n";
                continue;
            }

            // Get start and end of the statement
            SourceLocation assignStart = assignStmt->getBeginLoc();
            SourceLocation assignEnd =
                Lexer::getLocForEndOfToken(assignStmt->getEndLoc(), 0, SM, LO);
            Token afterAssign;
            if (!Lexer::getRawToken(assignEnd, afterAssign, SM, LO, true) &&
                afterAssign.is(tok::semi)) {
                assignEnd = afterAssign.getLocation();
            }
            // Double-check validity
            if (!assignStart.isValid() || !assignEnd.isValid()) {
                llvm::outs() << "[Debug] Invalid assign stmt range; skipping\n";
                continue;
            }
            // const BinaryOperator *assignStmt =
            // findEnclosingStmt<BinaryOperator>(firstSrcWrite.expr, Ctx);
            // if (!assignStmt) {
            // llvm::outs() << "[Debug] Couldn't find assignment statement for first source "
            //"write; skipping\n";
            // continue;
            //}
            // SourceLocation assignStart = assignStmt->getSourceRange().getBegin();
            // SourceLocation assignEnd =
            // Lexer::getLocForEndOfToken(assignStmt->getSourceRange().getEnd(), 0, SM, LO);
            // Token afterAssign;
            // if (!Lexer::getRawToken(assignEnd, afterAssign, SM, LO, true) &&
            // afterAssign.is(tok::semi)) {
            // assignEnd = afterAssign.getLocation();
            //}
            // if (!assignStart.isValid() || !assignEnd.isValid()) {
            // llvm::outs() << "[Debug] Invalid assign stmt range; skipping\n";
            // continue;
            //}
            // Extract RHS text
            const Expr *rhs = assignStmt->getRHS()->IgnoreParenImpCasts();
            std::string rhsText =
                Lexer::getSourceText(CharSourceRange::getTokenRange(rhs->getSourceRange()), SM, LO)
                    .str();

            // Build tmp declaration with initializer
            std::string tmpDecl = srcC + " " + tmpName + " = " + rhsText;

            edits.push_back({SM.getFileOffset(assignStart), assignStart, assignEnd, tmpDecl});

            // Keep assign range offsets so we can skip memberexprs inside it
            unsigned assignStartOff = SM.getFileOffset(assignStart);
            unsigned assignEndOff = SM.getFileOffset(assignEnd);

            // 3) Replace member expressions — skip those that are inside the replaced assignStmt
            for (const auto &a : seq) {
                SourceLocation meStart = a.expr->getSourceRange().getBegin();
                SourceLocation meEnd =
                    Lexer::getLocForEndOfToken(a.expr->getSourceRange().getEnd(), 0, SM, LO);
                if (!meStart.isValid() || !meEnd.isValid())
                    continue;
                unsigned meOff = SM.getFileOffset(meStart);

                // If this MemberExpr sits inside the assignment we already replaced, skip it
                if (meOff >= assignStartOff && meOff <= assignEndOff)
                    continue;

                if (a.field == srcField) {
                    // replace in.flt with tmpName
                    edits.push_back({meOff, meStart, meEnd, tmpName});
                } else if (a.field == dstField && !a.isWrite) {
                    // replace in.num with func(tmpName)
                    std::string repl = funcName + "(" + tmpName + ")";
                    edits.push_back({meOff, meStart, meEnd, repl});
                }
            }

            // 4) Insert conversion function above the function (keep this as-is)
            SourceLocation funcInsertLoc = FD->getSourceRange().getBegin();
            TheRewriter.InsertTextBefore(funcInsertLoc, funcCode + "\n");

            // 5) Apply edits in descending file offset order to avoid overlapping edit issues
            std::sort(edits.begin(), edits.end(), [](const Edit &A, const Edit &B) {
                return A.offset > B.offset; // descending
            });

            for (const auto &e : edits) {
                // Replace the text in one shot
                TheRewriter.ReplaceText(CharSourceRange::getCharRange(e.start, e.end), e.text);
            }

            llvm::outs() << "Rewrote union pun for variable '" << VD->getNameAsString()
                         << "' using " << funcName << " with tmp " << tmpName << "\n";
        }
    }

  private:
    Rewriter &TheRewriter;
};

static llvm::cl::OptionCategory MyToolCategory("my-tool options");
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
static cl::extrahelp MoreHelp("\nMore help text...\n");

class RewriteAction : public ASTFrontendAction {
  public:
    RewriteAction() {}

    void EndSourceFileAction() override {
        SourceManager &SM = TheRewriter.getSourceMgr();
        if (auto FE = SM.getFileEntryRefForID(SM.getMainFileID())) {
            llvm::outs() << "=== Rewritten File: " << FE->getName() << " ===\n";
        }
        TheRewriter.getEditBuffer(SM.getMainFileID()).write(llvm::outs());
    }

    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file) override {
        TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
        Finder.addMatcher(FunctionMatcher, &FA);
        return Finder.newASTConsumer();
    }

  private:
    Rewriter TheRewriter;
    FunctionAccessAnalyzer FA{TheRewriter};
    MatchFinder Finder;
};

int main(int argc, const char **argv) {
    auto ExpectedParser = CommonOptionsParser::create(argc, argv, MyToolCategory);
    if (!ExpectedParser) {
        llvm::errs() << ExpectedParser.takeError();
        return 1;
    }
    CommonOptionsParser &OptionsParser = ExpectedParser.get();
    ClangTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());
    return Tool.run(newFrontendActionFactory<RewriteAction>().get());
}
