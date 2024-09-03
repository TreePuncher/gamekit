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
	std::vector<std::filesystem::path> paths;

	for (size_t i = 1; i < argc; i++)
	{
		std::filesystem::path p{ argv[i] };
		if (std::filesystem::exists(p))
			paths.push_back(p);
	}

	//paths.push_back({ "TestHeaders/Header.hpp" });
	paths.push_back({ "TestHeaders/Clang.hpp" });

	auto results = FlexKit::ParseHeaders(paths);

	if (results.has_value())
	{

	}
	else
		return -1;

	return 0;
}
