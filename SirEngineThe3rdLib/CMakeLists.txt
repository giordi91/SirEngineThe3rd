cmake_minimum_required(VERSION 3.13.0)

project(SirEngineThe3rdLib)   

    #looking for  files
    file(GLOB_RECURSE SOURCE_FILES "src/*.cpp" "src/*.h")

    message (STATUS ${CMAKE_CURRENT_SOURCE_DIR})
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/vendors/includes
						${CMAKE_CURRENT_SOURCE_DIR}/src
	)

	#lets add specific windows build definitions
	if (WIN32)
		add_compile_definitions(SE_PLATFORM_WINDOWS SE_BUILD_DLL _UNICODE QUIT_ESCAPE GRAPHICS_API=DX12)
	endif (WIN32)

	#making sure to add the common cpp flags, that are defined in the main cpp file
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_CXX_FLAGS}")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DSE_DEBUG")


	MACRO(SOURCE_GROUP_BY_FOLDER target)
	  SET(SOURCE_GROUP_DELIMITER "/")
	  SET(last_dir "")
	  SET(files "")
	  FOREACH(file ${SOURCE_FILES})
		file(RELATIVE_PATH relative_file "${PROJECT_SOURCE_DIR}/{target}" ${file})
		GET_FILENAME_COMPONENT(dir "${relative_file}" PATH)
		IF (NOT "${dir}" STREQUAL "${last_dir}")
		  IF (files)
			SOURCE_GROUP("${last_dir}" FILES ${files})
		  ENDIF (files)
		  SET(files "")
		ENDIF (NOT "${dir}" STREQUAL "${last_dir}")
		SET(files ${files} ${file})
		SET(last_dir "${dir}")
	  ENDFOREACH(file)
	  IF (files)
		SOURCE_GROUP("${last_dir}" FILES ${files})
	  ENDIF (files)
	ENDMACRO(SOURCE_GROUP_BY_FOLDER)

	SOURCE_GROUP_BY_FOLDER(SirEngineThe3rdLib)

    #adding the executable
    add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${INCLUDES_FILES})

	SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

	target_link_libraries(${PROJECT_NAME} d3dcompiler DXGI D3D12 imgui DXTK12 farmhash)

