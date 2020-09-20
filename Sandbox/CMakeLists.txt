cmake_minimum_required(VERSION 3.13.0)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

include (${CMAKE_SOURCE_DIR}/cmake/cmakeUtils.cmake)

project(Sandbox)   

    #looking for  files
    file(GLOB_RECURSE SOURCE_FILES "src/*.cpp" "src/*.h")

    include_directories(${CMAKE_SOURCE_DIR}/SirEngineThe3rdLib/src
				${CMAKE_SOURCE_DIR}/builtDependencies/includes
	)

	#lets check if we are on windows:
	if (WIN32)
		add_compile_definitions(SE_PLATFORM_WINDOWS _UNICODE _CRT_SECURE_NO_WARNINGS)
	endif (WIN32)

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