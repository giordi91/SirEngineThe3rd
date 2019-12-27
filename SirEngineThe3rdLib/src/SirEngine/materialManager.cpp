#include "SirEngine/materialManager.h"
#include <cassert>
#include <string>
#include <unordered_map>

namespace SirEngine {
/*
namespace materialKeys {
static const char *KD = "kd";
static const char *KS = "ks";
static const char *KA = "ka";
static const char *SHINESS = "shiness";
static const char *ALBEDO = "albedo";
static const char *NORMAL = "normal";
static const char *METALLIC = "metallic";
static const char *ROUGHNESS = "roughness";
static const char *HEIGHT = "height";
static const char *AO = "ao";
static const char *SEPARATE_ALPHA = "separateAlpha";
static const char *ROUGHNESS_MULT = "roughnessMult";
static const char *METALLIC_MULT = "metallicMult";
static const char *THICKNESS = "thickness";
static const char *QUEUE = "queue";
static const char *TYPE = "type";
static const std::string DEFAULT_STRING = "";
static const char *RS_KEY = "rs";
static const char *PSO_KEY = "pso";

static const std::unordered_map<std::string, SirEngine::SHADER_QUEUE_FLAGS>
    STRING_TO_QUEUE_FLAG{
        {"forward", SirEngine::SHADER_QUEUE_FLAGS::FORWARD},
        {"deferred", SirEngine::SHADER_QUEUE_FLAGS::DEFERRED},
        {"shadow", SirEngine::SHADER_QUEUE_FLAGS::SHADOW},
    };
static const std::unordered_map<std::string, SirEngine::SHADER_TYPE_FLAGS>
    STRING_TO_TYPE_FLAGS{
        {"pbr", SirEngine::SHADER_TYPE_FLAGS::PBR},
        {"forwardPbr", SirEngine::SHADER_TYPE_FLAGS::FORWARD_PBR},
        {"forwardPhong", SirEngine::SHADER_TYPE_FLAGS::FORWARD_PHONG},
        {"forwardPhongAlphaCutout",
         SirEngine::SHADER_TYPE_FLAGS::FORWARD_PHONG_ALPHA_CUTOUT},
        {"skin", SirEngine::SHADER_TYPE_FLAGS::SKIN},
        {"hair", SirEngine::SHADER_TYPE_FLAGS::HAIR},
        {"hairSkin", SirEngine::SHADER_TYPE_FLAGS::HAIRSKIN},
        {"debugLinesColors", SirEngine::SHADER_TYPE_FLAGS::DEBUG_LINES_COLORS},
        {"debugLinesSingleColor",
         SirEngine::SHADER_TYPE_FLAGS::DEBUG_LINES_SINGLE_COLOR},
        {"debugPointsColors",
         SirEngine::SHADER_TYPE_FLAGS::DEBUG_POINTS_COLORS},
        {"debugPointsSingleColor",
         SirEngine::SHADER_TYPE_FLAGS::DEBUG_POINTS_SINGLE_COLOR},
        {"debugTrianglesColors",
         SirEngine::SHADER_TYPE_FLAGS::DEBUG_TRIANGLE_COLORS},
        {"debugTrianglesSingleColor",
         SirEngine::SHADER_TYPE_FLAGS::DEBUG_TRIANGLE_SINGLE_COLOR},
        {"skinCluster", SirEngine::SHADER_TYPE_FLAGS::SKINCLUSTER},
        {"skinSkinCluster", SirEngine::SHADER_TYPE_FLAGS::SKINSKINCLUSTER},
        {"forwardPhongAlphaCutoutSkin",
         SirEngine::SHADER_TYPE_FLAGS::FORWARD_PHONG_ALPHA_CUTOUT_SKIN},
        {"forwardParallax", SirEngine::SHADER_TYPE_FLAGS::FORWARD_PARALLAX},
        {"shadowSkinCluster",
         SirEngine::SHADER_TYPE_FLAGS::SHADOW_SKIN_CLUSTER},
    };
static const std::unordered_map<SirEngine::SHADER_TYPE_FLAGS, std::string>
    TYPE_FLAGS_TO_STRING{
        {SirEngine::SHADER_TYPE_FLAGS::PBR, "pbr"},
        {SirEngine::SHADER_TYPE_FLAGS::FORWARD_PBR, "forwardPbr"},
        {SirEngine::SHADER_TYPE_FLAGS::FORWARD_PHONG_ALPHA_CUTOUT,
         "forwardPhongAlphaCutout"},
        {SirEngine::SHADER_TYPE_FLAGS::SKIN, "skin"},
        {SirEngine::SHADER_TYPE_FLAGS::HAIR, "hair"},
        {SirEngine::SHADER_TYPE_FLAGS::HAIRSKIN, "hairSkin"},
        {SirEngine::SHADER_TYPE_FLAGS::DEBUG_LINES_COLORS, "debugLinesColors"},
        {SirEngine::SHADER_TYPE_FLAGS::DEBUG_LINES_SINGLE_COLOR,
         "debugLinesSingleColor"},
        {SirEngine::SHADER_TYPE_FLAGS::DEBUG_POINTS_COLORS,
         "debugPointsColors"},
        {SirEngine::SHADER_TYPE_FLAGS::DEBUG_POINTS_SINGLE_COLOR,
         "debugPointsSingleColor"},
        {SirEngine::SHADER_TYPE_FLAGS::DEBUG_TRIANGLE_COLORS,
         "debugTrianglesColors"},
        {SirEngine::SHADER_TYPE_FLAGS::DEBUG_TRIANGLE_SINGLE_COLOR,
         "debugTrianglesSingleColor"},
        {SirEngine::SHADER_TYPE_FLAGS::SKINCLUSTER, "skinCluster"},
        {SirEngine::SHADER_TYPE_FLAGS::SKINSKINCLUSTER, "skinSkinCluster"},
        {SirEngine::SHADER_TYPE_FLAGS::FORWARD_PHONG_ALPHA_CUTOUT_SKIN,
         "forwardPhongAlphaCutoutSkin"},
        {SirEngine::SHADER_TYPE_FLAGS::FORWARD_PARALLAX, "forwardParallax"},
        {SirEngine::SHADER_TYPE_FLAGS::SHADOW_SKIN_CLUSTER,
         "shadowSkinCluster"},
    };

} // namespace materialKeys

const char *
MaterialManager::getStringFromShaderTypeFlag(const SHADER_TYPE_FLAGS type) {
  const auto found = materialKeys::TYPE_FLAGS_TO_STRING.find(type);
  if (found != materialKeys::TYPE_FLAGS_TO_STRING.end()) {
    return found->second.c_str();
  }

  assert(0 && "Could not find flag");
  const auto unknown =
      materialKeys::TYPE_FLAGS_TO_STRING.find(SHADER_TYPE_FLAGS::UNKNOWN);
  return unknown->second.c_str();
}
*/
} // namespace SirEngine
