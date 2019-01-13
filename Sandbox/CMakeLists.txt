cmake_minimum_required(VERSION 3.6.0)

project(Sandbox)   

    #looking for  files
    file(GLOB_RECURSE SOURCE_FILES "src/*.cpp" "src/*.h")

    include_directories(${CMAKE_SOURCE_DIR}/SirEngineThe3rdLib/src
				${CMAKE_SOURCE_DIR}/SirEngineThe3rdLib/vendors/includes
	)

	#lets check if we are on windows:
	if (WIN32)
		add_compile_definitions(SE_PLATFORM_WINDOWS _UNICODE)
	endif (WIN32)

    #defining standard compiling flags
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_CXX_FLAGS}")
	#if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	#	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4324 /wd4146 /wd4458 /wd4267 /wd4100 /wd4244 /wd4141 /wd4291 /wd4624 ")
	#	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MD")
	#	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
	#endif()

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_CXX_FLAGS}")

    #adding the executable
	add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${INCLUDES_FILES})
	SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)


	target_link_libraries(${PROJECT_NAME} d3dcompiler DXGI D3D12 SirEngineThe3rdLib)
	#d3dcompiler.lib;DXGI.lib;D3D12.lib;