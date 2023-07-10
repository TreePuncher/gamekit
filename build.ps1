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

git clone https://monotonezombie.ddns.net/game-dev/scrap_assets.git assets

Copy-Item -Recurse -Path "assets\game_assets" "Builds\release\Assets"

if(-Not (Test-Path -Path "Builds\release\Assets\Shaders")){
    Copy-Item -Recurse -Path "Shaders" "Builds\release\Assets\Shaders"
}

if(!$?) { Exit $LASTEXITCODE }
