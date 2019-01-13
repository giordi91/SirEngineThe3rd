cmake_minimum_required(VERSION 3.6.0)

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
  set(COMMON_CXX_FLAGS "${COMMON_CXX_FLAGS}  /std:c++17 /W4 /arch:AVX2 /fp:fast /MP")
endif()

#options
#option(BUILD_TESTS "Whether or not to build the tests" ON)
#option(BUILD_JIT   "Whether or not to build the jit engine" ON)
#option(BUILD_REPL  "Whether or not to build the interpreter" ON)

#just an overal log of the passed options
#MESSAGE( STATUS "Building with the following options")
#MESSAGE( STATUS "BUILD JIT:                    " ${BUILD_JIT})
#MESSAGE( STATUS "BUILD REPL:                   " ${BUILD_REPL})
#MESSAGE( STATUS "BUILD TESTS:                  " ${BUILD_TESTS})
#adding core

add_subdirectory(vendors)
add_subdirectory(SirEngineThe3rdLib)
add_subdirectory(Sandbox)
add_subdirectory(ResourceCompilerLib)
add_subdirectory(ResourceCompiler)
add_subdirectory(ModelCompilerPlugin)
#if(${BUILD_JIT} STREQUAL "ON")
#add_subdirectory(src/jit)
#endif()
#if(${BUILD_REPL} STREQUAL "ON")
#    add_subdirectory(src/repl)
#endif()

