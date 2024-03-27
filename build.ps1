$ErrorActionPreference = "Continue"

Write-Output "Running build on $Env:computername ..."

$PathMSBuild = "C:\Program Files\Microsoft Visual Studio\2022\Community\Msbuild\Current\Bin\amd64"
$PathSln = "FlexKit.sln"

Write-Output "PathMSBuild = $PathMSBuild"
Write-Output "PathSolutionFile = $PathSln"

# append path to env
$Env:path += ";" + $PathMSBuild

&"msbuild" $PathSln -t:restore -p:RestorePackagesConfig=true -maxcpucount:32

&"msbuild" $PathSln -target:TextureStreamingTest "/p:BuildInParallel=true" "/p:Configuration=Release" "/p:CL_MPcount=36"  -maxcpucount:32
if(!$?) { Exit $LASTEXITCODE }

./copy_assets.ps1

if(!$?) { Exit $LASTEXITCODE }
