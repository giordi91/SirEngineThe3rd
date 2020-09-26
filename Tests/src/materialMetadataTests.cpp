#include "SirEngine/io/fileUtils.h"
#include "SirEngine/globals.h"
#include "SirEngine/graphics/materialMetadata.h"
#include "SirEngine/log.h"
#include "SirEngine/memory/cpu/stackAllocator.h"
#include "catch/catch.hpp"

bool isFlagSet(SirEngine::graphics::MATERIAL_RESOURCE_FLAGS flags,
               SirEngine::graphics::MATERIAL_RESOURCE_FLAGS wantedFlag) {
  auto flagsCast = static_cast<uint32_t>(flags);
  auto wantedCast = static_cast<uint32_t>(wantedFlag);
  return (flagsCast & wantedCast) > 0;
}
TEST_CASE("metadata parse 2", "[material]") {
  const char* path = "../testData/forwardPhongPSO.json";
  SirEngine::globals::FRAME_ALLOCATOR = new SirEngine::StackAllocator();
  SirEngine::globals::FRAME_ALLOCATOR->initialize(1024 * 1024 * 10);
  SirEngine::graphics::MaterialMetadata metadata =
      SirEngine::graphics::extractMetadataFromPSO(path);

  REQUIRE(metadata.objectResourceCount == 10);
  REQUIRE(metadata.frameResourceCount == 1);
  REQUIRE(metadata.passResourceCount == 4);

  REQUIRE(metadata.objectResources[0].type ==
          SirEngine::graphics::MATERIAL_RESOURCE_TYPE::CONSTANT_BUFFER);
  REQUIRE(strcmp(metadata.objectResources[0].name, "g_push") == 0);
  REQUIRE(metadata.objectResources[0].set == 3);
  REQUIRE(metadata.objectResources[0].binding == 0);
  REQUIRE(metadata.objectResources[0].visibility ==
          SirEngine::GRAPHICS_RESOURCE_VISIBILITY_VERTEX);
  REQUIRE(isFlagSet(metadata.objectResources[0].flags,
                    SirEngine::graphics::MATERIAL_RESOURCE_FLAGS::PUSH_CONSTANT_BUFFER));

  REQUIRE(metadata.objectResources[1].type ==
          SirEngine::graphics::MATERIAL_RESOURCE_TYPE::BUFFER);
  REQUIRE(strcmp(metadata.objectResources[1].name, "vertices") == 0);
  REQUIRE(metadata.objectResources[1].set == 3);
  REQUIRE(metadata.objectResources[1].binding == 1);
  REQUIRE(metadata.objectResources[1].visibility ==
          SirEngine::GRAPHICS_RESOURCE_VISIBILITY_VERTEX);
  REQUIRE(isFlagSet(metadata.objectResources[1].flags,
                    SirEngine::graphics::MATERIAL_RESOURCE_FLAGS::READ_ONLY));

  REQUIRE(metadata.objectResources[2].type ==
          SirEngine::graphics::MATERIAL_RESOURCE_TYPE::BUFFER);
  REQUIRE(strcmp(metadata.objectResources[2].name, "normals") == 0);
  REQUIRE(metadata.objectResources[2].set == 3);
  REQUIRE(metadata.objectResources[2].binding == 2);
  REQUIRE(metadata.objectResources[2].visibility ==
          SirEngine::GRAPHICS_RESOURCE_VISIBILITY_VERTEX);
  REQUIRE(isFlagSet(metadata.objectResources[2].flags,
                    SirEngine::graphics::MATERIAL_RESOURCE_FLAGS::READ_ONLY));

  REQUIRE(metadata.objectResources[3].type ==
          SirEngine::graphics::MATERIAL_RESOURCE_TYPE::BUFFER);
  REQUIRE(strcmp(metadata.objectResources[3].name, "uvs") == 0);
  REQUIRE(metadata.objectResources[3].set == 3);
  REQUIRE(metadata.objectResources[3].binding == 3);
  REQUIRE(metadata.objectResources[3].visibility ==
          SirEngine::GRAPHICS_RESOURCE_VISIBILITY_VERTEX);
  REQUIRE(isFlagSet(metadata.objectResources[3].flags,
                    SirEngine::graphics::MATERIAL_RESOURCE_FLAGS::READ_ONLY));

  REQUIRE(metadata.objectResources[4].type ==
          SirEngine::graphics::MATERIAL_RESOURCE_TYPE::BUFFER);
  REQUIRE(strcmp(metadata.objectResources[4].name, "tangents") == 0);
  REQUIRE(metadata.objectResources[4].set == 3);
  REQUIRE(metadata.objectResources[4].binding == 4);
  REQUIRE(metadata.objectResources[4].visibility ==
          SirEngine::GRAPHICS_RESOURCE_VISIBILITY_VERTEX);
  REQUIRE(isFlagSet(metadata.objectResources[4].flags,
                    SirEngine::graphics::MATERIAL_RESOURCE_FLAGS::READ_ONLY));

  REQUIRE(metadata.objectResources[5].type ==
          SirEngine::graphics::MATERIAL_RESOURCE_TYPE::TEXTURE);
  REQUIRE(strcmp(metadata.objectResources[5].name, "albedoTex") == 0);
  REQUIRE(metadata.objectResources[5].set == 3);
  REQUIRE(metadata.objectResources[5].binding == 5);
  REQUIRE(metadata.objectResources[5].visibility ==
          SirEngine::GRAPHICS_RESOURCE_VISIBILITY_FRAGMENT);

  REQUIRE(metadata.objectResources[6].type ==
          SirEngine::graphics::MATERIAL_RESOURCE_TYPE::TEXTURE);
  REQUIRE(strcmp(metadata.objectResources[6].name, "tangentTex") == 0);
  REQUIRE(metadata.objectResources[6].set == 3);
  REQUIRE(metadata.objectResources[6].binding == 6);
  REQUIRE(metadata.objectResources[6].visibility ==
          SirEngine::GRAPHICS_RESOURCE_VISIBILITY_FRAGMENT);

  REQUIRE(metadata.objectResources[7].type ==
          SirEngine::graphics::MATERIAL_RESOURCE_TYPE::TEXTURE);
  REQUIRE(strcmp(metadata.objectResources[7].name, "metallicTex") == 0);
  REQUIRE(metadata.objectResources[7].set == 3);
  REQUIRE(metadata.objectResources[7].binding == 7);
  REQUIRE(metadata.objectResources[7].visibility ==
          SirEngine::GRAPHICS_RESOURCE_VISIBILITY_FRAGMENT);
  REQUIRE(metadata.objectResources[8].type ==
          SirEngine::graphics::MATERIAL_RESOURCE_TYPE::TEXTURE);
  REQUIRE(strcmp(metadata.objectResources[8].name, "roughnessTex") == 0);
  REQUIRE(metadata.objectResources[8].set == 3);
  REQUIRE(metadata.objectResources[8].binding == 8);
  REQUIRE(metadata.objectResources[8].visibility ==
          SirEngine::GRAPHICS_RESOURCE_VISIBILITY_FRAGMENT);

  REQUIRE(metadata.objectResources[9].type ==
          SirEngine::graphics::MATERIAL_RESOURCE_TYPE::CONSTANT_BUFFER);
  REQUIRE(strcmp(metadata.objectResources[9].name, "materialConfig") == 0);
  REQUIRE(metadata.objectResources[9].set == 3);
  REQUIRE(metadata.objectResources[9].binding == 9);
  REQUIRE(metadata.objectResources[9].visibility ==
          SirEngine::GRAPHICS_RESOURCE_VISIBILITY_FRAGMENT);

	
}
TEST_CASE("metadata parse 1", "[material]") {
  const char* path = "../testData/grassForwardPSO.json";
  SirEngine::globals::FRAME_ALLOCATOR = new SirEngine::StackAllocator();
  SirEngine::globals::FRAME_ALLOCATOR->initialize(1024 * 1024 * 10);
  SirEngine::graphics::MaterialMetadata metadata =
      SirEngine::graphics::extractMetadataFromPSO(path);

  REQUIRE(metadata.objectResourceCount == 7);
  REQUIRE(metadata.frameResourceCount == 1);
  REQUIRE(metadata.passResourceCount == 4);

  REQUIRE(metadata.objectResources[0].type ==
          SirEngine::graphics::MATERIAL_RESOURCE_TYPE::BUFFER);
  REQUIRE(strcmp(metadata.objectResources[0].name, "points") == 0);
  REQUIRE(metadata.objectResources[0].set == 3);
  REQUIRE(metadata.objectResources[0].binding == 0);
  REQUIRE(metadata.objectResources[0].visibility ==
          SirEngine::GRAPHICS_RESOURCE_VISIBILITY_VERTEX);
  REQUIRE(isFlagSet(metadata.objectResources[0].flags,
                    SirEngine::graphics::MATERIAL_RESOURCE_FLAGS::READ_ONLY));

  REQUIRE(metadata.objectResources[1].type ==
          SirEngine::graphics::MATERIAL_RESOURCE_TYPE::BUFFER);
  REQUIRE(strcmp(metadata.objectResources[1].name, "tileIndices") == 0);
  REQUIRE(metadata.objectResources[1].set == 3);
  REQUIRE(metadata.objectResources[1].binding == 1);
  REQUIRE(metadata.objectResources[1].visibility ==
          SirEngine::GRAPHICS_RESOURCE_VISIBILITY_VERTEX);
  REQUIRE(isFlagSet(metadata.objectResources[1].flags,
                    SirEngine::graphics::MATERIAL_RESOURCE_FLAGS::READ_ONLY));

  REQUIRE(metadata.objectResources[2].type ==
          SirEngine::graphics::MATERIAL_RESOURCE_TYPE::TEXTURE);
  REQUIRE(strcmp(metadata.objectResources[2].name, "windTex") == 0);
  REQUIRE(metadata.objectResources[2].set == 3);
  REQUIRE(metadata.objectResources[2].binding == 2);
  REQUIRE(metadata.objectResources[2].visibility ==
          SirEngine::GRAPHICS_RESOURCE_VISIBILITY_VERTEX);

  REQUIRE(metadata.objectResources[3].type ==
          SirEngine::graphics::MATERIAL_RESOURCE_TYPE::CONSTANT_BUFFER);
  REQUIRE(strcmp(metadata.objectResources[3].name, "grassConfig") == 0);
  REQUIRE(metadata.objectResources[3].set == 3);
  REQUIRE(metadata.objectResources[3].binding == 3);
  REQUIRE(metadata.objectResources[3].visibility ==
          (SirEngine::GRAPHICS_RESOURCE_VISIBILITY_VERTEX |
           SirEngine::GRAPHICS_RESOURCE_VISIBILITY_FRAGMENT));

  REQUIRE(metadata.objectResources[4].type ==
          SirEngine::graphics::MATERIAL_RESOURCE_TYPE::TEXTURE);
  REQUIRE(strcmp(metadata.objectResources[4].name, "albedoTex") == 0);
  REQUIRE(metadata.objectResources[4].set == 3);
  REQUIRE(metadata.objectResources[4].binding == 4);
  REQUIRE(metadata.objectResources[4].visibility ==
          SirEngine::GRAPHICS_RESOURCE_VISIBILITY_FRAGMENT);

  REQUIRE(metadata.objectResources[5].type ==
          SirEngine::graphics::MATERIAL_RESOURCE_TYPE::BUFFER);
  REQUIRE(strcmp(metadata.objectResources[5].name, "tilesCulling") == 0);
  REQUIRE(metadata.objectResources[5].set == 3);
  REQUIRE(metadata.objectResources[5].binding == 5);
  REQUIRE(metadata.objectResources[5].visibility ==
          SirEngine::GRAPHICS_RESOURCE_VISIBILITY_VERTEX);
  REQUIRE(isFlagSet(metadata.objectResources[5].flags,
                    SirEngine::graphics::MATERIAL_RESOURCE_FLAGS::READ_ONLY));

  REQUIRE(metadata.objectResources[6].type ==
          SirEngine::graphics::MATERIAL_RESOURCE_TYPE::BUFFER);
  REQUIRE(strcmp(metadata.objectResources[6].name, "lodToUse") == 0);
  REQUIRE(metadata.objectResources[6].set == 3);
  REQUIRE(metadata.objectResources[6].binding == 6);
  REQUIRE(metadata.objectResources[6].visibility ==
          SirEngine::GRAPHICS_RESOURCE_VISIBILITY_VERTEX);
  REQUIRE(isFlagSet(metadata.objectResources[6].flags,
                    SirEngine::graphics::MATERIAL_RESOURCE_FLAGS::READ_ONLY));
}
