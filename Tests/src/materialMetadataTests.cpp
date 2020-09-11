#include "SirEngine/fileUtils.h"
#include "SirEngine/globals.h"
#include "SirEngine/log.h"
#include "SirEngine/materialManager.h"
#include "SirEngine/memory/cpu/stackAllocator.h"
#include "catch/catch.hpp"

TEST_CASE("metadata parse 1", "[material]") {
  const char* path = "../testData/grassForwardPSO.json";
  SirEngine::globals::FRAME_ALLOCATOR = new SirEngine::StackAllocator();
  SirEngine::globals::FRAME_ALLOCATOR->initialize(1024 * 1024 * 10);
  SirEngine::MaterialMetadata metadata = SirEngine::extractMetadata(path);

  REQUIRE(metadata.objectResourceCount == 7);
  REQUIRE(metadata.frameResourceCount == 1);
  REQUIRE(metadata.passResourceCount == 4);

  REQUIRE(metadata.objectResources[0].type ==
          SirEngine::MATERIAL_RESOURCE_TYPE::BUFFER);
  REQUIRE(strcmp(metadata.objectResources[0].name, "vertices") == 0);
  REQUIRE(metadata.objectResources[0].set == 3);
  REQUIRE(metadata.objectResources[0].binding == 0);
  REQUIRE(metadata.objectResources[0].visibility ==
          SirEngine::GRAPHICS_RESOURCE_VISIBILITY_VERTEX);
  REQUIRE(metadata.objectResources[0].flags ==
          SirEngine::MATERIAL_RESOURCE_FLAGS::READ_ONLY);

  REQUIRE(metadata.objectResources[1].type ==
          SirEngine::MATERIAL_RESOURCE_TYPE::BUFFER);
  REQUIRE(strcmp(metadata.objectResources[1].name, "tilesIndices") == 0);
  REQUIRE(metadata.objectResources[1].set == 3);
  REQUIRE(metadata.objectResources[1].binding == 1);
  REQUIRE(metadata.objectResources[1].visibility ==
          SirEngine::GRAPHICS_RESOURCE_VISIBILITY_VERTEX);
  REQUIRE(metadata.objectResources[1].flags ==
          SirEngine::MATERIAL_RESOURCE_FLAGS::READ_ONLY);

  REQUIRE(metadata.objectResources[2].type ==
          SirEngine::MATERIAL_RESOURCE_TYPE::TEXTURE);
  REQUIRE(strcmp(metadata.objectResources[2].name, "windTex") == 0);
  REQUIRE(metadata.objectResources[2].set == 3);
  REQUIRE(metadata.objectResources[2].binding == 2);
  REQUIRE(metadata.objectResources[2].visibility ==
          SirEngine::GRAPHICS_RESOURCE_VISIBILITY_VERTEX);

  REQUIRE(metadata.objectResources[3].type ==
          SirEngine::MATERIAL_RESOURCE_TYPE::CONSTANT_BUFFER);
  REQUIRE(strcmp(metadata.objectResources[3].name, "ConfigData") == 0);
  REQUIRE(metadata.objectResources[3].set == 3);
  REQUIRE(metadata.objectResources[3].binding == 3);
  REQUIRE(metadata.objectResources[3].visibility ==
          (SirEngine::GRAPHICS_RESOURCE_VISIBILITY_VERTEX |
           SirEngine::GRAPHICS_RESOURCE_VISIBILITY_FRAGMENT));

  REQUIRE(metadata.objectResources[4].type ==
          SirEngine::MATERIAL_RESOURCE_TYPE::TEXTURE);
  REQUIRE(strcmp(metadata.objectResources[4].name, "albedoTex") == 0);
  REQUIRE(metadata.objectResources[4].set == 3);
  REQUIRE(metadata.objectResources[4].binding == 4);
  REQUIRE(metadata.objectResources[4].visibility ==
          SirEngine::GRAPHICS_RESOURCE_VISIBILITY_FRAGMENT);

  REQUIRE(metadata.objectResources[5].type ==
          SirEngine::MATERIAL_RESOURCE_TYPE::BUFFER);
  REQUIRE(strcmp(metadata.objectResources[5].name, "tilesCulling") == 0);
  REQUIRE(metadata.objectResources[5].set == 3);
  REQUIRE(metadata.objectResources[5].binding == 5);
  REQUIRE(metadata.objectResources[5].visibility ==
          SirEngine::GRAPHICS_RESOURCE_VISIBILITY_VERTEX);
  REQUIRE(metadata.objectResources[5].flags ==
          SirEngine::MATERIAL_RESOURCE_FLAGS::READ_ONLY);

  REQUIRE(metadata.objectResources[6].type ==
          SirEngine::MATERIAL_RESOURCE_TYPE::BUFFER);
  REQUIRE(strcmp(metadata.objectResources[6].name, "lodData") == 0);
  REQUIRE(metadata.objectResources[6].set == 3);
  REQUIRE(metadata.objectResources[6].binding == 6);
  REQUIRE(metadata.objectResources[6].visibility ==
          SirEngine::GRAPHICS_RESOURCE_VISIBILITY_VERTEX);
  REQUIRE(metadata.objectResources[6].flags ==
          SirEngine::MATERIAL_RESOURCE_FLAGS::READ_ONLY);
}
