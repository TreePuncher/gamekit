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
			if (auto res = scn::scan<uint32_t, uint32_t, uint32_t>(match, R"([[fk::Values(num={}, binding={}, set={})]])"); res)
			{
				ctx.shaderOffset = endres;

				auto [num, set, binding] = res->values();
				handler.CBVBinding(set, binding, ctx);

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
			else if (auto res = scn::scan<uint32_t, uint32_t, std::string_view>(match, R"([[fk::StructuredBuffer(set={}, binding={}, type={})]])"); res)
			{
				ctx.shaderOffset = endres;

				auto [set, binding, type] = res->values();
				handler.StructuredBuffer(set, binding, type, ctx);

				continue;
			}
			else if (auto res = scn::scan<std::string_view, uint32_t, uint32_t, std::string_view>(match, R"([[fk::Texture2D(ID={}, set={}, binding={}, type={})]])"); res)
			{
				ctx.shaderOffset = endres;
				
			    auto [ID, set, binding, type] = res->values();
				handler.Texture2DBinding(ID, set, binding, type, ctx);

				continue;
			}
			else if (auto res = scn::scan<std::string_view, uint32_t, uint32_t, std::string_view>(match, R"([[fk::Texture3D(ID={}, set={}, binding={}, type={})]])"); res)
			{
				ctx.shaderOffset = endres;
				
			    auto [ID, set, binding, type] = res->values();
				handler.Texture3DBinding(ID, set, binding, type, ctx);

				continue;
			}
			else if (auto res = scn::scan<uint32_t>(match, R"([[fk::CBV(set={})]])"); res)
			{
				ctx.shaderOffset = endres;
				
			    auto [binding] = res->values();
				handler.CBVPushBuffer(binding, ctx);

				continue;
			}
			//else if (auto res = scn::scan<scn::regex_matches, uint32_t>(match, R"([[fk::InlineValues(id="{:/(\w|\d)*/}", num={})]])"); res)
			else if (auto res = scn::scan<uint32_t>(match, R"([[fk::PushConstants(num={})]])"); res)
			{
				ctx.shaderOffset = endres;
			    
			    auto [num] = res->values();
				handler.PushConstants(match.size(), num, ctx);

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

			shader.erase(res, endres - res);
		}

		return result;
	}

	struct VulkanAttributeHandler
	{
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

		// cbv in descriptor heap
		static void CBVBinding(uint32_t set, uint32_t binding, PreprocessorContext& ctx)
		{
			auto line = std::format("cbuffer constants_{0}_{1} : register(b{0}, space{1})", set, binding);
			ctx.shader.replace(ctx.begin, ctx.end, line);
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin) + line.size();
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

		static void StructuredBuffer(uint32_t binding, uint32_t set, std::string_view type, PreprocessorContext& ctx)
		{
			ctx.shader.replace(ctx.begin, ctx.end, "");
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin);
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
	};


	struct DirectXAttributeHandler
	{
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
					.id				= id
				});
		}


		// cbv in descriptor heap
		static void CBVBinding(uint32_t set, uint32_t binding, PreprocessorContext& ctx)
		{
			auto line = std::format("cbuffer constants_{0}_{1} : register(b{0}, space{1})", set, binding);
			ctx.shader.replace(ctx.begin, ctx.end, line);
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin) + line.size();
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


		static void StructuredBuffer(uint32_t binding, uint32_t set, std::string_view type, PreprocessorContext& ctx)
		{
			ctx.shader.replace(ctx.begin, ctx.end, "");
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin);
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
				[&](const Tag& tag, const std::string_view& bindingSpace) -> std::string
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


		    std::string sections;
			for (auto& tag : tags)
			{
			    if (std::string_view{ tag.idBegin, tag.idEnd } == "CBV")
			    {
					auto res = ProcessArgs(tag, "b");

					if (sections.size())
						sections += ", ";

					sections += res;
					int x = 0;
			    }
				else if (std::string_view{ tag.idBegin, tag.idEnd } == "SRVTextured")
				{
					auto res = ProcessArgs(tag, "t");

					if (sections.size())
						sections += ", ";

					sections += res;
					int x = 0;
				}
				else if (std::string_view{ tag.idBegin, tag.idEnd } == "SRVBuffer")
				{
					auto res = ProcessArgs(tag, "t");

					if (sections.size())
						sections += ", ";

					sections += res;
					int x = 0;
				}
				else if (std::string_view{ tag.idBegin, tag.idEnd } == "SRVStructured")
				{
					auto res = ProcessArgs(tag, "t");

					if (sections.size())
						sections += ", ";

					sections += res;
					int x = 0;
				}
				else if (std::string_view{ tag.idBegin, tag.idEnd } == "UAVBuffer")
				{
					auto res = ProcessArgs(tag, "u");

					if (sections.size())
						sections += ", ";

					sections += res;
					int x = 0;
				}
				else if (std::string_view{ tag.idBegin, tag.idEnd } == "UAVTexture")
				{
					auto res = ProcessArgs(tag, "u");

					if (sections.size())
						sections += ", ";

					sections += res;
					int x = 0;
				}
				else if (std::string_view{ tag.idBegin, tag.idEnd } == "UAVStructured")
				{
					auto res = ProcessArgs(tag, "u");

					if (sections.size())
						sections += ", ";

					sections += res;
					int x = 0;
				}
				else if (std::string_view{ tag.idBegin, tag.idEnd } == "Sampler")
				{
					auto res = ProcessArgs(tag, "s");

					if (sections.size())
						sections += ", ";

					sections += res;
					int x = 0;
				}
			}

			rootSignatures.back().sections += std::format(" DescriptorTable({})", sections);

			ctx.shader.replace(ctx.begin, ctx.end, "");
			ctx.shaderOffset = std::distance(ctx.shader.begin(), ctx.begin);
		}


		//static void InlineValues(scn::regex_matches id_match, uint32_t lineLength, uint32_t numValues, PreprocessorContext& ctx)
		void PushConstants(uint32_t lineLength, uint32_t numValues, PreprocessorContext& ctx)
		{
			std::string idStr = std::format("inline_{}", rand());
			//auto id = id_match.at(0).and_then([](auto m) { return std::optional<std::string>{m.get()}; }).value_or(std::string{ "" });

			static const std::regex structRegex{ R"(\{(\w|\d|\s|\;)*\};)" };

			uint32_t space;

			while (true)
			{
				space = rand();
				for (auto& rootSig : rootSignatures)
				{
					auto res = std::ranges::find(rootSig.spacesInUse, space);

					if (res != rootSig.spacesInUse.end())
						continue;
				}

 				std::string signatureSegment = std::format(" RootConstants(num32BitConstants={}, b0, space={})", numValues, space);

				for (auto& rootSig : rootSignatures)
				{
					rootSig.spacesInUse.push_back(space);
					if (rootSig.sections.size())
						rootSig.sections += ", ";

					rootSig.sections += signatureSegment;
				}
				break;
			}

			std::string replacement = std::format("cbuffer {} : register(b0, space{})", idStr, space);

			auto structBlock = std::sregex_iterator(
				ctx.shader.begin() + ctx.shaderOffset,
				ctx.shader.end(),
				structRegex);

			//auto pos = ctx.itr->position() + ctx.shaderOffset;
			auto pos	= ctx.shaderOffset;
			auto posEnd = ctx.shaderOffset + lineLength;
			auto block	= structBlock->position();
			
			if (posEnd + 2 >= block)
			{	// formatted correctly? Maybe?
				ctx.shader.replace(pos - lineLength, lineLength, replacement);
			}
			
			ctx.shaderOffset = pos - lineLength + replacement.size();
		}

		void LocalRootValues(uint32_t numValues, uint32_t binding, uint32_t space, PreprocessorContext& ctx)
		{
			rootSignatures.back().sections += std::format(" RootConstants(num={}, b{}, space{})", numValues, binding, space);

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

		void LocalRootSignature(std::string_view signatureID, PreprocessorContext& ctx) const
		{
			for (const auto& rootSigDef : rootSignatures)
			{
				if (rootSigDef.name == signatureID)
				{
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


		struct RootSignatureDefinition
		{
			std::vector<std::string>	flags;
			std::vector<uint32_t>		spacesInUse;
			std::string	sections;
			std::string	name;

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

				return out;
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
		return ShaderProprocessor(shader, type, DirectXAttributeHandler{ .rootSignatures{} }, allocator);
	}
}


