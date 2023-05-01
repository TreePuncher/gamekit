$ErrorActionPreference = "Continue"

Write-Output "Running build on $Env:computername ..."

$PathMSBuild = "C:\Program Files\Microsoft Visual Studio\2022\Community\Msbuild\Current\Bin\amd64"
$PathSln = "FlexKit.sln"

Write-Output "PathMSBuild = $PathMSBuild"
Write-Output "PathSolutionFile = $PathSln"

# append path to env
$Env:path += ";" + $PathMSBuild

&"msbuild" $PathSln -t:restore -p:RestorePackagesConfig=true
&"msbuild" $PathSln -target:TextureStreamingTest "/p:Configuration=Release"

if(!$?)
{
    &"msbuild" $PathSln -target:TextureStreamingTest "/p:Configuration=Release"
}

if(!$?) { Exit $LASTEXITCODE }
