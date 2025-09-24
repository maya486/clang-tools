#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Rewrite/Core/Rewriter.h"

using namespace clang;
using namespace clang::tooling;
using namespace clang::ast_matchers;

class FunctionLogger : public MatchFinder::MatchCallback {
public:
    FunctionLogger(Rewriter &R) : TheRewriter(R) {}

    void run(const MatchFinder::MatchResult &Result) override {
        const FunctionDecl *Func = Result.Nodes.getNodeAs<FunctionDecl>("func");
        if (!Func || !Func->hasBody()) return;

        const Stmt *Body = Func->getBody();
        SourceLocation Start = Body->getBeginLoc();

        std::string Name = Func->getNameInfo().getName().getAsString();
        std::string Code = "printf(\"[INJECTED] Entering " + Name + "\\n\");\n";

        TheRewriter.InsertText(Start, Code, true, true);
    }

private:
    Rewriter &TheRewriter;
};

class InjectAction : public ASTFrontendAction {
public:
    void EndSourceFileAction() override {
        TheRewriter.getEditBuffer(TheRewriter.getSourceMgr().getMainFileID()).write(llvm::outs());
    }

    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file) override {
        TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());

        Matcher.addMatcher(functionDecl(isDefinition()).bind("func"), &Logger);
        return Matcher.newASTConsumer();
    }

private:
    Rewriter TheRewriter;
    FunctionLogger Logger{TheRewriter};
    MatchFinder Matcher;
};
    
static llvm::cl::OptionCategory MyToolCategory("injector options");
int main(int argc, const char **argv) {
    //CommonOptionsParser OptionsParser(argc, argv, MyToolCategory);
    auto ExpectedParser = CommonOptionsParser::create(argc, argv, MyToolCategory);
    if (!ExpectedParser) {
        llvm::errs() << ExpectedParser.takeError();
        return 1;
    }
    CommonOptionsParser &OptionsParser = ExpectedParser.get();
    ClangTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());
    return Tool.run(newFrontendActionFactory<InjectAction>().get());
}
