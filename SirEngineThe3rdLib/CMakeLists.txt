cmake_minimum_required(VERSION 3.6.0)

project(SirEngineThe3rdLib)   

    #looking for  files
    file(GLOB_RECURSE SOURCE_FILES "src/*.cpp" "src/*.h")

    message (STATUS ${CMAKE_CURRENT_SOURCE_DIR})
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/vendors/includes
						${CMAKE_CURRENT_SOURCE_DIR}/src
	)

	#lets add specific windows build definitions
	if (WIN32)
		add_compile_definitions(SE_PLATFORM_WINDOWS SE_BUILD_DLL _UNICODE QUIT_ESCAPE)
	endif (WIN32)

	#making sure to add the common cpp flags, that are defined in the main cpp file
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_CXX_FLAGS}")


    #adding the executable
    add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${INCLUDES_FILES})
	#set(CMAKE_BINARY_DIR  )
	#SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
	SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

	target_link_libraries(${PROJECT_NAME} d3dcompiler DXGI D3D12)
	#d3dcompiler.lib;DXGI.lib;D3D12.lib;