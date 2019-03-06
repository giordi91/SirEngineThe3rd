cmake_minimum_required(VERSION 3.13.0)

project(ResourceCompiler)   




    #looking for  files
    message (STATUS ${CMAKE_CURRENT_SOURCE_DIR})
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/vendors/includes
						${CMAKE_CURRENT_SOURCE_DIR}
						${CMAKE_SOURCE_DIR}/SirEngineThe3rdLib/src
						${CMAKE_SOURCE_DIR}/SirEngineThe3rdLib/vendors/includes
						${CMAKE_SOURCE_DIR}/ResourceCompilerLib/src
						${CMAKE_SOURCE_DIR}/ResourceCompilerLib/vendors/includes
	)

	#lets add specific windows build definitions
	if (WIN32)
		#if on windows brute force copying data in both release and debug, i am done
		#struggling with this. Using a custom command seems to work with a specific file
		#not with folders
		file(COPY "../data" DESTINATION "${CMAKE_BINARY_DIR}/bin/Debug" )
		file(COPY "../data" DESTINATION "${CMAKE_BINARY_DIR}/bin/Release" )
		add_compile_definitions(RC_PLATFORM_WINDOWS SE_PLATFORM_WINDOWS _UNICODE)
	else(WIN32)
		file(COPY "../data" DESTINATION "${CMAKE_BINARY_DIR}/bin" )
	endif (WIN32)

	#making sure to add the common cpp flags, that are defined in the main cpp file
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_CXX_FLAGS}")

	link_directories(${CMAKE_BINARY_DIR}/bin/$<CONFIGURATION>)

    #adding the executable
    add_executable(${PROJECT_NAME} main.cpp)
	SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
	target_link_libraries(${PROJECT_NAME} SirEngineThe3rdLib ResourceCompilerLib)
