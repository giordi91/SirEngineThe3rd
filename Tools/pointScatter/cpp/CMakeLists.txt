cmake_minimum_required(VERSION 3.13.0)

project(ScatterPoint)   

	find_package(OpenMP REQUIRED)
    include_directories(
        ${CMAKE_SOURCE_DIR}/builtDependencies/includes
        ${CMAKE_CURRENT_SOURCE_DIR}
	)

	add_compile_definitions(_UNICODE _CRT_SECURE_NO_WARNINGS)

	#making sure to add the common cpp flags, that are defined in the main cpp file
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_CXX_FLAGS}")

	if(WIN32)
		link_directories(${CMAKE_SOURCE_DIR}/builtDependencies/windows/$<CONFIGURATION>)
	endif()

    #adding the executable
    add_executable(${PROJECT_NAME} main.cpp)
	SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
	target_link_libraries(${PROJECT_NAME} PRIVATE OpenMP::OpenMP_CXX)

	#setting working directory
	set_target_properties(
    ${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIGURATION>)


	#setting the plug-in folder in the solution
	set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER tools)
