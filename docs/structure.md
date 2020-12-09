# Engine structure

## Folder structure
As soon as you clone the respository, you will be faced with many folders,
let us start break them down a bit to figure out what is what.

- builtDependencies : this is where the result of the "vendors" cmake goes. Due to the build getting too slow, the dependencies build step has been separated and the built dependencies committed in the repo in this folder.
- cmake: utility files for cmake
- data: this is where I store small files that the engine uses, like shaders, materials, pso etc. Mostly kept in the repo to make it easy to work on. Heavy assets like makes and textures are not here
- Editor: Here is where the source of the Editor lives, the Editor can be considered an application leveraging the engine library to do what it needs
- SirEngineThe3rdLib: This is the source for the engine itself
- Tools: This folders contains all the tooling around the engine, from the resource processing to tools to generate data like the grass points
- Tests: The engine is not fully tested, but what is tested lives here
- vendoers: the folder with all the dependencies' source (mostly submodules).

I am not going to spend too much on all the folders, I will be mostly focusing on the engine codebase, which is the most interesting part, but I might be biased :D 

## Generic VS Platform/API specific

The Engine source code tries to split immediately the generic code from the platform/API specific code. Whatever is generic like animation code, scripting engine or graphics interfaces lives in the SirEngine folder, everything else is done inside platform.

Organization inside platform is not great and probably too nested, but the most importa thing to notice it that it branches down into Vulkan and Dx12 folders, this is whre the bulk of the graphics code lives and we are going to investigate this shortly. 

## From main to first frame 

Here we are going to go through how the games starts from main and ends up into the main loop.

The actual entry point of the game is inside the engine library and not inside the application, I am not particularly sold on this setup but it comes mostly from how [The Cherno](https://www.youtube.com/watch?v=meARMOmTLgE&list=PLlrATfBNZ98dC-V-N3m0Go4deliWHPFwT&index=5) setups his engine and I was curious to try it.
You can find the entry point in the [entryPoint.h](https://github.com/giordi91/SirEngineThe3rd/blob/develop/SirEngineThe3rdLib/src/SirEngine/entryPoint.h) file.
The entry point will be in charge to instantiate the application.

The instantiation happens in the createApplication method, which is actually the method that needs to be provided by the "application/game". That is how the inversion of the entry point is implemented.

### Layers

The application is is a collection of "Layers" layers encapuslate the required logic and can be stacked on top of each other. They will be evaluated bottom to top every frame. 
For example the editor application has two layers, the "rendering" layer and the "editor layer". 
The firstlayer is in charge to render the actual frame, the "editor" layer instead  encapuslates the whole editor UI and logic and interaction for setupping levels, resources etc (well, if it was fully implemented, which is not, is just a stub for now).

By not creating the editor layer and not adding it to the stack, it should be possible to completely remove the editor overhead (most of it at least) and be left with a lean runtime engine.

### Application
The editor applicaton (or the game), is a subclass of the Application class. The base class has built in the logic of spawing a window, iterating the layeres, sending events to the layers and so on. 
The only thing that our Editor application does extra is instantiating the right layers and push them on the stack.

When the base application is instantiated the whole engine gets bootstrapped. A configuration file is read and used to setup the window etc. At the same time all the engine managers get created. After the application construction the engine is ready to go!

Once the ```run()``` method is called on the application the main loop is started, ```run()``` will never return unless shut down has been requested.

During the main loop the layers are evaluated, events processed and the frame is rendered. For more informations check the [application.cpp](https://github.com/giordi91/SirEngineThe3rd/blob/develop/SirEngineThe3rdLib/src/SirEngine/application.cpp)
file
## Managers
The engine heavily relies on the concept of handles and managers to load and manage data. I have wrote a 
[blog article](https://giordi91.github.io/post/resourcesystem/)
about it. 

The base class of the manager is declared in the SirEngine part of the library, meanwhile the actual API Specific implementation will live in platform. 
For example the generic interface for a texture manager lives 
[here](https://github.com/giordi91/SirEngineThe3rd/blob/develop/SirEngineThe3rdLib/src/SirEngine/textureManager.h)
meanwhile here you can find the 
[VK](https://github.com/giordi91/SirEngineThe3rd/blob/develop/SirEngineThe3rdLib/src/platform/windows/graphics/vk/vkTextureManager.h)
version and here the 
[dx12](https://github.com/giordi91/SirEngineThe3rd/blob/develop/SirEngineThe3rdLib/src/platform/windows/graphics/dx12/dx12TextureManager.h)
version. 

## You implementation

The custom part of the logic the user wishes to implement comes in the form of a layer, for example the editor layer in the Editor project.
You can see in there custom logic to load data, build a render graph and then render. 

You are not forced to use a render graph, is just one of the tools you can use to setup your frame. What you do in the graphics layer is up to you.

# Core 
A lot of code has been written to support the engine, especially when it comes to memory management, a lot of custom allocators/containers have been writen and can be found here: 
[here](https://github.com/giordi91/SirEngineThe3rd/tree/develop/SirEngineThe3rdLib/src/SirEngine/memory).
version. 
Tests for the containers to give an idea in how they works can be found  
[here](https://github.com/giordi91/SirEngineThe3rd/tree/develop/Tests/src).

# Conclusion
This was a brief overview of some part of the engine, if you wish to know more about specific part open an issue and I will expand on it.
