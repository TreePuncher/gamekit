function Test-ReparsePoint([string]$path) {
  
  return 
}


$file = Get-Item "Builds\release\assets" -Force -ea SilentlyContinue

if(![bool]($file.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
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
}