cmake_minimum_required(VERSION 3.13.0)

project(SirEngineThe3rd)
# Set global property (all targets are impacted)
set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CMAKE_COMMAND} -E time")
#preventing in source build
if ( ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR} )
        message( FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there. You may need to remove CMakeCache.txt." )
    endif()


#enable c++ 17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)



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
  set(COMMON_CXX_FLAGS "${COMMON_CXX_FLAGS}  /std:c++17 /W4 /arch:AVX2 /fp:fast /MP /DNOMINMAX")


endif()


#options
option(BUILD_AMD "Wheter or not build on AMD hardware" OFF)
option(BUILD_DX12 "Wheter or not build on AMD hardware" ON)
option(BUILD_VULKAN "Wheter or not build on AMD hardware" OFF)#off because does not build on github actions

#just an overal log of the passed options
MESSAGE( STATUS "Building with the following options")
MESSAGE( STATUS "BUILD AMD:                    " ${BUILD_AMD})
MESSAGE( STATUS "BUILD DX12:                   " ${BUILD_DX12})
MESSAGE( STATUS "BUILD Vulkan:                 " ${BUILD_VULKAN})


set_property(GLOBAL PROPERTY USE_FOLDERS ON)

#subfolders
add_subdirectory(SirEngineThe3rdLib)
add_subdirectory(Editor)
add_subdirectory(Tools/resourceCompiler)
add_subdirectory(Tools/resourceProcessing)
add_subdirectory(Tools/pointScatter/cpp)
add_subdirectory(Tests)

