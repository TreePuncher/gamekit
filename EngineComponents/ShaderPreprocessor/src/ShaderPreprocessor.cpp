#include "ShaderPreprocessor.hpp"
#include <regex>
#include <fmt/format.h>
#include <scn/scan.h>
#include <scn/regex.h>
#include <flat_map>
#include <print>

namespace FlexKit
{

	struct PreprocessorContext
	{
		size_t						shaderOffset = 0;
		std::string&				shader;
		std::string::iterator		begin;
		std::string::iterator		end;

		Vector<ShaderAttribute>&	attributes;
		uint32_t&					CBVcount;
		uint32_t&					tableCount;
		const SHADER_TYPE			type;
	};


	enum class ParseErrorTypes
	{
		SyntaxError,
		TagNotFound
	};


	struct Tag
	{
		std::string_view::const_iterator    idBegin;
		std::string_view::const_iterator    idEnd;

		std::string_view::const_iterator    paramsBegin;
		std::string_view::const_iterator    parameEnd;

		std::vector<std::string_view>       arguments;
	};


	struct DescriptorTableValues
	{
		uint32_t         setIdx;
		std::vector<Tag> tags;
	};



	std::expected<DescriptorTableValues, ParseErrorTypes> HandleDesctriptorTable(PreprocessorContext& ctx, const std::string_view inputView)
	{
		DescriptorTableValues out;
		constexpr std::string_view tag{"[[fk::DescriptorSet("};

		if (inputView.contains("DescriptorSet"))
		{
			auto res		= inputView.find(tag);
			auto itrView	= inputView.begin() + res + tag.size();
			auto endView	= [&] { auto res = inputView.find("]]"); return (res == std::string_view::npos) ? itrView : inputView.begin() + res + 2; }();
			int counter		= 1;
			int brackets	= 2;

			std::vector<Tag>        tags;
			std::optional<Tag>      current;
			std::optional<std::string_view::const_iterator> subArgIdx;

			while (itrView < endView)
			{
				if (std::isalnum(*itrView))
				{
					if (counter == 1)
					{
						if (!current.has_value())
							current = Tag{ .idBegin = itrView, .idEnd = itrView };
						else
							current->idEnd = itrView + 1;
					}
					else if (!subArgIdx)
						subArgIdx = itrView;
				}
				else
				{
					switch (*itrView)
					{
					case '(':
						if (counter > 1)
							return std::unexpected{ ParseErrorTypes::SyntaxError };

						current.value().paramsBegin = itrView + 1;
						counter++;
						break;
					case ')':
						if (counter > 0)
							counter--;
						else
							return std::unexpected{ ParseErrorTypes::SyntaxError };

						if (counter > 0)
						{
							if (subArgIdx)
								current.value().arguments.push_back(std::string_view{ subArgIdx.value(), itrView });

							current.value().parameEnd = itrView;
							tags.push_back(std::move(*current));
							subArgIdx.reset();
							current.reset();
						}
						break;
					case '[':
						return std::unexpected{ ParseErrorTypes::SyntaxError };
					case ']':
						if (brackets > 0)
							brackets--;
						else
							return std::unexpected{ ParseErrorTypes::SyntaxError };
						break;
					case ',':
						current.value().arguments.push_back(std::string_view{ subArgIdx.value(), itrView });
						subArgIdx.reset();
						break;
					default:
						break;
					}
				}

				if (counter == 0 && brackets == 0)
				{
					endView = itrView;
					break;
				}
				itrView++;
			}

			if (counter != 0 || brackets != 0)
				return std::unexpected{ ParseErrorTypes::SyntaxError };
			else
				return DescriptorTableValues{ .setIdx = ctx.tableCount++, .tags = tags };
		}

		return std::unexpected{ ParseErrorTypes::TagNotFound };
	}


	inline const std::regex attributeRegex{ R"(\[\[fk::\w*\((\w*?|\n*?|\(|\)|\=|\s|\,|\")*?\)?\]\])" };


	PreprocessorResult ShaderProprocessor(std::string& shader, const SHADER_TYPE type, auto&& handler, iAllocator& allocator)
	{
		PreprocessorResult result{ .attributes{ allocator } };
            
		PreprocessorContext ctx{
			.shader		= shader,
			.attributes = result.attributes,
			.CBVcount	= result.CBVcount,
			.tableCount = result.tableCount,
			.type		= type
		};

		for (auto res = shader.find(R"([[fk::)", ctx.shaderOffset); res != std::string::npos;  res = shader.find(R"([[fk::)", ctx.shaderOffset))
		{
			auto beginItr = shader.begin() + res;
			auto endres = shader.find(R"(]])", res);

			if (endres == std::string::npos)
				break;

			endres += 2;

			auto endItr = shader.begin() + endres;
			ctx.begin	= beginItr;
			ctx.end		= endItr;

			std::string_view match{ beginItr, endItr };
			{
				const auto res = HandleDesctriptorTable(ctx, match);
				if (!res && res.error() == ParseErrorTypes::SyntaxError)
				{
					return {};
				}
				else if (res)
				{
					ctx.shaderOffset = endres;

					auto [num, descriptorTags] = res.value();
					handler.DefineDescriptorSet(num, descriptorTags, ctx);

					continue;
				}
			}
		    if (auto res = scn::scan<uint32_t, uint32_t>(match, R"([[fk::CBV(binding={}, set={})]])"); res)
			{
				ctx.shaderOffset = endres;

				auto [set, binding] = res->values();
				handler.CBVBinding(set, binding, ctx);

				continue;
			}
			else if (auto res = scn::scan<std::string_view>(match, R"([[fk::CBV(id={:[^)]})]])"); res)
			{
				ctx.shaderOffset = endres;

				auto [id] = res->values();
				handler.CBVRoot(id, ctx);

				continue;
			}
			else if (auto res = scn::scan<uint32_t, uint32_t, uint32_t>(match, R"([[fk::Values(num={}, binding={}, set={})]])"); res)
			{
				ctx.shaderOffset = endres;

				auto [num, set, binding] = res->values();
				handler.CBVBinding(set, binding, ctx);

				continue;
			}
			
			else if (auto res = scn::scan<std::string_view, uint32_t, std::string_view>(match, R"([[fk::UAVStructured(id={:/[[:alnum:]_]+(^,\s)?/n}, binding={}, type={:/[[:alnum:]_]+(^,\s)?/n})]])"); res)
			{
				ctx.shaderOffset = endres;

				auto [id, binding, type] = res->values();
				handler.UAVStructuredRoot(id, binding, type, ctx);

				continue;
			}
			else if (auto res = scn::scan<uint32_t, uint32_t, std::string_view>(match, R"([[fk::StructuredRW(set={}, binding={}, type={})]])"); res)
			{
				ctx.shaderOffset = endres;

				auto [set, binding, type] = res->values();
				handler.Texture2D(set, binding, type, ctx);

				continue;
			}
			else if (auto res = scn::scan<uint32_t, uint32_t, std::string_view>(match, R"([[fk::RWTexture2D(set={}, binding={}, type={})]])"); res)
			{
				ctx.shaderOffset = endres;

				auto [set, binding, type] = res->values();
				handler.RWTexture2D(set, binding, type, ctx);

			    continue;
			}
			else if (auto res = scn::scan<uint32_t, uint32_t, std::string_view>(match, R"([[fk::RWTexture3D(set={}, binding={}, type={})]])"); res)
			{
				ctx.shaderOffset = endres;
				
			    auto [set, binding, type] = res->values();
				handler.RWTexture3D(set, binding, type, ctx);

				continue;
			}
			else if (auto res = scn::scan<uint32_t, uint32_t, std::string_view>(match, R"([[fk::CubeMap(set={}, binding={}, type={})]])"); res)
			{
				ctx.shaderOffset = endres;
				
			    auto [set, binding, type] = res->values();
				handler.CubeMap(set, binding, type, ctx);

				continue;
			}
			else if (auto res = scn::scan<std::string_view, uint32_t, uint32_t, std::string_view>(match, R"([[fk::StructuredBuffer(id={:/[[:alnum:]_]+(^,\s)?/n}, set={}, binding={}, type={:/[[:alnum:]_]+(^,\s)?/n})]])"); res)
			{
				ctx.shaderOffset = endres;

				auto [id, set, binding, type] = res->values();
				handler.StructuredBuffer(id, set, binding, type, ctx);

				continue;
			}
			else if (auto res = scn::scan<std::string_view, uint32_t, std::string_view>(match, R"([[fk::StructuredBuffer(id={:/[[:alnum:]_]+(^,\s)?/n}, binding={}, type={:/[[:alnum:]_]+(^,\s)?/n})]])"); res)
			{
				ctx.shaderOffset = endres;

				auto [id, binding, type] = res->values();
				handler.StructuredBufferRoot(id, binding, type, ctx);

				continue;
			}
			//else if (auto res = scn::scan<std::string_view, uint32_t, uint32_t, std::string_view>(match, R"([[Texture2D(ID={:/[[:alnum:]_]+(^,\s)?/n}, binding={}, set={}, type={:/[[:alnum:]_]+(^,\s)?/n})]])"); res)
			else if (auto res = scn::scan<std::string_view, uint32_t, uint32_t, std::string_view>(match, R"([[fk::Texture2D(id={:/[[:alnum:]_]+/n}, binding={}, set={}, type={:/[[:alnum:]_]+/n})]])"); res)
			{
				ctx.shaderOffset = endres;
				
			    auto [ID, binding, set, type] = res->values();
				handler.Texture2DBinding(ID, set, binding, type, ctx);

				continue;
			}
			else if (auto res = scn::scan<std::string_view, uint32_t, uint32_t, std::string_view>(match, R"([[fk::Texture2DArray(id={:/[[:alnum:]_]+/n}, offset={}, set={}, type={:/[[:alnum:]_]+/n})]])"); res)
			{
				ctx.shaderOffset = endres;

				auto [ID, offset, set, type] = res->values();
				handler.Texture2DArray(ID, set, offset, type, ctx);

				continue;
			}
			else if (auto res = scn::scan<std::string_view, uint32_t, uint32_t, std::string_view>(match, R"([[fk::Texture3D(id={}, set={}, binding={}, type={:[^)]})]])"); res)
			{
				ctx.shaderOffset = endres;
				
			    auto [ID, set, binding, type] = res->values();
				handler.Texture3DBinding(ID, set, binding, type, ctx);

				continue;
			}
			else if (auto res = scn::scan<uint32_t>(match, R"([[fk::CBV(binding={})]])"); res)
			{
				ctx.shaderOffset = endres;
				
			    auto [binding] = res->values();
				handler.CBVPushBuffer(binding, ctx);

				continue;
			}
			else if (auto res = scn::scan<uint32_t>(match, R"([[fk::PushConstants(num={})]])"); res)
			{
				ctx.shaderOffset = endres;
			    
			    auto [num] = res->values();
				handler.PushConstants(match.size(), num, ctx);

				continue;
			}
			else if (auto res = scn::scan<uint32_t, std::string_view>(match, R"([[fk::PushConstants(num={}, id={:[^)]})]])"); res)
			{
				ctx.shaderOffset = endres;

				auto [num, rootSigID] = res->values();
				handler.LocalRootValues(num, rootSigID, ctx);

				continue;
				}
			else if (auto res = scn::scan<std::string_view>(match, R"([[fk::BeginRootSignatureDef(id={:[^)]})]])"))
			{
				ctx.shaderOffset = endres;

				auto [rootSigID] = res->values();
				handler.BeginRootSig(rootSigID, ctx);

				continue;
			}
			else if (auto res = scn::scan<std::string_view>(match, R"([[fk::RootFlags({:[^)]})]])"))
			{
				ctx.shaderOffset = endres;
				
			    auto [rootFlag] = res->values();
				handler.RootFlags(rootFlag, ctx);

				continue;
			}
			else if (auto res = scn::scan<std::string_view>(match, R"([[fk::RootSignature(id={:[^)]})]])"))
			{
				ctx.shaderOffset = endres;

				auto [id] = res->values();
				handler.RootSignature(id, ctx);

				continue;
			}
			else if (auto res = scn::scan<std::string_view>(match, R"([[fk::LocalRootSignature(id={:[^)]})]])"))
			{
				ctx.shaderOffset = endres;

				auto [id] = res->values();
				handler.LocalRootSignature(id, ctx);

				continue;
			}
			else if (auto res = scn::scan<uint32_t, uint32_t, std::string_view>(match, R"([[fk::AccelerationStructure(binding={}, set={}, id={:[^)]})]])"))
			{
				ctx.shaderOffset = endres;

				auto [binding, set, id] = res->values();
				handler.AccelerationStructure(id, binding, set, ctx);

				continue;
			}
			else if (auto res = scn::scan<std::string_view>(match, R"([[fk::DefineRootSignature(id={:[^)]})]])"))
			{
				ctx.shaderOffset = endres;

				auto [id] = res->values();
				handler.DefineRootSignature(id, ctx);

				continue;
			}
			else if (auto res = scn::scan<std::string_view>(match, R"([[fk::UAVByteBuffer(id={:[^)]})]])"))
			{
				ctx.shaderOffset = endres;

				auto [id] = res->values();
				handler.UAVByteBuffer(id, ctx);

				continue;
			}

			shader.erase(res, endres - res);
		}

		return result;
	}

	struct VulkanAttributeHandler
	{
		void LocalRootValues(uint32_t numValues, std::string_view rootSigID, PreprocessorContext& ctx)
		{
		    
		}
		// push buffer
		static void CBVPushBuffer(uint32_t binding, PreprocessorContext& ctx)
		{
			std::string id = fmt::format("cb_{}", rand());
			auto line = std::format("cbuffer {} : register(b{})", id, binding);
			ctx.shader.replace(ctx.begin, ctx.end, line);
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin) + line.size();

			ctx.attributes.push_back(
				ShaderAttributeConstantValues{
					.binding		= (uint16_t)binding,
					.pipelineStage	= (uint32_t)ctx.type,
					.id = id
				});
		}

		// cbv in pipeline interface
		static void CBVRoot(std::string_view rootSigID, PreprocessorContext& ctx)
		{
			ctx.shader.replace(ctx.begin, ctx.end, "");
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin);
		}

		// cbv in descriptor heap
		static void CBVBinding(uint32_t set, uint32_t binding, PreprocessorContext& ctx)
		{
			auto line = std::format("cbuffer constants_{0}_{1} : register(b{0}, space{1})", set, binding);
			ctx.shader.replace(ctx.begin, ctx.end, line);
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin) + line.size();
		}

		static void UAVStructuredRoot(std::string_view id, uint32_t binding, std::string_view type, PreprocessorContext& ctx)
		{
			ctx.shader.replace(ctx.begin, ctx.end, "");
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin);
		}

		static void Texture2D(uint32_t set, uint32_t binding, std::string_view type, PreprocessorContext& ctx)
		{
			auto line = std::format("Texture2D<{}> {} : register(s{}, space{})", type, rand(), set, binding);
			ctx.shader.replace(ctx.begin, ctx.end, line);
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin) + line.size();
		}

		static void Texture2DID(std::string_view ID, uint32_t set, uint32_t binding, std::string_view type, PreprocessorContext& ctx)
		{
			auto line = std::format("Texture2D<{}> {} : register(s{}, space{})", type, ID, set, binding);
			ctx.shader.replace(ctx.begin, ctx.end, line);
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin) + line.size();
		}

		static void Texture2DBinding(std::string_view ID, uint32_t set, uint32_t binding, std::string_view type, PreprocessorContext& ctx)
		{
			auto line = std::format("Texture2D<{}> {} : register(s{}, space{})", type, ID, set, binding);
			ctx.shader.replace(ctx.begin, ctx.end, line);
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin) + line.size();
		}

		void Texture2DArray(std::string_view ID, uint32_t offset, uint32_t set, std::string_view type, PreprocessorContext& ctx)
		{
			auto line = std::format("Texture2D<{}> {}[] : register(s{}, space{})", type, ID, offset, set);
			ctx.shader.replace(ctx.begin, ctx.end, line);
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin) + line.size();
		}

		static void RWTexture2D(uint32_t set, uint32_t binding, std::string_view type, PreprocessorContext& ctx)
		{
			ctx.shader.replace(ctx.begin, ctx.end, "");
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin);
		}

		static void RWTexture3D(uint32_t set, uint32_t binding, std::string_view type, PreprocessorContext& ctx)
		{
			ctx.shader.replace(ctx.begin, ctx.end, "");
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin);
		}

		static void Texture3DBinding(std::string_view ID, uint32_t set, uint32_t binding, std::string_view type, PreprocessorContext& ctx)
		{
			ctx.shader.replace(ctx.begin, ctx.end, "");
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin);
		}

		static void CubeMap(uint32_t set, uint32_t binding, std::string_view type, PreprocessorContext& ctx)
		{
			ctx.shader.replace(ctx.begin, ctx.end, "");
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin);
		}

		static void StructuredBuffer(std::string_view id, uint32_t binding, uint32_t set, std::string_view type, PreprocessorContext& ctx)
		{
			ctx.shader.replace(ctx.begin, ctx.end, "");
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin);
		}

		static void StructuredBufferRoot(std::string_view id, uint32_t binding, std::string_view type, PreprocessorContext& ctx)
		{
			FK_ASSERT(false, "Unimplemented!");

			ctx.shader.replace(ctx.begin, ctx.end, "//Unimplemented!!");
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin + sizeof(R"(//Unimplemented!!)"));
		}

		static void DefineDescriptorSet(int32_t set, std::vector<Tag> tags, PreprocessorContext& ctx)
		{
			ctx.shader.replace(ctx.begin, ctx.end, "");
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin);
		}


		//static void InlineValues(scn::regex_matches id_match, uint32_t lineLength, uint32_t numValues, PreprocessorContext& ctx)
		static void PushConstants(uint32_t lineLength, uint32_t numValues, PreprocessorContext& ctx)
		{
			std::string type_id = std::format("type_{}", rand());

			/*
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
            */

			//auto pos	= ctx.itr->position() + ctx.shaderOffset;
			//auto posEnd = ctx.itr->position() + ctx.shaderOffset + ctx.itr->length();
			//auto block	= structBlock->position();
			//
			//if (posEnd + 2 >= block)
			//{	// formatted correctly? Maybe?
			//	ctx.shader.replace(pos, ctx.itr->length(), replacement);
			//
			//	auto offset = replacement.length() + structBlock->length() + 2;
			//	ctx.shader.insert(pos + offset, insertLine);
			//}
			//
			//ctx.shaderOffset = posEnd;
		}

		static void BeginRootSig(std::string_view ID, PreprocessorContext& ctx)
		{
			ctx.shader.replace(ctx.begin, ctx.end, "");
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin);
		}

		static void EndRootSig(PreprocessorContext& ctx)
		{
			ctx.shader.replace(ctx.begin, ctx.end, "");
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin);
		}

		static void RootFlags(std::string_view, PreprocessorContext& ctx)
		{
			ctx.shader.replace(ctx.begin, ctx.end, "");
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin);
		}

		static void RootSignature(std::string_view, PreprocessorContext& ctx)
		{
			ctx.shader.replace(ctx.begin, ctx.end, "");
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin);
		}

		static void LocalRootSignature(std::string_view, PreprocessorContext& ctx)
		{
			ctx.shader.replace(ctx.begin, ctx.end, "");
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin);
		}

		static void DefineRootSignature(std::string_view, PreprocessorContext& ctx)
		{
			ctx.shader.replace(ctx.begin, ctx.end, "");
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin);
		}

		static void AccelerationStructure(std::string_view id, uint32_t set, uint32_t binding, PreprocessorContext& ctx)
		{
			auto line = std::format("RaytracingAccelerationStructure {} : register(t{}, space{})", id, binding, set);
			ctx.shader.replace(ctx.begin, ctx.end, line);
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin) + line.size();
		}

		static void UAVByteBuffer(std::string_view id, PreprocessorContext& ctx)
		{
			ctx.shader.replace(ctx.begin, ctx.end, "");
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin);
		}
	};


	struct DirectXAttributeHandler
	{
		// push buffer
		void CBVPushBuffer(const uint32_t binding, PreprocessorContext& ctx)
		{
			if (rootSignatures.size() == 0)
				return;
            
			RootSignatureDefinition& definition = rootSignatures.back();

			auto checkBindingIsFree = [&]() -> bool
			    {
					uint32_t idx = 0;
					for (const RootSignatureEntryTypes entry : definition.entries)
					{
						if (entry == RootSignatureEntryTypes::CBV)
						{
							if (binding == idx)
								return false;
						    
						    idx++;
						}
					}

					return true;
			    };

			if (!checkBindingIsFree())
			{
				std::string_view error = "//Root signature binding index already in use!";
				ctx.shader.replace(ctx.begin, ctx.end, error);
				ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin) + error.size();
			}
			else
			{
			    uint32_t entrySpace = 0xfffffeff - definition.entries.size();

			    while (definition.IsSpaceInUse(entrySpace))
				    entrySpace--;

			    definition.spacesInUse.push_back(entrySpace);
			    definition.entries.push_back(RootSignatureEntryTypes::CBV);
			    definition.entrySpace.push_back(entrySpace);

			    std::string rootSignatureSection = std::format("CBV(b{0}, space={1}, visibility=SHADER_VISIBILITY_ALL, flags = DATA_STATIC_WHILE_SET_AT_EXECUTE)", binding, entrySpace);
			    if (definition.sections.size())
				    definition.sections += ", ";

			    definition.sections += rootSignatureSection;

			    std::string line = std::format("cbuffer buffer_{0}_{1} : register(b{0}, space{1})", binding, entrySpace);
			    ctx.shader.replace(ctx.begin, ctx.end, line);
			    ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin) + line.size();
			}
		}

		void CBVRoot(std::string_view rootSigID, PreprocessorContext& ctx)
		{
			auto res = std::find_if(
				rootSignatures.begin(), rootSignatures.end(),
                [&](const RootSignatureDefinition& definition) -> bool
                {
					return definition.name == rootSigID;
                }
			);

			if (res != rootSignatures.end())
			{
				RootSignatureDefinition& definition = *res;
				uint32_t entrySpace = 0xfffffeff - definition.entries.size();
				uint32_t binding	= definition.entries.size();

				while (definition.IsSpaceInUse(binding))
					entrySpace--;

				definition.spacesInUse.push_back(entrySpace);
				definition.entries.push_back(RootSignatureEntryTypes::CBV);
				definition.entrySpace.push_back(entrySpace);

				std::string rootSignatureSection = std::format("CBV(b{0}, space={1}, visibility=SHADER_VISIBILITY_ALL, flags = DATA_STATIC_WHILE_SET_AT_EXECUTE)", binding, entrySpace);
				if(definition.sections.size())
					definition.sections += ", ";

				definition.sections += rootSignatureSection;

				std::string line = std::format("cbuffer buffer_{0}_{1} : register(b{0}, space{1})", binding, entrySpace);
			    ctx.shader.replace(ctx.begin, ctx.end, line);
				ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin) + line.size();
			}
			else
			{
				std::string_view error = "//root signature ID not defined!";
				ctx.shader.replace(ctx.begin, ctx.end, error);
				ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin) + error.size();
			}
		}

		void UAVStructuredRoot(std::string_view id, uint32_t binding, std::string_view type, PreprocessorContext& ctx)
		{
			if (rootSignatures.size() == 0)
				return;

			RootSignatureDefinition& definition = rootSignatures.back();

			auto checkBindingIsFree = [&]() -> bool
				{
					uint32_t idx = 0;
					for (const RootSignatureEntryTypes entry : definition.entries)
					{
						if (entry == RootSignatureEntryTypes::UAV)
						{
							if (binding == idx)
								return false;

							idx++;
						}
					}

					return true;
				};

			if (!checkBindingIsFree())
			{
				std::string_view error = "//Root signature binding index already in use!";
				ctx.shader.replace(ctx.begin, ctx.end, error);
				ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin) + error.size();
			}
			else
			{
				uint32_t entrySpace = 0xfffffeff - definition.entries.size();

				while (definition.IsSpaceInUse(entrySpace))
					entrySpace--;

				definition.spacesInUse.push_back(entrySpace);
				definition.entries.push_back(RootSignatureEntryTypes::CBV);
				definition.entrySpace.push_back(entrySpace);

				std::string rootSignatureSection = std::format("UAV(u{0}, space={1}, visibility=SHADER_VISIBILITY_ALL, flags = DATA_STATIC_WHILE_SET_AT_EXECUTE)", binding, entrySpace);
				if (definition.sections.size())
					definition.sections += ", ";

				definition.sections += rootSignatureSection;

				std::string line = std::format("RWStructuredBuffer<{2}> {3} : register(u{0}, space{1});", binding, entrySpace, type, id);
				ctx.shader.replace(ctx.begin, ctx.end, line);
				ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin) + line.size();
			}
		}

		// cbv in descriptor heap
		static void CBVBinding(uint32_t set, uint32_t binding, PreprocessorContext& ctx)
		{
			auto line = std::format("cbuffer constants_{0}_{1} : register(b{0}, space{1})", set, binding);
			ctx.shader.replace(ctx.begin, ctx.end, line);
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin) + line.size();
		}

		//static void InlineValues(scn::regex_matches id_match, uint32_t lineLength, uint32_t numValues, PreprocessorContext& ctx)
		void PushConstants(uint32_t lineLength, uint32_t numValues, PreprocessorContext& ctx)
		{
			std::string idStr = std::format("inline_{}", rand());
			//auto id = id_match.at(0).and_then([](auto m) { return std::optional<std::string>{m.get()}; }).value_or(std::string{ "" });

			static const std::regex structRegex{ R"(\{(\w|\d|\s|\;)*\};)" };

			uint32_t space = 0xffffff00;

			while (true)
			{
				TRYAGAIN:
				for (auto& rootSig : rootSignatures)
				{
					if (rootSig.IsSpaceInUse(space))
					{
						space--;
						goto TRYAGAIN;
					}
				}
				break;
			}

			std::string signatureSegment = std::format(" RootConstants(num32BitConstants={}, b0, space={})", numValues, space);

			for (auto& rootSig : rootSignatures)
			{
				rootSig.spacesInUse.push_back(space);
				if (rootSig.sections.size())
					rootSig.sections += ", ";

				rootSig.sections += signatureSegment;

				rootSig.entries.push_back(RootSignatureEntryTypes::Values);
				rootSig.entrySpace.push_back(space);
			}

			std::string replacement = std::format("cbuffer {} : register(b0, space{})", idStr, space);

			auto structBlock = std::sregex_iterator(
				ctx.shader.begin() + ctx.shaderOffset,
				ctx.shader.end(),
				structRegex);

			//auto pos = ctx.itr->position() + ctx.shaderOffset;
			auto pos = ctx.shaderOffset;
			auto posEnd = ctx.shaderOffset + lineLength;
			auto block = structBlock->position();

			if (posEnd + 2 >= block)
			{	// formatted correctly? Maybe?
				ctx.shader.replace(pos - lineLength, lineLength, replacement);
			}

			ctx.shaderOffset = pos - lineLength + replacement.size();
		}

		void LocalRootValues(uint32_t numValues, uint32_t binding, uint32_t space, PreprocessorContext& ctx)
		{
			auto& def = rootSignatures.back();
			def.sections += std::format(" RootConstants(num={}, b{}, space{})", numValues, binding, space);
			def.entries.push_back(RootSignatureEntryTypes::Values);
			def.entrySpace.push_back(space);

			ctx.shader.replace(ctx.begin, ctx.end, "");
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin);
		}


		void LocalRootValues(uint32_t numValues, std::string_view rootSigID, PreprocessorContext& ctx)
		{
			if (
			auto res = std::find_if(rootSignatures.begin(), rootSignatures.end(),
				[&](RootSignatureDefinition& def)
				{
					return def.name == rootSigID;
				}); res != rootSignatures.end())
			{
				auto& def = *res;
				uint32_t binding = def.entries.size();
				def.entries.push_back(RootSignatureEntryTypes::Values);

				uint32_t space = 0xffff0000;
				while (def.IsSpaceInUse(space))
					space--;

				def.sections += std::format(" RootConstants(num={}, b{}, space{})", numValues, binding, space);
				def.entrySpace.push_back(space);

				std::string idStr = std::format("localrootConsants_{}_{}", binding, space);
				std::string replacement = std::format("cbuffer {} : register(b0, space{})", idStr, space);
				ctx.shader.replace(ctx.begin, ctx.end, replacement);
				ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin) + replacement.size();
			}
			else
			{
				std::string errorLine = std::format("// Failed to find RootSignatureID: {}", rootSigID);
				ctx.shader.replace(ctx.begin, ctx.end, errorLine);
				ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin) + errorLine.size();
			}
		}


		static void Texture2D(uint32_t set, uint32_t binding, std::string_view type, PreprocessorContext& ctx)
		{
			auto line = std::format("Texture2D<{}> {} : register(s{}, space{})", type, rand(), set, binding);
			ctx.shader.replace(ctx.begin, ctx.end, line);
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin) + line.size();
		}


		static void Texture2DID(std::string_view ID, uint32_t set, uint32_t binding, std::string_view type, PreprocessorContext& ctx)
		{
			auto line = std::format("Texture2D<{}> {} : register(s{}, space{})", type, ID, set, binding);
			ctx.shader.replace(ctx.begin, ctx.end, line);
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin) + line.size();
		}


		void Texture2DBinding(std::string_view ID, uint32_t set, uint32_t binding, std::string_view type, PreprocessorContext& ctx)
		{
			auto& def			= rootSignatures.back();
			uint32_t space		= 0xffffff00;


			for (auto [e, s] : zip(def.entries, def.entrySpace))
			{
				if (e == RootSignatureEntryTypes::DescriptorHeap)
				{
					if (set == 0)
					{
						space = s;
						break;
					}
					else set--;
				}
			}

			auto line = std::format("Texture2D<{}> {} : register(t{}, space{});", type, ID, binding, space);
			ctx.shader.replace(ctx.begin, ctx.end, line);
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin) + line.size();
		}

		void Texture2DArray(std::string_view ID, uint32_t set, uint32_t binding, std::string_view type, PreprocessorContext& ctx)
		{
			auto& def = rootSignatures.back();
			uint32_t space = 0xffffff00;


			for (auto [e, s] : zip(def.entries, def.entrySpace))
			{
				if (e == RootSignatureEntryTypes::DescriptorHeap)
				{
					if (set == 0)
					{
						space = s;
						break;
					}
					else set--;
				}
			}

			auto line = std::format("Texture2D<{}> {}[] : register(t{}, space{});", type, ID, binding, space);
			ctx.shader.replace(ctx.begin, ctx.end, line);
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin) + line.size();
		}

		static void RWTexture2D(uint32_t set, uint32_t binding, std::string_view type, PreprocessorContext& ctx)
		{
			ctx.shader.replace(ctx.begin, ctx.end, "");
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin);
		}


		static void RWTexture3D(uint32_t set, uint32_t binding, std::string_view type, PreprocessorContext& ctx)
		{
			ctx.shader.replace(ctx.begin, ctx.end, "");
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin);
		}


		static void Texture3DBinding(std::string_view ID, uint32_t set, uint32_t binding, std::string_view type, PreprocessorContext& ctx)
		{
			ctx.shader.replace(ctx.begin, ctx.end, "");
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin);
		}


		static void CubeMap(uint32_t set, uint32_t binding, std::string_view type, PreprocessorContext& ctx)
		{
			ctx.shader.replace(ctx.begin, ctx.end, "");
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin);
		}


		static void StructuredBuffer(std::string_view id, uint32_t binding, uint32_t set, std::string_view type, PreprocessorContext& ctx)
		{
			ctx.shader.replace(ctx.begin, ctx.end, "");
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin);
		}
		

		static void StructuredBufferRoot(std::string_view id, uint32_t binding, std::string_view type, PreprocessorContext& ctx)
		{
			FK_ASSERT(false, "Unimplemented!");

			ctx.shader.replace(ctx.begin, ctx.end, "//Unimplemented!!");
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin + sizeof(R"(//Unimplemented!!)"));
		}


		void UAVByteBuffer(std::string_view id, PreprocessorContext& ctx)
		{
			auto& def = rootSignatures.back();

			uint32_t space = 0xffffff00;
			uint32_t binding = def.entries.size();
			def.entries.push_back(RootSignatureEntryTypes::UAV);;
			while (def.IsSpaceInUse(space))
				space--;
			def.entrySpace.push_back(space);
			def.spacesInUse.push_back(space);
			if (def.sections.size())
				def.sections += ", ";
			def.sections += std::format("UAV(u{}, space={})", binding, space);

			auto line = std::format("RWByteAddressBuffer {} : register(u{}, space{});", id, binding, space);
			ctx.shader.replace(ctx.begin, ctx.end, line);
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin) + line.size();
		}


		void DefineDescriptorSet(uint32_t num, std::vector<Tag> tags, PreprocessorContext& ctx)
		{
			std::flat_map<uint32_t, uint32_t>	spaces;
			uint32_t							offsetCurrent = 0;
			uint32_t							setCount = 0;

			constexpr auto SplitString =
				[](std::string_view target)
				{
					std::vector<std::string_view> stagesSplit;
					auto flagStart = target.begin();
					auto itr = target.find("|");

					if (itr == std::string_view::npos)
						stagesSplit.emplace_back(target);
					else
						for (; itr != std::string_view::npos;)
						{
							auto flag = std::string_view{ flagStart,  target.begin() + itr };
							stagesSplit.push_back(flag);

							auto next = target.find("|", itr + 1);
							if (next != std::string_view::npos)
							{
								flagStart = target.begin() + itr + 1;
								itr = next;
							}
							else
							{
								stagesSplit.push_back(std::string_view{ target.begin() + itr + 1,  target.end() });
								break;
							}
						}

					return stagesSplit;
				};

			auto ProcessArgs =
				[&](const Tag& tag, const std::string_view& bindingSpace, uint32_t s) -> std::string
				{
					std::string flagStr;
					std::string numStr;
					uint32_t	num = 0;

					std::vector<std::string> visibilityStr;

					for (const auto& arg : tag.arguments)
					{
						if (arg == "num=unbounded")
						{
							numStr = "numDescriptors = unbounded";
							num = 0xffffffff;
						}
						else if (auto res = scn::scan<uint32_t>(arg, "num={}"); res)
						{
							auto [numDescriptors] = res->values();
							numStr = std::format("numDescriptors = {}", numDescriptors);
							num += numDescriptors;
						}
						else if (auto res = scn::scan<std::string_view>(arg, "visibility={}"); res)
						{
							auto [visibilityInputStr] = res->values();
							auto visibilityStages = SplitString(visibilityInputStr);

							for (const auto& v : visibilityStages)
							{
								if (v == "All")
								{
									visibilityStr.emplace_back("SHADER_VISIBILITY_ALL");
								}
								else if (v == "vertex")
								{
									visibilityStr.emplace_back("SHADER_VISIBILITY_VERTEX");
								}
								else if (v == "hull")
								{
									visibilityStr.emplace_back("SHADER_VISIBILITY_HULL");
								}
								else if (v == "domain")
								{
									visibilityStr.emplace_back("SHADER_VISIBILITY_DOMAIN");
								}
								else if (v == "geometry")
								{
									visibilityStr.emplace_back("SHADER_VISIBILITY_GEOMETRY");
								}
								else if (v == "pixel")
								{
									visibilityStr.emplace_back("SHADER_VISIBILITY_PIXEL");
								}
								else if (v == "amplification")
								{
									visibilityStr.emplace_back("SHADER_VISIBILITY_AMPLIFICATION");
								}
								else if (v == "mesh")
								{
									visibilityStr.emplace_back("SHADER_VISIBILITY_MESH");
								}
							}
						}
						else if (auto res = scn::scan<std::string_view>(arg, "flags={}"); res)
						{
							auto [flagInputStr] = res->values();
							auto flags = SplitString(flagInputStr);

							for (const auto& f : flags)
							{
								if (f == "static")
								{
									if (flagStr.size())
										flagStr += "|";
									flagStr += "DATA_STATIC_WHILE_SET_AT_EXECUTE";
								}
								else if (f == "volatile")
								{
									if (flagStr.size())
										flagStr += "|";
									flagStr += "DATA_VOLATILE";
								}
								else if (f == "staticatexecution")
								{
									if (flagStr.size())
										flagStr += "|";
									flagStr += "DATA_STATIC_WHILE_SET_AT_EXECUTE";
								}
								else if (f == "descriptorvolatile")
								{
									if (flagStr.size())
										flagStr += "|";
									flagStr += "DESCRIPTORS_VOLATILE";
								}
								else
									std::print("Unrecognized Flag: {}\n", f);
							}
						}
					}

					if (offsetCurrent == 0xffffffff)
						return "";

					if (numStr.empty())
						return "";

					std::string type;
					if (bindingSpace == "b")
						type = "CBV";
					else if (bindingSpace == "t")
						type = "SRV";
					else if (bindingSpace == "u")
						type = "UAV";
					else if (bindingSpace == "s")
						type = "Sampler";


					std::string temp;
					temp += std::format("{}({}{}, {}", type, bindingSpace, offsetCurrent, numStr);

					offsetCurrent += num;
				    
#if 0
					uint32_t set = setCount++;
					for (auto& spacesInUse = rootSignatures.back().spacesInUse;;)
					{
						if (std::ranges::find(spacesInUse, set) == spacesInUse.end())
						{
							spacesInUse.push_back(set);
							break;
						}
						else
							set++;
					}

					if (set != 0)
					{
						temp += std::format(", space = {}", set);
					}
#endif

					temp += std::format(", space = {}", s);


					if (!flagStr.empty())
						temp += std::format(", {}", flagStr);

					if (visibilityStr.empty())
					{
						return temp + ") ";
					}
					else
					{
						std::string out;
						for (auto& v : visibilityStr)
							out += (out.size() ? ", " : "") + temp + std::format(", visibility = {})", v);

						return out;
					}
				};

			auto& definition = rootSignatures.back();

			uint32_t space = 0xffffff00;
			while (definition.IsSpaceInUse(space))
				space--;

		    std::string sections;
			for (auto& tag : tags)
			{
			    if (std::string_view{ tag.idBegin, tag.idEnd } == "CBV")
			    {
					auto res = ProcessArgs(tag, "b", space);

					if (sections.size())
						sections += ", ";

					sections += res;
			    }
				else if (std::string_view{ tag.idBegin, tag.idEnd } == "SRVTexture")
				{
					auto res = ProcessArgs(tag, "t", space);

					if (sections.size())
						sections += ", ";

					sections += res;
				}
				else if (std::string_view{ tag.idBegin, tag.idEnd } == "SRVBuffer")
				{
					auto res = ProcessArgs(tag, "t", space);

					if (sections.size())
						sections += ", ";

					sections += res;
				}
				else if (std::string_view{ tag.idBegin, tag.idEnd } == "SRVStructured")
				{
					auto res = ProcessArgs(tag, "t", space);

					if (sections.size())
						sections += ", ";

					sections += res;
				}
				else if (std::string_view{ tag.idBegin, tag.idEnd } == "UAVBuffer")
				{
					auto res = ProcessArgs(tag, "u", space);

					if (sections.size())
						sections += ", ";

					sections += res;
				}
				else if (std::string_view{ tag.idBegin, tag.idEnd } == "UAVTexture")
				{
					auto res = ProcessArgs(tag, "u", space);

					if (sections.size())
						sections += ", ";

					sections += res;
				}
				else if (std::string_view{ tag.idBegin, tag.idEnd } == "UAVStructured")
				{
					auto res = ProcessArgs(tag, "u", space);

					if (sections.size())
						sections += ", ";

					sections += res;
				}
				else if (std::string_view{ tag.idBegin, tag.idEnd } == "Sampler")
				{
					auto res = ProcessArgs(tag, "s", space);

					if (sections.size())
						sections += ", ";

					sections += res;
				}
			}

			definition.entrySpace.push_back(space);
			definition.spacesInUse.push_back(space);
			definition.entries.push_back(RootSignatureEntryTypes::DescriptorHeap);
			if (definition.sections.size())
				definition.sections += ", ";

			definition.sections += std::format(" DescriptorTable({})", sections);

			ctx.shader.replace(ctx.begin, ctx.end, "");
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin);
		}


		void BeginRootSig(std::string_view ID, PreprocessorContext& ctx)
		{
			rootSignatures.push_back(RootSignatureDefinition{ .name = std::string{ ID } });

			ctx.shader.replace(ctx.begin, ctx.end, "");
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin);
		}


		void RootFlags(std::string_view flags, PreprocessorContext& ctx)
		{
			if (rootSignatures.size())
			{
				auto flagStart = flags.begin();
				auto itr = flags.find("|");

				if (itr == std::string_view::npos)
					rootSignatures.back().flags.emplace_back(flags);
				else
				    for (; itr != std::string_view::npos;)
				    {
					    auto flag = std::string_view{ flagStart,  flags.begin() + itr };
					    rootSignatures.back().flags.emplace_back(flag);

					    auto next = flags.find("|", itr + 1);
						if (next != std::string_view::npos)
						{
							flagStart = flags.begin() + itr + 1;
							itr = next;
						}
						else
						{
							rootSignatures.back().flags.emplace_back(std::string_view{ flags.begin() + itr + 1,  flags.end() });
							break;
						}
				    }
			}

			ctx.shader.replace(ctx.begin, ctx.end, "");
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin);
		}


		void RootSignature(std::string_view signatureID, PreprocessorContext& ctx) const
		{
			for (const auto& rootSigDef : rootSignatures)
			{
				if (rootSigDef.name == signatureID)
				{
					std::string rootSignature = rootSigDef.GetSignatureDefinition();
					std::string line;

					if (rootSignature.size())
					{
						line = std::format(R"([RootSignature("{}")])", rootSignature);

					}
					else
					    line = std::format(R"(// !!ROOT SIGNATURE {} NOT DEFINED!!";)", signatureID);

					ctx.shader.replace(ctx.begin, ctx.end, line);
					ctx.shaderOffset += line.size();
					return;
				}
			}

			ctx.shader.replace(ctx.begin, ctx.end, "");
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin);
		}


		void LocalRootSignature(std::string_view signatureID, PreprocessorContext& ctx)
		{
			for (auto& rootSigDef : rootSignatures)
			{
				if (rootSigDef.name == signatureID)
				{
					rootSigDef.local = true;

					std::string rootSignature = rootSigDef.GetSignatureDefinition();
					std::string line;

					if (rootSignature.size())
					{
						line = std::format(R"([LocalRootSignature("{}")])", rootSignature);

					}
					else
						line = std::format(R"(// !!ROOT SIGNATURE {} NOT DEFINED!!";)", signatureID);

					ctx.shader.replace(ctx.begin, ctx.end, line);
					ctx.shaderOffset += line.size();
					return;
				}
			}

			ctx.shader.replace(ctx.begin, ctx.end, "");
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin);
		}


		void DefineRootSignature(std::string_view signatureID, PreprocessorContext& ctx) const
		{
			for (const auto& rootSigDef : rootSignatures)
			{
				if (rootSigDef.name == signatureID)
				{
					std::string rootSignature = rootSigDef.GetSignatureDefinition();
					std::string line;

					if (rootSignature.size())
					{
						line = std::format(R"(#define {} "{}")", signatureID, rootSignature);
					}
					else
						line = std::format(R"(// !!ROOT SIGNATURE {} NOT DEFINED!!";)", signatureID);

					ctx.shader.replace(ctx.begin, ctx.end, line);
					ctx.shaderOffset += line.size();
					return;
				}
			}

			ctx.shader.replace(ctx.begin, ctx.end, "");
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin);
		}


		static void AccelerationStructure(std::string_view id, uint32_t binding, uint32_t set, PreprocessorContext& ctx)
		{
			auto line = std::format("RaytracingAccelerationStructure {} : register(t{}, space{});", id, binding, set);
			ctx.shader.replace(ctx.begin, ctx.end, line);
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin) + line.size();
		}

		enum class RootSignatureEntryTypes
		{
			Values, CBV, SRV, UAV, DescriptorHeap
		};


		struct RootSignatureDefinition
		{
			std::vector<RootSignatureEntryTypes>	entries;
			std::vector<uint32_t>					entrySpace;
			std::vector<std::string>				flags;
			std::vector<uint32_t>					spacesInUse;

			std::string	sections;
			std::string	name;

			bool local = false;

			std::string GetSignatureDefinition() const
			{
				std::string out;

				if (flags.size())
				{
					std::string flagList = flags.front();
				    for (auto& f : std::span{ flags.begin() + 1, flags.end() })
					    flagList += (flagList.empty() ? "" : "| ") + f;

					out += std::format("RootFlags({})", flagList);
			    }	

				if (out.size())
					out += ", ";

				out += sections;


				if (out.size())
					out += ", ";
				out += R"(StaticSampler(s0, filter = FILTER_MIN_MAG_MIP_POINT), StaticSampler(s1, filter = FILTER_MIN_MAG_POINT_MIP_LINEAR))";

				return out;
			}

			bool IsSpaceInUse(uint32_t space) const
			{
				auto res = std::ranges::find(spacesInUse, space);

				return (res != spacesInUse.end());
			}
		};

		std::vector<RootSignatureDefinition> rootSignatures;
	};

	PreprocessorResult VKShaderProprocessor(std::string& shader, const SHADER_TYPE type, iAllocator& allocator)
	{
		return ShaderProprocessor(shader, type, VulkanAttributeHandler{}, allocator);
	}

	PreprocessorResult DXShaderProprocessor(std::string& shader, const SHADER_TYPE type, iAllocator& allocator)
	{
		auto handler = DirectXAttributeHandler{ .rootSignatures{} };
		auto res = ShaderProprocessor(shader, type, handler, allocator);

		for (auto& def : handler.rootSignatures)
		{
			if (def.local)
			{
				auto str = def.GetSignatureDefinition();
				res.attributes.emplace_back(
					ShaderAttributeLocalRootSignature{
						.id = def.name, 
						.rootSigDefinition = str
					});
			}
		}

		return res;
	}
}


