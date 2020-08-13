cmake_minimum_required(VERSION 3.13.0)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

include (${CMAKE_SOURCE_DIR}/cmake/cmakeUtils.cmake)

project(SirEngineThe3rdLib)   
	if(${BUILD_VULKAN})
		find_package(Vulkan REQUIRED)
	endif(${BUILD_VULKAN})

    #looking for  files
	#generic platform indepenent sir engine files
    file(GLOB_RECURSE SOURCE_FILES_SIR_ENGINE "src/SirEngine/*.cpp" "src/SirEngine/*.h")
	list (APPEND SOURCE_FILES ${SOURCE_FILES_SIR_ENGINE})

	#windows specific files like the window
    file(GLOB SOURCE_FILES_WINDOWS "src/platform/windows/*.cpp" "src/platform/windows/*.h")
	list (APPEND SOURCE_FILES ${SOURCE_FILES_WINDOWS})

	#dx12 api backend
	if(${BUILD_DX12})
		file(GLOB_RECURSE SOURCE_FILES_DX12 "src/platform/windows/graphics/dx12/*.cpp" "src/platform/windows/graphics/dx12/*.h")
		list (APPEND SOURCE_FILES ${SOURCE_FILES_DX12})
	endif(${BUILD_DX12})

	#vulkan api backend
	if(${BUILD_VULKAN})
		file(GLOB_RECURSE SOURCE_FILES_VK "src/platform/windows/graphics/vk/*.cpp" "src/platform/windows/graphics/vk/*.h")
		list (APPEND SOURCE_FILES ${SOURCE_FILES_VK})
	endif(${BUILD_VULKAN})

	#specific source files used on AMD hardware only (like hardware queries)
	if(${BUILD_AMD})
		file(GLOB SOURCE_FILES_HARDWARE_AMD "src/platform/windows/hardware/AMD/*.cpp" "src/platform/windows/hardware/AMD/*.h")
		list (APPEND SOURCE_FILES ${SOURCE_FILES_HARDWARE_AMD})
	endif(${BUILD_AMD})


    message (STATUS ${CMAKE_CURRENT_SOURCE_DIR})

    include_directories(
						${CMAKE_SOURCE_DIR}/builtDependencies/includes
						${CMAKE_CURRENT_SOURCE_DIR}/src
						${Vulkan_INCLUDE_DIRS}
						"../vendors/meshoptimizer/src/"
						${CMAKE_SOURCE_DIR}/vendors/glm
						${CMAKE_SOURCE_DIR}/vendors/gli
	)

	#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /experimental:external  /external:I ${CMAKE_SOURCE_DIR}/vendors/glm /external:W0"  )

	ENABLE_SYSTEM_HEADERS()
	ADD_EXTERNAL_HEADER(${CMAKE_SOURCE_DIR}/vendors/glm)
	ADD_EXTERNAL_HEADER(${CMAKE_SOURCE_DIR}/vendors/gli)
	FINALIZE_EXTERNAL_HEADER()

	if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
		#LNK4099 is for missing pbd of the compiled dependencies, I won't debug them so i don't add them
		# to git to save space
		add_compile_options(/wd4251 /wd4275 /wd4099)
	endif()

	#lets add specific windows build definitions
	if (WIN32)
		add_compile_definitions(NO_MIN_MAX _CRT_NONSTDC_NO_DEPRECATE)
		if(${BUILD_AMD})
			add_compile_definitions(BUILD_AMD SE_PLATFORM_WINDOWS SE_BUILD_DLL _CRT_SECURE_NO_WARNINGS _UNICODE QUIT_ESCAPE GRAPHICS_API=DX12)
		else(${BUILD_AMD})
			add_compile_definitions(SE_PLATFORM_WINDOWS SE_BUILD_DLL _CRT_SECURE_NO_WARNINGS _UNICODE QUIT_ESCAPE)
		endif(${BUILD_AMD})

		if(${BUILD_VULKAN})
			add_compile_definitions(VK_USE_PLATFORM_WIN32_KHR VK_NO_PROTOTYPES BUILD_VK)
		endif(${BUILD_VULKAN})
		if(${BUILD_DX12})
			add_compile_definitions(BUILD_DX12)
		endif()
	endif (WIN32)



	#making sure to add the common cpp flags, that are defined in the main cpp file
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_CXX_FLAGS}")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DSE_DEBUG")

	#grouping in the IDE based on path
	SOURCE_GROUP_BY_FOLDER(SirEngineThe3rdLib)

	#finding all the files that are not c++, like shader, json definitions etc
	#but is nice to have in the editor for quick editing
    file(GLOB_RECURSE ASSETS_FILES "../data/assets/*.json" )
	SOURCE_GROUP("data/assets" FILES ${ASSETS_FILES})
    file(GLOB MATERIAL_FILES "../data/materials/*.json" )
    file(GLOB MATERIAL_TYPES_FILES "../data/materials/types/*.json" )
	SOURCE_GROUP("data/materials" FILES ${MATERIAL_FILES})
	SOURCE_GROUP("data/materials/types" FILES ${MATERIAL_TYPES_FILES})
    file(GLOB_RECURSE PSO_FILES "../data/pso/*.json" )
	SOURCE_GROUP("data/pso" FILES ${PSO_FILES})
    file(GLOB_RECURSE RS_FILES "../data/rs/*.json" )
	SOURCE_GROUP("data/rs" FILES ${RS_FILES})
    file(GLOB_RECURSE SHADERS_DX12_COMMON_FILES "../data/shaders/DX12/common/*.hlsl" "../data/shaders/DX12/common/*.h")
    file(GLOB_RECURSE SHADERS_VK_COMMON_FILES "../data/shaders/vk/common/*.glsl" "../data/shaders/vk/common/*.h")
	SOURCE_GROUP("data/shaders/DX12/common" FILES ${SHADERS_DX12_COMMON_FILES})
	SOURCE_GROUP("data/shaders/vk/common" FILES ${SHADERS_VK_COMMON_FILES})
    file(GLOB_RECURSE SHADERS_DX12_RASTERIZATION_FILES "../data/shaders/DX12/rasterization/*.hlsl" )
    file(GLOB_RECURSE SHADERS_DX12_COMPUTE_FILES "../data/shaders/DX12/compute/*.hlsl" )
    file(GLOB_RECURSE SHADERS_VK_RASTERIZATION_FILES "../data/shaders/vk/rasterization/*.glsl" )
	SOURCE_GROUP("data/shaders/DX12/rasterization" FILES ${SHADERS_DX12_RASTERIZATION_FILES})
	SOURCE_GROUP("data/shaders/DX12/compute" FILES ${SHADERS_DX12_COMPUTE_FILES})
	SOURCE_GROUP("data/shaders/vk/rasterization" FILES ${SHADERS_VK_RASTERIZATION_FILES})
    file(GLOB_RECURSE SHADERS_DX12_COMPUTE_FILES "../data/shaders/DX12/compute/*.hlsl" )
    file(GLOB_RECURSE SHADERS_VK_COMPUTE_FILES "../data/shaders/VK/compute/*.glsl" )
	SOURCE_GROUP("data/shaders/DX12/compute" FILES ${SHADERS_DX12_COMPUTE_FILES})
	SOURCE_GROUP("data/shaders/vk/compute" FILES ${SHADERS_VK_COMPUTE_FILES})
    file(GLOB_RECURSE SCENE_FILES "../data/scenes/*.json" )
	SOURCE_GROUP("data/scenes" FILES ${SCENE_FILES})
	SOURCE_GROUP("data" FILES "../data/execute.json")
	SOURCE_GROUP("data" FILES "../data/executeFullDebug.json")
	SOURCE_GROUP("data" FILES "../data/executeFullDebugAMD.json")
	SOURCE_GROUP("data" FILES "../data/executeShaderDebug.json")
	SOURCE_GROUP("data" FILES "../data/executeTexture.json")
	SOURCE_GROUP("data" FILES "../data/engineConfig.json")
	SOURCE_GROUP("data" FILES "../data/executeMeshes.json")
    file(GLOB_RECURSE SCRIPTS_FILES "../data/scripts/*.lua" )
	SOURCE_GROUP("data/scripts" FILES ${SCRIPTS_FILES})



	#NOTE: when passing arrays/lists to macro, to work 
	#properly put the variable in quotes "${MY_VAR}"
	SET_AS_HEADERS("${SHADERS_DX12_RASTERIZATION_FILES}")
	SET_AS_HEADERS("${SHADERS_DX12_COMPUTE_FILES}")
	SET_AS_HEADERS("${SHADERS_DX12_COMMON_FILES}")
	SET_AS_HEADERS("${SHADERS_VK_RASTERIZATION_FILES}")
	SET_AS_HEADERS("${SHADERS_VK_COMPUTE_FILES}")
	SET_AS_HEADERS("${SHADERS_VK_COMPUTE_FILES}")
	SET_AS_HEADERS("${SHADERS_VK_COMMON_FILES}")

	if(WIN32)
		link_directories(${CMAKE_SOURCE_DIR}/builtDependencies/windows/$<CONFIGURATION> )
	endif()

    #adding the executable
    add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${INCLUDES_FILES} ${ASSETS_FILES}
	${MATERIAL_FILES}
	${MATERIAL_TYPES_FILES}
	${PSO_FILES}
	${RS_FILES}
	${SHADERS_DX12_COMMON_FILES}
	${SHADERS_DX12_RASTERIZATION_FILES}
	${SHADERS_DX12_COMPUTE_FILES}
	${SHADERS_VK_COMMON_FILES}
	${SHADERS_VK_RASTERIZATION_FILES}
	${SHADERS_VK_COMPUTE_FILES}
	${SCRIPTS_FILES}
	"../data/execute.json"
	"../data/executeFullDebugAMD.json"
	"../data/executeFullDebug.json"
	"../data/executeShaderDebug.json"
	"../data/executeShaderDebug.json"
	"../data/executeTexture.json"
	"../data/engineConfig.json"
	"../data/executeMeshes.json"
	${SCENE_FILES}
	)

	SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

	target_link_libraries(${PROJECT_NAME} d3dcompiler dxcompiler farmhash DXGI D3D12 imgui DXTK12 WinPixEventRuntime lualib   meshoptimizer)

	if(${BUILD_VULKAN})
		target_link_libraries(${PROJECT_NAME} ${Vulkan_LIBRARIES} )
		target_link_libraries(${PROJECT_NAME}  debug glslangd  optimized glslang)
		target_link_libraries(${PROJECT_NAME}  debug HLSLd optimized HLSL)
		target_link_libraries(${PROJECT_NAME}  debug OGLCompilerd optimized OGLCompiler)
		target_link_libraries(${PROJECT_NAME}  debug OSDependentd optimized OSDependent)
		target_link_libraries(${PROJECT_NAME}  debug SPIRVd optimized SPIRV)
	endif(${BUILD_VULKAN})

	if(WIN32)
		#copying needed libs from pix event dependency to output build folder
		COPY_FILE_TO_BIN_DIRECTORY( "${CMAKE_SOURCE_DIR}/vendors/PixEvent/bin/WinPixEventRuntime.lib")
		COPY_FILE_TO_BIN_DIRECTORY( "${CMAKE_SOURCE_DIR}/vendors/PixEvent/bin/WinPixEventRuntime.dll")
		COPY_FILE_TO_BIN_DIRECTORY( "${CMAKE_SOURCE_DIR}/vendors/PixEvent/bin/WinPixEventRuntime_UAP.lib")
		COPY_FILE_TO_BIN_DIRECTORY( "${CMAKE_SOURCE_DIR}/vendors/PixEvent/bin/WinPixEventRuntime_UAP.dll")
	endif()


