cmake_minimum_required(VERSION 3.6.0)


file(COPY "spdlog/include/spdlog" DESTINATION "${CMAKE_SOURCE_DIR}/SirEngineThe3rdLib/vendors/includes" )
file(COPY "cxxopts/include/cxxopts.hpp" DESTINATION "${CMAKE_SOURCE_DIR}/ResourceCompiler/vendors/includes/cxxopts" )
add_subdirectory(imgui)

