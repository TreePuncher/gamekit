call "%VS2019INSTALLDIR%\VC\Auxiliary\Build\vcvarsamd64_x86.bat"

if not exist Dependencies\sdks\physx_sdk\physx\compiler\vc16win64\PhysXSDK.sln call "Dependencies\sdks\physx_sdk\physx\generate_projects.bat" vc16win64

devenv Dependencies\sdks\physx_sdk\physx\compiler\vc16win64\PhysXSDK.sln /build debug
devenv Dependencies\sdks\physx_sdk\physx\compiler\vc16win64\PhysXSDK.sln /build release

if not exist "builds\debug" mkdir "builds\debug"
if not exist "builds\release" mkdir "builds\release"
if not exist "Dependencies\libs\debug" mkdir "Dependencies\libs\debug"
if not exist "Dependencies\libs\release" mkdir "Dependencies\libs\release"

for %%f in (Dependencies\sdks\physx_sdk\physx\bin\win.x86_64.vc142.mt\debug\*.dll) do copy /y "%%~ff" "builds\debug\"
for %%f in (Dependencies\sdks\physx_sdk\physx\bin\win.x86_64.vc142.mt\debug\*.lib) do copy /y "%%~ff" "Dependencies\libs\debug\"

for %%f in (Dependencies\sdks\physx_sdk\physx\bin\win.x86_64.vc142.mt\release\*.dll) do copy /y "%%~ff" "builds\release\"
for %%f in (Dependencies\sdks\physx_sdk\physx\bin\win.x86_64.vc142.mt\release\*.lib) do copy /y "%%~ff" "Dependencies\libs\release\"

devenv Dependencies\sdks\angelscript\sdk\angelscript\projects\msvc2017\angelscript.sln /upgrade

devenv Dependencies\sdks\angelscript\sdk\angelscript\projects\msvc2017\angelscript.sln /build "Debug|x64"
devenv Dependencies\sdks\angelscript\sdk\angelscript\projects\msvc2017\angelscript.sln /build "Release|x64"

for %%f in (Dependencies\sdks\angelscript\sdk\angelscript\lib\*64d.lib) do copy /y "%%~ff" "Dependencies\libs\debug\"
for %%f in (Dependencies\sdks\angelscript\sdk\angelscript\lib\*64.lib) do copy /y "%%~ff" "Dependencies\libs\release\"

devenv Dependencies\sdks\raknet\lib\Libstatic\LibStatic_vc9.vcproj /upgrade

devenv Dependencies\sdks\raknet\Lib\LibStatic\LibStatic_vc9.vcxproj /build "Debug|x64"
devenv Dependencies\sdks\raknet\Lib\LibStatic\LibStatic_vc9.vcxproj /build "Release|x64"

copy "Dependencies\sdks\raknet\Lib\LibStatic\Lib\LibStatic_vc9_LibStatic_Debug_x64.lib" "Dependencies\libs\debug\raknet_debug_x64.lib"
copy "Dependencies\sdks\raknet\Lib\LibStatic\Lib\LibStatic_vc9_LibStatic_Release_x64.lib" "Dependencies\libs\release\raknet_release_x64.lib"

devenv Dependencies\sdks\crunch\crn.2008.sln /upgrade
devenv Dependencies\sdks\crunch\crn.2008.sln /build "Debug|x64"
devenv Dependencies\sdks\crunch\crn.2008.sln /build "Release|x64"

copy "Dependencies\sdks\crunch\lib\VC9\Debug\x64\crnlibD_x64_VC9.lib" "Dependencies\libs\debug\crunch.lib"
copy "Dependencies\sdks\crunch\lib\VC9\Release\x64\crnlib_x64_VC9.lib" "Dependencies\libs\release\crunch.lib"

if exist "builds\debug" mkdir "builds\debug" copy "%fmodapidir%\api\core\lib\fmod64.dll" "builds\debug"
if exist "builds\debug" mkdir "builds\debug" copy "%fmodapidir%\api\core\lib\fmod64.dll" "builds\release"

if exist "builds\debug" mkdir "builds\debug" copy "%fmodapidir%\api\lowlevel\lib\fmod64.dll" "builds\debug"
if exist "builds\debug" mkdir "builds\debug" copy "%fmodapidir%\api\lowlevel\lib\fmod64.dll" "builds\release"

if not exist "builds\debug" mkdir "builds\debug"
if not exist "builds\debug\assets" mkdir "builds\debug\assets"
if not exist "builds\debug\assets\shaders" mkdir "builds\debug\assets\shaders"

if not exist "builds\release" mkdir "builds\release"
if not exist "builds\release\assets" mkdir "builds\release\assets"
if not exist "builds\release\assets\shaders" mkdir "builds\release\assets\shaders"

for %%f in (shaders\*.hlsl) do copy /y "%%~ff" "builds\debug\assets\shaders"
for %%f in (shaders\*.hlsl) do copy /y "%%~ff" "builds\release\assets\shaders"

exit