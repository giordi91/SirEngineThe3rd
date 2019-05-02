cmake_minimum_required(VERSION 3.13.0)

project(textureCompilerPlugin)   

	set_property(GLOBAL PROPERTY USE_FOLDERS ON)


    #looking for  files
    file(GLOB_RECURSE SOURCE_FILES "*.cpp" "*.h")

    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/vendors/includes
						${CMAKE_CURRENT_SOURCE_DIR}
						${CMAKE_SOURCE_DIR}/SirEngineThe3rdLib/src
						${CMAKE_SOURCE_DIR}/SirEngineThe3rdLib/vendors/includes
						${CMAKE_SOURCE_DIR}/ResourceCompilerLib/src
						${CMAKE_SOURCE_DIR}/ResourceCompilerLib/vendors/includes
	)

	#lets add specific windows build definitions
	if (WIN32)
		add_compile_definitions(RC_PLATFORM_WINDOWS RC_BUILD_PLUGIN_DLL SE_PLATFORM_WINDOWS _UNICODE)
	endif (WIN32)

	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DSE_DEBUG")

	#making sure to add the common cpp flags, that are defined in the main cpp file
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_CXX_FLAGS}")

	link_directories(${CMAKE_BINARY_DIR}/bin/$<CONFIGURATION>)

    #adding the executable
    add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${INCLUDES_FILES})

	set_target_properties (${PROJECT_NAME} PROPERTIES
    FOLDER plugins
)

    #message (STATUS  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} SHIT)
	target_link_libraries(${PROJECT_NAME} Compressonator SirEngineThe3rdLib ResourceCompilerLib DirectXTex crunch)

	#This madness is needed because in order to have a plug-in folder inside the 
	#output dir, I have to do a post copy, by default on windows it adds a Debug/Release 
	#after you build it. putting plug-ins in the wrong folder, might need to put this
	#being a if(WIN32)
	add_custom_command(
    TARGET ${PROJECT_NAME} 
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:${PROJECT_NAME}>
        ${CMAKE_BINARY_DIR}/bin/$<CONFIGURATION>/plugins/$<TARGET_FILE_NAME:${PROJECT_NAME}>
	)
