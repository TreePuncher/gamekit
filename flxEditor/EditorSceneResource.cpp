#include "PCH.h"
#include "EditorSceneResource.h"
#include "ComponentBlobs.h"
#include "MeshUtils.h"
#include "EditorSceneEntityComponents.h"
#include "EditorTextureResources.h"
#include "MeshResource.h"
#include "ResourceUtilities.h"

#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_INCLUDE_JSON

#include <nlohmann\json.hpp>
#include <fstream>
#include <memory>
#include <sstream>
#include <regex>
#include <filesystem>
#include <fmt\format.h>
#include <stb_image.h>
#include <stb_image_write.h>
#include <tiny_gltf.h>
#include <ranges>

namespace FlexKit
{	/************************************************************************************************/

	using std::views::zip;
	using std::views::iota;

	std::optional<MeshResource_ptr> ExtractGeometry(tinygltf::Mesh& mesh)
	{
		return {};
	}

	float3 Rejection(float3 lhs, float3 rhs)
	{
		return lhs.dot(rhs) * rhs - lhs;
	}

	void GenerateTangents(auto indices, auto points, auto normals, auto uvCoords, auto& tangentsOut)
	{
		tangentsOut.clear();

		std::vector<float3> tangentBuffer;
		std::vector<float3> bitangentBuffer;
		tangentBuffer.resize(normals.size());
		bitangentBuffer.resize(normals.size());
		memset(tangentBuffer.data(), 0, tangentBuffer.size() * sizeof(float3));
		memset(bitangentBuffer.data(), 0, bitangentBuffer.size() * sizeof(float3));

		for (auto itr = indices.begin(); itr < indices.end(); itr+= 3)
		{
			const uint32_t idx0 = *(itr + 0);
			const uint32_t idx1 = *(itr + 1);
			const uint32_t idx2 = *(itr + 2);

			const float3 p0 = points[idx0];
			const float3 p1 = points[idx1];
			const float3 p2 = points[idx2];

			const float2 w0 = uvCoords[idx0];
			const float2 w1 = uvCoords[idx1];
			const float2 w2 = uvCoords[idx2];

			const float3 e1 = p1 - p0;
			const float3 e2 = p2 - p0;

			const float x1 = w1.x - w0.x;
			const float x2 = w2.x - w0.x;

			const float y1 = w1.y - w0.y;
			const float y2 = w2.y - w0.y;

			const float r = 1.0f / (x1 * y2 - x2 * y1);
			const float3 tangent	= (e1 * y2 - e2 * y1) * r;
			const float3 bitangent	= (e2 * x1 - e1 * x2) * r;

			tangentBuffer[idx0] += tangent;
			tangentBuffer[idx1] += tangent;
			tangentBuffer[idx2] += tangent;
			bitangentBuffer[idx0] += bitangent;
			bitangentBuffer[idx1] += bitangent;
			bitangentBuffer[idx2] += bitangent;
		}

		for (auto&& [t, b, n] : zip(tangentBuffer, bitangentBuffer, normals))
		{
			const auto rejected = Rejection(t, n).normal();
			const auto dp = t.cross(b).dot(n);

			t = rejected * (dp > 0.0f ? 1.0f : -1.0f);
		}

		tangentsOut.reserve(tangentBuffer.size());

		for (float3& t : tangentBuffer)
			tangentsOut.push_back(t);
	}


	/************************************************************************************************/


	std::pair<ResourceList, std::vector<size_t>>  GatherGeometry(tinygltf::Model& model)
	{
		using namespace tinygltf;

		ResourceList		resources;
		std::vector<size_t>	meshMap;

		for (auto& mesh : model.meshes)
		{
			uint64_t GUID = rand();
			std::string LOD;

			if (mesh.extras.Has("internal"))
			{
				meshMap.push_back(-1);
				continue;
			}

			if (mesh.extras.Has("ResourceID"))
			{
				auto& resourceID = mesh.extras.Get("ResourceID");
				if (resourceID.IsInt())
					GUID = resourceID.Get<int>();
			}

			if (mesh.extras.Has("LOD"))
			{
				auto& LODproperty = mesh.extras.Get("LOD");
				if (LODproperty.IsString())
					LOD = LODproperty.Get<std::string>();
			}

			if (auto res = std::find_if(
				std::begin(resources),
				std::end(resources),
				[&](Resource_ptr& resource)
				{
					return resource->GetResourceGUID() == GUID;
				}); res != std::end(resources))
			{
				meshMap.push_back((*res)->GetResourceGUID());
				continue;
			}

			std::vector<tinygltf::Mesh*> lodSources;
			// Fetch LOD's

			auto fetchLOD =
				[&](const std::string lodMeshName, auto _THIS) -> void
				{
					for (auto& mesh : model.meshes)
					{
						if (mesh.name == lodMeshName)
						{
							lodSources.push_back(&mesh);

							if (mesh.extras.Has("LOD"))
							{
								auto LODid = mesh.extras.Get("LOD").Get<std::string>();
								_THIS(LODid, _THIS);
							}
						}
					}
				};

			lodSources.push_back(&mesh);

			if(LOD.size())
				fetchLOD(LOD, fetchLOD);

			for (auto& source : lodSources)
			{
				if (source->primitives.size() != lodSources[0]->primitives.size())
				{
					std::cout << "Invalid Input LOD";
					return {};
				}
			}

			std::vector<LODLevel>		lodLevels;
			std::vector<std::string>	morphTargetNames;

			for(auto sourceMesh : lodSources)
			{
				auto&	primitives = sourceMesh->primitives;
				auto	meshTokens = FlexKit::MeshUtilityFunctions::TokenList{ SystemAllocator };

				struct XYZ {
					float x;
					float y;
					float z;

					XYZ() = default;

					XYZ(const float3& xyz)
					{
						x = xyz.x;
						y = xyz.y;
						z = xyz.z;
					}

					XYZ& operator = (const float3& xyz)
					{
						x = xyz.x;
						y = xyz.y;
						z = xyz.z;

						return *this;
					}

					operator float3 () const { return { x, y, z }; }
				};

				struct UV {
					float x;
					float y;

					operator float2 () const { return { x, y };		}
					operator float3 () const { return { x, y, 0 };	}
				};


				LODLevel lod;
				for(auto& primitive : primitives)
				{
					MeshDesc newMesh;

					std::span<uint32_t>			indices;
					std::span<XYZ>				points;
					std::span<XYZ>				normals;
					std::span<float4>			tangents;
					std::vector<std::span<UV>>	UVChannels;

					for(auto& attribute : primitive.attributes)
					{
						auto& bufferAcessor	= model.accessors[attribute.second];
						auto& bufferView	= model.bufferViews[bufferAcessor.bufferView];
						auto* buffer		= model.buffers[bufferView.buffer].data.data() + bufferView.byteOffset;

						auto stride			= bufferView.byteStride == 0 ? GetComponentSizeInBytes(bufferAcessor.componentType) * GetNumComponentsInType(bufferAcessor.type) : bufferView.byteStride;
						auto elementCount	= bufferView.byteLength / stride;

						static const std::regex texcoordPattern		{ R"(TEXCOORD_[0-9]+)" };
						static const std::regex jointPattern		{ R"(JOINTS_[0-9]+)" };

						if (attribute.first == "POSITION")
						{							
							for (size_t i = 0; i < bufferAcessor.minValues.size(); i++)
							{
								newMesh.MinV[i] = (float)bufferAcessor.minValues[i];
								newMesh.MaxV[i] = (float)bufferAcessor.maxValues[i];
							}

							points = { (XYZ*)buffer, elementCount };
						}
						else if (attribute.first == "TANGENT")
						{
							//tangents = { (float4*)buffer, elementCount };
						}
						else if (attribute.first == "NORMAL")
						{
							normals = { (XYZ*)buffer, elementCount };
						}
						else if (std::regex_search(attribute.first, texcoordPattern))
						{
							newMesh.UV = true;

							uint32_t idx = 0; 
							static const std::regex idx_pattern{ R"([0-9]+)" };

							std::smatch results;
							auto res = std::regex_search(
								attribute.first,
								results, 
								idx_pattern);


							idx = res ? std::atoi(results.begin()->str().c_str()) : 0;

							UVChannels.emplace_back((UV*)buffer, elementCount);
						}
						else if (std::regex_search(attribute.first, jointPattern))
						{
							newMesh.Weights = true;

							uint32_t idx = 0; 
							static const std::regex idx_pattern{ R"([0-9]+)" };

							std::smatch results;
							auto res = std::regex_search(
								attribute.first,
								results, 
								idx_pattern);

							idx = res ? std::atoi(results.begin()->str().c_str()) : 0;

							auto& weightsAcessor	= model.accessors[primitive.attributes[fmt::format("WEIGHTS_{0}", idx)]];
							auto& weightsView		= model.bufferViews[weightsAcessor.bufferView];
							auto* weightsbuffer		= model.buffers[weightsView.buffer].data.data() + weightsView.byteOffset;

							const auto weightsStride	= weightsView.byteStride == 0 ? GetComponentSizeInBytes(weightsAcessor.componentType) * GetNumComponentsInType(weightsAcessor.type) : weightsView.byteStride;

							for (size_t I = 0; I < elementCount; I++)
							{
								uint4_16	joints;
								float4		weights;


								if(stride == 4)
								{
									uint8_t joints_small[4];
									memcpy(joints_small, buffer + stride * I, 4);

									joints[0] = joints_small[0];
									joints[1] = joints_small[1];
									joints[2] = joints_small[2];
									joints[3] = joints_small[3];
								}
								else if (stride == 8)
									memcpy(&joints, buffer + stride * I, stride);

								memcpy(&weights, weightsbuffer + weightsStride * I, weightsStride);

								const float sum = weights[0] + weights[1] + weights[2] + weights[3];

								MeshUtilityFunctions::OBJ_Tools::AddWeightToken({ weights.xyz(), joints }, meshTokens);
							}
						}

					}

					auto& indexAccessor		= model.accessors[primitive.indices];
					auto& indexBufferView	= model.bufferViews[indexAccessor.bufferView];
					auto& indexBuffer		= model.buffers[indexBufferView.buffer];
					 
					const auto indexComponentType	= indexAccessor.componentType;
					const auto indexType			= indexAccessor.type;

					const auto indexStride	= indexBufferView.byteStride == 0 ? GetComponentSizeInBytes(indexComponentType) * GetNumComponentsInType(indexType) : indexBufferView.byteStride;
					const auto indexCount	= indexBufferView.byteLength / indexStride;

					auto* buffer = model.buffers[indexBufferView.buffer].data.data() + indexBufferView.byteOffset;


					std::vector<uint32_t>	uint32Indices;
					std::vector<float4>		generatedTangents;

					if (indexStride == 2)
					{
						std::span<uint16_t> uint16Indices{ (uint16_t*)buffer, indexCount };
						std::ranges::copy(uint16Indices, std::back_inserter(uint32Indices));

						indices = uint32Indices;
					}
					else if(indexStride == 4)
						indices = { (uint32_t*)buffer, indexCount };


					if (!tangents.size() && normals.size() && UVChannels.size()) // Generate Tangents
					{
						GenerateTangents(indices, points, normals, UVChannels[0], generatedTangents);
						tangents = generatedTangents;
					}

					for (auto& point : points)
						MeshUtilityFunctions::OBJ_Tools::AddVertexToken(point, meshTokens);

					if (normals.size() && tangents.size())
					{
						for (auto&& [normal, tangent] : zip(normals, tangents))
						{
							auto tangentAdjusted = (tangent.w != 0) ? float4{ tangent.xyz() * tangent.w, tangent.w } : tangent;

							MeshUtilityFunctions::OBJ_Tools::AddNormalToken(normal, tangentAdjusted.xyz(), meshTokens);
						}
					}

					for (auto&& [idx, uvs] : zip(iota(0), UVChannels))
						for(auto uv : uvs)
							MeshUtilityFunctions::OBJ_Tools::AddTexCordToken(uv, idx, meshTokens);

					size_t					morphTargetCount = 0;
					uint32_t				morphTargetVertexCount = 0;
					std::vector<uint32_t>	morphTargetStart;

					for(auto& morphChannels : primitive.targets)
					{
						auto position		= morphChannels.find("POSITION");
						auto tangent		= morphChannels.find("TANGENT");
						auto normal			= morphChannels.find("NORMAL");

						morphTargetStart.push_back(morphTargetVertexCount);

						std::span<XYZ>		morphPointSpan;
						std::span<XYZ>		morphNormalSpan;// { (XYZ*)normalBuffer, normalElementCount };
						std::span<float4>	morphTangentSpan;// = tangentbuffer != nullptr ? std::span<XYZ>{ (XYZ*)normalBuffer, normalElementCount } : std::span<XYZ>{};

						auto& positionAcessor		= model.accessors[position->second];
						auto& positionView			= model.bufferViews[positionAcessor.bufferView];
						auto* positionBuffer		= model.buffers[positionView.buffer].data.data() + positionView.byteOffset;

						auto& normalAcessor			= model.accessors[normal->second];
						auto& normalView			= model.bufferViews[normalAcessor.bufferView];
						auto* normalBuffer			= model.buffers[normalView.buffer].data.data() + normalView.byteOffset;

						auto positionStride			= positionView.byteStride == 0 ? GetComponentSizeInBytes(positionAcessor.componentType) * GetNumComponentsInType(positionAcessor.type) : positionView.byteStride;
						auto positionElementCount	= positionView.byteLength / positionStride;
						morphPointSpan = { (XYZ*)positionBuffer, positionElementCount };;

						auto normalStride			= normalView.byteStride == 0 ? GetComponentSizeInBytes(normalAcessor.componentType) * GetNumComponentsInType(normalAcessor.type) : normalView.byteStride;
						auto normalElementCount		= normalView.byteLength / normalStride;
						morphNormalSpan = { (XYZ*)normalBuffer, normalElementCount };

						if (tangent != morphChannels.end())
						{
							auto& tangentAcessor		= model.accessors[tangent->second];
							auto& tangentView			= model.bufferViews[tangentAcessor.bufferView];
							auto* tangentbuffer			= model.buffers[tangentView.buffer].data.data() + tangentView.byteOffset;

							auto tangentStride			= tangentView.byteStride == 0 ? GetComponentSizeInBytes(tangentAcessor.componentType) * GetNumComponentsInType(tangentAcessor.type) : tangentView.byteStride;
							auto tangentElementCount	= tangentView.byteLength / tangentStride;
							morphTangentSpan = std::span<float4>{ (float4*)tangentbuffer, tangentElementCount };
						}

						std::vector<float4> generatedMorphTangents;

						if (!morphTangentSpan.size())
						{	// Generate Tangents
							std::vector<float3>	morphPoints;
							std::vector<float3>	morphNormals;

							morphPoints.reserve(points.size());
							morphPoints.reserve(normals.size());

							std::ranges::copy(points, std::back_inserter(morphPoints));
							std::ranges::copy(normals, std::back_inserter(morphNormals));

							for (auto&& [mp, p, mn, n] : zip(morphPoints, morphPointSpan, morphNormals, morphNormalSpan))
							{
								//mp += p;
								//mn += n;
							}

							GenerateTangents(indices, morphPoints, morphNormals, UVChannels[0], generatedMorphTangents);

							for (auto&& [mt, t] : zip(generatedMorphTangents, tangents))
							{
								auto tangentAdjusted = (t.w != 0) ? float4{ t.xyz() * t.w, 0 } : t;

								mt = float4{ mt.xyz(), 0 } - tangentAdjusted;
								//mt = float4{ mt.xyz() * mt.w, 0 } - float4{ t.xyz() * t.w, 0 };
							}

							morphTangentSpan = generatedMorphTangents;
						}

						for (auto&& [position, normal, tangent] : zip(morphPointSpan, morphNormalSpan, morphTangentSpan))
						{
							const MorphTargetVertexToken token{
								.position	= position,
								.normal		= normal,
								.tangent	= tangent,
								.morphIdx	= (uint32_t)morphTargetCount,
							};

							meshTokens.push_back(token);
						}

						morphTargetVertexCount += normalElementCount;
						morphTargetCount++;
					}

					newMesh.Normals		= normals.size() != 0;
					newMesh.Tangents	= tangents.size() != 0;

					for (size_t I = 0; I < indexCount; I+=3)
					{
						VertexToken tokens[3];

						for(size_t II = 0; II < 3; II++)
						{
							uint32_t idx = 0;
							memcpy(&idx, buffer + indexStride * (I + II), indexStride);

							tokens[II].vertex.push_back(VertexField{ idx, VertexField::Point });
							tokens[II].vertex.push_back(VertexField{ idx, VertexField::Normal });


							if (newMesh.Tangents)
								tokens[II].vertex.push_back(VertexField{ idx, VertexField::Tangent });

							if (newMesh.UV)
								tokens[II].vertex.push_back(VertexField{ idx, VertexField::TextureCoordinate });

							if (newMesh.Weights) {
								tokens[II].vertex.push_back(VertexField{ idx, VertexField::JointIndex });
								tokens[II].vertex.push_back(VertexField{ idx, VertexField::JointWeight });
							}

							for (uint32_t morphIdx = 0; morphIdx < morphTargetCount; morphIdx++)
								tokens[II].vertex.push_back(VertexField{ morphTargetStart[morphIdx] + idx, VertexField::MorphTarget});

							std::sort(
								tokens[II].vertex.begin(), tokens[II].vertex.end(),
								[&](VertexField& lhs, VertexField& rhs)
								{
									return lhs.type < rhs.type;
								});
						}

						meshTokens.push_back(tokens[0]);
						meshTokens.push_back(tokens[2]);
						meshTokens.push_back(tokens[1]);
					}

					newMesh.tokens		= std::move(meshTokens);
					newMesh.faceCount	= indexCount / 3;
					
					lod.subMeshs.emplace_back(std::move(newMesh));
				}

				lodLevels.emplace_back(std::move(lod));
			}

			auto meshResource		= CreateMeshResource(lodLevels, mesh.name, {}, false);
			meshResource->TriMeshID	= GUID;

			auto& extraValues = mesh.extras.Get<tinygltf::Value::Object>();
			auto& targetNames = extraValues["targetNames"].Get<tinygltf::Value::Array>();

			for (size_t I = 0; I < meshResource->data->morphTargetBuffers.size(); I++)
				meshResource->data->morphTargetBuffers[I].name = targetNames[I].Get<std::string>();

			resources.emplace_back(std::move(meshResource));

			meshMap.push_back(GUID);
		}

		return { resources, meshMap };
	}


	/************************************************************************************************/


	ResourceList GatherScenes(tinygltf::Model& model, std::vector<size_t>& meshMap, std::map<int, Resource_ptr>& imageMap, std::vector<Resource_ptr>& skinMap, const gltfImportOptions& options)
	{
		using namespace tinygltf;

		ResourceList resources;

		for (auto& scene : model.scenes)
		{
			SceneResource_ptr	sceneResource_ptr	= std::make_shared<SceneResource>();
			auto&				sceneObject			= sceneResource_ptr->Object();

			auto& name = scene.name;
			std::map<int, int> sceneNodeMap;

			auto AddNode =
				[&](const int nodeIdx, const int parent, auto& AddNode) -> void
			{
				auto& node = model.nodes[nodeIdx];

				SceneNode newNode;
				newNode.parent = parent != -1 ? sceneNodeMap[parent] : -1;

				if (node.translation.size())
					for (size_t I = 0; I < 3; I++)
						newNode.position[I] = (float)node.translation[I];
				else
					newNode.position = float3{ 0, 0, 0 };

				if (node.rotation.size())
					for (size_t I = 0; I < 4; I++)
						newNode.orientation[I] = (float)node.rotation[I];
				else
					newNode.orientation = Quaternion{ 0, 0, 0, 1 };


				if (node.scale.size())
					newNode.scale = float3{ (float)node.scale[0], (float)node.scale[1], (float)node.scale[2] };
				else
					newNode.scale = { 1, 1, 1 };

				if (node.mesh != -1 ||
					node.skin != -1 ||
					node.children.size() ||
					node.extensions.find("KHR_lights_punctual") != node.extensions.end())
				{
					const auto nodeHndl		= sceneObject->AddSceneNode(newNode);
					sceneNodeMap[nodeIdx]	= nodeHndl;

					SceneEntity entity;
					entity.id = node.name;

					auto nodeComponent = std::make_shared<EntitySceneNodeComponent>(nodeHndl);
					entity.components.push_back(nodeComponent);

					if (node.mesh != -1)
					{
						auto brush				= std::make_shared<EntityBrushComponent>(meshMap[node.mesh]);
						auto materialComponent	= std::make_shared<EntityMaterialComponent>();

						for (auto& subEntity : model.meshes[node.mesh].primitives)
						{
							EntityMaterial subMaterial;

							if (options.importMaterials && subEntity.material != -1)
							{
								auto& material = model.materials[subEntity.material];

								auto AddTexture = [&](const ParameterMap& material, auto& string) {
									if (auto res = material.find(string); res != material.end() && res->first == string)
									{
										int index = res->second.json_double_value.find("index")->second;

										if (imageMap.find(index) != imageMap.end())
										{
											auto resource	= imageMap[index];
											auto assetGUID	= resource->GetResourceGUID();

											std::cout << "Searched for " << string << ". Found resource: " << resource->GetResourceID() << "\n";

											subMaterial.textures.emplace_back(assetGUID, 0xffffffff);
										}
										else
										{
											std::cout << "Texture Not Found!\n" << "Failed to find: " << string << "\n";
										}
									}
								};


								if(options.importTextures)
								{
									if (material.pbrMetallicRoughness.baseColorTexture.index != -1)
									{
										if (model.textures.size() > material.pbrMetallicRoughness.baseColorTexture.index)
										{
											auto idx = model.textures[material.pbrMetallicRoughness.baseColorTexture.index].source;

											if (imageMap.size() > idx && imageMap.at(idx) != nullptr)
												subMaterial.textures.emplace_back(imageMap.at(idx)->GetResourceGUID(), GetTypeGUID(ALBEDO));
										}
									}

									if (material.normalTexture.index != -1)
									{
										if (model.textures.size() > material.normalTexture.index)
										{
											auto idx = model.textures[material.normalTexture.index].source;

											if (imageMap.size() > idx && imageMap.at(idx) != nullptr)
												subMaterial.textures.emplace_back(imageMap.at(idx)->GetResourceGUID(), GetTypeGUID(NORMAL));
										}
									}

									if (material.pbrMetallicRoughness.metallicRoughnessTexture.index != -1)
									{
										if (model.textures.size() > material.pbrMetallicRoughness.metallicRoughnessTexture.index)
										{
											auto idx = model.textures[material.pbrMetallicRoughness.metallicRoughnessTexture.index].source;

											if (imageMap.size() > idx && imageMap.at(idx) != nullptr)
												subMaterial.textures.emplace_back(imageMap.at(idx)->GetResourceGUID(), GetTypeGUID(METALLICROUGHNESS));
										}
									}
								}
							}

							materialComponent->materials.emplace_back(std::move(subMaterial));
						}

						entity.components.push_back(brush);
						entity.components.push_back(materialComponent);
					}

					if (node.skin != -1)
					{
						auto assetID = skinMap[node.skin]->GetResourceGUID();
						auto skeletonComponent = std::make_shared<EntitySkeletonComponent>(assetID);
						entity.components.push_back(skeletonComponent);
					}

					auto GetNumber = [&](auto& value)
					{
						return float(value.IsNumber() ? value.Get<double>() : value.Get<int>());
					};

					for (auto& ext : node.extensions)
					{
						if (ext.first == "KHR_lights_punctual")
						{
							auto lightIdx = ext.second.Get<tinygltf::Value::Object>()["light"].Get<int>();

							auto& extension		= model.extensions["KHR_lights_punctual"];
							auto& lights		= extension.Get<tinygltf::Value::Object>()["lights"];

							auto& light			= lights.Get(lightIdx);
							auto& color			= light.Get("color");
							float intensity		= GetNumber(light.Get("intensity"));
							float range			= light.Has("range") ? ([](float F) { return F == 0.0 ? 1000 : F; }(GetNumber(light.Get("range")))) : 1000;

							float3 K = {
								GetNumber(color.Get(0)),
								GetNumber(color.Get(1)),
								GetNumber(color.Get(2)),
							};

							const auto pointLight = std::make_shared<EntityLightComponent>(K, float2{ intensity, range });
							entity.components.push_back(pointLight);
						}
					}

					sceneObject->AddSceneEntity(entity);
				}

				for (auto child : node.children)
					AddNode(child, nodeIdx, AddNode);
			};

			for (auto nodeIdx : scene.nodes)
				AddNode(nodeIdx, -1, AddNode);


			if (scene.extras.Has("ResourceID"))
				sceneResource_ptr->SetResourceGUID(scene.extras.Get("ResourceID").Get<int>());
			else
				sceneResource_ptr->SetResourceGUID(rand());

			sceneResource_ptr->ID = scene.name;

			resources.push_back(sceneResource_ptr);
		}

		return resources;
	}


	/************************************************************************************************/


	struct ImageLoaderDesc
	{
		tinygltf::Model&				model;
		ResourceList&					resources;
		std::map<int, Resource_ptr>&	imageMap;
		bool							enableImageLoading;
	};


	bool loadImage(tinygltf::Image* image, const int image_idx, std::string* err, std::string* warn, int req_width, int req_height, const unsigned char* bytes, int size, void* user_ptr)
	{
		ImageLoaderDesc* loader = reinterpret_cast<ImageLoaderDesc*>(user_ptr);

		if (!loader->enableImageLoading)
			return false;

		int x		= 0;
		int y		= 0;
		int comp	= 0;

		auto datass		= stbi_load_from_memory(bytes, size, &x, &y, &comp, 3);
		float* floats	= (float*)datass;
		auto resource	= CreateTextureResource(floats, x * y * 3, { x, y }, 3, image->name, "DXT7");

		loader->resources.push_back(resource);
		loader->imageMap[image_idx] = resource;

		return true;
	}


	/************************************************************************************************/


	ResourceList GatherDeformers(tinygltf::Model& model)
	{
		ResourceList resources;

		for (auto& skin : model.skins)
		{
			std::vector<uint32_t>			parentLinkage;
			std::map<uint32_t, uint32_t>	nodeMap;
			std::vector<float4x4>			jointPoses;

			// Build linkage
			for (auto _ : skin.joints)
				parentLinkage.push_back((uint32_t)INVALIDHANDLE);

			for (auto _ : skin.joints)
				jointPoses.emplace_back();

			for (uint32_t I = 0; I < skin.joints.size(); I++)
				nodeMap[skin.joints[I]] = I;

			GUID_t ID = rand();

			const auto bufferViewIdx	= model.accessors[skin.inverseBindMatrices].bufferView;
			const auto& bufferView		= model.bufferViews[bufferViewIdx];
			const auto& buffer			= model.buffers[bufferView.buffer];

			const auto inverseMatrices	= reinterpret_cast<const FlexKit::float4x4*>(buffer.data.data() + bufferView.byteOffset);

			// Fetch properties
			for (auto& joint : skin.joints)
			{
				auto& node = model.nodes[joint];

				if (node.extras.Has("ResourceID") && node.extras.Get("ResourceID").IsInt())
					ID = node.extras.Get("ResourceID").GetNumberAsInt();

				auto jointIdx = nodeMap[joint];

				for (auto& child : node.children)
					parentLinkage[nodeMap[child]] = jointIdx;
			}

			auto skeleton   = std::make_shared<SkeletonResource>();
			skeleton->guid  = ID;
			skeleton->ID    = skin.name + ".skeleton";

			// Fetch Bind Pose
			for (size_t I = 0; I < parentLinkage.size(); I++)
			{
				auto parent			= JointHandle(parentLinkage[I]);
				auto inversePose	= Float4x4ToXMMATIRX(inverseMatrices[I]);

				SkeletonJoint joint;
				joint.mParent	= parent;
				joint.mID		= model.nodes[skin.joints[I]].name;
				skeleton->AddJoint(joint, inversePose);
			}

			resources.push_back(skeleton);
		}

		return resources;
	}


	/************************************************************************************************/


	ResourceList GatherAnimations(tinygltf::Model& model)
	{
		ResourceList resources;

		for (auto& animation : model.animations)
		{
			auto animationResource	= std::make_shared<AnimationResource>();

			animationResource->ID	= animation.name;
			auto& channels			= animation.channels;
			auto& samplers			= animation.samplers;

			for (auto& channel : channels)
			{
				AnimationResource::Track track;

				auto& sampler = samplers[channel.sampler];

				auto& input		= model.accessors[sampler.input];
				auto& output	= model.accessors[sampler.output];

				auto& inputView		= model.bufferViews[input.bufferView];
				auto& outputView	= model.bufferViews[output.bufferView];

				auto name = model.nodes[channel.target_node].name;

				track.targetChannel = AnimationTrackTarget{
										.Channel	= channel.target_path,
										.Target		= name,
										.type		= TrackType::Skeletal };

				union float3Value
				{
					float floats[3];
				};

				const size_t	frameCount			= outputView.byteLength / 16;
				float*			startTime			= (float*)(model.buffers[inputView.buffer].data.data() + inputView.byteOffset);

				const uint channelID =  channel.target_path == "rotation" ? 0 :
										channel.target_path == "translation" ? 1 :
										channel.target_path == "scale" ? 2 : -1;

				const auto& r			= model.nodes[channel.target_node].rotation;
				const auto& p			= model.nodes[channel.target_node].translation;
				const Quaternion Q		= r.size() ?  Quaternion{ (float)r[0], (float)r[1], (float)r[2], (float)r[3] }.normal() : Quaternion::Identity();
				const Quaternion Q_i	= Q.Inverse();
				const auto position		= p.size() ? float4{ (float)p[0], (float)p[1], (float)p[2], 0 } : float4{ 0 };

				if (channelID == 2)
					continue;

				for (size_t I = 0; I < frameCount; ++I)
				{
					AnimationKeyFrame keyFrame;
					keyFrame.Begin			= startTime[I];
					keyFrame.End			= startTime[I + 1];
					keyFrame.interpolator	= AnimationInterpolator{ .Type = AnimationInterpolator::InterpolatorType::Linear };

					switch(channelID)
					{
					case 0:
					{
						Quaternion* rotationChannel	= (Quaternion*)(model.buffers[outputView.buffer].data.data() + outputView.byteOffset);
						auto rotationKey			= rotationChannel[I] * Q_i;
						keyFrame.Value				= float4{ rotationKey[0], rotationKey[1], rotationKey[2], rotationKey[3] };
					}   break;
					case 1:
					{
						float3Value* float3Channel	= (float3Value*)(model.buffers[outputView.buffer].data.data() + outputView.byteOffset);
						keyFrame.Value				= float4{ float3Channel->floats[0], float3Channel->floats[1], float3Channel->floats[2], 1} - position;
					}   break;
					default:
						break;
					}

					track.KeyFrames.emplace_back(keyFrame);
				}

				animationResource->tracks.emplace_back(std::move(track));
			}

			resources.emplace_back(animationResource);
		}

		return resources;
	}


	/************************************************************************************************/


	ResourceList CreateSceneFromGlTF(const std::filesystem::path& fileDir, const gltfImportOptions& options, MetaDataList& MD)
	{
		using namespace tinygltf;
		Model model;
		TinyGLTF loader;
		std::string err;
		std::string warn;

		ResourceList				textureResources;
		std::map<int, Resource_ptr>	imageMap;

		ImageLoaderDesc imageLoader{ model, textureResources, imageMap };
		imageLoader.enableImageLoading = options.importTextures;

		loader.SetImageLoader(loadImage, &imageLoader);
		
		if (auto res = loader.LoadBinaryFromFile(&model, &err, &warn, fileDir.string()); res == true)
		{
			auto deformers					= GatherDeformers(model);
			auto animations					= GatherAnimations(model);
			auto [meshResources, meshMap]	= GatherGeometry(model);

			auto scenes = GatherScenes(model, meshMap, imageMap, deformers, options);

			ResourceList resources;

			if(options.importAnimations)
				for (auto resource : animations)
					resources.push_back(resource);

			if (options.importDeformers)
				for (auto resource : deformers)
					resources.push_back(resource);

			if(options.importMeshes)
				for (auto resource : meshResources)
					resources.push_back(resource);

			if (options.importScenes)
				for (auto resource : scenes)
					resources.push_back(resource);

			if (options.importTextures)
				for (auto resource : textureResources)
					resources.push_back(resource);

			return resources;
		}

		return {};
	}


	/************************************************************************************************/


	ResourceBlob SceneObject::CreateBlob() const
	{
		// Create Scene Node Table
		SceneNodeBlock::Header sceneNodeHeader;
		sceneNodeHeader.blockSize = (uint32_t)(SceneNodeBlock::GetHeaderSize() + sizeof(SceneNodeBlock::SceneNode) * nodes.size());
		sceneNodeHeader.blockType = SceneBlockType::NodeTable;
		sceneNodeHeader.nodeCount = (uint32_t)nodes.size();

		Blob nodeBlob{ sceneNodeHeader };
		for (auto& node : nodes)
		{
			SceneNodeBlock::SceneNode n;
			n.orientation	= node.orientation;
			n.position		= node.position;
			n.scale			= node.scale;
			n.parent		= node.parent;

			/*
			if (n.parent == 0)
			{// if Parent is root, bake rotation, fixes weird blender rotation issues
				n.orientation   = nodes[0].Q * n.orientation;
				n.position      = nodes[0].Q.Inverse() * n.position;
			}
			*/

			nodeBlob += Blob{ n };
		}


		Blob entityBlock;
		for (auto& entity : entities)
		{
			EntityBlock::Header entityHeader;
			entityHeader.blockType		= SceneBlockType::EntityBlock;
			entityHeader.blockSize		= sizeof(EntityBlock::Header);
			entityHeader.componentCount	= 1;//2 + entity.components.size() + 1; // TODO: Create a proper material component

			Blob componentBlock;
			componentBlock		+= CreateIDComponent(entity.id);

			for (auto& component : entity.components)
			{
				componentBlock += component->GetBlob();
				entityHeader.componentCount++;
			}

			entityHeader.blockSize += componentBlock.size();

			entityBlock += Blob{ entityHeader };
			entityBlock += componentBlock;
		}

		// Create Scene Resource Header
		SceneResourceBlob	header;
		header.blockCount	= 2 + entities.size();
		header.GUID			= guid;
		strncpy(header.ID, ID.c_str(), 64);
		header.Type			= EResourceType::EResource_Scene;
		header.ResourceSize	= sizeof(header) + nodeBlob.size() + entityBlock.size();
		Blob headerBlob{ header };


		auto [_ptr, size] = (headerBlob + nodeBlob + entityBlock).Release();

		ResourceBlob out;
		out.buffer			= (char*)_ptr;
		out.bufferSize		= size;
		out.GUID			= guid;
		out.ID				= ID;
		out.resourceType	= EResourceType::EResource_Scene;

		return out;
	}


	/************************************************************************************************/


	EntityComponent_ptr SceneEntity::FindComponent(ComponentID id)
	{
		auto res = std::find_if(
			std::begin(components),
			std::end(components),
			[&](FlexKit::EntityComponent_ptr& component)
			{
				return component->id == id;
			});

		return res != std::end(components) ? *res : nullptr;
	}


}/************************************************************************************************/



/**********************************************************************

Copyright (c) 2015 - 2023 Robert May

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**********************************************************************/
