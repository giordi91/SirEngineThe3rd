#include "resourceProcessing/processor.h"
#include "SirEngine/animation/animationClip.h"
#include "SirEngine/animation/animationManager.h"
#include "SirEngine/io/argsUtils.h"
#include "SirEngine/log.h"
#include "catch/catch.hpp"

void compileAnim(const char *in, const char *out) {
  SirEngine::ResourceProcessing::Processor p;
  p.initialize();
  p.process("animationCompilerPlugin", in, out, "");
}

TEST_CASE("animation key 1", "[animation]") {
  compileAnim("../testData/idle1.json", "../testData/idle1.clip");
}

TEST_CASE("animation no data", "[animation]") {
  compileAnim("../testData/noMetaAnim.json", "../testData/noMetaAnim.clip");
}

TEST_CASE("animation knight", "[animation]") {
  compileAnim("../testData/knightBIdle.json", "../testData/knightBIdle.clip");
}

TEST_CASE("animation key 1 read", "[animation]") {
  compileAnim("../testData/idle1.json", "../testData/idle1.clip");
  SirEngine::AnimationManager animManager;
  animManager.init();
  SirEngine::globals::ANIMATION_MANAGER = &animManager;

  // now we can read it
  SirEngine::AnimationClip *clip =
      animManager.loadAnimationClip("idle1", "../testData/idle1.clip");

  REQUIRE(clip->m_metadataCount == 238);
  REQUIRE(clip->findFirstMetadataFrame(
              SirEngine::ANIM_CLIP_KEYWORDS::L_FOOT_DOWN) == 1);
  REQUIRE(clip->findFirstMetadataFrame(
              SirEngine::ANIM_CLIP_KEYWORDS::R_FOOT_DOWN) == 1);
}
TEST_CASE("animation key 2 read", "[animation]") {
  compileAnim("../testData/idle2.json", "../testData/idle2.clip");

  SirEngine::AnimationManager animManager;
  animManager.init();
  SirEngine::globals::ANIMATION_MANAGER = &animManager;

  // now we can read it
  SirEngine::AnimationClip *clip =
      animManager.loadAnimationClip("idle2", "../testData/idle2.clip");

  REQUIRE(clip->m_metadataCount == 3);
  REQUIRE(clip->findFirstMetadataFrame(
              SirEngine::ANIM_CLIP_KEYWORDS::L_FOOT_DOWN) == 30);
  REQUIRE(clip->findFirstMetadataFrame(
              SirEngine::ANIM_CLIP_KEYWORDS::R_FOOT_DOWN) == 10);
}

TEST_CASE("animation key 2 read from frame", "[animation]") {
  compileAnim("../testData/idle2.json", "../testData/idle2.clip");

  SirEngine::AnimationManager animManager;
  animManager.init();
  SirEngine::globals::ANIMATION_MANAGER = &animManager;

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
      SirEngine::ANIM_CLIP_KEYWORDS::R_FOOT_DOWN, 70, false);
  REQUIRE(resultFrame == -1);
  resultFrame = clip->findMetadataFrameFromGivenFrame(
      SirEngine::ANIM_CLIP_KEYWORDS::L_FOOT_DOWN, 35, false);
  REQUIRE(resultFrame == -1);
  resultFrame = clip->findMetadataFrameFromGivenFrame(
      SirEngine::ANIM_CLIP_KEYWORDS::L_FOOT_DOWN, 30);
  REQUIRE(resultFrame == 30);
  resultFrame = clip->findMetadataFrameFromGivenFrame(
      SirEngine::ANIM_CLIP_KEYWORDS::R_FOOT_DOWN, 70);
  REQUIRE(resultFrame == 10);
  resultFrame = clip->findMetadataFrameFromGivenFrame(
      SirEngine::ANIM_CLIP_KEYWORDS::L_FOOT_DOWN, 35);
  REQUIRE(resultFrame == 30);
}

TEST_CASE("animation key 3 read from frame", "[animation]") {
  // compiling animation clip on the fly, then we read it back as the engine
  // would
  compileAnim("../testData/knightBWalk.json", "../testData/knightBWalk.clip");

  SirEngine::AnimationManager animManager;
  animManager.init();
  SirEngine::globals::ANIMATION_MANAGER = &animManager;

  // now we can read it
  SirEngine::AnimationClip *clip =
      animManager.loadAnimationClip("idle2", "../testData/knightBWalk.clip");

  REQUIRE(clip->m_metadataCount == 42);
  int resultFrame = clip->findMetadataFrameFromGivenFrame(
      SirEngine::ANIM_CLIP_KEYWORDS::R_FOOT_DOWN, 30);
  REQUIRE(resultFrame == 8);
  resultFrame = clip->findMetadataFrameFromGivenFrame(
      SirEngine::ANIM_CLIP_KEYWORDS::R_FOOT_DOWN, 35);
  REQUIRE(resultFrame == 8);
  resultFrame = clip->findMetadataFrameFromGivenFrame(
      SirEngine::ANIM_CLIP_KEYWORDS::R_FOOT_DOWN, 6);
  REQUIRE(resultFrame == 8);
  resultFrame = clip->findMetadataFrameFromGivenFrame(
      SirEngine::ANIM_CLIP_KEYWORDS::R_FOOT_DOWN, 10);
  REQUIRE(resultFrame == 10);
  resultFrame = clip->findMetadataFrameFromGivenFrame(
      SirEngine::ANIM_CLIP_KEYWORDS::L_FOOT_DOWN, 25);
  REQUIRE(resultFrame == 25);
  resultFrame = clip->findMetadataFrameFromGivenFrame(
      SirEngine::ANIM_CLIP_KEYWORDS::L_FOOT_DOWN, 23);
  REQUIRE(resultFrame == 25);
  resultFrame = clip->findMetadataFrameFromGivenFrame(
      SirEngine::ANIM_CLIP_KEYWORDS::L_FOOT_DOWN, 30);
  REQUIRE(resultFrame == 30);
  resultFrame = clip->findMetadataFrameFromGivenFrame(
      SirEngine::ANIM_CLIP_KEYWORDS::L_FOOT_DOWN, 35);
  REQUIRE(resultFrame == 35);
  resultFrame = clip->findMetadataFrameFromGivenFrame(
      SirEngine::ANIM_CLIP_KEYWORDS::L_FOOT_DOWN, 0);
  REQUIRE(resultFrame == 0);
  // although the original animation is from 1 to 36, we don't keep track of the
  // offset we don't need it, so the effective range is 1-35
  resultFrame = clip->findMetadataFrameFromGivenFrame(
      SirEngine::ANIM_CLIP_KEYWORDS::L_FOOT_DOWN, 10);
  REQUIRE(resultFrame == 25);
  resultFrame = clip->findMetadataFrameFromGivenFrame(
      SirEngine::ANIM_CLIP_KEYWORDS::L_FOOT_DOWN, 9);
  REQUIRE(resultFrame == 9);
}
