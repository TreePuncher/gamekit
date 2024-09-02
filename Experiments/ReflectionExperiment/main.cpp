#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/IRBuilder.h>
// Declares clang::SyntaxOnlyAction.
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
// Declares llvm::cl::extrahelp.
#include "llvm/Support/CommandLine.h"
#include <clang-c/Index.h>
#include <iostream>
#include <string>
#include <cstdint>
#include <filesystem>
#include <expected>
#include <EditorReflection.hpp>

int main(int argc, const char* argv[])
{
	for (size_t i = 1; i < argc; i++)
	{
		auto results = FlexKit::ParseHeader(argv[i]);
		if (results.has_value())
		{

		}
		else
			return -1;
	}

	return 0;
}
