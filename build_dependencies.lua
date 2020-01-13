premake.modules.build_dependencies = {}
local m = premake.modules.build_dependencies

local p = premake

newaction {
	trigger = "build_dependencies",
	description = "build dependencies, setup lib folders, copy dlls",

	onStart = function()
	end,

	onWorkspace = function(wks)
	end,

	onProject = function(prj)
	end,

	execute = function()
		print("Building dependencies")
		os.execute("win64_build_dependencies.bat")
	end,

	onEnd = function()
		print "If Physx fails to build, goto PsAllocator.h line 40, change the line from:"
		print "#include <typeinfo.h>"
		print "to:"
		print "#include <typeinfo>"

		print "If Raknet fails to build, upgrade the project to latest VS version"
		print "Then rebuild"
	end
}

return m