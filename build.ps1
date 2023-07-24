$ErrorActionPreference = "Continue"

Write-Output "Running build on $Env:computername ..."

$PathMSBuild = "C:\Program Files\Microsoft Visual Studio\2022\Community\Msbuild\Current\Bin\amd64"
$PathSln = "FlexKit.sln"

Write-Output "PathMSBuild = $PathMSBuild"
Write-Output "PathSolutionFile = $PathSln"

# append path to env
$Env:path += ";" + $PathMSBuild

&"msbuild" $PathSln -t:restore -p:RestorePackagesConfig=true -maxcpucount:32

&"msbuild" $PathSln -target:TextureStreamingTest -target:AnimationTest -target:MergePathSortTest -target:HairRenderingExample  "/p:BuildInParallel=true" "/p:Configuration=Release" "/p:CL_MPcount=36"  -maxcpucount:32
if(!$?) { Exit $LASTEXITCODE }

git clone https://gitlab.monotonezombie.com/game-dev/scrap_assets.git assets
if(!$?) { Exit $LASTEXITCODE }

Copy-Item -Recurse -Path "assets\game_assets" "Builds\release\Assets"
if(!$?) { Exit $LASTEXITCODE }

Copy-Item -Path "assets\dxil.dll" "Builds\release\dxil.dll"
if(!$?) { Exit $LASTEXITCODE }


if(-Not (Test-Path -Path "Builds\release\Assets\Shaders")){
    Copy-Item -Recurse -Path "Shaders" "Builds\release\Assets\Shaders"
    if(!$?) { Exit $LASTEXITCODE }
}

if(!$?) { Exit $LASTEXITCODE }
