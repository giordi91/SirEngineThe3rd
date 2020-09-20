cmake_minimum_required(VERSION 3.13.0)

project(PSOCompilerPlugin)   

	set_property(GLOBAL PROPERTY USE_FOLDERS ON)


    #looking for  files
    file(GLOB_RECURSE SOURCE_FILES "*.cpp" "*.h")

    include_directories(
						${CMAKE_SOURCE_DIR}/builtDependencies/includes
						${CMAKE_CURRENT_SOURCE_DIR}
						${CMAKE_SOURCE_DIR}/SirEngineThe3rdLib/src
						${CMAKE_SOURCE_DIR}/ResourceCompilerLib/src
						${CMAKE_SOURCE_DIR}/vendors/glm/
	)

	#lets add specific windows build definitions
	if (WIN32)
		add_compile_definitions(RC_PLATFORM_WINDOWS RC_BUILD_PLUGIN_DLL SE_PLATFORM_WINDOWS _UNICODE _CRT_SECURE_NO_WARNINGS)
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

	endif (WIN32)

	#making sure to add the common cpp flags, that are defined in the main cpp file
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_CXX_FLAGS}")


	if(WIN32)
		link_directories(${CMAKE_SOURCE_DIR}/builtDependencies/windows/$<CONFIGURATION>)
	endif()
    #adding the executable
    add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${INCLUDES_FILES})

	set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER plugins)

	target_link_libraries(${PROJECT_NAME} SirEngineThe3rdLib ResourceCompilerLib)

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
