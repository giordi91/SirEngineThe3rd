cmake_minimum_required(VERSION 3.13.0)

project(ResourceProcessing)   


	file(GLOB_RECURSE SOURCE_FILES "src/resourceProcessing/*.cpp" "src/resourceProcessing/*.h")



    #looking for  files
    message (STATUS ${CMAKE_CURRENT_SOURCE_DIR})
    include_directories(
						${CMAKE_SOURCE_DIR}/builtDependencies/includes
						${CMAKE_CURRENT_SOURCE_DIR}
						${CMAKE_SOURCE_DIR}/SirEngineThe3rdLib/src
						${CMAKE_SOURCE_DIR}/ResourceCompilerLib/src
						${CMAKE_SOURCE_DIR}/vendors/glm
						${CMAKE_CURRENT_SOURCE_DIR}/includes
	)

	
	

	add_compile_definitions(RC_PLATFORM_WINDOWS SE_PLATFORM_WINDOWS _UNICODE _CRT_SECURE_NO_WARNINGS)


	#making sure to add the common cpp flags, that are defined in the main cpp file
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_CXX_FLAGS}")

	if(WIN32)
		link_directories(${CMAKE_SOURCE_DIR}/builtDependencies/windows/$<CONFIGURATION>)
	endif()

    #adding the executable
    add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES})
	SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
	target_link_libraries(${PROJECT_NAME} SirEngineThe3rdLib)

	#setting the plug-in folder in the solution
	set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER tools)

