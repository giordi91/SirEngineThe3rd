cmake_minimum_required(VERSION 3.13.0)

project(ShaderCompilerPlugin)   

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

	#making sure to add the common cpp flags, that are defined in the main cpp file
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_CXX_FLAGS}")


	link_directories(${CMAKE_BINARY_DIR}/bin/$<CONFIGURATION>)
    #adding the executable
    add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${INCLUDES_FILES})

	#setting the plug-in folder in the solution
	set_target_properties (${PROJECT_NAME} PROPERTIES
    FOLDER plugins)

    #message (STATUS  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} SHIT)
	target_link_libraries(${PROJECT_NAME} SirEngineThe3rdLib ResourceCompilerLib d3dcompiler.lib dxcompiler.lib  DXGI.lib D3D12.lib)

	#This madness is needed because in order to have a plugin folder inside the 
	#output dir, I have to do a post copy, by default on windows it adds a Debug/Release 
	#after you build it. putting plugins in the wrong folder, might need to put this
	#being a if(WIN32)
	add_custom_command(
    TARGET ${PROJECT_NAME} 
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:${PROJECT_NAME}>
        ${CMAKE_BINARY_DIR}/bin/$<CONFIGURATION>/plugins/$<TARGET_FILE_NAME:${PROJECT_NAME}>
	)

	#lets try to find and copy the dll we need
	set(VS_TOOLKIT_PATH "C:/Program Files (x86)/Windows Kits/10/bin/")

	#C:\Program Files (x86)\Windows Kits\10\bin\10.0.17763.0\x64
	MESSAGE(STATUS "---------------------------------------------> ${VS_TOOLKIT_PATH}${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/x64")

	SET(DXIL_DLL_PATH "${VS_TOOLKIT_PATH}${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/x64/dxil.dll")
	SET(DXC_DLL_PATH "${VS_TOOLKIT_PATH}${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/x64/dxcompiler.dll")

	if(EXISTS ${DXIL_DLL_PATH})
		add_custom_command(
		TARGET ${PROJECT_NAME} 
		PRE_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy
		   ${DXIL_DLL_PATH}
		   ${CMAKE_BINARY_DIR}/bin/$<CONFIGURATION>/dxil.dll
		)
	else()
		MESSAGE(WARNING "Could not find dxil.dll, tried to copy from: ${DXIL_DLL_PATH}")
	endif()

	if(EXISTS ${DXC_DLL_PATH})
		add_custom_command(
		TARGET ${PROJECT_NAME} 
		PRE_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy
		   ${DXC_DLL_PATH}
		   ${CMAKE_BINARY_DIR}/bin/$<CONFIGURATION>/dxcompiler.dll
		)
	else()
		MESSAGE(WARNING "Could not find dxcompiler.dll, tried to copy from: ${DXC_DLL_PATH}")
	endif()
