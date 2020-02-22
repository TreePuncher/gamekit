call "%VS2019INSTALLDIR%\VC\Auxiliary\Build\vcvarsamd64_x86.bat"

devenv Dependencies\sdks\physx_sdk\physx\compiler\vc16win64\PhysXSDK.sln /clean debug
devenv Dependencies\sdks\physx_sdk\physx\compiler\vc16win64\PhysXSDK.sln /clean release

devenv Dependencies\sdks\angelscript\sdk\angelscript\projects\msvc2017\angelscript.sln /clean "Debug|x64"
devenv Dependencies\sdks\angelscript\sdk\angelscript\projects\msvc2017\angelscript.sln /clean "Release|x64"

devenv Dependencies\sdks\raknet\Lib\LibStatic\LibStatic_vc9.vcxproj /clean "Debug|x64"
devenv Dependencies\sdks\raknet\Lib\LibStatic\LibStatic_vc9.vcxproj /clean "Release|x64"

devenv Dependencies\sdks\crunch\crn.2008.sln "Debug|x64"
devenv Dependencies\sdks\crunch\crn.2008.sln "Release|x64"

devenv Dependencies\sdks\crunch\crn.2008.sln "Debug|x64" /clean "Debug|x64"
devenv Dependencies\sdks\crunch\crn.2008.sln "Release|x64" /clean "Release|x64"

exit