#include "SirEngine/graphics/nodeGraph.h"
#include "SirEngine/memory/cpu/resizableVector.h"
#include "catch/catch.hpp"
#include "graphNodesDefinitions.h"

using namespace SirEngine;
// defining proxy nodes

static int getIndexOfNodeOfType(const ResizableVector<GNode *> &nodes,
                                const char *type) {
  const int count = nodes.size();
  for (int i = 0; i < count; ++i) {
    if (nodes[i]->isOfType(type)) {
      return i;
    }
  }
  return -1;
}

TEST_CASE("create node", "[graphics,graph]") {
  StringPool stringPool(1024);
  ThreeSizesPool allocator(1024);
  GraphAllocators allocs{&stringPool, &allocator};

  LegacyAssetNode node(allocs);
  const char *name = node.getName();
  REQUIRE(strcmp(name, "AssetManagerNode") == 0);
}

TEST_CASE("check node plugs", "[graphics,graph]") {
  StringPool stringPool(1024);
  ThreeSizesPool allocator(1024);
  GraphAllocators allocs{&stringPool, &allocator};

  LegacyAssetNode node(allocs);
  GPlug *plug1 = node.getPlug(LegacyAssetNode::MESHES);
  GPlug *plug2 = node.getPlug(LegacyAssetNode::MATERIALS);
  GPlug *plug3 = node.getPlug(LegacyAssetNode::MATRICES);

  REQUIRE(strcmp(plug1->name, "meshes") == 0);
  REQUIRE(strcmp(plug2->name, "materials") == 0);
  REQUIRE(strcmp(plug3->name, "matrices") == 0);
}

TEST_CASE("check node plugs 2", "[graphics,graph]") {
  StringPool stringPool(1024);
  ThreeSizesPool allocator(1024);
  GraphAllocators allocs{&stringPool, &allocator};

  LegacyDebugNode node(allocs);
  GPlug *plug1 = node.getPlug(LegacyDebugNode::IN_TEXTURE);
  GPlug *plug2 = node.getPlug(LegacyDebugNode::OUT_TEXTURE);

  REQUIRE(strcmp(plug1->name, "inTexture") == 0);
  REQUIRE(strcmp(plug2->name, "outTexture") == 0);
}

TEST_CASE("testing connection between two nodes", "[graphics,graph]") {
  StringPool stringPool(1024);
  ThreeSizesPool allocator(1024);
  GraphAllocators allocs{&stringPool, &allocator};

  LegacyAssetNode asset(allocs);
  LegacyGBufferPassPBR gbuffer(allocs);

  DependencyGraph graph;
  graph.addNode(&asset);
  graph.addNode(&gbuffer);
  graph.setFinalNode(&gbuffer);
  bool res = graph.connectNodes(&asset, LegacyAssetNode::MESHES, &gbuffer,
                                LegacyGBufferPassPBR::MESHES);

  bool isConnected1 = graph.isConnected(&asset, LegacyAssetNode::MESHES,
                                        &gbuffer, LegacyGBufferPassPBR::MESHES);
  bool isConnected2 = graph.isConnected(&gbuffer, LegacyGBufferPassPBR::MESHES,
                                        &asset, LegacyAssetNode::MESHES);
  REQUIRE(graph.nodeCount() == 2);
  REQUIRE(graph.getFinalNode() == &gbuffer);
  REQUIRE(res == true);
  REQUIRE(isConnected1 == true);
  REQUIRE(isConnected2 == true);
}
TEST_CASE("testing connection between two nodes 2", "[graphics,graph]") {
  StringPool stringPool(1024);
  ThreeSizesPool allocator(1024);
  GraphAllocators allocs{&stringPool, &allocator};

  LegacyAssetNode asset(allocs);
  LegacyGBufferPassPBR gbuffer(allocs);
  LegacyFinalBlitNode blit(allocs);

  DependencyGraph graph;
  graph.addNode(&asset);
  graph.addNode(&gbuffer);
  graph.addNode(&blit);
  graph.setFinalNode(&gbuffer);
  bool res = graph.connectNodes(&asset, LegacyAssetNode::MESHES, &gbuffer,
                                LegacyGBufferPassPBR::MESHES);

  bool isConnected1 = graph.isConnected(&blit, LegacyFinalBlitNode::IN_TEXTURE,
                                        &gbuffer, LegacyGBufferPassPBR::MESHES);
  bool isConnected2 = graph.isConnected(&blit, LegacyFinalBlitNode::IN_TEXTURE,
                                        &gbuffer, LegacyGBufferPassPBR::MESHES);
  bool isConnected3 = graph.isConnected(&gbuffer, LegacyGBufferPassPBR::MESHES,
                                        &blit, LegacyFinalBlitNode::IN_TEXTURE);
  bool isConnected4 = graph.isConnected(&gbuffer, LegacyGBufferPassPBR::MESHES,
                                        &blit, LegacyFinalBlitNode::IN_TEXTURE);
  REQUIRE(graph.nodeCount() == 3);
  REQUIRE(graph.getFinalNode() == &gbuffer);
  REQUIRE(res == true);
  REQUIRE(isConnected1 == false);
  REQUIRE(isConnected2 == false);
  REQUIRE(isConnected3 == false);
  REQUIRE(isConnected4 == false);
}
TEST_CASE("find node of type", "[graphics,graph]") {
  StringPool stringPool(1024);
  ThreeSizesPool allocator(1024);
  GraphAllocators allocs{&stringPool, &allocator};

  LegacyAssetNode asset(allocs);
  LegacyGBufferPassPBR gbuffer(allocs);
  LegacyFinalBlitNode blit(allocs);

  DependencyGraph graph;
  graph.addNode(&asset);
  graph.addNode(&gbuffer);
  graph.addNode(&blit);
  graph.setFinalNode(&gbuffer);

  const GNode *found1 = graph.findNodeOfType(asset.getType());
  const GNode *found2 = graph.findNodeOfType(gbuffer.getType());
  const GNode *found3 = graph.findNodeOfType(blit.getType());

  REQUIRE(found1 == &asset);
  REQUIRE(found2 == &gbuffer);
  REQUIRE(found3 == &blit);
}

TEST_CASE("finalize graph 1", "[graphics,graph]") {
  StringPool stringPool(1024);
  ThreeSizesPool allocator(1024);
  GraphAllocators allocs{&stringPool, &allocator};

  LegacyAssetNode asset(allocs);
  LegacyGBufferPassPBR gbuffer(allocs);
  LegacyFinalBlitNode blit(allocs);

  DependencyGraph graph;
  graph.addNode(&asset);
  graph.addNode(&gbuffer);
  graph.addNode(&blit);
  graph.setFinalNode(&blit);

  bool res = graph.connectNodes(&asset, LegacyAssetNode::MESHES, &gbuffer,
                                LegacyGBufferPassPBR::MESHES);
  REQUIRE(res == true);
  res = graph.connectNodes(&gbuffer, LegacyGBufferPassPBR::GEOMETRY_RT, &blit,
                           LegacyFinalBlitNode::IN_TEXTURE);
  REQUIRE(res == true);
  graph.finalizeGraph();

  const ResizableVector<GNode *> &list = graph.getLinearizedGraph();
  int assetId = getIndexOfNodeOfType(list, asset.getType());
  int gbufferId = getIndexOfNodeOfType(list, gbuffer.getType());
  int blitId = getIndexOfNodeOfType(list, blit.getType());
  REQUIRE(assetId != -1);
  REQUIRE(gbufferId != -1);
  REQUIRE(blitId != -1);
  REQUIRE(assetId < gbufferId);
  REQUIRE(gbufferId < blitId);
}

TEST_CASE("remove connection", "[graphics,graph]") {
  StringPool stringPool(1024);
  ThreeSizesPool allocator(1024);
  GraphAllocators allocs{&stringPool, &allocator};

  LegacyAssetNode asset(allocs);
  LegacyGBufferPassPBR gbuffer(allocs);
  LegacyFinalBlitNode blit(allocs);

  DependencyGraph graph;
  graph.addNode(&asset);
  graph.addNode(&gbuffer);
  graph.addNode(&blit);
  graph.setFinalNode(&blit);

  bool res = graph.connectNodes(&asset, LegacyAssetNode::MESHES, &gbuffer,
                                LegacyGBufferPassPBR::MESHES);
  REQUIRE(res == true);
  res = graph.connectNodes(&gbuffer, LegacyGBufferPassPBR::GEOMETRY_RT, &blit,
                           LegacyFinalBlitNode::IN_TEXTURE);
  REQUIRE(res == true);
  graph.finalizeGraph();

  int outSourceCount;
  const GPlug *outPlugs = asset.getOutputPlugs(outSourceCount);
  int inDestCount;
  const GPlug *inPlugs = gbuffer.getInputPlugs(inDestCount);
  asset.removeConnection(&outPlugs[GNode::getPlugIndex(
                             static_cast<uint32_t>(LegacyAssetNode::MESHES))],
                         &inPlugs[GNode::getPlugIndex(static_cast<uint32_t>(
                             LegacyGBufferPassPBR::MESHES))]);
  REQUIRE(res == true);
  res = graph.isConnected(&asset, LegacyAssetNode::MESHES, &gbuffer,
                          LegacyGBufferPassPBR::MESHES);
  REQUIRE(res == false);

  res = graph.connectNodes(&outPlugs[GNode::getPlugIndex(
                               static_cast<uint32_t>(LegacyAssetNode::MESHES))],
                           &inPlugs[GNode::getPlugIndex(static_cast<uint32_t>(
                               LegacyGBufferPassPBR::MESHES))]);
  REQUIRE(res == true);
  res = graph.isConnected(&asset, LegacyAssetNode::MESHES, &gbuffer,
                          LegacyGBufferPassPBR::MESHES);
  REQUIRE(res == true);
}

TEST_CASE("remove node", "[graphics,graph]") {
  StringPool stringPool(1024);
  ThreeSizesPool allocator(1024);
  GraphAllocators allocs{&stringPool, &allocator};

  LegacyAssetNode asset(allocs);
  LegacyGBufferPassPBR gbuffer(allocs);
  LegacyFinalBlitNode blit(allocs);

  DependencyGraph graph;
  graph.addNode(&asset);
  graph.addNode(&gbuffer);
  graph.addNode(&blit);
  graph.setFinalNode(&blit);

  bool found = graph.findNodeOfType("GBufferPassPBRProxy");
  REQUIRE(found == true);
  bool removed = graph.removeNode(&gbuffer);
  REQUIRE(removed == true);
  found = graph.findNodeOfType("GBufferPassPBRProxy");
  REQUIRE(found == false);
}

TEST_CASE("sort graph 1", "[graphics,graph]") {
  StringPool stringPool(1024 * 1024 * 10);
  ThreeSizesPool allocator(1024 * 1024 * 10);
  GraphAllocators allocs{&stringPool, &allocator};

  DependencyGraph graph;

  AssetManagerNode assetNode(allocs);
  FinalBlitNode finalBlit(allocs);
  SimpleForward simpleForward(allocs);
  PostProcessStack postProcess(allocs);
  GBufferPassPBR gbufferPass(allocs);
  DeferredLightingPass lighting(allocs);
  SkyBoxPass sky(allocs);
  DebugDrawNode debugDraw(allocs);
  ShadowPass shadowPass(allocs);

  // temporary graph for testing
  graph.addNode(&assetNode);
  graph.addNode(&finalBlit);
  graph.addNode(&gbufferPass);
  graph.addNode(&lighting);
  graph.addNode(&sky);
  graph.addNode(&simpleForward);
  graph.addNode(&postProcess);
  graph.addNode(&debugDraw);
  graph.addNode(&shadowPass);
  graph.setFinalNode(&finalBlit);

  graph.connectNodes(&assetNode, AssetManagerNode::ASSET_STREAM, &gbufferPass,
                     GBufferPassPBR::ASSET_STREAM);

  graph.connectNodes(&assetNode, AssetManagerNode::ASSET_STREAM, &shadowPass,
                     ShadowPass::ASSET_STREAM);

  graph.connectNodes(&gbufferPass, GBufferPassPBR::GEOMETRY_RT, &lighting,
                     DeferredLightingPass::GEOMETRY_RT);
  graph.connectNodes(&gbufferPass, GBufferPassPBR::NORMALS_RT, &lighting,
                     DeferredLightingPass::NORMALS_RT);
  graph.connectNodes(&gbufferPass, GBufferPassPBR::SPECULAR_RT, &lighting,
                     DeferredLightingPass::SPECULAR_RT);
  graph.connectNodes(&gbufferPass, GBufferPassPBR::DEPTH_RT, &lighting,
                     DeferredLightingPass::DEPTH_RT);

  graph.connectNodes(&shadowPass, ShadowPass::DIRECTIONAL_SHADOW_RT, &lighting,
                     DeferredLightingPass::DIRECTIONAL_SHADOW_RT);

  graph.connectNodes(&lighting, DeferredLightingPass::LIGHTING_RT, &sky,
                     SkyBoxPass::IN_TEXTURE);

  graph.connectNodes(&gbufferPass, GBufferPassPBR::DEPTH_RT, &sky,
                     SkyBoxPass::DEPTH);

  // connecting forward
  graph.connectNodes(&sky, SkyBoxPass::OUT_TEX, &simpleForward,
                     SimpleForward::IN_TEXTURE);
  graph.connectNodes(&gbufferPass, GBufferPassPBR::DEPTH_RT, &simpleForward,
                     SimpleForward::DEPTH_RT);

  graph.connectNodes(&assetNode, AssetManagerNode::ASSET_STREAM, &simpleForward,
                     SimpleForward::ASSET_STREAM);
  graph.connectNodes(&simpleForward, SimpleForward::OUT_TEXTURE, &postProcess,
                     PostProcessStack::IN_TEXTURE);

  graph.connectNodes(&gbufferPass, GBufferPassPBR::DEPTH_RT, &postProcess,
                     PostProcessStack::DEPTH_RT);

  graph.connectNodes(&postProcess, PostProcessStack::OUT_TEXTURE, &debugDraw,
                     DebugDrawNode::IN_TEXTURE);
  graph.connectNodes(&gbufferPass, GBufferPassPBR::DEPTH_RT, &debugDraw,
                     DebugDrawNode::DEPTH_RT);

  graph.connectNodes(&debugDraw, DebugDrawNode::OUT_TEXTURE, &finalBlit,
                     FinalBlitNode::IN_TEXTURE);

  graph.finalizeGraph();

  const ResizableVector<GNode *> &linearizedGraph = graph.getLinearizedGraph();
  int assetIdx = getIndexOfNodeOfType(linearizedGraph, "AssetManagerNode");
  int gbufferIdx = getIndexOfNodeOfType(linearizedGraph, "GBufferPassPBR");
  int shadowIdx = getIndexOfNodeOfType(linearizedGraph, "ShadowPass");
  int finalBlitIdx = getIndexOfNodeOfType(linearizedGraph, "FinalBlit");
  int lightingIdx =
      getIndexOfNodeOfType(linearizedGraph, "DeferredLightingPass");
  int skyIdx = getIndexOfNodeOfType(linearizedGraph, "SkyBoxPass");
  int simpleForwardIdx = getIndexOfNodeOfType(linearizedGraph, "SimpleForward");
  int postProcessIdx =
      getIndexOfNodeOfType(linearizedGraph, "PostProcessStack");
  int debugDrawIdx = getIndexOfNodeOfType(linearizedGraph, "DebugDrawNode");

  REQUIRE(assetIdx != -1);
  REQUIRE(gbufferIdx != -1);
  REQUIRE(shadowIdx != -1);
  REQUIRE(finalBlitIdx != -1);
  REQUIRE(lightingIdx != -1);
  REQUIRE(skyIdx != -1);
  REQUIRE(simpleForwardIdx != -1);
  REQUIRE(postProcessIdx != -1);
  REQUIRE(debugDrawIdx != -1);

  REQUIRE(assetIdx < gbufferIdx);
  REQUIRE(assetIdx < shadowIdx);
  REQUIRE(debugDrawIdx < finalBlitIdx);
  REQUIRE(lightingIdx < postProcessIdx);
  REQUIRE(postProcessIdx < debugDrawIdx);
  REQUIRE(simpleForwardIdx < postProcessIdx);
  REQUIRE(lightingIdx < simpleForwardIdx);
  REQUIRE(gbufferIdx < lightingIdx);
}
