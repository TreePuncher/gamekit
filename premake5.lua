-- premake5.lua

require "build_dependencies"
require "clean_dependencies"

workspace "TestGame"
   configurations { "Debug", "Release" }

project "TestGame"
	kind "ConsoleApp"
	language "C++"
	targetdir "builds/%{cfg.buildcfg}"

	basedir "TestGame"

	includedirs { 
		"%fmodapidir%/api/lowlevel/inc", 
		"C:/Program Files/Autodesk/FBX/FBX SDK/2020.0.1/include/**",

		"coreutilities", 
		"graphicsutilities", 
		"physicsutilities", 

		"Dependencies/sdks",
		"Dependencies/sdks/angelscript/**",

		"Dependencies/sdks/fbxsdk/include",

		"Dependencies/sdks/physx_sdk/physx/include/",
		"Dependencies/sdks/physx_sdk/physx/include/**",
		"Dependencies/sdks/physx_sdk/pxshared/**",

		"Dependencies/sdks/stb",
		"Dependencies/sdks/stb/**",

		"Dependencies/sdks/DirectXTK/Src",
		"Dependencies/sdks/DirectXTK/Inc",
		"Dependencies/sdks/DirectX-Graphics-Samples/Libraries/D3DX12/"}

	architecture "x86_64"

	vpaths { ["Headers/*"] = "**.h", ["Source/*"] = "**.cpp", ["Shaders"] = "**.hlsl"}

	files{
		"TestGame/**.h", 
		"TestGame/**.cpp", 
		"coreutilities/*.h", 
		"coreutilities/*.cpp", 
		"graphicsutilities/*.h",
		"graphicsutilities/*.cpp", 
		"physicsutilities/*.h", 
		"physicsutilities/*.cpp", 
		"shaders/*.hlsl"}

-- Unity build, so only the win64.cpp needs to be compiled
	filter{"files:**.cpp"}
		flags {"ExcludeFromBuild"}

	filter{"files:TestGame/Win64.cpp"}
		removeflags{"ExcludeFromBuild"}

	filter "configurations:Debug"
		libdirs {
			"Dependencies/libs/debug", 
			"%fmodapidir%/api/lowlevel/lib",
			"C:/Program Files/Autodesk/FBX/FBX SDK/2020.0.1/lib/vs2017/x86/debug"
		}
		defines "_DEBUG"
		symbols "On"
		optimize "Debug"
		buildoptions { "/std:c++latest", "/MTd" }

	filter "configurations:Release"
		libdirs {
			"Dependencies/libs/release", 
			"%fmodapidir%/api/lowlevel/lib"
		}
		defines "NDEBUG"
		optimize "Full"
		buildoptions { "/std:c++latest", "/MT"}

project "ResourceBuilder"
	kind "ConsoleApp"
	language "C++"
	targetdir "builds/%{cfg.buildcfg}"

	basedir "FlexKitResourceCompiler"

	includedirs { 
		"%fmodapidir%/api/lowlevel/inc", 
		"C:/Program Files/Autodesk/FBX/FBX SDK/2020.0.1/include",
		"C:/Program Files/Autodesk/FBX/FBX SDK/2020.0.1/include/**",

		"coreutilities", 
		"graphicsutilities", 
		"physicsutilities", 

		"Dependencies/sdks",
		"Dependencies/sdks/angelscript/**",

		"Dependencies/sdks/fbxsdk/include",

		"Dependencies/sdks/physx_sdk/physx/include/",
		"Dependencies/sdks/physx_sdk/physx/include/**",
		"Dependencies/sdks/physx_sdk/pxshared/**",

		"Dependencies/sdks/stb",
		"Dependencies/sdks/stb/**",

		"Dependencies/sdks/DirectXTK/Src",
		"Dependencies/sdks/DirectXTK/Inc",
		"Dependencies/sdks/DirectX-Graphics-Samples/Libraries/D3DX12/" }

	architecture "x86_64"

	vpaths { ["Headers"] = "**.h", ["Source"] = "**.cpp" }

	files{"FlexKitResourceCompiler/**.h", "FlexKitResourceCompiler/**.cpp"}

	filter{"files:FlexKitResourceCompiler/**.cpp"}
		flags {"ExcludeFromBuild"}

-- Unity build, so only the win64.cpp needs to be compiled
	filter{"files:FlexKitResourceCompiler/FlexKitResourceCompiler.cpp"}
		removeflags{"ExcludeFromBuild"}


	filter "configurations:Debug"
		libdirs {
			"Dependencies/libs/debug", 
			"C:/Program Files/Autodesk/FBX/FBX SDK/2020.0.1/lib/vs2017/x64/debug" }
		defines "_DEBUG"
		symbols "On"
		buildoptions { "/std:c++latest", "/MTd" }
		optimize "Debug"

	filter "configurations:Release"
		libdirs {
			"Dependencies/libs/release",
			"C:/Program Files/Autodesk/FBX/FBX SDK/2020.0.1/lib/vs2017/x64/release"	}
		defines "NDEBUG"
		buildoptions { "/std:c++latest", "/MT" }
		optimize "Full"
