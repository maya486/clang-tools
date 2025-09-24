#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"
using namespace clang::tooling;
using namespace llvm;

using namespace clang;
using namespace clang::ast_matchers;

StatementMatcher LoopMatcher =
  forStmt(hasLoopInit(declStmt(hasSingleDecl(varDecl(
    hasInitializer(integerLiteral(equals(0)))))))).bind("forLoop");

class LoopPrinter : public MatchFinder::MatchCallback {
public :
  virtual void run(const MatchFinder::MatchResult &Result) {
    if (const ForStmt *FS = Result.Nodes.getNodeAs<clang::ForStmt>("forLoop"))
      FS->dump();
  }
};

DeclarationMatcher FunctionMatcher =
  functionDecl(isDefinition()).bind("funcDecl");

class CFGDumper : public MatchFinder::MatchCallback {
public:
  virtual void run(const MatchFinder::MatchResult &Result) {
    if (const FunctionDecl *FD = Result.Nodes.getNodeAs<FunctionDecl>("funcDecl")) {
      if (FD->hasBody()) {
        ASTContext *Context = Result.Context;

        std::unique_ptr<CFG> cfg = CFG::buildCFG(
            FD, FD->getBody(), Context, CFG::BuildOptions());

        if (cfg) {
          cfg->dump(Context->getLangOpts(), true);
        } else {
          llvm::errs() << "Failed to build CFG.\n";
        }
      }
    }
  }
};


// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static llvm::cl::OptionCategory MyToolCategory("my-tool options");

// CommonOptionsParser declares HelpMessage with a description of the common
// command-line options related to the compilation database and input files.
// It's nice to have this help message in all tools.
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

// A help message for this specific tool can be added afterwards.
static cl::extrahelp MoreHelp("\nMore help text...\n");

int main(int argc, const char **argv) {
  auto ExpectedParser = CommonOptionsParser::create(argc, argv, MyToolCategory);
  if (!ExpectedParser) {
    // Fail gracefully for unsupported options.
    llvm::errs() << ExpectedParser.takeError();
    return 1;
  }
  CommonOptionsParser& OptionsParser = ExpectedParser.get();
  ClangTool Tool(OptionsParser.getCompilations(),
                 OptionsParser.getSourcePathList());

  LoopPrinter Printer;
  CFGDumper CFGPrinter;
  MatchFinder Finder;
  //Finder.addMatcher(LoopMatcher, &Printer);
  Finder.addMatcher(FunctionMatcher, &CFGPrinter);

  return Tool.run(newFrontendActionFactory(&Finder).get());
}
//int main(int argc, const char **argv) {
  //auto ExpectedParser = CommonOptionsParser::create(argc, argv, MyToolCategory);
  //if (!ExpectedParser) {
    //// Fail gracefully for unsupported options.
    //llvm::errs() << ExpectedParser.takeError();
    //return 1;
  //}
  //CommonOptionsParser& OptionsParser = ExpectedParser.get();
  //ClangTool Tool(OptionsParser.getCompilations(),
                 //OptionsParser.getSourcePathList());
  //return Tool.run(newFrontendActionFactory<clang::SyntaxOnlyAction>().get());
//}


//using namespace clang;
//using namespace clang::ast_matchers;

//StatementMatcher LoopMatcher =
  //forStmt(hasLoopInit(declStmt(hasSingleDecl(varDecl(
    //hasInitializer(integerLiteral(equals(0)))))))).bind("forLoop");

//class LoopPrinter : public MatchFinder::MatchCallback {
//public :
  //virtual void run(const MatchFinder::MatchResult &Result) {
    //if (const ForStmt *FS = Result.Nodes.getNodeAs<clang::ForStmt>("forLoop"))
      //FS->dump();
  //}
//};
//int main(int argc, const char **argv) {
  //auto ExpectedParser = CommonOptionsParser::create(argc, argv, MyToolCategory);
  //if (!ExpectedParser) {
    //// Fail gracefully for unsupported options.
    //llvm::errs() << ExpectedParser.takeError();
    //return 1;
  //}
  //CommonOptionsParser& OptionsParser = ExpectedParser.get();
  //ClangTool Tool(OptionsParser.getCompilations(),
                 //OptionsParser.getSourcePathList());

  //LoopPrinter Printer;
  //MatchFinder Finder;
  //Finder.addMatcher(LoopMatcher, &Printer);

  //return Tool.run(newFrontendActionFactory(&Finder).get());
//}

//#include "clang/Tooling/CommonOptionsParser.h"
//#include "clang/Tooling/Tooling.h"
//#include "clang/ASTMatchers/ASTMatchers.h"
//#include "clang/ASTMatchers/ASTMatchFinder.h"
//#include "clang/Rewrite/Core/Rewriter.h"
//#include "clang/Frontend/FrontendActions.h"
//#include "llvm/Support/CommandLine.h"

//using namespace clang;
//using namespace clang::tooling;
//using namespace clang::ast_matchers;

//static llvm::cl::OptionCategory MyToolCategory("function-injector options");

//class FunctionPrinter : public MatchFinder::MatchCallback {
//public:
    //FunctionPrinter() {}

    //void run(const MatchFinder::MatchResult &Result) override {
        //if (!Rewrite.getSourceMgr().getMainFileID()) {
            //// Initialize Rewriter once from ASTContext
            //Rewrite.setSourceMgr(Result.Context->getSourceManager(), Result.Context->getLangOpts());
        //}

        //if (const FunctionDecl *FD = Result.Nodes.getNodeAs<FunctionDecl>("func")) {
            //if (FD->hasBody()) {
                //SourceLocation Start = FD->getBody()->getBeginLoc();
                //if (Start.isValid() && Rewrite.getSourceMgr().isWrittenInMainFile(Start)) {
                    //Rewrite.InsertText(Start, "// Injected comment\n", true, true);
                //}
            //}
        //}
    //}

    //Rewriter &getRewriter() { return Rewrite; }

//private:
    //Rewriter Rewrite;
//};


//int main(int argc, const char **argv) {
    //auto ExpectedParser = CommonOptionsParser::create(argc, argv, MyToolCategory);
    //if (!ExpectedParser) {
        //llvm::errs() << ExpectedParser.takeError();
        //return 1;
    //}
    //CommonOptionsParser &OptionsParser = ExpectedParser.get();

    //ClangTool Tool(OptionsParser.getCompilations(), OptionsParser.getSourcePathList());

    //FunctionPrinter Printer;
    //MatchFinder Finder;
    //Finder.addMatcher(functionDecl(isDefinition()).bind("func"), &Printer);

    //int Result = Tool.run(newFrontendActionFactory(&Finder).get());

    //// Output rewritten main file contents to stdout
    //const RewriteBuffer *RewriteBuf = Printer.getRewriter().getRewriteBufferFor(
        //Printer.getRewriter().getSourceMgr().getMainFileID());
    //if (RewriteBuf) {
        //llvm::outs() << std::string(RewriteBuf->begin(), RewriteBuf->end());
    //}

    //return Result;
//}

