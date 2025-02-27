#include "EditorUSDImport.h"

#include <boost/core/enable_if.hpp>
#include <filesystem>

#undef emit

#include <pxr/base/tf/type.h>
#include <pxr/base/tf/debug.h>
#include <pxr/base/tf/diagnosticbase.h>
#include "pxr/base/tf/envSetting.h"
#include <pxr/base/plug/registry.h>

#include <pxr/usd/ar/asset.h>
#include <pxr/usd/ar/defineResolver.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usdgeom/xform.h>
#include <pxr/usd/usdgeom/primvar.h>
#include <pxr/usd/usdSkel/skeleton.h>

#include <print>
#include <tuple>

#include "EditorSceneEntityComponents.h"
#include "EditorSceneResource.h"

#include "EditorUSDParserInterface.hpp"

#include <MathUtilities.hpp>
#include <fmt/printf.h>
#include <fmt/format.h>
#include <ThreadUtilities.hpp>



#include "ui_EditorImportSceneDialog.h"
#include <QtWidgets/QComboBox.h>


/************************************************************************************************/



template <> struct fmt::formatter<FlexKit::float4x4> {

	constexpr auto parse(format_parse_context& ctx)
		-> format_parse_context::iterator
	{
		return ctx.begin();
	}

	auto format(const FlexKit::float4x4& m, format_context& ctx) const
		->format_context::iterator
	{
		return fmt::format_to(ctx.out(),
			"|[ {:f}, {:f}, {:f}, {:f} ]|\n"
			"|[ {:f}, {:f}, {:f}, {:f} ]|\n"
			"|[ {:f}, {:f}, {:f}, {:f} ]|\n"
			"|[ {:f}, {:f}, {:f}, {:f} ]|",
			m(0, 0), m(0, 1), m(0, 2), m(0, 3),
			m(1, 0), m(1, 1), m(1, 2), m(1, 3),
			m(2, 0), m(2, 1), m(2, 2), m(2, 3),
			m(3, 0), m(3, 1), m(3, 2), m(3, 3)
		);
	}
};

FlexKit::float4x4 PXRmatrix2Float4x4(const pxr::GfMatrix4d& rhs)
{
	FlexKit::float4x4 out;

	for (int i = 0; i < rhs.numRows; i++)
	{
		auto r0 = rhs.GetRow(i);
		out(i, 0) = r0[0];
		out(i, 1) = r0[1];
		out(i, 2) = r0[2];
		out(i, 3) = r0[3];
	}

	return out;
}

pxr::GfMatrix4d Float4x42PXRmatrix(const FlexKit::float4x4& rhs)
{
	pxr::GfMatrix4d out;

	for (int i = 0; i < rhs.RowCount(); i++)
	{
		pxr::GfVec4d row;
		row[0] = rhs(i, 0);
		row[1] = rhs(i, 0);
		row[2] = rhs(i, 0);
		row[3] = rhs(i, 0);

		out.SetRow(i, row);
	}

	return out;
}


/************************************************************************************************/


PrimitiveParserInterface_* GetPrimitiveParser(const std::string& name)
{
	if (auto res = PrimitiveParserInterface_::parsers.find(name); res != PrimitiveParserInterface_::parsers.end())
		return res->second;
	else
		return nullptr;
}


/************************************************************************************************/


struct USDParseContext
{
	USDParseContext(
		FlexKit::SceneObject&	IN_scene,
		FlexKit::ThreadManager&	IN_threads) :
			scene		{ IN_scene },
			threads		{ IN_threads },
			pendingWork	{ IN_threads }{}

	FlexKit::SceneObject&	scene;
	FlexKit::ThreadManager& threads;
	FlexKit::WorkBarrier	pendingWork;

	enum class ETask
	{
		Import,
		Update,
		Skip
	};

	struct ResourceImportTask
	{
		std::string path;
		ETask		task;
	};
	std::vector<ResourceImportTask> modelsToImport;

	bool enableLoadingMeshes = true;

	void AddMeshLoadTask(std::string path)
	{
		if (enableLoadingMeshes)
		{
			modelsToImport.emplace_back(path, ETask::Import);
		}
	}


	void AddTask(std::function<void (FlexKit::iAllocator& allocator)> task)
	{
		auto& workItem = FlexKit::CreateWorkItem(task);
		pendingWork.AddWork(workItem);
		PushToLocalQueue(workItem);
	}
};


/************************************************************************************************/


struct MeshParser : PrimitiveParserInterface<MeshParser>
{
	~MeshParser() override = default;

	void ParseObject(USDParseContext& ctx, pxr::UsdPrim& primitive) final
	{
		using std::views::enumerate;

		auto& path = primitive.GetPath().GetString();
		fmt::print("Mesh Found: {}\nAttributes:\n", primitive.GetName().GetString());

		auto&& attributes = primitive.GetAttributes();
		for (auto& attribute : attributes)
			fmt::print("\t{} : {}\n", attribute.GetName().GetString(), attribute.GetTypeName().GetCPPTypeName());

		auto pointsAttrib			= primitive.GetAttribute(pxr::TfToken{ "points" });
		auto UVMapAttrib			= primitive.GetAttribute(pxr::TfToken{ "primvars:UVMap" });
		auto normalsAttrib			= primitive.GetAttribute(pxr::TfToken{ "normals" });
		auto faceVertexCountsAttrib	= primitive.GetAttribute(pxr::TfToken{ "faceVertexCounts" });

		pxr::VtArray<pxr::GfVec3f>	points;
		pxr::VtArray<pxr::GfVec2f>	UVMaps;
		pxr::VtArray<pxr::GfVec3f>	normals;
		pxr::VtArray<int>			faceSizes;

		pointsAttrib.Get(&points);
		UVMapAttrib.Get(&UVMaps);
		normalsAttrib.Get(&normals);
		faceVertexCountsAttrib.Get(&faceSizes);
	}

	const char* GetTypeName() const final
	{
		return "Mesh";
	}
};


/************************************************************************************************/


USDImporter::USDImporter(EditorProject& IN_project, class FlexKit::ThreadManager& IN_threads) :
	project{ IN_project },
	threads{ IN_threads }
{
	auto& plugRegistry	= pxr::PlugRegistry::GetInstance();
	plugRegistry.RegisterPlugins(std::filesystem::current_path().string() + "\\plugInfo.json");
}


/************************************************************************************************/


struct ImportOptions
{
	bool importAnimations	= true;
	bool importDeformers	= true;
	bool importMeshes		= true;
	bool importMaterials	= true;
	bool importScenes		= true;
	bool importTextures		= true;
};


/************************************************************************************************/


class Dialog : public QWidget
{
public:
	Dialog(std::string IN_fileDir, EditorProject& IN_project, FlexKit::ThreadManager& IN_threads) :
		fileDir	{ std::move(IN_fileDir) },
		project	{ IN_project },
		threads { IN_threads }
	{
		ui.setupUi(this);
		ui.importButton->setEnabled(false);

		setAttribute(Qt::WA_DeleteOnClose, true);

		connect(ui.cancelButton, &QPushButton::clicked, this, &Dialog::close);
		connect(ui.importButton, &QPushButton::clicked, this, &Dialog::OnImport);

		pxr::TfErrorMark errors;

		if (usdStage = pxr::UsdStage::Open(fileDir); usdStage)
		{
			std::vector<std::string>			resourcePaths;
			std::map<std::string, std::string>	prototypeMap;

			fmt::print("Printing Prototypes : \n");
			for (auto& protoType : usdStage->GetPrototypes())
			{
				fmt::print("ProtoType Name: {}\n", protoType.GetName().GetString());
				fmt::print("Path: {}\n", protoType.GetChildren().front().GetPath().GetString());
			}

			for (auto&& child : usdStage->GetPseudoRoot().GetChildren())
			{
				auto FindAssetInfo = [](this auto& self, const pxr::UsdPrim& prim) -> pxr::VtDictionary
					{
						if (!prim.IsValid())
							return {};

						auto assetInfo = prim.GetAssetInfo();
						if (assetInfo.size() == 0)
							return self(prim.GetParent());
						else
							return assetInfo;
					};

				auto GetInstanceLocalPath = [](this auto& self, const pxr::UsdPrim& prim) -> std::string
					{
						if (prim.GetParent().GetParent().IsInPrototype())
							return self(prim.GetParent()) + "/" + prim.GetName().GetString();
						else
							return "";
					};


				auto processChild = [&](this auto& self,  auto&& prim) -> void
					{
						const auto name = prim.GetName();
						fmt::print("Name: {}, Type: {}\n", name.GetString(), prim.GetTypeName().GetString());
						fmt::print("Path {}\n", prim.GetPath().GetString());
						fmt::print("Asset Info: \nCount: {}\n", prim.GetAssetInfo().size());

						if (prim.IsInstanceProxy())
							fmt::print("Is Instance Proxy\n");

						pxr::VtDictionary dictionary = prim.GetAssetInfo();
						for (auto&& [first, second] : dictionary)
						{
							fmt::print("[{}, {}]", second.GetType().GetTypeName(), first);
							if (second.GetTypeName() == "string")
								fmt::print(":= {}\n", second.Get<std::string>());
							else if (second.GetTypeName() == "SdfAssetPath")
								fmt::print(":= {}\n", second.Get<pxr::SdfAssetPath>().GetAssetPath());
							else
								fmt::print("\n");
						}

						fmt::print("Relations: {}\n", prim.GetRelationships().size());
						for (pxr::UsdRelationship& relationship : prim.GetRelationships())
							fmt::print("path: {}\n", relationship.GetPath().GetString());

						if (prim.GetTypeName() == "Mesh")
						{
							pxr::UsdPrim p = prim;
							for (auto& scheme : p.GetAppliedSchemas())
								fmt::print("Scheme: {}", scheme.GetString());

							if (prim.IsInstanceProxy())
							{
								pxr::UsdPrim instancePrototype	= prim.GetPrimInPrototype();
								if (instancePrototype.IsValid())
								{
									const std::string path			= instancePrototype.GetPath().GetAsString();
									const std::string prototypeName = instancePrototype.GetName().GetString();

									pxr::VtDictionary assetInfo = FindAssetInfo(prim);

									fmt::print("Prototype Found: {}\n", prototypeName);
									if (auto res = assetInfo.find("identifier"); res != assetInfo.end())
									{
										auto&& [first, second] = *res;
										auto path = second.Get<pxr::SdfAssetPath>().GetAssetPath();

										auto instancePath = "@." + path + "@<" + GetInstanceLocalPath(instancePrototype) + ">";
										resourcePaths.emplace_back(instancePath);
									}

									if (auto res = std::ranges::find(resourcePaths, instancePrototype.GetName().GetString()); res == resourcePaths.end())
										resourcePaths.emplace_back(prototypeName);
								}
							}
							else
							{
								fmt::print("Adding Mesh Path to work list: {}\n", prim.GetPath().GetString());
								resourcePaths.emplace_back(prim.GetPath().GetString());
							}
						}
						else
						{
							fmt::print("Primitive Type: {}\n", prim.GetTypeName().GetString());
						}

						if (!prim.IsInPrototype())
							for (auto&& childPrim : prim.GetFilteredChildren(pxr::UsdTraverseInstanceProxies()))
								self(childPrim);
					};

					processChild(child);
			}

			//if(auto parent = prim.GetParent(); parent.IsValid())
			//	fmt::print("Parent: {}\n", parent.GetName().GetString());
			//
			//std::string typeName = prim.GetTypeName().GetString();
			//if (auto parser = GetPrimitiveParser(typeName); parser)
			//	parser->ParseObject(context, prim);
			//else
			//{
			//	if (prim.GetAttributes().size())
			//	{
			//		fmt::print("No parser found, dumping attributes!\n");
			//		fmt::print("Primitive Attributes:\n");
			//
			//		for (auto&& attribute : prim.GetAttributes())
			//			fmt::print("\tattribute: {}, Type: {}\n", attribute.GetName().GetString(), attribute.GetTypeName().GetCPPTypeName());
			//	}
			//}

			std::ranges::sort(resourcePaths);
			auto [first, last] = std::ranges::unique(resourcePaths);
			resourcePaths.erase(first, last);

			ui.resourceImportList->setRowCount(resourcePaths.size());

			for (auto&& [idx, path] : enumerate(resourcePaths))
			{
				QTableWidgetItem* newItem = new QTableWidgetItem{ QString{ path.c_str() } };
				QComboBox* comboBox = new QComboBox{};

				comboBox->addItems({ "Import", "Update", "Ignore" });
				comboBox->setCurrentIndex(0);

				ui.resourceImportList->setCellWidget(idx, 0, comboBox);
				ui.resourceImportList->setItem(idx, 1, newItem);
			}

			ui.importButton->setEnabled(true);
		}
		else
		{
			if (!errors.IsClean())
			{
				for (pxr::TfError const& error : errors)
				{
					std::print("{}\n", error.GetDiagnosticCodeAsString());
				}
			}

			close();
			return;
		}

		show();
	}


	void OnImport()
	{
		hide();

		ImportOptions	options;

		options.importAnimations	= ui.importAnimations->isChecked();
		options.importDeformers		= ui.importDeformers->isChecked();
		options.importMaterials		= ui.importMaterials->isChecked();
		options.importMeshes		= ui.importMeshes->isChecked();
		options.importScenes		= ui.importScenes->isChecked();
		options.importTextures		= ui.importTextures->isChecked();

		EditorTask_ptr task = std::make_shared<EditorTask>();

		task->SetDescription("Loading gltf file");
		task->SetName("import");

		FlexKit::SceneObject scene;
		USDParseContext context{ scene, threads };

		auto& workItem =
			FlexKit::CreateWorkItem(
			[task, this, options](auto& threadLocalAllocator) mutable
			{
				FlexKit::WorkBarrier barrier{ threads };

				close();
			});

		PostTask(workItem, task);
	}

	std::string				fileDir;
	EditorProject&			project;
	FlexKit::ThreadManager& threads;
	pxr::UsdStageRefPtr		usdStage;
	Ui_ImportglTFDialog		ui;
};


/************************************************************************************************/


bool USDImporter::Import(const std::string& fileDir)
{
	pxr::TfType type = pxr::TfType::Find<pxr::ArResolver>();
	auto name = type.GetTypeName();

	if (!pxr::UsdStage::IsSupportedFile(fileDir))
		return false;

	new Dialog{ fileDir, project, threads };

	return true;
}


/**********************************************************************

Copyright (c) 2019-2024 Robert May

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
