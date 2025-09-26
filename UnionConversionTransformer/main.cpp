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
        //if (canon->isSpecificBuiltinType(BuiltinType::Float)) {
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
        //const Expr *MEexpr = ME; // your MemberExpr*
        //SourceRange range = MEexpr->getSourceRange();

        //std::string text = Lexer::getSourceText(CharSourceRange::getTokenRange(range),
                                                //Ctx.getSourceManager(), Ctx.getLangOpts())
                               //.str();

        //llvm::outs() << "[Debug] MemberExpr: " << text << "\n";
        const FieldDecl *FD = dyn_cast<FieldDecl>(ME->getMemberDecl());
        if (!FD)
            return true;

        llvm::outs() << "[Debug] Visiting MemberExpr: " << FD->getNameAsString() << "\n";

        // const RecordDecl *parentRD = dyn_cast<RecordDecl>(FD->getParent());
        //  Works for anonymous unions
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
    srcC = (srcType->isFloatingType()) ? ((srcSize == 32) ? "float" : "double")
                                       : (srcType->isUnsignedIntegerType() ? "uint" : "int") +
                                             std::to_string(srcSize) + "_t";
    dstC = (dstType->isFloatingType()) ? ((dstSize == 32) ? "float" : "double")
                                       : (dstType->isUnsignedIntegerType() ? "uint" : "int") +
                                             std::to_string(dstSize) + "_t";

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
            const auto &seq = pair.second;
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

            unsigned writes = 0, reads = 0;
            const MemberExpr *writeExpr = nullptr;
            const MemberExpr *readExpr = nullptr;
            const BinaryOperator *assignStmt = nullptr;
            // const ReturnStmt *retStmt = nullptr;

            for (const auto &a : seq) {
                auto parents = Ctx.getParents(*a.expr);
                if (a.isWrite) {
                    ++writes;
                    writeExpr = a.expr;
                    if (!parents.empty())
                        // assignStmt = dyn_cast_or_null<BinaryOperator>(parents[0].get<Stmt>());
                        assignStmt = findEnclosingStmt<BinaryOperator>(a.expr, Ctx);
                } else {
                    ++reads;
                    readExpr = a.expr;
                    // if (!parents.empty())
                    // retStmt = findEnclosingStmt<ReturnStmt>(a.expr, Ctx);
                    // retStmt = dyn_cast_or_null<ReturnStmt>(parents[0].get<Stmt>());
                }
            }

            // Debug prints
            llvm::outs() << "[Debug] Variable " << VD->getNameAsString() << " has writes=" << writes
                         << ", reads=" << reads << "\n";

            if (assignStmt)
                llvm::outs() << "[Debug] Assignment stmt: "
                             << Lexer::getSourceText(
                                    CharSourceRange::getTokenRange(assignStmt->getSourceRange()),
                                    Ctx.getSourceManager(), Ctx.getLangOpts())
                             << "\n";
            else
                llvm::outs() << "[Debug] Assignment stmt: <null>\n";

            // if (retStmt)
            // llvm::outs() << "[Debug] Return stmt: "
            //<< Lexer::getSourceText(
            // CharSourceRange::getTokenRange(retStmt->getSourceRange()),
            // Ctx.getSourceManager(), Ctx.getLangOpts())
            //<< "\n";
            // else
            // llvm::outs() << "[Debug] Return stmt: <null>\n";

            // bool ok = (writes == 1 && reads == 1 && assignStmt && retStmt);
            bool ok = (writes == 1 && reads == 1 && assignStmt);
            if (!ok)
                continue;

            const FieldDecl *srcField =
                llvm::dyn_cast<FieldDecl>(writeExpr->getMemberDecl());                        // LHS
            const FieldDecl *dstField = llvm::dyn_cast<FieldDecl>(readExpr->getMemberDecl()); // RHS

            QualType srcType = srcField->getType();
            QualType dstType = dstField->getType();

            std::string funcName = generateFuncName(srcType, dstType, Ctx);
            std::string funcCode = generateUnionConversionFunction(srcType, dstType, Ctx);

            if (!funcName.empty() && !funcCode.empty()) {
                // Insert new conversion method just above this method
                SourceLocation funcInsertLoc = FD->getSourceRange().getBegin();
                TheRewriter.InsertTextBefore(funcInsertLoc, funcCode + "\n");

                // Remove union declaration
                // TheRewriter.RemoveText(VD->getSourceRange());
                // SourceLocation unionStart = VD->getBeginLoc();
                // SourceLocation unionEnd = Lexer::getLocForEndOfToken(
                // VD->getEndLoc(), 0, Ctx.getSourceManager(), Ctx.getLangOpts());
                // TheRewriter.RemoveText(CharSourceRange::getCharRange(unionStart, unionEnd));
                SourceRange unionRange = VD->getSourceRange();
                unionRange.setEnd(Lexer::getLocForEndOfToken(
                    unionRange.getEnd(), 0, Ctx.getSourceManager(), Ctx.getLangOpts()));
                TheRewriter.RemoveText(unionRange);

                // Remove write to union
                // TheRewriter.RemoveText(assignStmt->getSourceRange());
                SourceRange assignRange = assignStmt->getSourceRange();
                assignRange.setEnd(Lexer::getLocForEndOfToken(
                    assignRange.getEnd(), 0, Ctx.getSourceManager(), Ctx.getLangOpts()));
                TheRewriter.RemoveText(assignRange);

                // Remove the read out of the union (and replace with new method call)

                // Get RHS text of write assignment: in.flt = flt gets "flt"
                const Expr *rhs = assignStmt->getRHS()->IgnoreParenImpCasts();
                SourceRange rhsRange = rhs->getSourceRange();
                std::string rhsText =
                    Lexer::getSourceText(CharSourceRange::getTokenRange(rhsRange),
                                         Ctx.getSourceManager(), Ctx.getLangOpts())
                        .str();

                // Get the read location to replace:
                SourceLocation readStart = readExpr->getSourceRange().getBegin();
                SourceLocation readEnd =
                    Lexer::getLocForEndOfToken(readExpr->getSourceRange().getEnd(), 0,
                                               Ctx.getSourceManager(), Ctx.getLangOpts());

                // SourceLocation startLoc = VD->getSourceRange().getBegin();
                // SourceLocation endLoc = Lexer::getLocForEndOfToken(
                // retStmt->getEndLoc(), 0, Ctx.getSourceManager(), Ctx.getLangOpts());
                // CharSourceRange fullRange = CharSourceRange::getCharRange(startLoc, endLoc);

                std::string newText = funcName + "(" + rhsText + ")";
                // TheRewriter.ReplaceText(retStmt->getSourceRange(), newReturn);
                TheRewriter.ReplaceText(CharSourceRange::getCharRange(readStart, readEnd), newText);

                llvm::outs() << "Rewrote union pun in function '" << FD->getNameAsString()
                             << "' using " << funcName << "\n";
            }
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
        // Finder.addMatcher(UnionMatcher, &UC);
        Finder.addMatcher(FunctionMatcher, &FA);
        return Finder.newASTConsumer();
    }

  private:
    Rewriter TheRewriter;
    // UnionChecker UC;
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
