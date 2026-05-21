#include <algorithm>
#include <Assets.hpp>
#include <cstdio>
#include <filesystem>
#include <print>
#include <Serialization.hpp>
#include <string_view>
#include <vector>

int main(const int args, const char* argv[])
{
	std::vector<std::string_view> inputFiles;
	std::string_view output;

	for (int i = 1; i < args; i++)
	{
		auto arg = std::string_view{ argv[i] };
		if ("-o" == arg)
		{
			if(i + 1 < args)
				output = std::string_view{ argv[i + 1] };

			i++;
		}
		else
			inputFiles.push_back(arg);
	}

	struct IncludedResource{
		FlexKit::Resource	entry;
		std::string_view	file;
	};

	std::vector<IncludedResource> resources;
	resources.reserve(inputFiles.size());

	for (auto& file : inputFiles)
	{
		auto F = fopen(file.data(), "rb");

		if (!F)
			continue;

		IncludedResource ir{
			.file = file
		};

		std::filesystem::path p{ file };
		size_t actualFileSize = std::filesystem::file_size(p);

		const char* _ptr = (const char*)&ir.entry;
		const auto sizeofe = sizeof(ir.entry);
		int read = fread(&ir.entry, 1, sizeof(ir.entry), F);
		if (read != sizeof(ir.entry))
			std::print("Failed to read asset: {}!\n", file);

		fclose(F);

		if (ir.entry.ResourceSize == actualFileSize)
		{
			resources.push_back(ir);
		}
		else
		{
			std::print("File size in header does not match actual file size!\nFile: {}\n", file);
		}
	}

	std::sort(
		std::begin(resources),
		std::end(resources),
		[](const IncludedResource& lhs, const IncludedResource& rhs)
		{
			return lhs.entry.GUID < rhs.entry.GUID;
		});

	FlexKit::ResourceTable table;

	table.ResourceCount = resources.size();
	table.MagicNumber	= 1234;
	table.Version		= 3;

	FlexKit::Blob blob{ (FlexKit::iAllocator&)FlexKit::SystemAllocator };
	FlexKit::Blob assetBlob{ (FlexKit::iAllocator&)FlexKit::SystemAllocator };

	blob += table;

	size_t currentAssetOffset = sizeof(table) + sizeof(FlexKit::ResourceEntry) * resources.size();
	std::vector<char> tmp;
	for (IncludedResource& ir : resources)
	{
		FlexKit::ResourceEntry entry;
		entry.GUID				= ir.entry.GUID;
		entry.ResouceLOC		= nullptr;
		entry.Type				= ir.entry.Type;
		entry.ResourcePosition	= currentAssetOffset;

		memcpy(entry.ID, ir.entry.ID, 64);
		
		blob += entry;
		currentAssetOffset += ir.entry.ResourceSize;

		tmp.resize(ir.entry.ResourceSize);
		auto F = fopen(ir.file.data(), "rb");
		auto bytesRead = fread(tmp.data(), 1, ir.entry.ResourceSize, F);

		if (bytesRead != ir.entry.ResourceSize)
		{
			std::print("did not read all bytes!\n");
			int x = 0;
			fclose(F);
			return -1;
		}

		fclose(F);
		std::print("Packed: {}\n", ir.entry.ID);

		assetBlob += FlexKit::Blob{ tmp.data(), tmp.size() };
	}

	auto F = fopen(output.data(), "wb");

	if (!F)
	{
		std::print("Failed to open output file!\n");
		return -1;
	}
	FlexKit::WriteBlob(blob, F);
	FlexKit::WriteBlob(assetBlob, F);
	fclose(F);

	return 0;
}
