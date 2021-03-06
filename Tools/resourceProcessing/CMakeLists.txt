cmake_minimum_required(VERSION 3.13.0)

include (${CMAKE_SOURCE_DIR}/cmake/cmakeUtils.cmake)

project(ResourceProcessing)   

	if(${BUILD_VULKAN})
		find_package(Vulkan REQUIRED)
	endif(${BUILD_VULKAN})

	file(GLOB_RECURSE SOURCE_FILES "src/resourceProcessing/*.cpp" "src/resourceProcessing/*.h")

    #looking for  files
    message (STATUS ${CMAKE_CURRENT_SOURCE_DIR})
    include_directories(
						${CMAKE_SOURCE_DIR}/builtDependencies/includes
						${CMAKE_CURRENT_SOURCE_DIR}/src
						${CMAKE_SOURCE_DIR}/SirEngineThe3rdLib/src
						${CMAKE_SOURCE_DIR}/vendors/glm
						${Vulkan_INCLUDE_DIRS}
	)

	
	

	add_compile_definitions(RC_PLATFORM_WINDOWS SE_PLATFORM_WINDOWS _UNICODE _CRT_SECURE_NO_WARNINGS)

	#grouping in the IDE based on path
	SOURCE_GROUP_BY_FOLDER(ResourceProcessing)

	#making sure to add the common cpp flags, that are defined in the main cpp file
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_CXX_FLAGS}")

	if(WIN32)
		link_directories(${CMAKE_SOURCE_DIR}/builtDependencies/windows/$<CONFIGURATION>)
	endif()

    #adding the executable
    add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES})
	SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
	target_link_libraries(${PROJECT_NAME} SirEngineThe3rdLib crunch Compressonator)

	if(${BUILD_VULKAN})
		target_link_libraries(${PROJECT_NAME} ${Vulkan_LIBRARIES} )
		target_link_libraries(${PROJECT_NAME}  debug glslangd  optimized glslang)
		target_link_libraries(${PROJECT_NAME}  debug HLSLd optimized HLSL)
		target_link_libraries(${PROJECT_NAME}  debug OGLCompilerd optimized OGLCompiler)
		target_link_libraries(${PROJECT_NAME}  debug OSDependentd optimized OSDependent)
		target_link_libraries(${PROJECT_NAME}  debug SPIRVd optimized SPIRV)
		target_link_libraries(${PROJECT_NAME}  debug spirv-cross-cored optimized spirv-cross-core)
		target_link_libraries(${PROJECT_NAME}  debug spirv-cross-hlsld optimized spirv-cross-hlsl)
		target_link_libraries(${PROJECT_NAME}  debug spirv-cross-glsld optimized spirv-cross-glsl)
		target_link_libraries(${PROJECT_NAME}  debug spirv-cross-cppd optimized spirv-cross-cpp)
	endif(${BUILD_VULKAN})

	#setting the plug-in folder in the solution
	set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER tools)

