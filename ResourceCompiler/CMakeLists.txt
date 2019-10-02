cmake_minimum_required(VERSION 3.13.0)

project(ResourceCompiler)   




    #looking for  files
    message (STATUS ${CMAKE_CURRENT_SOURCE_DIR})
    include_directories(
						${CMAKE_SOURCE_DIR}/builtDependencies/includes
						${CMAKE_CURRENT_SOURCE_DIR}
						${CMAKE_SOURCE_DIR}/SirEngineThe3rdLib/src
						${CMAKE_SOURCE_DIR}/ResourceCompilerLib/src
	)

	
	file(COPY "../data" DESTINATION "${CMAKE_BINARY_DIR}/bin" )
	if(NOT EXISTS "../external")
        	file(MAKE_DIRECTORY "../external")
	endif()

	file(COPY "../external" DESTINATION "${CMAKE_BINARY_DIR}/bin/data" )
	

	add_compile_definitions(RC_PLATFORM_WINDOWS SE_PLATFORM_WINDOWS _UNICODE)

	#making sure right folders are created
	file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin/data/processed)
	file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin/data/processed/meshes)
	file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin/data/processed/meshes/flatGround)
	file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin/data/processed/meshes/knightB)
	file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin/data/processed/meshes/eye)

	file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin/data/processed/rs)
	file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin/data/processed/shaders)
	file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin/data/processed/shaders/compute)
	file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin/data/processed/shaders/rasterization)
	file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin/data/processed/textures)
	file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin/data/processed/textures/cerberus)
	file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin/data/processed/textures/flatGround)
	file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin/data/processed/textures/knightB)
	file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin/data/processed/textures/eye)
	file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin/data/processed/clip/knightB)

	#making sure to add the common cpp flags, that are defined in the main cpp file
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_CXX_FLAGS}")

	if(WIN32)
		link_directories(${CMAKE_SOURCE_DIR}/builtDependencies/windows/$<CONFIGURATION>)
	endif()

    #adding the executable
    add_executable(${PROJECT_NAME} main.cpp)
	SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
	target_link_libraries(${PROJECT_NAME} SirEngineThe3rdLib ResourceCompilerLib)

	#setting working directory
	set_target_properties(
    ${PROJECT_NAME} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIGURATION>)
