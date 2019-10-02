
A dx12 game engine (kind of).

# SirEngineThe3rd [![Actions Status](https://github.com/giordi91/SirEngineThe3rd/workflows/pipelines/badge.svg)](https://github.com/giordi91/SirEngineThe3rd/actions) 


Initially inspired by The Cherno project game engine series: https://www.youtube.com/user/TheChernoProject

This is my attempt to a dx12 engine, this is my third iteration to a so called engine. 
My first one was a poor Opengl viewport, then a more serious approach to a dx11 engine, but mostly geared toward getting stuff on screen quickly. 
Finally this is my third attempt trying to use dx12. 

It is my pleasure to introduce you to Sir Engine, the 3rd of his name.

Here a I will keep a chronological list of the progress:

##### Table of Contents  
[0.1.0: basic window](#v010)  
[0.2.0: basic engine arch](#v020)  
[0.3.0: PBR shader](#v030)  
[0.3.0: Fully skinned character](#v040)  

## 0.1.0 <a name="v010"/>
This version is the most basic version of the engine, but starts to put togheter the foundation of the engine:
* Event system
* Layer stack
* Imgui
* Dx12 init
* Swap chain resize and clear color
* Started basic resource compiler for meshes

More info on resource compiler here:
link-coming-soon

![alt text](./images/01_clearImgui.jpg "test")

## 0.2.0 <a name="v020"/>
This next version of the engine adds the major facilities of the engine arch, render
loop is still hard-coded and will come in 0.3.0, added features:
* Asset managers, with concept of identity for an asset
* Several managers to allocated resources and keep track of it
* Initial memory layout, intended fast path for rendering, anything custom will be off the fast path
* A lot of work on ResourceCompiler, everything that could be compiled is using the compiler textures include
* Now assets can be rendered in a simple way, without knowing how many you have upfront etc.

More info on this new system here:
link-coming-soon

![alt text](./images/02_basicEngineArch.jpg "test")

## 0.3.0 <a name="v030"/>
This main goal of this version was to give a first good crack at the graphics: features:
* PBR shader
* basic workflow for the needed textures in a PBR shader
* Added mip map generation of textures in the compiler to reduce aliasing
* Basic rendering graph
* AMD work to fetch data out of the card like core freq etc

![alt text](./images/03_PBR.png "test")

## 0.4.0 <a name="v040"/>
The main goeal of this release was to get a full character in, this required several shader
to be created for the head, and animation skinning implementation.
Features:
* Screen space subsurface scattering, basic implementation, required extension to use stencil
* Alpha cut out for hair, added forward pass to support transparen shader
* Added animation import and evaluation, (compiler plugin to compile animations)
* Added debug renderer to debug skeletons
* Added skincluster shaders and matrices upload
* Skinning required refactoring of the whole buffer management

![alt text](./images/04_walk.gif "test")

***Credits***

Copyright (c) 2010-2016 Richard Geldreich, Jr., Tenacious Software, and Binomial LLC
