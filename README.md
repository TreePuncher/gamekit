# FlexKit
A small game engine focusing on power and flexibility. The goal is to make writing a performant game engine simpler and quickler with simple but powerful components. The engine is built around an ECS, a frame graph, and a work stealing work scheduler. Only windows 10 is targetable at the moment.

external Dependencies
  * AngelScript
  * Autodesk FBX 
  * Nvidia PhysX
  * DirectX 12
  * fmod
  *	stb_image
  * boost
  * Qt

Additional dependencies are pulled in automatically via git submodules.

## Build Instructions:

### Windows:
Install fmod sdk, fbx sdk, and vcpkg. 
clone and open TestGame.sln
Hit F7 to build. Vcpkg will grab all other dependencies. 

## Screenshots:
### PBR shading
![PBR Shading](/screenshots/sponza.jpg)
### Clustered light culling
![Clustered Light Culling](/screenshots/CLC.jpg)
### PCF Shadow Mapping
![PCF Shadow Mapping](/screenshots/PCFShadowMapping.jpg)
![Contact hardening with PCSS](/screenshots/PercentageCloserSoftShadows.jpg)
### Streaming Tiled Textures
![TextureStreaming And Tiled Resources](/screenshots/StreamingTiledTextures.jpg)
### Basic Profiler
![Basic in engine profiler](/screenshots/profiler.png)
