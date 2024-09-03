#Editor
cmake_minimum_required(VERSION 3.23)

if(WIN32)
	project(Editor LANGUAGES CXX VERSION 0.0.1)

	set(CMAKE_CXX_STANDARD 23)
	set(CMAKE_CXX_STANDARD_REQUIRED ON)

	set(CMAKE_AUTOMOC ON)
	set(CMAKE_AUTORCC ON)
	set(CMAKE_AUTOUIC ON)

	find_package(pxr CONFIG REQUIRED)
	find_package(Qt6 REQUIRED COMPONENTS Core Widgets GUI)

	qt_standard_project_setup()

	file (GLOB CPP_FILES "*.h")

	add_executable(editor
		CSGComponent.cpp
		CSGRendering.cpp
		DXRenderWindow.cpp
		EditorAnimationResource.cpp
		EditorAnimatorComponent.cpp
		EditorApplication.cpp
		EditorCodeEditor.cpp
		EditorColliderComponent.cpp
		EditorGameplayComponents.cpp
		EditorInspectors.cpp
		EditorInspectorView.cpp
		EditorMainWindow.cpp
		EditorMeshResourceViewer.cpp
		EditorOperators.cpp
		EditorOutliner.cpp
		EditorOutputWindow.cpp
		EditorPlayer.cpp
		EditorPrefabEditor.cpp
		EditorPrefabInputTab.cpp
		EditorPrefabObject.cpp
		EditorPrefabRenderer.cpp
		EditorProject.cpp
		EditorRenderer.cpp
		EditorResourcePickerDialog.cpp
		EditorSceneEntityComponents.cpp
		EditorSceneResource.cpp
		EditorScriptEngine.cpp
		EditorSelectedPrefabObject.cpp
		EditorSelectObjectDialog.cpp
		EditorTaskList.cpp
		EditorTaskManager.cpp
		EditorTextureImporter.cpp
		EditorTextureResources.cpp
		EditorTextureViewer.cpp
		EditorUndoRedo.cpp
		EditorUSDImport.cpp
		EditorViewport.cpp
		ExtraTu.cpp
		FBXResourceUtilities.cpp
		FlexKitResourceCompiler.cpp
		flxEditor-Win64.cpp
		gltfImport.cpp
		MaterialResource.cpp
		MeshResource.cpp
		MetaData.cpp
		ModelViewerWidget.cpp
		PCH.cpp
		ResourceBrowserWidget.cpp
		ResourceUtilities.cpp
		SharedEngineMemory.cpp
		TaskManager.cpp
		TTFontLoader.cpp
		ViewportScene.cpp

		${CPP_FILES}
	)

	set(CMAKE_AUTOMOC ON)
	set(CMAKE_AUTOUIC ON)
	set(CMAKE_AUTORCC ON)

	target_include_directories(editor PRIVATE ${PROJECT_SOURCE_DIR})
	target_include_directories(editor PRIVATE "${FLEX_ROOT}/ThirdParty/cmp/include")
	target_include_directories(editor PRIVATE $<BUILD_INTERFACE:${AUTOGEN_INCLUDE_DIR}>)

	target_link_libraries(editor PRIVATE EditorReflection)
	target_link_libraries(editor PRIVATE flex Qt::Core Qt::Widgets Qt::Gui)
	target_link_libraries(editor PRIVATE ar arch gf js tf kind ndr pcp plug sdf tf ts usd usdProc vt work)
	
	set_target_properties(editor PROPERTIES UNITY_BUILD ON)

	target_compile_definitions(editor PUBLIC UNICODE)
    target_compile_options(editor PUBLIC /UTF-8 /bigobj)

	if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
		target_link_libraries(editor PRIVATE ${FLEX_ROOT}/ThirdParty/cmp/lib/bin/x64/CMP_Core_MDd.lib)
	elseif(${CMAKE_BUILD_TYPE} STREQUAL "Release")
		target_link_libraries(editor PRIVATE ${FLEX_ROOT}/ThirdParty/cmp/lib/bin/x64/CMP_Core_MD.lib)
	endif()

	FLEX_COPYASSETS(editor)

	add_custom_command(
		TARGET editor POST_BUILD
		COMMAND Qt6::windeployqt ARGS $<TARGET_FILE:editor>)
endif()
