cmake_minimum_required(VERSION 3.13.0)

project(Tests)   

    #looking for  files
	file(GLOB_RECURSE SOURCE_FILES "src/*.cpp" "src/*.h")


    message (STATUS ${CMAKE_CURRENT_SOURCE_DIR})
    include_directories(
						${CMAKE_CURRENT_SOURCE_DIR}
						${CMAKE_SOURCE_DIR}/SirEngineThe3rdLib/src
						${CMAKE_SOURCE_DIR}/ResourceCompilerLib/src
						${CMAKE_SOURCE_DIR}/builtDependencies/includes
	)

	#lets add specific windows build definitions
	if (WIN32)
		add_compile_definitions(RC_PLATFORM_WINDOWS SE_PLATFORM_WINDOWS _UNICODE)
	endif (WIN32)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DSE_DEBUG")

	#making sure to add the common cpp flags, that are defined in the main cpp file
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_CXX_FLAGS}")

	file(COPY "testData" DESTINATION "${CMAKE_BINARY_DIR}/bin" )


	if(WIN32)
		link_directories(${CMAKE_SOURCE_DIR}/builtDependencies/windows/$<CONFIGURATION>)
	endif()
    #adding the executable
    add_executable(${PROJECT_NAME} ${SOURCE_FILES})
	SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
	target_link_libraries(${PROJECT_NAME} SirEngineThe3rdLib)

	#setting working directory
	set_target_properties(
    ${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIGURATION>)

