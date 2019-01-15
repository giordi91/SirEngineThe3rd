cmake_minimum_required(VERSION 3.13.0)

project(ResourceCompilerLib)   

    #looking for  files
    file(GLOB_RECURSE SOURCE_FILES "ResourceCompilerLib/*.cpp" "ResourceCompilerLib/*.h")

    message (STATUS ${CMAKE_CURRENT_SOURCE_DIR})
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/vendors/includes
						${CMAKE_CURRENT_SOURCE_DIR}
						${CMAKE_SOURCE_DIR}/SirEngineThe3rdLib/src
						${CMAKE_SOURCE_DIR}/SirEngineThe3rdLib/vendors/includes
	)

	#lets add specific windows build definitions
	if (WIN32)
		add_compile_definitions(RC_PLATFORM_WINDOWS RC_BUILD_DLL SE_PLATFORM_WINDOWS _UNICODE)
	endif (WIN32)

	#making sure to add the common cpp flags, that are defined in the main cpp file
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_CXX_FLAGS}")


    #adding the executable
    add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${INCLUDES_FILES})
	SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
	target_link_libraries(${PROJECT_NAME} SirEngineThe3rdLib)

