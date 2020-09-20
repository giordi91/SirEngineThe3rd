cmake_minimum_required(VERSION 3.13.0)

project(VkShaderCompilerPlugin)   

	set_property(GLOBAL PROPERTY USE_FOLDERS ON)

	find_package(Vulkan REQUIRED)
    #looking for  files
    file(GLOB_RECURSE SOURCE_FILES "*.cpp" "*.h")

    include_directories(
						${CMAKE_SOURCE_DIR}/builtDependencies/includes
						${CMAKE_CURRENT_SOURCE_DIR}
						${CMAKE_SOURCE_DIR}/SirEngineThe3rdLib/src
						${CMAKE_SOURCE_DIR}/ResourceCompilerLib/src
						${Vulkan_INCLUDE_DIRS}
						${CMAKE_SOURCE_DIR}/vendors/glm
	)

	#lets add specific windows build definitions
	if (WIN32)
		add_compile_definitions(RC_PLATFORM_WINDOWS RC_BUILD_PLUGIN_DLL SE_PLATFORM_WINDOWS _UNICODE _CRT_SECURE_NO_WARNINGS)
		add_compile_definitions(VK_USE_PLATFORM_WIN32_KHR VK_NO_PROTOTYPES)
	endif (WIN32)

	ENABLE_SYSTEM_HEADERS()
	ADD_EXTERNAL_HEADER(${CMAKE_SOURCE_DIR}/vendors/glm)
	ADD_EXTERNAL_HEADER(${CMAKE_SOURCE_DIR}/vendors/gli)
	FINALIZE_EXTERNAL_HEADER()

	#making sure to add the common cpp flags, that are defined in the main cpp file
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_CXX_FLAGS}")

	if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
		#LNK4099 is for missing pbd of the compiled dependencies, I won't debug them so i don't add them
		# to git to save space
		add_compile_options(/wd4251 /wd4275 /wd4099)
	endif()

	if(WIN32)
		link_directories(${CMAKE_SOURCE_DIR}/builtDependencies/windows/$<CONFIGURATION>)
	endif()
    #adding the executable
    add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${INCLUDES_FILES})

	#setting the plug-in folder in the solution
	set_target_properties (${PROJECT_NAME} PROPERTIES
    FOLDER plugins)

    #message (STATUS  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} SHIT)
	target_link_libraries(${PROJECT_NAME} SirEngineThe3rdLib ResourceCompilerLib)

	target_link_libraries(${PROJECT_NAME} ${Vulkan_LIBRARIES} )
	target_link_libraries(${PROJECT_NAME}  debug glslangd  optimized glslang)
	target_link_libraries(${PROJECT_NAME}  debug HLSLd optimized HLSL)
	target_link_libraries(${PROJECT_NAME}  debug OGLCompilerd optimized OGLCompiler)
	target_link_libraries(${PROJECT_NAME}  debug OSDependentd optimized OSDependent)
	target_link_libraries(${PROJECT_NAME}  debug SPIRVd optimized SPIRV)
	target_link_libraries(${PROJECT_NAME}  debug spirv-cross-cored optimized spirv-cross-core)
	target_link_libraries(${PROJECT_NAME}  debug spirv-cross-cppd optimized spirv-cross-cpp)
	target_link_libraries(${PROJECT_NAME}  debug spirv-cross-hlsld optimized spirv-cross-hlsl)
	target_link_libraries(${PROJECT_NAME}  debug spirv-cross-glsld optimized spirv-cross-glsl)

	#This madness is needed because in order to have a plugin folder inside the 
	#output dir, I have to do a post copy, by default on windows it adds a Debug/Release 
	#after you build it. putting plugins in the wrong folder, might need to put this
	#being a if(WIN32)
	add_custom_command(
    TARGET ${PROJECT_NAME} 
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:${PROJECT_NAME}>
        ${CMAKE_BINARY_DIR}/bin/$<CONFIGURATION>/plugins/$<TARGET_FILE_NAME:${PROJECT_NAME}>
	)

