call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" amd64

set FLAGS=-Zi -LD 
cl "GameState.cpp" /D_DEBUG /DCOMPILE_DLL  %FLAGS% ^
/I "C:\Users\Robert\Documents\Dropbox\Dropbox\reps\gamekit\Dependencies\sdks\physx\include" ^
/I "C:\Users\Robert\Documents\Dropbox\Dropbox\reps\gamekit\Dependencies\sdks\physx\include" ^
/I "C:\Users\Robert\Documents\Dropbox\Dropbox\reps\gamekit\Dependencies\sdks\fbxsdk\include" ^
/I "C:\Users\Robert\Documents\Dropbox\Dropbox\reps\gamekit\Dependencies\sdks\physx\include\foundation" ^
/I "C:\Users\Robert\Documents\Dropbox\Dropbox\reps\gamekit\Dependencies\sdks\physx\include\common" ^
/I "C:\Users\Robert\Documents\Dropbox\Dropbox\reps\gamekit\Dependencies\sdks\physx\include\extensions" ^
/I "C:\Users\Robert\Documents\Dropbox\Dropbox\reps\gamekit\Dependencies\sdks\physx\source\foundation\include" ^
/I "C:\Users\Robert\Documents\Dropbox\Dropbox\reps\gamekit\Dependencies\sdks\raknet\source" ^
/I "C:\Users\Robert\Documents\Dropbox\Dropbox\reps\gamekit\Dependencies\sdks\OpenSubdiv-master" ^
/I "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\include" ^
/I "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\atlmfc\include" ^
/I "C:\Program Files (x86)\Windows Kits\10\Include\10.0.10586.0\ucrt" ^
/I "C:\Program Files (x86)\Windows Kits\10\Include\10.0.10586.0\um" ^
/I "C:\Program Files (x86)\Windows Kits\10\Include\10.0.10586.0\shared" ^
/I "C:\Program Files (x86)\Windows Kits\10\Include\10.0.10586.0\winrt" ^
/I "C:\Program Files (x86)\Windows Kits\NETFXSDK\4.6\Include\um" ^
/I "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\include" ^
/I "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\atlmfc\include" /link ^
/LIBPATH "C:\Users\Robert\Documents\Dropbox\Dropbox\reps\gamekit\Dependencies\lib\debug_x64\osdCPU.lib" ^
/LIBPATH "C:\Users\Robert\Documents\Dropbox\Dropbox\reps\gamekit\Dependencies\lib\debug_x64\osdGPU.lib" ^
/LIBPATH "C:\Users\Robert\Documents\Dropbox\Dropbox\reps\gamekit\Dependencies\lib\debug_x64\LowLevelDebug.lib" ^
/LIBPATH "C:\Users\Robert\Documents\Dropbox\Dropbox\reps\gamekit\Dependencies\lib\debug_x64\LowLevelClothDebug.lib" ^
/LIBPATH "C:\Users\Robert\Documents\Dropbox\Dropbox\reps\gamekit\Dependencies\lib\debug_x64\physx3Debug_x64.lib" ^
/LIBPATH "C:\Users\Robert\Documents\Dropbox\Dropbox\reps\gamekit\Dependencies\lib\debug_x64\PhysX3CharacterKinematicDebug_x64.lib" ^
/LIBPATH "C:\Users\Robert\Documents\Dropbox\Dropbox\reps\gamekit\Dependencies\lib\debug_x64\PhysX3CommonDEBUG_x64.lib" ^
/LIBPATH "C:\Users\Robert\Documents\Dropbox\Dropbox\reps\gamekit\Dependencies\lib\debug_x64\PhysX3ExtensionsDEBUG.lib" ^
/LIBPATH "C:\Users\Robert\Documents\Dropbox\Dropbox\reps\gamekit\Dependencies\lib\debug_x64\PhysXProfileSDKDEBUG.lib" ^
/LIBPATH "C:\Users\Robert\Documents\Dropbox\Dropbox\reps\gamekit\Dependencies\lib\debug_x64\PhysXVisualDebuggerSDKDEBUG.lib" ^
/LIBPATH "C:\Users\Robert\Documents\Dropbox\Dropbox\reps\gamekit\Dependencies\lib\debug_x64\SceneQueryDEBUG.lib" ^
/LIBPATH "C:\Users\Robert\Documents\Dropbox\Dropbox\reps\gamekit\Dependencies\lib\debug_x64\SimulationControllerDEBUG.lib" ^
/LIBPATH "C:\Users\Robert\Documents\Dropbox\Dropbox\reps\gamekit\Dependencies\lib\debug_x64\FlexKitd.lib" ^
/LIBPATH "kernel32.lib" ^
/LIBPATH "user32.lib" ^
/LIBPATH "gdi32.lib" ^
/LIBPATH "winspool.lib" ^
/LIBPATH "comdlg32.lib" ^
/LIBPATH "advapi32.lib" ^
/LIBPATH "shell32.lib" ^
/LIBPATH "ole32.lib" ^
/LIBPATH "oleaut32.lib" ^
/LIBPATH "uuid.lib" ^
/LIBPATH "odbc32.lib" ^
/LIBPATH "odbccp32.lib" ^
/OUT:"C:\Users\Robert\Documents\Dropbox\Dropbox\reps\gamekit\\builds\debug_x64\TestGameState.dll" /NXCOMPAT /PDB:"C:\Users\Robert\Documents\Dropbox\Dropbox\reps\gamekit\\builds\debug_x64\TestGameState.pdb" ^ 
/DYNAMICBASE "kernel32.lib" "user32.lib" "gdi32.lib" "winspool.lib" "comdlg32.lib" "advapi32.lib" "shell32.lib" "ole32.lib" "oleaut32.lib" "uuid.lib" "odbc32.lib" "odbccp32.lib" ^ 
/IMPLIB:"C:\Users\Robert\Documents\Dropbox\Dropbox\reps\gamekit\\builds\debug_x64\TestGameState.lib" /DLL /MACHINE:X64 /ERRORREPORT:PROMPT /NOLOGO /LIBPATH:"C:\Program Files (x86)\Visual Leak Detector\lib\Win64" /TLBID:1 