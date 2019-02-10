A dx12 game engine (kind of).

Initially inspired by The Cherno project game engine series: https://www.youtube.com/user/TheChernoProject

This is my attempt to a dx12 engine, this is my third iteration to a so called engine. 
My first one was a poor Opengl viewport, then a more serious approach to a dx11 engine, but mostly geared toward getting stuff on screen quickly. 
Finally this is my third attempt trying to use dx12. 

It is my pleasure to introduce you to Sir Engine, the 3rd of his name.

Here a I will keep a chronological list of the progress:

##### Table of Contents  
[0.1.0: basic window](#v010)  
[0.2.0: basic engine arch](#v020)  

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

