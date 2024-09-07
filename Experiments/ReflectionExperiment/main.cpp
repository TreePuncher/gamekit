#include <clang-c/Index.h>
#include <string>
#include <print>
#include <cstdint>
#include <filesystem>
#include <expected>
#include <EditorReflection.hpp>
#include <containers.hpp>

int main(int argc, const char* argv[])
{
	std::vector<std::filesystem::path> paths;

	for (size_t i = 1; i < argc; i++)
	{
		std::filesystem::path p{ argv[i] };
		if (std::filesystem::exists(p))
			paths.push_back(p);
	}

	int x = sizeof(FlexKit::Vector<int>);

	paths.push_back("testHeaders/clang.hpp");

	auto results = FlexKit::ParseHeaders(paths);

	if (results.has_value())
	{
		auto&& [types, componentObjects] = results.value();

		std::print("Components found!\n");

		for (auto& object : componentObjects)
		{
			std::print("Component Name: {}\n", object.componentName);
			for(auto& field : object.fields)
				std::print("\t Field Name: {}, Type: {}, Options: {}\n", field.name, field.type, field.annotation);
		}
	}
	else
		return -1;

	return 0;
}
