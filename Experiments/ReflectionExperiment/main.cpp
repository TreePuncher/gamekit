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
			for (auto& subType : object.subTypes)
			{
				auto processSubType = [&](this auto& self, auto& subType) -> void
					{
						std::visit(FlexKit::overloaded{
								[&](FlexKit::TypeArgument& type)
								{
									std::print("\t\tComponent Subtype: {}\n", type.name);
									for (auto f : type.structInfo->fields)
										std::print("\t\t\tField Name: {}, Type: {}, Options: {}\n", f.name, f.type, f.annotation);

									for (auto& f : type.structInfo->templateArguments)
										self(f);
								},
								[](FlexKit::TemplateType& type)
								{
									std::print("\t\ttemplate Name: {}, ID: {}\n", type.name, type.typeIdx);
								},
								[](FlexKit::IntegerLiteral& type)
								{
									std::print("\t\tInteger Value Name: {}, value: {}\n", type.name, type.value);
								},
								[](FlexKit::FloatLiteral& type)
								{
									std::print("\t\tFloat Value Name: {}, value: {}\n", type.name, type.value);
								},
								[](FlexKit::StringLiteral& type)
								{
									std::print("\t\tString Value Name: {}, value: {}\n", type.name, type.value);
								}
							}, subType);
					};

				processSubType(subType);
			}
		}
	}
	else
		return -1;

	return 0;
}
