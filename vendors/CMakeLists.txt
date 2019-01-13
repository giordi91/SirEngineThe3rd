cmake_minimum_required(VERSION 3.6.0)


file(COPY "spdlog/include/spdlog" DESTINATION "${CMAKE_SOURCE_DIR}/SirEngineThe3rdLib/vendors/includes" )
file(COPY "cxxopts/include/cxxopts.hpp" DESTINATION "${CMAKE_SOURCE_DIR}/ResourceCompiler/vendors/includes/cxxopts" )

file(COPY "cxxopts/include/cxxopts.hpp" DESTINATION "${CMAKE_SOURCE_DIR}/ModelCompilerPlugin/vendors/includes/cxxopts" )
file(COPY "tinyobjloader/tiny_obj_loader.h" DESTINATION "${CMAKE_SOURCE_DIR}/ModelCompilerPlugin/vendors/includes/tinyobjloader" )
file(COPY "json/include/nlohmann" DESTINATION "${CMAKE_SOURCE_DIR}/ModelCompilerPlugin/vendors/includes" )


add_subdirectory(imgui)

