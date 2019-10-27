#include "SirEngine/animation/animationClip.h"
#include "SirEngine/animation/animationManager.h"
#include "SirEngine/argsUtils.h"
#include "SirEngine/fileUtils.h"
#include "SirEngine/log.h"
#include "catch/catch.hpp"
#include "resourceCompilerLib/resourcePlugin.h"

void compileAnim(const char *in, const char *out) {

  PluginRegistry *registry = PluginRegistry::getInstance();
  const ResourceProcessFunction func =
      registry->getFunction("animationCompilerPlugin");
  if (func == nullptr) {
    SE_CORE_ERROR("Resource compiler: could not find requested plugin "
                  "animationCompilerPlugin");
    return;
  }

  func(in, out, "");
}

TEST_CASE("animation key 1", "[animation]") {

  // initialize memory pools and loggers
  SirEngine::StringPool stringPool(1024);
  SirEngine::globals::STRING_POOL = &stringPool;
  SirEngine::Log::init();

  PluginRegistry::init();
  PluginRegistry *registry = PluginRegistry::getInstance();
  registry->loadPluginsInFolder("plugins");
  // compiling animation clip on the fly, then we read it back as the engine
  // would
  compileAnim("../testData/idle1.json", "../testData/idle1.clip");

  SirEngine::Log::free();
}

TEST_CASE("animation no data", "[animation]") {

  // initialize memory pools and loggers
  SirEngine::StringPool stringPool(1024);
  SirEngine::globals::STRING_POOL = &stringPool;
  SirEngine::Log::init();

  PluginRegistry::init();
  PluginRegistry *registry = PluginRegistry::getInstance();
  registry->loadPluginsInFolder("plugins");
  // compiling animation clip on the fly, then we read it back as the engine
  // would
  compileAnim("../testData/noMetaAnim.json", "../testData/noMetaAnim.clip");

  SirEngine::Log::free();
}

TEST_CASE("animation knight", "[animation]") {

  // initialize memory pools and loggers
  SirEngine::StringPool stringPool(1024);
  SirEngine::globals::STRING_POOL = &stringPool;
  SirEngine::Log::init();

  PluginRegistry::init();
  PluginRegistry *registry = PluginRegistry::getInstance();
  registry->loadPluginsInFolder("plugins");
  // compiling animation clip on the fly, then we read it back as the engine
  // would
  compileAnim("../testData/knightBIdle.json", "../testData/knightBIdle.clip");

  SirEngine::Log::free();
}

TEST_CASE("animation key 1 read", "[animation]") {

  // initialize memory pools and loggers
  SirEngine::StringPool stringPool(1024 * 1024 * 10);
  SirEngine::globals::STRING_POOL = &stringPool;
  SirEngine::Log::init();
  SirEngine::ThreeSizesPool pool(1024 * 1024 * 10);
  SirEngine::globals::PERSISTENT_ALLOCATOR = &pool;

  SirEngine::AnimationManager animManager;
  animManager.init();
  SirEngine::globals::ANIMATION_MANAGER = &animManager;

  PluginRegistry::init();
  PluginRegistry *registry = PluginRegistry::getInstance();
  registry->loadPluginsInFolder("plugins");
  // compiling animation clip on the fly, then we read it back as the engine
  // would
  compileAnim("../testData/idle1.json", "../testData/idle1.clip");

  // now we can read it
  SirEngine::AnimationClip *clip =
      animManager.loadAnimationClip("idle1", "../testData/idle1.clip");

  REQUIRE(clip->m_metadataCount == 238);
  REQUIRE(clip->findFirstMetadataFrame(
              SirEngine::ANIM_CLIP_KEYWORDS::L_FOOT_DOWN) == 1);
  REQUIRE(clip->findFirstMetadataFrame(
              SirEngine::ANIM_CLIP_KEYWORDS::R_FOOT_DOWN) == 1);

  SirEngine::Log::free();
}
TEST_CASE("animation key 2 read", "[animation]") {

  // initialize memory pools and loggers
  SirEngine::StringPool stringPool(1024 * 1024 * 10);
  SirEngine::globals::STRING_POOL = &stringPool;
  SirEngine::Log::init();
  SirEngine::ThreeSizesPool pool(1024 * 1024 * 10);
  SirEngine::globals::PERSISTENT_ALLOCATOR = &pool;

  SirEngine::AnimationManager animManager;
  animManager.init();
  SirEngine::globals::ANIMATION_MANAGER = &animManager;

  PluginRegistry::init();
  PluginRegistry *registry = PluginRegistry::getInstance();
  registry->loadPluginsInFolder("plugins");
  // compiling animation clip on the fly, then we read it back as the engine
  // would
  compileAnim("../testData/idle2.json", "../testData/idle2.clip");

  // now we can read it
  SirEngine::AnimationClip *clip =
      animManager.loadAnimationClip("idle2", "../testData/idle2.clip");

  REQUIRE(clip->m_metadataCount == 3);
  REQUIRE(clip->findFirstMetadataFrame(
              SirEngine::ANIM_CLIP_KEYWORDS::L_FOOT_DOWN) == 30);
  REQUIRE(clip->findFirstMetadataFrame(
              SirEngine::ANIM_CLIP_KEYWORDS::R_FOOT_DOWN) == 10);

  SirEngine::Log::free();
}

TEST_CASE("animation key 2 read from frame", "[animation]") {
  // initialize memory pools and loggers
  SirEngine::StringPool stringPool(1024 * 1024 * 10);
  SirEngine::globals::STRING_POOL = &stringPool;
  SirEngine::Log::init();
  SirEngine::ThreeSizesPool pool(1024 * 1024 * 10);
  SirEngine::globals::PERSISTENT_ALLOCATOR = &pool;

  SirEngine::AnimationManager animManager;
  animManager.init();
  SirEngine::globals::ANIMATION_MANAGER = &animManager;

  PluginRegistry::init();
  PluginRegistry *registry = PluginRegistry::getInstance();
  registry->loadPluginsInFolder("plugins");
  // compiling animation clip on the fly, then we read it back as the engine
  // would
  compileAnim("../testData/idle2.json", "../testData/idle2.clip");

  // now we can read it
  SirEngine::AnimationClip *clip =
      animManager.loadAnimationClip("idle2", "../testData/idle2.clip");

  REQUIRE(clip->m_metadataCount == 3);
  int resultFrame = clip->findMetadataFrameFromGivenFrame(
      SirEngine::ANIM_CLIP_KEYWORDS::R_FOOT_DOWN, 20);
  REQUIRE(resultFrame == 60);
  resultFrame = clip->findMetadataFrameFromGivenFrame(
      SirEngine::ANIM_CLIP_KEYWORDS::L_FOOT_DOWN, 20);
  REQUIRE(resultFrame == 30);
  resultFrame = clip->findMetadataFrameFromGivenFrame(
      SirEngine::ANIM_CLIP_KEYWORDS::R_FOOT_DOWN, 9);
  REQUIRE(resultFrame == 10);
  resultFrame = clip->findMetadataFrameFromGivenFrame(
      SirEngine::ANIM_CLIP_KEYWORDS::R_FOOT_DOWN, 70);
  REQUIRE(resultFrame == -1);
  resultFrame = clip->findMetadataFrameFromGivenFrame(
      SirEngine::ANIM_CLIP_KEYWORDS::L_FOOT_DOWN, 35);
  REQUIRE(resultFrame == -1);
  resultFrame = clip->findMetadataFrameFromGivenFrame(
      SirEngine::ANIM_CLIP_KEYWORDS::L_FOOT_DOWN, 30);
  REQUIRE(resultFrame == 30);

  SirEngine::Log::free();
}
