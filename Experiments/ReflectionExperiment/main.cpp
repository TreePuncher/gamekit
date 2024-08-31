#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/IRBuilder.h>
// Declares clang::SyntaxOnlyAction.
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
// Declares llvm::cl::extrahelp.
#include "llvm/Support/CommandLine.h"

using llvm::LLVMContext;
using llvm::IRBuilder;
using llvm::Module;
using llvm::FunctionType;
using llvm::Function;
using llvm::BasicBlock;
using llvm::Type;
using llvm::ArrayRef;

using namespace clang::tooling;
using namespace llvm;
using namespace llvm::cl;

inline static const char* TestInput = R"(

struct T {
	int x;
};

)";

inline static cl::OptionCategory MyToolCategory("my-tool options");

int main(int argc, const char* argv[])
{
	LLVMContext context;
	auto ExpectedParser = CommonOptionsParser::create(argc, argv, MyToolCategory);
	if (!ExpectedParser) {
		// Fail gracefully for unsupported options.
		llvm::errs() << ExpectedParser.takeError();
		return 1;
	}

	CommonOptionsParser& OptionsParser = ExpectedParser.get();
	ClangTool Tool(OptionsParser.getCompilations(),
		OptionsParser.getSourcePathList());


	return 0;
}
