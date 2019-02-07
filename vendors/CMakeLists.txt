cmake_minimum_required(VERSION 3.6.0)


file(COPY "Catch2/single_include/catch2/" DESTINATION "${CMAKE_SOURCE_DIR}/Tests/vendors/includes/catch" )
file(COPY "spdlog/include/spdlog" DESTINATION "${CMAKE_SOURCE_DIR}/SirEngineThe3rdLib/vendors/includes" )

file(COPY "cxxopts/include/cxxopts.hpp" DESTINATION "${CMAKE_SOURCE_DIR}/ResourceCompiler/vendors/includes/cxxopts" )
file(COPY "cxxopts/include/cxxopts.hpp" DESTINATION "${CMAKE_SOURCE_DIR}/ResourceCompilerPlugins/ModelCompilerPlugin/vendors/includes/cxxopts" )
file(COPY "cxxopts/include/cxxopts.hpp" DESTINATION "${CMAKE_SOURCE_DIR}/ResourceCompilerPlugins/rootSignatureCompilerPlugin/vendors/includes/cxxopts" )
file(COPY "cxxopts/include/cxxopts.hpp" DESTINATION "${CMAKE_SOURCE_DIR}/ResourceCompilerPlugins/TextureCompilerPlugin/vendors/includes/cxxopts" )


file(COPY "Compressonator/Compressonator/Header" DESTINATION "${CMAKE_SOURCE_DIR}/ResourceCompilerPlugins/TextureCompilerPlugin/vendors/includes/Compressonator" )


file(COPY "tinyobjloader/tiny_obj_loader.h" DESTINATION "${CMAKE_SOURCE_DIR}/ResourceCompilerPlugins/ModelCompilerPlugin/vendors/includes/tinyobjloader" )
file(COPY "json/include/nlohmann" DESTINATION "${CMAKE_SOURCE_DIR}/ResourceCompilerLib/vendors/includes" )
file(COPY "json/include/nlohmann" DESTINATION "${CMAKE_SOURCE_DIR}/SirEngineThe3rdLib/vendors/includes" )

file(COPY "stb/stb_image.h" DESTINATION "${CMAKE_SOURCE_DIR}/ResourceCompilerPlugins/TextureCompilerPlugin/vendors/includes/stb" )


add_subdirectory(imgui)
add_subdirectory(DirectXTK12)
add_subdirectory(Compressonator)
add_subdirectory(DirectXTex)
add_subdirectory(farmhash)

