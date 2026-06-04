#include <Containers.hpp>
#include <RenderSystemInterface.hpp>


namespace FlexKit
{
	struct PreprocessorResult
	{
		Vector<ShaderAttribute>	attributes;
		uint32_t				CBVcount	= 0;
		uint32_t				tableCount	= 0;
	};

	PreprocessorResult VKShaderProprocessor(std::string& shader, const SHADER_TYPE type, iAllocator& allocator = SystemAllocator);
	PreprocessorResult DXShaderProprocessor(std::string& shader, const SHADER_TYPE type, iAllocator& allocator = SystemAllocator);

}
