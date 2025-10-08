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

static constexpr bool VERBOSE = false;

struct TransformationLog {
    bool foundUnion = false;
    bool replacedUnion = false;
};

static TransformationLog gLog;

DeclarationMatcher FunctionMatcher = functionDecl(isDefinition()).bind("funcDecl");

static bool isIntFloatSizedTwoFieldUnion(const RecordDecl *RD, ASTContext &Ctx,
                                         const FieldDecl *&out_int_field,
                                         const FieldDecl *&out_float_field) {
    if (VERBOSE)
        llvm::outs() << "[Debug] Checking Union\n";
    if (!RD || !RD->isUnion() || !RD->isCompleteDefinition()) {
        if (VERBOSE)
            llvm::outs() << "[Debug] Union is not complete\n";
        return false;
    }

    const FieldDecl *int_field = nullptr;
    const FieldDecl *float_field = nullptr;
    uint64_t int_width = 0, float_width = 0;
    int num_fields = 0;
    if (VERBOSE)
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
            if (VERBOSE)
                llvm::outs() << "[Debug] Union has invalid type\n";
            return false;
        }
    }

    if (VERBOSE)
        llvm::outs() << "[Debug] Union b\n";

    if (num_fields != 2 || int_field == nullptr || float_field == nullptr) {
        if (VERBOSE)
            llvm::outs() << "[Debug] Union does not have 2 fields, 1 int, 1 float\n";
        return false;
    }

    if (int_width != float_width) {
        if (VERBOSE)
            llvm::outs() << "[Debug] Union does not have int and float field same width\n";
        return false;
    }

    if (VERBOSE)
        llvm::outs() << "[Debug] Union c\n";
    out_int_field = int_field;
    out_float_field = float_field;
    gLog.foundUnion = true;
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

        if (VERBOSE)
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

        if (VERBOSE)
            llvm::outs() << "[Debug] Parent is union: ";
        if (parentRD->getIdentifier())
            if (VERBOSE)
                llvm::outs() << parentRD->getName() << "\n";
            else if (VERBOSE)
                llvm::outs() << "(anonymous)\n";

        const FieldDecl *int_field = nullptr, *float_field = nullptr;
        if (!isIntFloatSizedTwoFieldUnion(parentRD, Ctx, int_field, float_field)) {
            if (VERBOSE)
                llvm::outs() << "[Debug] Union failed int/float sized check\n";
            return true;
        }

        if (VERBOSE)
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

        if (VERBOSE)
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

        if (VERBOSE)
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
template <typename T> const T *findEnclosingStmt(const Stmt *S, ASTContext &Ctx) {
    const Stmt *Current = S;

    while (Current) {
        auto Parents = Ctx.getParents(*Current);
        if (Parents.empty())
            break;

        const Stmt *ParentStmt = Parents[0].get<Stmt>();
        if (!ParentStmt)
            break;

        if (const T *Target = dyn_cast<T>(ParentStmt))
            return Target;

        Current = ParentStmt;
    }

    return nullptr;
}
void printParentChain(const Stmt *S, ASTContext &Ctx) {
    const Stmt *Current = S;
    llvm::outs() << "=== Parent Chain ===\n";
    while (Current) {
        Current->dump();
        auto Parents = Ctx.getParents(*Current);
        if (Parents.empty())
            break;
        Current = Parents[0].get<Stmt>();
    }
}
template <typename T> const T *findEnclosingStmt(const Expr *E, ASTContext &Ctx) {
    llvm::SmallVector<clang::DynTypedNode, 8> Worklist;

    // Seed with initial parents
    for (const DynTypedNode &ParentNode : Ctx.getParents(*E)) {
        Worklist.push_back(ParentNode);
    }

    while (!Worklist.empty()) {
        const DynTypedNode Node = Worklist.pop_back_val();

        if (const Stmt *S = Node.get<Stmt>()) {
            if (const T *Target = dyn_cast<T>(S))
                return Target;

            // Push parents of this Stmt
            for (const DynTypedNode &P : Ctx.getParents(*S))
                Worklist.push_back(P);

        } else if (const Decl *D = Node.get<Decl>()) {
            // Decl may be in DeclStmt, which is a Stmt
            for (const DynTypedNode &P : Ctx.getParents(*D))
                Worklist.push_back(P);
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

        if (Ctx.getSourceManager().isInSystemHeader(FD->getLocation())) {
            return; // skip STL, libc++
        }

        MemberAccessVisitor V(Ctx);
        traverseFunctionBody(Body, V);

        for (auto &pair : V.accesses) {
            handleVariableAccess(FD, pair.first, pair.second, Ctx);
        }
    }

  private:
    Rewriter &TheRewriter;

    void traverseFunctionBody(Stmt *Body, MemberAccessVisitor &V) {
        if (VERBOSE)
            llvm::outs() << "[Debug] Traversing Function Body for union accesses\n";
        V.TraverseStmt(Body);
        if (VERBOSE)
            llvm::outs() << "[Debug] Done traversing Function Body for union accesses\n";
    }

    void handleVariableAccess(const FunctionDecl *FD, const VarDecl *VD, std::vector<AccessRec> seq,
                              ASTContext &Ctx) {
        if (seq.empty())
            return;

        printAccesses(VD, seq, Ctx);
        std::sort(seq.begin(), seq.end(), [&Ctx](const AccessRec &A, const AccessRec &B) {
            return compareBySource(A, B, Ctx);
        });

        unsigned writes_f, reads_f, writes_i, reads_i;
        const FieldDecl *floatField = nullptr, *intField = nullptr;
        countFieldAccesses(seq, writes_f, reads_f, writes_i, reads_i, floatField, intField);

        bool tryFloatToInt = (reads_i >= 1 && writes_i == 0 && (writes_f + reads_f > 0));
        bool tryIntToFloat = (reads_f >= 1 && writes_f == 0 && (writes_i + reads_i > 0));
        if (!tryFloatToInt && !tryIntToFloat) {
            if (VERBOSE)
                llvm::outs() << "[Debug] Not a supported access pattern; skipping\n";
            return;
        }

        const FieldDecl *srcField = tryFloatToInt ? floatField : intField;
        const FieldDecl *dstField = tryFloatToInt ? intField : floatField;
        bool srcIsFloat = tryFloatToInt;

        if (!validateAccessSequence(seq, srcField, dstField, Ctx))
            return;

        SourceManager &SM = Ctx.getSourceManager();
        LangOptions LO = Ctx.getLangOpts();

        std::string srcC = getTypeString(srcField->getType(), Ctx);
        std::string funcName = generateFuncName(srcField->getType(), dstField->getType(), Ctx);
        std::string funcCode =
            generateUnionConversionFunction(srcField->getType(), dstField->getType(), Ctx);
        if (funcName.empty() || funcCode.empty()) {
            if (VERBOSE)
                llvm::outs() << "[Debug] Could not generate conversion function; skipping\n";
            return;
        }

        applyEdits(FD, VD, seq, srcField, dstField, srcC, funcName, funcCode, Ctx);
    }

    void printAccesses(const VarDecl *VD, const std::vector<AccessRec> &seq, ASTContext &Ctx) {
        if (VERBOSE)
            llvm::outs() << "  Variable: " << VD->getNameAsString() << " (" << seq.size()
                         << " accesses)\n";
        for (const auto &a : seq) {
            PresumedLoc ploc = Ctx.getSourceManager().getPresumedLoc(a.loc);
            std::string locStr =
                ploc.isValid()
                    ? (std::string(ploc.getFilename()) + ":" + std::to_string(ploc.getLine()))
                    : "<unknown>";
            if (VERBOSE)
                llvm::outs() << "    Field: " << a.field->getNameAsString() << " | "
                             << (a.isWrite ? "WRITE" : "READ") << " | at " << locStr << "\n";
        }
    }

    static bool compareBySource(const AccessRec &A, const AccessRec &B, ASTContext &Ctx) {
        const SourceManager &SM = Ctx.getSourceManager();
        if (A.loc.isInvalid() || B.loc.isInvalid())
            return false;
        return SM.getFileOffset(A.loc) < SM.getFileOffset(B.loc);
    }

    void countFieldAccesses(const std::vector<AccessRec> &seq, unsigned &writes_f,
                            unsigned &reads_f, unsigned &writes_i, unsigned &reads_i,
                            const FieldDecl *&floatField, const FieldDecl *&intField) {
        writes_f = reads_f = writes_i = reads_i = 0;
        for (const auto &a : seq) {
            QualType ft = a.field->getType();
            if (ft->isFloatingType())
                floatField = a.field;
            else if (ft->isIntegerType())
                intField = a.field;

            if (ft->isFloatingType()) {
                if (a.isWrite)
                    ++writes_f;
                else
                    ++reads_f;
            } else if (ft->isIntegerType()) {
                if (a.isWrite)
                    ++writes_i;
                else
                    ++reads_i;
            }
        }

        if (VERBOSE)
            llvm::outs() << "[Debug] counts: writes_f=" << writes_f << " reads_f=" << reads_f
                         << " writes_i=" << writes_i << " reads_i=" << reads_i << "\n";
    }

    void printExprAndCompoundStmt(const Expr *E, const ASTContext &Ctx, const CompoundStmt *C) {
        clang::LangOptions LO;
        LO.CPlusPlus = true;
        clang::PrintingPolicy PP(LO);

        std::string ExprStr, StmtStr;
        llvm::raw_string_ostream ExprOS(ExprStr), StmtOS(StmtStr);

        if (E)
            E->printPretty(ExprOS, nullptr, PP);
        if (C)
            C->printPretty(StmtOS, nullptr, PP);

        if (!C) {
            if (VERBOSE)
                llvm::outs() << "[Debug] Couldn't find an enclosing CompoundStmt for: "
                             << ExprOS.str() << "\n";
        } else {
            if (VERBOSE)
                llvm::outs() << "[Debug] Found enclosing CompoundStmt for: " << ExprOS.str() << "\n"
                             << "[Debug] CompoundStmt: " << StmtOS.str() << "\n";
        }
    }

    bool validateAccessSequence(const std::vector<AccessRec> &seq, const FieldDecl *srcField,
                                const FieldDecl *dstField, ASTContext &Ctx) {
        const CompoundStmt *enclosingCompound = nullptr;
        for (const auto &a : seq) {
            const CompoundStmt *C = findEnclosingStmt<CompoundStmt>(a.expr, Ctx);
            if (VERBOSE)
                printExprAndCompoundStmt(a.expr, Ctx, C);
            if (VERBOSE)
                printParentChain(a.expr, Ctx);
            if (!C || (enclosingCompound && enclosingCompound != C)) {
                if (VERBOSE)
                    llvm::outs() << "[Debug] Not all accesses in same CompoundStmt; skipping\n";
                return false;
            }
            enclosingCompound = C;
        }

        bool anyDstWrites = false;
        for (const auto &a : seq)
            if (a.field == dstField && a.isWrite)
                anyDstWrites = true;
        if (anyDstWrites) {
            if (VERBOSE)
                llvm::outs() << "[Debug] There are writes to the destination field; skipping\n";
            return false;
        }

        bool foundSrcWriteBeforeDstRead = false;
        for (size_t i = 0; i < seq.size(); ++i) {
            if (seq[i].field == dstField && !seq[i].isWrite) {
                for (size_t j = 0; j < i; ++j)
                    if (seq[j].field == srcField && seq[j].isWrite)
                        foundSrcWriteBeforeDstRead = true;
                break;
            }
        }

        if (!foundSrcWriteBeforeDstRead) {
            if (VERBOSE)
                llvm::outs() << "[Debug] No source write before destination read; skipping\n";
            return false;
        }

        return true;
    }

    std::string getTypeString(QualType T, ASTContext &Ctx) {
        uint64_t size = Ctx.getTypeSize(T);
        if (T->isFloatingType())
            return (size == 32) ? "float" : "double";
        else
            return (T->isUnsignedIntegerType() ? "uint" : "int") + std::to_string(size) + "_t";
    }

    void applyEdits(const FunctionDecl *FD, const VarDecl *VD, const std::vector<AccessRec> &seq,
                    const FieldDecl *srcField, const FieldDecl *dstField, const std::string &srcC,
                    const std::string &funcName, const std::string &funcCode, ASTContext &Ctx) {
        SourceManager &SM = Ctx.getSourceManager();
        LangOptions LO = Ctx.getLangOpts();
        std::vector<Edit> edits;

        std::string tmp_in_name = "__tenjin_tmp_in_" + VD->getNameAsString();
        std::string tmp_out_name = "__tenjin_tmp_out_" + VD->getNameAsString();

        // === Remove union declaration ===
        const DeclStmt *declStmt = findEnclosingStmt<DeclStmt>(VD, Ctx);
        if (!declStmt)
            return;

        SourceLocation start = declStmt->getBeginLoc();
        SourceLocation end = Lexer::getLocForEndOfToken(declStmt->getEndLoc(), 0, SM, LO);
        edits.push_back({SM.getFileOffset(start), start, end, ""});

        // === First write: used for temp input assignment ===
        auto firstSrcWriteIt = std::find_if(seq.begin(), seq.end(), [srcField](const AccessRec &a) {
            return a.field == srcField && a.isWrite;
        });
        if (firstSrcWriteIt == seq.end())
            return;
        const AccessRec &firstSrcWrite = *firstSrcWriteIt;

        const BinaryOperator *firstAssign =
            findEnclosingStmt<BinaryOperator>(firstSrcWrite.expr, Ctx);
        if (!firstAssign)
            return;

        SourceLocation assignStart = firstAssign->getBeginLoc();
        SourceLocation assignEnd = Lexer::getLocForEndOfToken(firstAssign->getEndLoc(), 0, SM, LO);
        const Expr *rhs = firstAssign->getRHS()->IgnoreParenImpCasts();
        std::string rhsText =
            Lexer::getSourceText(CharSourceRange::getTokenRange(rhs->getSourceRange()), SM, LO)
                .str();
        edits.push_back({SM.getFileOffset(assignStart), assignStart, assignEnd,
                         srcC + " " + tmp_in_name + " = " + rhsText });

        unsigned assignStartOff = SM.getFileOffset(assignStart);
        unsigned assignEndOff = SM.getFileOffset(assignEnd);

        // === Last write: location to insert the output conversion ===
        auto lastSrcWriteIt =
            std::find_if(seq.rbegin(), seq.rend(), [srcField](const AccessRec &a) {
                return a.field == srcField && a.isWrite;
            });
        if (lastSrcWriteIt == seq.rend())
            return;
        const AccessRec &lastSrcWrite = *lastSrcWriteIt;

        const Stmt *lastWriteStmt = findEnclosingStmt<Stmt>(lastSrcWrite.expr, Ctx);
        if (!lastWriteStmt)
            return;

        // SourceLocation insertAfterWrite =
        // Lexer::getLocForEndOfToken(lastWriteStmt->getEndLoc(), 0, SM, LO);
        // Step 1: Get end of the statement (after semicolon)
        SourceLocation afterStmtLoc =
            Lexer::getLocForEndOfToken(lastWriteStmt->getEndLoc(), 0, SM, LO);

        // Step 2: Get line number of the statement
        unsigned lineNumber = SM.getSpellingLineNumber(afterStmtLoc);
        SourceLocation lineStartLoc =
            SM.translateLineCol(SM.getFileID(afterStmtLoc), lineNumber, 1);

        // Step 3: Extract indentation from beginning of line
        llvm::StringRef lineText =
            Lexer::getSourceText(CharSourceRange::getCharRange(lineStartLoc, afterStmtLoc), SM, LO);

        std::string indentation;
        for (char c : lineText) {
            if (c == ' ' || c == '\t')
                indentation += c;
            else
                break;
        }

        // Step 4: Insert text with indentation
        std::string dstC = getTypeString(dstField->getType(), Ctx);
        std::string finalInsert = "\n" + indentation + dstC + " " + tmp_out_name + " = " +
                                  funcName + "(" + tmp_in_name + ");";
        TheRewriter.InsertTextAfterToken(afterStmtLoc, finalInsert);
        // SourceLocation insertAfterWrite = Lexer::findLocationAfterToken(
        // lastWriteStmt->getEndLoc(), tok::semi, SM, LO,
        // [>SkipTrailingWhitespaceAndNewLine=<]true);

        // edits.push_back({
        // SM.getFileOffset(insertAfterWrite),
        // insertAfterWrite,
        // insertAfterWrite,
        // dstC + " " + tmp_out_name + " = " + funcName + "(" + tmp_in_name + ");\n"
        //});

        // === Replace member expressions ===
        for (const auto &a : seq) {
            SourceLocation meStart = a.expr->getSourceRange().getBegin();
            SourceLocation meEnd =
                Lexer::getLocForEndOfToken(a.expr->getSourceRange().getEnd(), 0, SM, LO);
            if (!meStart.isValid() || !meEnd.isValid())
                continue;

            unsigned meOff = SM.getFileOffset(meStart);

            // Skip member expressions that were inside the removed assignment
            if (meOff >= assignStartOff && meOff <= assignEndOff)
                continue;

            if (a.field == srcField) {
                edits.push_back({meOff, meStart, meEnd, tmp_in_name});
            } else if (a.field == dstField && !a.isWrite) {
                edits.push_back({meOff, meStart, meEnd, tmp_out_name});
            }
        }

        // === Insert conversion function above function definition ===
        TheRewriter.InsertTextBefore(FD->getSourceRange().getBegin(), funcCode + "\n");

        // === Ensure #include <cstring> is present ===
        SourceLocation funcInsertLoc = FD->getSourceRange().getBegin();
        FileID FID = SM.getFileID(funcInsertLoc);
        const FileEntry *FE = SM.getFileEntryForID(FID);
        if (FE) {
            llvm::StringRef buffer = SM.getBufferData(FID);
            if (!buffer.contains("#include <cstring>")) {
                SourceLocation fileStart = SM.getLocForStartOfFile(FID);
                TheRewriter.InsertTextBefore(fileStart, "#include <cstring>\n");
            }
        }

        // === Apply all edits in reverse order ===
        std::sort(edits.begin(), edits.end(),
                  [](const Edit &A, const Edit &B) { return A.offset > B.offset; });
        for (const auto &e : edits)
            TheRewriter.ReplaceText(CharSourceRange::getCharRange(e.start, e.end), e.text);

        if (VERBOSE)
            llvm::outs() << "Rewrote union pun for variable '" << VD->getNameAsString()
                         << "' using " << funcName << " with tmps " << tmp_in_name << " "
                         << tmp_out_name << "\n";

        gLog.replacedUnion = true;
    }

    struct Edit {
        unsigned offset;
        SourceLocation start;
        SourceLocation end;
        std::string text;
    };
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
            if (VERBOSE) {
                llvm::outs() << "=== Rewritten File: " << FE->getName() << " ===\n";
            }

            llvm::errs() << "[SUMMARY] " << FE->getName()
                         << " union_found=" << (gLog.foundUnion ? "yes" : "no")
                         << " union_replaced=" << (gLog.replacedUnion ? "yes" : "no") << "\n";
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
