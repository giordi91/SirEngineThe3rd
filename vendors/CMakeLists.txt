cmake_minimum_required(VERSION 3.6.0)

project(buildDependencies)
# Set global property (all targets are impacted)
set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CMAKE_COMMAND} -E time")
#preventing in source build
if ( ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR} )
        message( FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt." )
    endif()


##enable c++ 17
#set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF)



#check the compiler
MESSAGE(STATUS "compiler id ${CMAKE_CXX_COMPILER_ID} ${MSVC}")
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  # using Clang
  set(COMMON_CXX_FLAGS "${COMMON_CXX_FLAGS} -g3 -O0 -Wall -pedantic -Wextra -m64 -mavx2 -mfma -ffast-math")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  # using GCC
  set(COMMON_CXX_FLAGS "${COMMON_CXX_FLAGS}   -Wall -pedantic -Wextra -m64 -mavx2 -mfma -ffast-math")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  # using Intel C++
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  # using Visual Studio C++
  set(COMMON_CXX_FLAGS "${COMMON_CXX_FLAGS}  /std:c++17 /W4 /arch:AVX2 /fp:fast /MP")
endif()


add_subdirectory(lua)
add_subdirectory(Compressonator)
add_subdirectory(crunch)
add_subdirectory(imgui)
add_subdirectory(DirectXTK12)
add_subdirectory(farmhash)
add_subdirectory(volk)


file(COPY "Catch2/single_include/catch2/" DESTINATION ${CMAKE_SOURCE_DIR}/../builtDependencies/includes/catch )
file(COPY "spdlog/include/spdlog" DESTINATION ${CMAKE_SOURCE_DIR}/../builtDependencies/includes/ )

file(COPY "cxxopts/include/cxxopts.hpp" DESTINATION ${CMAKE_SOURCE_DIR}/../builtDependencies/includes/cxxopts )

file(COPY "tinyobjloader/tiny_obj_loader.h" DESTINATION ${CMAKE_SOURCE_DIR}/../builtDependencies/includes/tinyobjloader )
file(COPY "json/include/nlohmann" DESTINATION ${CMAKE_SOURCE_DIR}/../builtDependencies/includes/ )
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/../builtDependencies/includes/amd)
file(COPY "display-library/include/adl_defines.h" DESTINATION ${CMAKE_SOURCE_DIR}/../builtDependencies/includes/amd )
file(COPY "display-library/include/adl_sdk.h" DESTINATION ${CMAKE_SOURCE_DIR}/../builtDependencies/includes/amd )
file(COPY "display-library/include/adl_structures.h" DESTINATION ${CMAKE_SOURCE_DIR}/../builtDependencies/includes/amd )

file(COPY "stb/stb_image.h" DESTINATION ${CMAKE_SOURCE_DIR}/../builtDependencies/includes/stb )

#pix event, might want to do this only on windows
if(WIN32)
file(COPY "PixEvent/Include/WinPixEventRuntime" DESTINATION ${CMAKE_SOURCE_DIR}/../builtDependencies/includes/WinPixEventRuntime)
file(COPY "PixEvent/bin/WinPixEventRuntime.lib" DESTINATION ${CMAKE_SOURCE_DIR}/../builtDependencies/windows/Release)
file(COPY "PixEvent/bin/WinPixEventRuntime.lib" DESTINATION ${CMAKE_SOURCE_DIR}/../builtDependencies/windows/Debug)
endif()
                                                                                                                        



