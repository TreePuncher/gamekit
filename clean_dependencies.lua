premake.modules.clean_dependencies = {}
local m = premake.modules.clean_dependencies
local p = premake

newaction {
	trigger = "clean_dependencies",
	description = "build dependencies, setup lib folders, copy dlls",

	onStart = function()
	end,

	onWorkspace = function(wks)
	end,

	onProject = function(prj)
	end,

	execute = function()
		print("cleaning dependencies")
		os.execute("win64_clean_dependencies.bat")
	end,

	onEnd = function()
		print("dependencies all clean!")
	end
}

return m