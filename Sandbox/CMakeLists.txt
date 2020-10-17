cmake_minimum_required(VERSION 3.13.0)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

include (${CMAKE_SOURCE_DIR}/cmake/cmakeUtils.cmake)

project(Sandbox)   
	if(${BUILD_VULKAN})
		find_package(Vulkan REQUIRED)
	endif(${BUILD_VULKAN})

    #looking for  files
    file(GLOB_RECURSE SOURCE_FILES "src/*.cpp" "src/*.h")

    include_directories(
		${CMAKE_SOURCE_DIR}/SirEngineThe3rdLib/src
		${CMAKE_SOURCE_DIR}/builtDependencies/includes
		${CMAKE_CURRENT_SOURCE_DIR}/src
		${Vulkan_INCLUDE_DIRS}
		${CMAKE_SOURCE_DIR}/vendors/glm
		${CMAKE_SOURCE_DIR}/vendors/gli
	)

	#lets check if we are on windows:
	#lets add specific windows build definitions
	if (WIN32)
		#TODO add D3DX12_NO_STATE_OBJECT_HELPERS
		add_compile_definitions(NO_MIN_MAX _CRT_NONSTDC_NO_DEPRECATE WIN32_LEAN_AND_MEAN  )
		if(${BUILD_AMD})
			add_compile_definitions(BUILD_AMD SE_PLATFORM_WINDOWS _CRT_SECURE_NO_WARNINGS _UNICODE QUIT_ESCAPE GRAPHICS_API=DX12)
		else(${BUILD_AMD})
			add_compile_definitions(SE_PLATFORM_WINDOWS _CRT_SECURE_NO_WARNINGS _UNICODE QUIT_ESCAPE)
		endif(${BUILD_AMD})

		if(${BUILD_VULKAN})
			add_compile_definitions(VK_USE_PLATFORM_WIN32_KHR VK_NO_PROTOTYPES BUILD_VK)
		endif(${BUILD_VULKAN})
		if(${BUILD_DX12})
			add_compile_definitions(BUILD_DX12)
		endif()
	endif (WIN32)

	ENABLE_SYSTEM_HEADERS()
	ADD_EXTERNAL_HEADER(${CMAKE_SOURCE_DIR}/vendors/glm)
	ADD_EXTERNAL_HEADER(${CMAKE_SOURCE_DIR}/vendors/gli)
	ADD_EXTERNAL_HEADER(${CMAKE_SOURCE_DIR}/vendors/spdlog)
	FINALIZE_EXTERNAL_HEADER()

	if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
		#LNK4099 is for missing pbd of the compiled dependencies, I won't debug them so i don't add them
		# to git to save space
		# LNK4204 is similar to 4099
		add_compile_options(/wd4251 /wd4275 /wd4099 /wd4204 /IGNORE:LNK4204 )
	endif()

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_CXX_FLAGS}")

	link_directories(${CMAKE_SOURCE_DIR}/builtDependencies/windows/$<CONFIGURATION>)
    #adding the executable
	add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${INCLUDES_FILES})
	SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)


	target_link_libraries(${PROJECT_NAME} d3dcompiler DXGI D3D12 SirEngineThe3rdLib WinPixEventRuntime lualib)

	SET_WORKING_DIRECTORY("${CMAKE_BINARY_DIR}/bin/$<CONFIGURATION>")