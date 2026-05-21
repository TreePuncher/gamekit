#include "ShaderPreprocessor.hpp"
#include <regex>
#include <cstdint>
#include <fmt/format.h>
#include <scn/scan.h>
#include <scn/xchar.h>
#include <scn/scan.h>
#include <scn/regex.h>

namespace FlexKit
{

	struct PreprocessorContext
	{
		std::sregex_iterator		itr;
		std::sregex_iterator		end;
		size_t						shaderOffset = 0;
		std::string&				shader;
		Vector<ShaderAttribute>&	attributes;
		uint32_t&					CBVcount;
		uint32_t&					tableCount;
		const SHADER_TYPE			type;
	};


	inline const std::regex attributeRegex{ R"(\[\[fk::\w*\((\w*?|\n*?|\(|\)|\=|\s|\,|\")*?\)?\]\])" };


	PreprocessorResult ShaderProprocessor(std::string& shader, const SHADER_TYPE type, auto&& handler, iAllocator& allocator)
	{
		PreprocessorResult result{ .attributes{ allocator } };

		PreprocessorContext ctx{
			.itr	= std::sregex_iterator(
					shader.begin(),
					shader.end(),
					attributeRegex),
			.end		= std::sregex_iterator{},
			.shader		= shader,
			.attributes = result.attributes,
			.CBVcount	= result.CBVcount,
			.tableCount = result.tableCount,
			.type		= type
		};

		std::sregex_iterator end;

		size_t shaderOffset = 0;

		while (ctx.itr != ctx.end)
		{
			auto match = ctx.itr->str();

			if (auto res = scn::scan<uint32_t, uint32_t>(match, R"([[fk::CBV(binding={}, set={})]])"); res)
			{
				auto [set, binding] = res->values();
				handler.CBVBinding(set, binding, ctx);

				ctx.itr = std::sregex_iterator(
					shader.begin() + ctx.shaderOffset,
					shader.end(),
					attributeRegex);

				continue;
			}
			else if (auto res = scn::scan<uint32_t, uint32_t, std::string_view>(match, R"([[fk::StructuredRW(set={}, binding={}, type={})]])"); res)
			{
				auto [set, binding, type] = res->values();
				handler.Texture2D(set, binding, type, ctx);

				ctx.itr = std::sregex_iterator(
					shader.begin() + ctx.shaderOffset,
					shader.end(),
					attributeRegex);

				continue;
			}
			else if (auto res = scn::scan<uint32_t, uint32_t, std::string_view>(match, R"([[fk::RWTexture2D(set={}, binding={}, type={})]])"); res)
			{
				auto [set, binding, type] = res->values();
				handler.RWTexture2D(set, binding, type, ctx);

				ctx.itr = std::sregex_iterator(
					shader.begin() + ctx.shaderOffset,
					shader.end(),
					attributeRegex);

				continue;
			}
			else if (auto res = scn::scan<uint32_t, uint32_t, std::string_view>(match, R"([[fk::RWTexture3D(set={}, binding={}, type={})]])"); res)
			{
				auto [set, binding, type] = res->values();
				handler.RWTexture3D(set, binding, type, ctx);

				ctx.itr = std::sregex_iterator(
					shader.begin() + ctx.shaderOffset,
					shader.end(),
					attributeRegex);

				continue;
			}
			else if (auto res = scn::scan<uint32_t, uint32_t, std::string_view>(match, R"([[fk::CubeMap(set={}, binding={}, type={})]])"); res)
			{
				auto [set, binding, type] = res->values();
				handler.CubeMap(set, binding, type, ctx);

				ctx.itr = std::sregex_iterator(
					shader.begin() + ctx.shaderOffset,
					shader.end(),
					attributeRegex);

				continue;
			}
			else if (auto res = scn::scan<uint32_t, uint32_t, std::string_view>(match, R"([[fk::StructuredBuffer(set={}, binding={}, type={})]])"); res)
			{
				auto [set, binding, type] = res->values();
				handler.StructuredBuffer(set, binding, type, ctx);

				ctx.itr = std::sregex_iterator(
					shader.begin() + ctx.shaderOffset,
					shader.end(),
					attributeRegex);

				continue;
			}
			else if (auto res = scn::scan<std::string_view, uint32_t, uint32_t, std::string_view>(match, R"([[fk::Texture2D(ID={}, set={}, binding={}, type={})]])"); res)
			{
				auto [ID, set, binding, type] = res->values();
				handler.Texture2DBinding(ID, set, binding, type, ctx);

				ctx.itr = std::sregex_iterator(
					shader.begin() + ctx.shaderOffset,
					shader.end(),
					attributeRegex);

				continue;
			}
			else if (auto res = scn::scan<std::string_view, uint32_t, uint32_t, std::string_view>(match, R"([[fk::Texture3D(ID={}, set={}, binding={}, type={})]])"); res)
			{
				auto [ID, set, binding, type] = res->values();
				handler.Texture3DBinding(ID, set, binding, type, ctx);

				ctx.itr = std::sregex_iterator(
					shader.begin() + ctx.shaderOffset,
					shader.end(),
					attributeRegex);

				continue;
			}
			else if (auto res = scn::scan<uint32_t, scn::regex_matches>(match, R"([[fk::DescriptorSet(set={}, {:/((\w)+\((\w|\=|\,|\s)*\)|(\s|\,)?)*/n})]])"); res)
			{
				auto [set, descriptors] = res->values();
				handler.DescriptorSet(set, descriptors, ctx);

				ctx.itr = std::sregex_iterator(
					shader.begin() + ctx.shaderOffset,
					shader.end(),
					attributeRegex);

				continue;
			}
			else if (auto res = scn::scan<uint32_t>(match, R"([[fk::CBV(set={})]])"); res)
			{
				auto [binding] = res->values();
				handler.CBVPushBuffer(binding, ctx);

				ctx.itr = std::sregex_iterator(
					shader.begin() + ctx.shaderOffset,
					shader.end(),
					attributeRegex);

				continue;
			}
			else if (auto res = scn::scan<scn::regex_matches, uint32_t>(match, R"([[fk::InlineValues(id="{:/(\w|\d)*/}", num={})]])"); res)
			{
				auto [matches, num] = res->values();
				handler.InlineValues(matches, num, ctx);

				ctx.itr = std::sregex_iterator(
					shader.begin() + ctx.shaderOffset,
					shader.end(),
					attributeRegex);

				continue;
			}
			else if (auto res = scn::scan<std::string_view>(match, R"([[fk::BeginRootSignature(id={:[^)]})]])"))
			{
				auto [rootSigID] = res->values();
				handler.BeginRootSig(rootSigID, ctx);

				ctx.itr = std::sregex_iterator(
					shader.begin() + ctx.shaderOffset,
					shader.end(),
					attributeRegex);

				continue;
			}
			else if (auto res = scn::scan<std::string_view>(match, R"([[fk::RootFlag({:[^)]})]])"))
			{
				auto [rootFlag] = res->values();
				handler.RootFlag(rootFlag, ctx);

				ctx.itr = std::sregex_iterator(
					shader.begin() + ctx.shaderOffset,
					shader.end(),
					attributeRegex);

				continue;
			}
			else if (auto res = scn::scan<std::string_view>(match, R"([[fk::RootSignature({:[^)]})]])"))
			{
				auto [id] = res->values();
				handler.RootSignature(id, ctx);

				ctx.itr = std::sregex_iterator(
					shader.begin() + ctx.shaderOffset,
					shader.end(),
					attributeRegex);

				continue;
			}

			shader.erase(ctx.itr->position() + ctx.shaderOffset, ctx.itr->length());

			ctx.shaderOffset += ctx.itr->position();
			ctx.itr = std::sregex_iterator(
				shader.begin() + ctx.shaderOffset,
				shader.end(),
				attributeRegex);
		}

		return result;
	}

	struct VulkanAttributeHandler
	{
		// push buffer
		static void CBVPushBuffer(uint32_t binding, PreprocessorContext& ctx)
		{
			size_t pos = ctx.itr->position() + ctx.shaderOffset;
			std::string id = fmt::format("cb_{}", rand());
			ctx.shader.replace(pos, ctx.itr->length(), std::format("cbuffer {} : register(b{})", id, binding));

			ctx.attributes.push_back(
				ShaderAttributeConstantValues{
					.binding		= (uint16_t)binding,
					.pipelineStage	= (uint32_t)ctx.type,
					.id = id
				});

			ctx.shaderOffset = pos;

			ctx.itr = std::sregex_iterator(
				ctx.shader.begin() + ctx.shaderOffset,
				ctx.shader.end(),
				attributeRegex);
		}

		// cbv in descriptor heap
		static void CBVBinding(uint32_t set, uint32_t binding, PreprocessorContext& ctx)
		{
			size_t pos = ctx.itr->position() + ctx.shaderOffset;

			auto line = std::format("cbuffer constants_{0}_{1} : register(b{0}, space{1})", set, binding);
			ctx.shader.replace(pos, ctx.itr->length(), line);

			ctx.shaderOffset = pos + line.length();

			ctx.itr = std::sregex_iterator(
				ctx.shader.begin() + ctx.shaderOffset,
				ctx.shader.end(),
				attributeRegex);
		}

		static void Texture2D(uint32_t set, uint32_t binding, std::string_view type, PreprocessorContext& ctx)
		{
			size_t pos = ctx.itr->position() + ctx.shaderOffset;

			auto line = std::format("Texture2D<{}> {} : register(s{}, space{})", type, rand(), set, binding);
			ctx.shader.replace(pos, ctx.itr->length(), line);

			ctx.shaderOffset = pos + line.length();

			ctx.itr = std::sregex_iterator(
				ctx.shader.begin() + ctx.shaderOffset,
				ctx.shader.end(),
				attributeRegex);
		}

		static void Texture2DID(std::string_view ID, uint32_t set, uint32_t binding, std::string_view type, PreprocessorContext& ctx)
		{
			size_t pos = ctx.itr->position() + ctx.shaderOffset;

			auto line = std::format("Texture2D<{}> {} : register(s{}, space{})", type, ID, set, binding);
			ctx.shader.replace(pos, ctx.itr->length(), line);

			ctx.shaderOffset = pos + line.length();

			ctx.itr = std::sregex_iterator(
				ctx.shader.begin() + ctx.shaderOffset,
				ctx.shader.end(),
				attributeRegex);
		}

		static void Texture2DBinding(std::string_view ID, uint32_t set, uint32_t binding, std::string_view type, PreprocessorContext& ctx)
		{
			size_t pos = ctx.itr->position() + ctx.shaderOffset;

			auto line = std::format("Texture2D<{}> {} : register(s{}, space{})", type, ID, set, binding);
			ctx.shader.replace(pos, ctx.itr->length(), line);

			ctx.shaderOffset = pos + line.length();

			ctx.itr = std::sregex_iterator(
				ctx.shader.begin() + ctx.shaderOffset,
				ctx.shader.end(),
				attributeRegex);
		}

		static void RWTexture2D(uint32_t set, uint32_t binding, std::string_view type, PreprocessorContext& ctx)
		{}

		static void RWTexture3D(uint32_t set, uint32_t binding, std::string_view type, PreprocessorContext& ctx)
		{}

		static void Texture3DBinding(std::string_view ID, uint32_t set, uint32_t binding, std::string_view type, PreprocessorContext& ctx)
		{}

		static void CubeMap(uint32_t set, uint32_t binding, std::string_view type, PreprocessorContext& ctx)
		{}

		static void StructuredBuffer(uint32_t binding, uint32_t set, std::string_view type, PreprocessorContext& ctx)
		{}

		static void DescriptorSet(uint32_t set, scn::regex_matches descriptors, PreprocessorContext& ctx)
		{
			uint32_t descriptorCount = 0;
			if (descriptors.size())
			{
				auto str = descriptors.at(0).value().get();
				auto itr = str.begin();

				ctx.tableCount = Max(ctx.tableCount, set + 1);
				ShaderAttributeDescriptorTable table{ .set = set };

				while (itr != str.end())
				{
					if (std::isspace(*itr) || std::ispunct(*itr))
						itr++;
					else if (std::string_view{ itr, itr + 3 } == "CBV")
					{
						auto remaining = (str.end() - itr);
						auto res = scn::scan<uint32_t>(std::string_view{ itr, itr + remaining }, "CBV(num={})");
						auto [num] = res->values();
						table.entries.push_back(ShaderAttributeDescriptorTableEntry{ .num = num, .type = ShaderResourceType::CBV });
						itr += 3;

						while (*itr != ')') itr++;
						itr++;
					}
					else if (std::string_view{ itr, itr + 10 } == "SRVTexture")
					{
						auto remaining = (str.end() - itr);
						auto res = scn::scan<uint32_t>(std::string_view{ itr, itr + remaining }, "SRVTexture(num={})");
						auto [num] = res->values();
						table.entries.push_back(ShaderAttributeDescriptorTableEntry{ .num = num, .type = ShaderResourceType::SRVTexture });
						itr += 3;

						while (*itr != ')') itr++;
						itr++;
					}
					else if (std::string_view{ itr, itr + 12 } == "SRVStructure")
					{
						auto remaining = (str.end() - itr);
						auto res = scn::scan<uint32_t>(std::string_view{ itr, itr + remaining }, "SRVBuffer(num={})");
						auto [num] = res->values();
						table.entries.push_back(ShaderAttributeDescriptorTableEntry{ .num = num, .type = ShaderResourceType::SRVBuffer });
						itr += 3;

						while (*itr != ')') itr++;
						itr++;
					}
					else if (std::string_view{ itr, itr + 3 } == "UAVTexture")
					{
						auto remaining = (str.end() - itr);
						auto res = scn::scan<uint32_t>(std::string_view{ itr, itr + remaining }, "UAVTexture(num={})");
						auto [num] = res->values();
						table.entries.push_back(ShaderAttributeDescriptorTableEntry{ .num = num, .type = ShaderResourceType::UAVTexture });
						itr += 3;

						while (*itr != ')') itr++;
						itr++;
					}
					else if (std::string_view{ itr, itr + 3 } == "UAVBuffer")
					{
						auto remaining = (str.end() - itr);
						auto res = scn::scan<uint32_t>(std::string_view{ itr, itr + remaining }, "UAVBuffer(num={})");
						auto [num] = res->values();
						table.entries.push_back(ShaderAttributeDescriptorTableEntry{ .num = num, .type = ShaderResourceType::UAVBuffer });
						itr += 3;

						while (*itr != ')') itr++;
						itr++;
					}
					else
						itr++;
				}

				ctx.attributes.push_back(table);
			}

			auto pos = ctx.itr->position() + ctx.shaderOffset;
			auto posEnd = ctx.itr->position() + ctx.shaderOffset + ctx.itr->length();

			ctx.shader.replace(pos, ctx.itr->length(), "");
		}


		static void InlineValues(scn::regex_matches id_match, uint32_t numValues, PreprocessorContext& ctx)
		{
			std::string type_id = std::format("type_{}", rand());

			auto id = id_match.at(0).and_then([](auto m) { return std::optional<std::string>{m.get()}; }).value_or(std::string{ "" });

			ctx.attributes.push_back(
				ShaderAttributeConstantValues{
					.num			= numValues,
					.pipelineStage	= (uint32_t)ctx.type,
					.id				= id,
				});

			static const std::regex structRegex{ R"(\{(\w|\d|\s|\;)*\};)" };

			std::string replacement = std::format("struct {}", type_id);
			std::string insertLine = std::format("\n[[vk::push_constant]] {} {};", type_id, id);

			auto structBlock = std::sregex_iterator(
				ctx.shader.begin() + ctx.shaderOffset,
				ctx.shader.end(),
				structRegex);

			auto pos	= ctx.itr->position() + ctx.shaderOffset;
			auto posEnd = ctx.itr->position() + ctx.shaderOffset + ctx.itr->length();
			auto block	= structBlock->position();

			if (posEnd + 2 >= block)
			{	// formatted correctly? Maybe?
				ctx.shader.replace(pos, ctx.itr->length(), replacement);

				auto offset = replacement.length() + structBlock->length() + 2;
				ctx.shader.insert(pos + offset, insertLine);
			}

			ctx.shaderOffset = posEnd;

			ctx.itr = std::sregex_iterator(
				ctx.shader.begin() + posEnd,
				ctx.shader.end(),
				attributeRegex);
		}

		static void BeginRootSig(std::string_view ID, PreprocessorContext& ctx)
		{
			auto pos = ctx.itr->position() + ctx.shaderOffset;
			auto posEnd = ctx.itr->position() + ctx.shaderOffset + ctx.itr->length();

			ctx.shader.replace(pos, ctx.itr->length(), "");
		}

		static void EndRootSig(PreprocessorContext& ctx)
		{
			auto pos = ctx.itr->position() + ctx.shaderOffset;
			auto posEnd = ctx.itr->position() + ctx.shaderOffset + ctx.itr->length();

			ctx.shader.replace(pos, ctx.itr->length(), "");
		}

		static void RootFlag(std::string_view, PreprocessorContext& ctx)
		{
			auto pos = ctx.itr->position() + ctx.shaderOffset;
			auto posEnd = ctx.itr->position() + ctx.shaderOffset + ctx.itr->length();

			ctx.shader.replace(pos, ctx.itr->length(), "");
		}

		static void RootSignature(std::string_view, PreprocessorContext& ctx)
		{
			auto pos = ctx.itr->position() + ctx.shaderOffset;
			auto posEnd = ctx.itr->position() + ctx.shaderOffset + ctx.itr->length();

			ctx.shader.replace(pos, ctx.itr->length(), "");
		}
	};


	struct DirectXAttributeHandler
	{
		// push buffer
		static void CBVPushBuffer(uint32_t binding, PreprocessorContext& ctx)
		{
			size_t pos = ctx.itr->position() + ctx.shaderOffset;
			std::string id = fmt::format("cb_{}", rand());
			ctx.shader.replace(pos, ctx.itr->length(), std::format("cbuffer {} : register(b{})", id, binding));

			ctx.attributes.push_back(
				ShaderAttributeConstantValues{
					.binding		= (uint16_t)binding,
					.pipelineStage	= (uint32_t)ctx.type,
					.id				= id
				});

			ctx.shaderOffset = pos;
		}

		// cbv in descriptor heap
		static void CBVBinding(uint32_t set, uint32_t binding, PreprocessorContext& ctx)
		{
			size_t pos = ctx.itr->position() + ctx.shaderOffset;

			auto line = std::format("cbuffer constants_{0}_{1} : register(b{0}, space{1})", set, binding);
			ctx.shader.replace(pos, ctx.itr->length(), line);

			ctx.shaderOffset = pos + line.length();
		}

		static void Texture2D(uint32_t set, uint32_t binding, std::string_view type, PreprocessorContext& ctx)
		{
			size_t pos = ctx.itr->position() + ctx.shaderOffset;

			auto line = std::format("Texture2D<{}> {} : register(s{}, space{})", type, rand(), set, binding);
			ctx.shader.replace(pos, ctx.itr->length(), line);

			ctx.shaderOffset = pos + line.length();
		}

		static void Texture2DID(std::string_view ID, uint32_t set, uint32_t binding, std::string_view type, PreprocessorContext& ctx)
		{
			size_t pos = ctx.itr->position() + ctx.shaderOffset;

			auto line = std::format("Texture2D<{}> {} : register(s{}, space{})", type, ID, set, binding);
			ctx.shader.replace(pos, ctx.itr->length(), line);

			ctx.shaderOffset = pos + line.length();
		}

		static void Texture2DBinding(std::string_view ID, uint32_t set, uint32_t binding, std::string_view type, PreprocessorContext& ctx)
		{
			size_t pos = ctx.itr->position() + ctx.shaderOffset;

			auto line = std::format("Texture2D<{}> {} : register(s{}, space{})", type, ID, set, binding);
			ctx.shader.replace(pos, ctx.itr->length(), line);

			ctx.shaderOffset = pos + line.length();
		}

		static void RWTexture2D(uint32_t set, uint32_t binding, std::string_view type, PreprocessorContext& ctx)
		{
			auto pos	= ctx.itr->position() + ctx.shaderOffset;
			auto posEnd = ctx.itr->position() + ctx.shaderOffset + ctx.itr->length();

			ctx.shader.replace(pos, ctx.itr->length(), "");
		}

		static void RWTexture3D(uint32_t set, uint32_t binding, std::string_view type, PreprocessorContext& ctx)
		{
			auto pos	= ctx.itr->position() + ctx.shaderOffset;
			auto posEnd = ctx.itr->position() + ctx.shaderOffset + ctx.itr->length();

			ctx.shader.replace(pos, ctx.itr->length(), "");
		}

		static void Texture3DBinding(std::string_view ID, uint32_t set, uint32_t binding, std::string_view type, PreprocessorContext& ctx)
		{
			auto pos	= ctx.itr->position() + ctx.shaderOffset;
			auto posEnd = ctx.itr->position() + ctx.shaderOffset + ctx.itr->length();

			ctx.shader.replace(pos, ctx.itr->length(), "");
		}

		static void CubeMap(uint32_t set, uint32_t binding, std::string_view type, PreprocessorContext& ctx)
		{
			auto pos	= ctx.itr->position() + ctx.shaderOffset;
			auto posEnd = ctx.itr->position() + ctx.shaderOffset + ctx.itr->length();

			ctx.shader.replace(pos, ctx.itr->length(), "");
		}

		static void StructuredBuffer(uint32_t binding, uint32_t set, std::string_view type, PreprocessorContext& ctx)
		{
			auto pos	= ctx.itr->position() + ctx.shaderOffset;
			auto posEnd = ctx.itr->position() + ctx.shaderOffset + ctx.itr->length();

			ctx.shader.replace(pos, ctx.itr->length(), "");
		}

		static void DescriptorSet(uint32_t set, scn::regex_matches descriptors, PreprocessorContext& ctx)
		{
			uint32_t descriptorCount = 0;
			if (descriptors.size())
			{
				auto str = descriptors.at(0).value().get();
				auto itr = str.begin();

				ctx.tableCount = Max(ctx.tableCount, set + 1);
				ShaderAttributeDescriptorTable table{ .set = set };

				while (itr != str.end())
				{
					if (std::isspace(*itr) || std::ispunct(*itr))
						itr++;
					else if (std::string_view{ itr, itr + 3 } == "CBV")
					{
						auto remaining = (str.end() - itr);
						auto res = scn::scan<uint32_t>(std::string_view{ itr, itr + remaining }, "CBV(num={})");
						auto [num] = res->values();
						table.entries.push_back(ShaderAttributeDescriptorTableEntry{ .num = num, .type = ShaderResourceType::CBV });
						itr += 3;

						while (*itr != ')') itr++;
						itr++;
					}
					else if (std::string_view{ itr, itr + 10 } == "SRVTexture")
					{
						auto remaining = (str.end() - itr);
						auto res = scn::scan<uint32_t>(std::string_view{ itr, itr + remaining }, "SRVTexture(num={})");
						auto [num] = res->values();
						table.entries.push_back(ShaderAttributeDescriptorTableEntry{ .num = num, .type = ShaderResourceType::SRVTexture });
						itr += 3;

						while (*itr != ')') itr++;
						itr++;
					}
					else if (std::string_view{ itr, itr + 12 } == "SRVStructure")
					{
						auto remaining = (str.end() - itr);
						auto res = scn::scan<uint32_t>(std::string_view{ itr, itr + remaining }, "SRVBuffer(num={})");
						auto [num] = res->values();
						table.entries.push_back(ShaderAttributeDescriptorTableEntry{ .num = num, .type = ShaderResourceType::SRVBuffer });
						itr += 3;

						while (*itr != ')') itr++;
						itr++;
					}
					else if (std::string_view{ itr, itr + 3 } == "UAVTexture")
					{
						auto remaining = (str.end() - itr);
						auto res = scn::scan<uint32_t>(std::string_view{ itr, itr + remaining }, "UAVTexture(num={})");
						auto [num] = res->values();
						table.entries.push_back(ShaderAttributeDescriptorTableEntry{ .num = num, .type = ShaderResourceType::UAVTexture });
						itr += 3;

						while (*itr != ')') itr++;
						itr++;
					}
					else if (std::string_view{ itr, itr + 3 } == "UAVBuffer")
					{
						auto remaining = (str.end() - itr);
						auto res = scn::scan<uint32_t>(std::string_view{ itr, itr + remaining }, "UAVBuffer(num={})");
						auto [num] = res->values();
						table.entries.push_back(ShaderAttributeDescriptorTableEntry{ .num = num, .type = ShaderResourceType::UAVBuffer });
						itr += 3;

						while (*itr != ')') itr++;
						itr++;
					}
					else
						itr++;
				}

				ctx.attributes.push_back(table);
			}

			auto pos = ctx.itr->position() + ctx.shaderOffset;
			auto posEnd = ctx.itr->position() + ctx.shaderOffset + ctx.itr->length();

			ctx.shader.replace(pos, ctx.itr->length(), "");
		}


		static void InlineValues(scn::regex_matches id_match, uint32_t numValues, PreprocessorContext& ctx)
		{
			std::string type_id = std::format("type_{}", rand());

			auto id = id_match.at(0).and_then([](auto m) { return std::optional<std::string>{m.get()}; }).value_or(std::string{ "" });

			ctx.attributes.push_back(
				ShaderAttributeConstantValues{
					.num = numValues,
					.pipelineStage = (uint32_t)ctx.type,
					.id = id,
				});

			static const std::regex structRegex{ R"(\{(\w|\d|\s|\;)*\};)" };

			std::string replacement = std::format("struct {}", type_id);
			std::string insertLine	= std::format("\n[[vk::push_constant]] {} {};", type_id, id);

			auto structBlock = std::sregex_iterator(
				ctx.shader.begin() + ctx.shaderOffset,
				ctx.shader.end(),
				structRegex);

			auto pos = ctx.itr->position() + ctx.shaderOffset;
			auto posEnd = ctx.itr->position() + ctx.shaderOffset + ctx.itr->length();
			auto block = structBlock->position();

			if (posEnd + 2 >= block)
			{	// formatted correctly? Maybe?
				ctx.shader.replace(pos, ctx.itr->length(), replacement);

				auto offset = replacement.length() + structBlock->length() + 2;
				ctx.shader.insert(pos + offset, insertLine);
			}

			ctx.shaderOffset = posEnd;
		}

		void BeginRootSig(std::string_view ID, PreprocessorContext& ctx)
		{
			auto pos = ctx.itr->position() + ctx.shaderOffset;
			auto posEnd = ctx.itr->position() + ctx.shaderOffset + ctx.itr->length();

			ctx.shader.replace(pos, ctx.itr->length(), "");
		}

		void RootFlag(std::string_view flag, PreprocessorContext& ctx)
		{
			flags.push_back(std::string{ flag });

			auto pos	= ctx.itr->position() + ctx.shaderOffset;
			auto posEnd = ctx.itr->position() + ctx.shaderOffset + ctx.itr->length();

			ctx.shader.replace(pos, ctx.itr->length(), "");
		}

		void RootSignature(std::string_view signatureID, PreprocessorContext& ctx)
		{
			size_t pos = ctx.itr->position() + ctx.shaderOffset;

			ctx.shader.replace(pos, ctx.itr->length(), "");
		}

		Vector<std::string> flags;
	};

	PreprocessorResult VKShaderProprocessor(std::string& shader, const SHADER_TYPE type, iAllocator& allocator)
	{
		return ShaderProprocessor(shader, type, VulkanAttributeHandler{}, allocator);
	}

	PreprocessorResult DXShaderProprocessor(std::string& shader, const SHADER_TYPE type, iAllocator& allocator)
	{
		return ShaderProprocessor(shader, type, DirectXAttributeHandler{ .flags{allocator} }, allocator);
	}
}


