cmake_minimum_required(VERSION 3.13.0)

project(SirEngineThe3rdLib)   

    #looking for  files
    file(GLOB_RECURSE SOURCE_FILES "src/*.cpp" "src/*.h")

    message (STATUS ${CMAKE_CURRENT_SOURCE_DIR})
    include_directories(
						${CMAKE_SOURCE_DIR}/builtDependencies/includes
						${CMAKE_CURRENT_SOURCE_DIR}/src
	)

	if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
		add_compile_options(/wd4251 /wd4275)
	endif()

	#lets add specific windows build definitions
	if (WIN32)
		if(${BUILD_AMD})
		add_compile_definitions(BUILD_AMD SE_PLATFORM_WINDOWS SE_BUILD_DLL _CRT_SECURE_NO_WARNINGS _UNICODE QUIT_ESCAPE GRAPHICS_API=DX12)
		else(${BUILD_AMD})
		add_compile_definitions(SE_PLATFORM_WINDOWS SE_BUILD_DLL _CRT_SECURE_NO_WARNINGS _UNICODE QUIT_ESCAPE GRAPHICS_API=DX12)
		endif(${BUILD_AMD})
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


	#finding the shaders stuff
    #looking for  files
    file(GLOB_RECURSE ASSETS_FILES "../data/assets/*.json" )
	SOURCE_GROUP("data/assets" FILES ${ASSETS_FILES})
    file(GLOB MATERIAL_FILES "../data/materials/*.json" )
    file(GLOB MATERIAL_TYPES_FILES "../data/materials/types/*.json" )
	SOURCE_GROUP("data/materials" FILES ${MATERIAL_FILES})
	SOURCE_GROUP("data/materials/types" FILES ${MATERIAL_TYPES_FILES})
    file(GLOB_RECURSE PSO_FILES "../data/pso/*.json" )
	SOURCE_GROUP("data/pso" FILES ${PSO_FILES})
    file(GLOB_RECURSE RS_FILES "../data/rs/*.json" )
	SOURCE_GROUP("data/rs" FILES ${RS_FILES})
    file(GLOB_RECURSE SHADERS_COMMON_FILES "../data/shaders/common/*.hlsl" "../data/shaders/common/*.h")
	SOURCE_GROUP("data/shaders/common" FILES ${SHADERS_COMMON_FILES})
    file(GLOB_RECURSE SHADERS_RASTERIZATION_FILES "../data/shaders/rasterization/*.hlsl" )
	SOURCE_GROUP("data/shaders/rasterization" FILES ${SHADERS_RASTERIZATION_FILES})
    file(GLOB_RECURSE SHADERS_COMPUTE_FILES "../data/shaders/compute/*.hlsl" )
	SOURCE_GROUP("data/shaders/compute" FILES ${SHADERS_COMPUTE_FILES})
    file(GLOB_RECURSE SCENE_FILES "../data/scenes/*.json" )
	SOURCE_GROUP("data/scenes" FILES ${SCENE_FILES})
	SOURCE_GROUP("data" FILES "../data/execute.json")
	SOURCE_GROUP("data" FILES "../data/executeFullDebug.json")
	SOURCE_GROUP("data" FILES "../data/executeFullDebugAMD.json")
	SOURCE_GROUP("data" FILES "../data/executeShaderDebug.json")
	SOURCE_GROUP("data" FILES "../data/executeTexture.json")
	SOURCE_GROUP("data" FILES "../data/engineConfig.json")
	SOURCE_GROUP("data" FILES "../data/executeMeshes.json")



	set_source_files_properties(
            ${SHADERS_RASTERIZATION_FILES}
            PROPERTIES HEADER_FILE_ONLY TRUE
         )
	set_source_files_properties(
            ${SHADERS_COMPUTE_FILES}
            PROPERTIES HEADER_FILE_ONLY TRUE
         )
	set_source_files_properties(
            ${SHADERS_COMMON_FILES}
            PROPERTIES HEADER_FILE_ONLY TRUE
         )

	if(WIN32)
		link_directories(${CMAKE_SOURCE_DIR}/builtDependencies/windows/$<CONFIGURATION>)
	endif()
    #adding the executable
    add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${INCLUDES_FILES} ${ASSETS_FILES}
	${MATERIAL_FILES}
	${MATERIAL_TYPES_FILES}
	${PSO_FILES}
	${RS_FILES}
	${SHADERS_COMMON_FILES}
	${SHADERS_RASTERIZATION_FILES}
	${SHADERS_COMPUTE_FILES}
	"../data/execute.json"
	"../data/executeFullDebugAMD.json"
	"../data/executeFullDebug.json"
	"../data/executeShaderDebug.json"
	"../data/executeShaderDebug.json"
	"../data/executeTexture.json"
	"../data/engineConfig.json"
	"../data/executeMeshes.json"
	${SCENE_FILES}
	)

	SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

	#file(COPY "PixEvent/bin/WinPixEventRuntime.dll" DESTINATION "${CMAKE_BINARY_DIR}/bin")
	target_link_libraries(${PROJECT_NAME} d3dcompiler dxcompiler farmhash DXGI D3D12 imgui DXTK12 WinPixEventRuntime lualib)

	#need to copy pix event
	add_custom_command(
    TARGET ${PROJECT_NAME} 
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
       ${CMAKE_SOURCE_DIR}/vendors/PixEvent/bin/WinPixEventRuntime.dll  
        ${CMAKE_BINARY_DIR}/bin/$<CONFIGURATION>/WinPixEventRuntime.dll
	)
	add_custom_command(
    TARGET ${PROJECT_NAME} 
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
       ${CMAKE_SOURCE_DIR}/vendors/PixEvent/bin/WinPixEventRuntime.lib
        ${CMAKE_BINARY_DIR}/bin/$<CONFIGURATION>/WinPixEventRuntime.lib
	)
	add_custom_command(
    TARGET ${PROJECT_NAME} 
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
       ${CMAKE_SOURCE_DIR}/vendors/PixEvent/bin/WinPixEventRuntime_UAP.lib
        ${CMAKE_BINARY_DIR}/bin/$<CONFIGURATION>/WinPixEventRuntime_UAP.lib
	)
	add_custom_command(
    TARGET ${PROJECT_NAME} 
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
       ${CMAKE_SOURCE_DIR}/vendors/PixEvent/bin/WinPixEventRuntime_UAP.dll
        ${CMAKE_BINARY_DIR}/bin/$<CONFIGURATION>/WinPixEventRuntime_UAP.dll
	)

