#include <Assets.hpp>
#include <string_view>
#include <string>
#include <Serialization.hpp>
#include <print>
#include <filesystem>
#include <fmt/format.h>
#include <expected>

#include <Unknwnbase.h>
#include <combaseapi.h>
#include <directx-dxc/dxcapi.h>

#include <MemoryUtilities.hpp>
#include <shaderpreprocessor.hpp>
#include <RenderSystemInterface.hpp>


uint64_t GenerateRandomID()
{
	srand(time(0));
	return uint64_t(rand()) | uint64_t(rand()) << 32;
}

struct AttributeBlock
{
	FlexKit::Blob	attributes;
	uint32_t		count = 0;

	uint32_t		BlockSize() const { return attributes.size(); }
};

AttributeBlock CreateAttributeBlock(const FlexKit::Vector<FlexKit::ShaderAttribute>& attributes)
{
	AttributeBlock out;

	for (const auto& attrib : attributes)
	{
		std::visit(
			Overloaded{
				[&](const FlexKit::ShaderAttributeConstantValues& cv)
				{
					FlexKit::ShaderAttributeConstantValuesBlock block;
					block.header.blockSize	= sizeof(block) + cv.id.size();
					block.header.type		= FlexKit::AttributeType::ConstantValues;
					block.num				= cv.num;
					block.pipelineStage		= cv.pipelineStage;
					block.binding			= cv.binding;
					block.stringSize		= cv.id.size();

					out.attributes += block;
					out.attributes += FlexKit::Blob{ cv.id.data(), cv.id.size() };
				},
				[&](const FlexKit::ShaderAttributeDescriptorTable& table)
				{
					const size_t attributesByteSize = table.entries.size() * sizeof(FlexKit::ShaderAttributeDescriptorTableEntry);

					FlexKit::ShaderAttributeDescriptorTableBlock block ;
					block.header.blockSize	= sizeof(block) + attributesByteSize;
					block.header.type		= FlexKit::AttributeType::DescriptorTable;
					block.set				= table.set;
					block.count				= table.entries.size();

					out.attributes += block;
					out.attributes +=
						FlexKit::Blob{
							(const char*)table.entries.data(),
							attributesByteSize };
				},
				[&](const FlexKit::ShaderAttributeFlag& flag)
				{
					FlexKit::ShaderAttributeFlagBlock block;
					block.flag				= flag.flag;
					block.header.blockSize	= sizeof(block);
					block.header.type		= FlexKit::AttributeType::RootSignatureFlag;

					out.attributes += block;
				},
				[&](const FlexKit::ShaderAttributeResource& resource)
				{
					FlexKit::ShaderAttributeResourceBlock header;
					header.header.blockSize = sizeof(header) + resource.id.size();
					header.header.type		= FlexKit::AttributeType::Resource;
					header.binding			= resource.binding;
					header.pipelineStage	= resource.pipelineStage;
					header.set				= resource.set;
					header.stringSize		= resource.id.size();

					out.attributes += header;
					out.attributes += FlexKit::Blob{ resource.id.data(), resource.id.size() };
				}
			}, attrib);
	}

	return out;
}

FlexKit::Blob CreateShaderResourceBlob(FlexKit::ShaderResourceBlob::ShaderAPI API, uint64_t assetHandle, std::string_view id, void* byteCode, size_t byteCodeSize, AttributeBlock attributes)
{
	FlexKit::Blob blob{};
	FlexKit::ShaderResourceBlob::Header header;

	std::memset(header.ID, 0, FlexKit::ShaderResourceBlob::ID_LENGTH);

	std::strncpy(
		header.ID, id.data(),
		FlexKit::Min(
			FlexKit::ShaderResourceBlob::ID_LENGTH,
			id.size()));

	header.GUID					= assetHandle;
	header.ResourceSize			= sizeof(header) + byteCodeSize + attributes.BlockSize();
	header.Type					= FlexKit::EResource_Shader;

	header.attributeCount		= attributes.count;
	header.byteCodeByteSize		= byteCodeSize;
	header.byteCodeOffset		= sizeof(header);
	header.api					= API;
	header.attributeOffset		= sizeof(header) + byteCodeSize;

	blob += header;
	blob += FlexKit::Blob{ (const char*)byteCode, byteCodeSize };

	if(attributes.BlockSize())
		blob += attributes.attributes;

	return blob;
}


struct IncludeHandler : public IDxcIncludeHandler
{
	HRESULT STDMETHODCALLTYPE LoadSource(LPCWSTR pFilename, IDxcBlob** ppIncludeSource) override
	{
		char fileStr[256];
		auto fileLength = wcstombs(fileStr, pFilename, 256);

		std::filesystem::path file{ fileStr };
		auto newFilePath = includePath.string() + R"(\)" + file.string();

		wchar_t fileW[256];
		mbstowcs(fileW, newFilePath.c_str(), 256);

		return handler->LoadSource(fileW, ppIncludeSource);
	}

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(
		REFIID				riid,
		void* __RPC_FAR*	ppvObject)
	{
		return E_FAIL;
	}

	std::filesystem::path   includePath;
	IDxcIncludeHandler* handler;

	ULONG AddRef()	{ return 0; }
	ULONG Release() { return 0; }
};


struct ShaderOptions
{
	bool enable16BitTypes	= false;
	bool hlsl2021			= false;
	bool enableDebug		= false;
	bool loadRootSignature	= false;
};

enum class CompileError
{
	Syntax,
	FileNotFound,
	FailedToInitializeCompiler,
	Unknown
};


std::expected<FlexKit::Blob, CompileError> CompileDX(
	const std::string_view	target,	const std::string_view out,
	const std::string_view	entry,	const std::string_view profile,
	const ShaderOptions&	options)
{
	IDxcUtils*			hlslUtils = nullptr;
	IDxcIncludeHandler* hlslIncludeHandler = nullptr;
	IDxcCompiler3*		hlslCompiler = nullptr;

	if (FAILED(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&hlslUtils))))
		return CompileError::FailedToInitializeCompiler;

	if (FAILED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&hlslCompiler))))
		return CompileError::FailedToInitializeCompiler;

	if (hlslUtils) hlslUtils->CreateDefaultIncludeHandler(&hlslIncludeHandler);

	std::filesystem::path filePath{ target };
	auto parentPath = filePath.parent_path();

	wchar_t entryPointW[64];
	wchar_t fileW[256];
	wchar_t filenameW[256];
	wchar_t profileW[64];

	size_t fileWLength = 0;
	if (entry.size())
		mbstowcs(entryPointW, entry.data(), 64);

	mbstowcs(profileW, profile.data(), 64);
	mbstowcs(fileW, target.data(), 256);
	mbstowcs(filenameW, filePath.filename().string().c_str(), 256);

	
	auto size = FlexKit::GetFileSize(filePath.string().c_str());
	std::string shaderStr;
	shaderStr.resize(size);
	FlexKit::LoadFileIntoBuffer(filePath.string().c_str(), (std::byte*)shaderStr.data(), size);

	auto type = [](const char* profile)
		{
			uint16_t code = *(uint16_t*)profile;

			switch (code)
			{
			case 0x7370:
				return FlexKit::SHADER_TYPE::Pixel;
			case 0x7376:
				return FlexKit::SHADER_TYPE::Vertex;
			case 0x7361:
				return FlexKit::SHADER_TYPE::Amplification;
			case 0x736d:
				return FlexKit::SHADER_TYPE::Mesh;
			case 0x7363:
				return FlexKit::SHADER_TYPE::Compute;
			case 0x7364:
				return FlexKit::SHADER_TYPE::Domain;
			case 0x7368:
				return FlexKit::SHADER_TYPE::Hull;
			default:
				return FlexKit::SHADER_TYPE::Unknown;
			}
		}(profile.data());

	auto res = FlexKit::DXShaderProprocessor(shaderStr, type, FlexKit::SystemAllocator);

	IDxcBlobEncoding* blob;
	auto HR1 = hlslUtils->CreateBlobFromPinned(shaderStr.data(), shaderStr.size(), DXC_CP_ACP, &blob);

	if (FAILED(HR1))
	{
		LPSTR string = nullptr;

		const auto msgLen = FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr,
			HR1,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPSTR)&string,
			0,
			nullptr);

		auto converted = fmt::format("Shader failed to load: {}", string);

		FK_LOG_ERROR(converted.c_str());

		LocalFree(string);

		return CompileError::FileNotFound;
	}

	IncludeHandler includeHandler;
	includeHandler.includePath = parentPath;
	includeHandler.handler = hlslIncludeHandler;


	IDxcCompiler2* debugCompiler = nullptr;
	hlslCompiler->QueryInterface<IDxcCompiler2>(&debugCompiler);

	std::vector<LPCWSTR> arguments;

#if USING(DEBUGSHADERS)
	arguments.push_back(L"-Od");
	arguments.push_back(L"/Zi");
	arguments.push_back(L"-Qembed_debug");
	arguments.push_back(L"-T");
	arguments.push_back(profileW);
	arguments.push_back(L"-E");
	arguments.push_back(entryPointW);
#else
	arguments.push_back(L"-O2");
#endif

	if (options.enable16BitTypes)
		arguments.push_back(L"-enable-16bit-types");

	if (options.hlsl2021)
		arguments.push_back(L"-HV 2021");

	IDxcOperationResult* result = nullptr;

	HRESULT HR2;
	try
	{
		DxcBuffer buffer{
			blob->GetBufferPointer(),
			blob->GetBufferSize(),
		};

		int _;

		blob->GetEncoding(&_, &buffer.Encoding);
		IDxcOperationResult* result = nullptr;

		HR2 = hlslCompiler->Compile(
			&buffer,
			arguments.data(),
			(UINT)arguments.size(),
			&includeHandler,
			IID_PPV_ARGS(&result));

		IDxcBlobEncoding* errors;
		result->GetErrorBuffer(&errors);


		if (FAILED(HR2))
		{
			auto errorString = (const char*)errors->GetBufferPointer();
			std::string formattedMessage =
				fmt::format("{}\nFailed to Compile Shader\nEntryPoint: {}\nFile : \n {}\nPress Enter to try again\n",
					errorString, entry.size() > 0 ? entry : "No Entry Point", target);

			FK_LOG_ERROR(formattedMessage.c_str());

			errors->Release();
		}
		else
		{
			IDxcBlob* blob;
			result->GetResult(&blob);

			auto buff		= blob->GetBufferPointer();
			auto buffSize	= blob->GetBufferSize();

			FlexKit::Blob out{ (const char*)buff, buffSize};

			blob->Release();

			return out;
		}
	}
	catch (...)
	{
		std::print("Unknown exception caught!");
	}

	return CompileError::Unknown;
}


struct VKComileArtifacts
{
	FlexKit::Blob byteCode;
	FlexKit::Vector<FlexKit::ShaderAttribute> attributes;
};


std::expected<VKComileArtifacts, CompileError> CompileVK(
	const std::string_view	target, const std::string_view	out,
	const std::string_view	entry, const std::string_view	profile,
	const ShaderOptions&	options)
{
	IDxcUtils* hlslUtils = nullptr;
	IDxcIncludeHandler* hlslIncludeHandler = nullptr;
	IDxcCompiler3* hlslCompiler = nullptr;

	if (FAILED(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&hlslUtils))))
		return std::unexpected{ CompileError::FailedToInitializeCompiler };

	if (FAILED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&hlslCompiler))))
		return std::unexpected{ CompileError::FailedToInitializeCompiler };

	if (hlslUtils) hlslUtils->CreateDefaultIncludeHandler(&hlslIncludeHandler);

	std::filesystem::path filePath{ target };
	auto parentPath = filePath.parent_path();

	wchar_t entryPointW[64];
	wchar_t fileW[256];
	wchar_t filenameW[256];
	wchar_t profileW[64];

	size_t fileWLength = 0;
	if (entry.size())
		mbstowcs(entryPointW, entry.data(), 64);

	mbstowcs(profileW, profile.data(), 64);
	mbstowcs(fileW, target.data(), 256);
	mbstowcs(filenameW, filePath.filename().string().c_str(), 256);


	auto size = FlexKit::GetFileSize(filePath.string().c_str());
	std::string shaderStr;
	shaderStr.resize(size);
	FlexKit::LoadFileIntoBuffer(filePath.string().c_str(), (std::byte*)shaderStr.data(), size);

	auto type = [](const char* profile)
		{
			uint16_t code = *(uint16_t*)profile;

			switch (code)
			{
			case 0x7370:
				return FlexKit::SHADER_TYPE::Pixel;
			case 0x7376:
				return FlexKit::SHADER_TYPE::Vertex;
			case 0x7361:
				return FlexKit::SHADER_TYPE::Amplification;
			case 0x736d:
				return FlexKit::SHADER_TYPE::Mesh;
			case 0x7363:
				return FlexKit::SHADER_TYPE::Compute;
			case 0x7364:
				return FlexKit::SHADER_TYPE::Domain;
			case 0x7368:
				return FlexKit::SHADER_TYPE::Hull;
			default:
				return FlexKit::SHADER_TYPE::Unknown;
			}
		}(profile.data());

	auto res = FlexKit::VKShaderProprocessor(shaderStr, type, FlexKit::SystemAllocator);

	IDxcBlobEncoding* blob;
	auto HR1 = hlslUtils->CreateBlobFromPinned(shaderStr.data(), shaderStr.size(), DXC_CP_ACP, &blob);

	if (FAILED(HR1))
	{
		LPSTR string = nullptr;

		const auto msgLen = FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr,
			HR1,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPSTR)&string,
			0,
			nullptr);

		auto converted = fmt::format("Shader failed to load: {}", string);

		FK_LOG_ERROR(converted.c_str());

		LocalFree(string);

		return std::unexpected{ CompileError::FileNotFound };
	}

	IncludeHandler includeHandler;
	includeHandler.includePath = parentPath;
	includeHandler.handler = hlslIncludeHandler;


	IDxcCompiler2* debugCompiler = nullptr;
	hlslCompiler->QueryInterface<IDxcCompiler2>(&debugCompiler);

	std::vector<LPCWSTR> arguments;

#if USING(DEBUGSHADERS)

	if (options.enableDebug)
	{
		arguments.push_back(L"-Od");
		arguments.push_back(L"/Zi");
		arguments.push_back(L"-Qembed_debug");
	}

	arguments.push_back(L"-spirv");
	arguments.push_back(L"-T");
	arguments.push_back(profileW);
	arguments.push_back(L"-E");
	arguments.push_back(entryPointW);
#else
	arguments.push_back(L"-O2");
#endif

	if (options.enable16BitTypes)
		arguments.push_back(L"-enable-16bit-types");

	if (options.hlsl2021)
		arguments.push_back(L"-HV 2021");

	IDxcOperationResult* result = nullptr;

	HRESULT HR2;
	try
	{
		DxcBuffer buffer{
			blob->GetBufferPointer(),
			blob->GetBufferSize(),
		};

		int _;

		blob->GetEncoding(&_, &buffer.Encoding);
		IDxcOperationResult* result = nullptr;

		HR2 = hlslCompiler->Compile(
			&buffer,
			arguments.data(),
			(UINT)arguments.size(),
			&includeHandler,
			IID_PPV_ARGS(&result));

		IDxcBlobEncoding* errors;
		result->GetErrorBuffer(&errors);

		IDxcBlob* blob;
		result->GetResult(&blob);

		auto buff		= blob->GetBufferPointer();
		auto buffSize	= blob->GetBufferSize();

		if (buff == nullptr || buffSize == 0)
		{
			auto errorString = (const char*)errors->GetBufferPointer();
			std::string formattedMessage =
				fmt::format("{}\nFailed to Compile Shader\nEntryPoint: {}\nFile : \n {}\nPress Enter to try again\n",
					errorString, entry.size() > 0 ? entry : "No Entry Point", target);

			FK_LOG_ERROR(formattedMessage.c_str());

			errors->Release();

			return std::unexpected{ CompileError::Syntax };
		}
		else
		{
			VKComileArtifacts artifacts;
			artifacts.attributes	= std::move(res.attributes);
			artifacts.byteCode		= FlexKit::Blob{ (const char*)buff, buffSize };

			blob->Release();

			return artifacts;
		}
	}
	catch (...)
	{
		std::print("Unknown exception caught!");
	}

	return std::unexpected{ CompileError::Unknown };
}


int main(const int args, const char* argv[])
{
	int errc = 0;

	uint64_t 	guid = GenerateRandomID();
	std::string target;
	std::string entry;
	std::string profile;
	std::string output = "a.out";
	std::string id = "";
	ShaderOptions options;

	enum class OutputMode
	{
		DX12,
		VK,
		NotSet
	}	mode = OutputMode::NotSet;

	for (int i = 0; i < args; i++)
	{
		std::string_view arg{ argv[i] };

		if (arg == "-d")
			options.enableDebug = true;
		else if (arg == "-vk")
			mode = OutputMode::VK;
		else if (arg == "-dx")
			mode = OutputMode::DX12;
		else if (arg == "-i")
		{
			if (i + 1 < args)
				target = std::string_view{ argv[i + 1] };
		}
		else if (arg == "-o")
		{
			if (i + 1 < args)
				output = std::string_view{ argv[i + 1] };
		}
		else if (arg == "-e")
		{
			if (i + 1 < args)
			{
				entry = std::string_view{ argv[i + 1] };
				if (id == "")
					id = entry;
			}
		}
		else if (arg == "-id")
		{
			if (i + 1 < args)
				id = std::string_view{ argv[i + 1] };
		}
		else if (arg == "-g")
		{
			if (i + 1 < args)
				guid = std::stoi(std::string{ argv[i + 1] });
		}
		else if (arg == "-p")
		{
			if (i + 1 < args)
				profile = std::string_view{ argv[i + 1] };
		}
		else if (arg == "--enable-fp16")
			options.enable16BitTypes = true;
		else if (arg == "--enable-hlsl2021")
			options.hlsl2021 = true;
		else if (arg == "--enable-debug")
			options.enableDebug = true;
		else if (arg == "--include-rootsig")
			options.loadRootSignature = true;
	}

	switch (mode)
	{
	case OutputMode::DX12:
	{
		auto res = CompileDX(target, output, entry, profile, options);

		if(res)
		{
			auto& shaderBlob = res.value();
			auto blob = CreateShaderResourceBlob(FlexKit::ShaderResourceBlob::ShaderAPI::HLSL, guid, id, res.value().data(), res.value().size(), {});

			FlexKit::ShaderResourceBlob* testView = (FlexKit::ShaderResourceBlob*)blob.data();
			FILE* f;
			f = fopen(output.c_str(), "wb");
			WriteBlob(blob, f);
			fclose(f);

			std::print("guid: {}\nassetID: {}\n", guid, id);
		}
		else
			std::print("Compile Failed!");

	}	break;
	case OutputMode::VK:
	{
		auto res = CompileVK(target, output, entry, profile, options);

		if (res)
		{
			auto& shaderBlob = res.value();
			auto attributeBlobs = CreateAttributeBlock(shaderBlob.attributes);

			auto blob = CreateShaderResourceBlob(
				FlexKit::ShaderResourceBlob::ShaderAPI::SPIRV,
				guid, id, res.value().byteCode.data(), res.value().byteCode.size(),
				attributeBlobs);

			FILE* f;
			f = fopen(output.c_str(), "wb");
			WriteBlob(blob, f);
			fclose(f);

			std::print("guid: {:#}\nassetID: {}\n", guid, id);
		}
		else
			std::print("Compile Failed!");
	}	break;
	case OutputMode::NotSet:
		break;
	}

	return errc;
}
