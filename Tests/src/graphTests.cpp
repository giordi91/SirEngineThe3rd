#include "SirEngine/graphics/nodeGraph.h"
#include "SirEngine/memory/resizableVector.h"
#include "catch/catch.hpp"

using namespace SirEngine;
// defining proxy nodes

class GAssetManagerNodeProxy final : public SirEngine::GNode {
public:
  enum PLUGS {
    MATRICES = OUTPUT_PLUG_CODE(0),
    MESHES = OUTPUT_PLUG_CODE(1),
    MATERIALS = OUTPUT_PLUG_CODE(2),
    COUNT = 3
  };

  explicit GAssetManagerNodeProxy(GraphAllocators &allocators)
      : GNode("AssetManagerNode", "AssetManagerNodeProxy", allocators) {
    // lets create the plugs
    auto *plugs = static_cast<GPlug *>(
        allocators.allocator->allocate(sizeof(GPlug) * PLUGS::COUNT));
    m_inputPlugs = nullptr;
    m_inputPlugsCount = 0;
    m_outputPlugs = plugs;
    m_outputPlugsCount = PLUGS::COUNT;

    // setup connection pool
    defaultInitializeConnectionPool(0, 3);

    GPlug &matrices = m_outputPlugs[PLUG_INDEX(PLUGS::MATRICES)];
    matrices.plugValue = 0;
    matrices.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_CPU_BUFFER;
    matrices.nodePtr = this;
    matrices.name = "matrices";

    GPlug &meshes = m_outputPlugs[PLUG_INDEX(PLUGS::MESHES)];
    meshes.plugValue = 0;
    meshes.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_MESHES;
    meshes.nodePtr = this;
    meshes.name = "meshes";

    GPlug &materials = m_outputPlugs[PLUG_INDEX(PLUGS::MATERIALS)];
    materials.plugValue = 0;
    materials.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_CPU_BUFFER;
    materials.nodePtr = this;
    materials.name = "materials";
  }
  virtual ~GAssetManagerNodeProxy() = default;
};
class GDebugNodeProxy final : public SirEngine::GNode {
public:
  enum PLUGS {
    IN_TEXTURE = INPUT_PLUG_CODE(0),
    OUT_TEXTURE = OUTPUT_PLUG_CODE(0),
    COUNT = 2
  };

  explicit GDebugNodeProxy(GraphAllocators &allocs)
      : GNode("DEBUG", "DebugNode", allocs) {
    // lets create the plugs
    auto *plugs = static_cast<GPlug *>(
        allocs.allocator->allocate(sizeof(GPlug) * PLUGS::COUNT));

    m_inputPlugs = plugs;
    m_inputPlugsCount = 1;
    m_outputPlugs = plugs + 1;
    m_outputPlugsCount = 1;

    // setup connection pool
    defaultInitializeConnectionPool(1, 1);

    GPlug &inTexture = m_inputPlugs[PLUG_INDEX(PLUGS::IN_TEXTURE)];
    inTexture.plugValue = 0;
    inTexture.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
    inTexture.nodePtr = this;
    inTexture.name = "inTexture";

    GPlug &outTexture = m_outputPlugs[PLUG_INDEX(PLUGS::OUT_TEXTURE)];
    outTexture.plugValue = 0;
    outTexture.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
    outTexture.nodePtr = this;
    outTexture.name = "outTexture";
  }
  virtual ~GDebugNodeProxy() = default;
};
class GFinalBlitNodeProxy final : public SirEngine::GNode {
public:
  enum PLUGS { IN_TEXTURE = INPUT_PLUG_CODE(0), COUNT = 1 };
  GFinalBlitNodeProxy(GraphAllocators &allocs)
      : GNode("finalBlit", "FinalBlitNodeProxy", allocs) {
    // lets create the plugs
    auto *plugs = static_cast<GPlug *>(
        allocs.allocator->allocate(sizeof(GPlug) * PLUGS::COUNT));
    m_inputPlugs = plugs;
    m_inputPlugsCount = PLUGS::COUNT;
    m_outputPlugs = nullptr;
    m_outputPlugsCount = 0;

    // setup connection pool
    defaultInitializeConnectionPool(1, 0);

    GPlug &inTexture = m_inputPlugs[PLUG_INDEX(PLUGS::IN_TEXTURE)];
    inTexture.plugValue = 0;
    inTexture.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
    inTexture.nodePtr = this;
    inTexture.name = "inTexture";
  }
  virtual ~GFinalBlitNodeProxy() = default;
};

class GGBufferPassPBRProxy final : public SirEngine::GNode {
public:
  enum PLUGS {
    GEOMETRY_RT = OUTPUT_PLUG_CODE(0),
    NORMALS_RT = OUTPUT_PLUG_CODE(1),
    SPECULAR_RT = OUTPUT_PLUG_CODE(2),
    DEPTH_RT = OUTPUT_PLUG_CODE(3),
    MATRICES = INPUT_PLUG_CODE(0),
    MESHES = INPUT_PLUG_CODE(1),
    MATERIALS = INPUT_PLUG_CODE(2),
    COUNT = 7
  };

  GGBufferPassPBRProxy(GraphAllocators &allocs)
      : GNode("finalBlit", "GBufferPassPBRProxy", allocs) {
    auto *plugs = static_cast<GPlug *>(
        allocs.allocator->allocate(sizeof(GPlug) * PLUGS::COUNT));

    m_inputPlugs = plugs;
    m_inputPlugsCount = 3;
    m_outputPlugs = plugs + 3;
    m_outputPlugsCount = 4;

    // setup connection pool
    defaultInitializeConnectionPool(3, 4);

    GPlug &geometryBuffer = m_outputPlugs[PLUG_INDEX(PLUGS::GEOMETRY_RT)];
    geometryBuffer.plugValue = 0;
    geometryBuffer.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
    geometryBuffer.nodePtr = this;
    geometryBuffer.name = "geometry";

    GPlug &normalBuffer = m_outputPlugs[PLUG_INDEX(PLUGS::NORMALS_RT)];
    normalBuffer.plugValue = 0;
    normalBuffer.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
    normalBuffer.nodePtr = this;
    normalBuffer.name = "normal";

    GPlug &specularBuffer = m_outputPlugs[PLUG_INDEX(PLUGS::SPECULAR_RT)];
    specularBuffer.plugValue = 0;
    specularBuffer.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
    specularBuffer.nodePtr = this;
    specularBuffer.name = "specular";

    GPlug &depthBuffer = m_outputPlugs[PLUG_INDEX(PLUGS::DEPTH_RT)];
    depthBuffer.plugValue = 0;
    depthBuffer.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
    depthBuffer.nodePtr = this;
    depthBuffer.name = "depth";

    // lets create the plugs
    GPlug &matrices = m_inputPlugs[PLUG_INDEX(PLUGS::MATRICES)];
    matrices.plugValue = 0;
    matrices.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_CPU_BUFFER;
    matrices.nodePtr = this;
    matrices.name = "matrices";

    GPlug &meshes = m_inputPlugs[PLUG_INDEX(PLUGS::MESHES)];
    meshes.plugValue = 0;
    meshes.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_MESHES;
    meshes.nodePtr = this;
    meshes.name = "meshes";

    GPlug &materials = m_inputPlugs[PLUG_INDEX(PLUGS::MATERIALS)];
    materials.plugValue = 0;
    materials.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_CPU_BUFFER;
    materials.nodePtr = this;
    materials.name = "materials";
  }
  virtual ~GGBufferPassPBRProxy() = default;
};
/*
class DeferredLightingProxy final : public SirEngine::GraphNode {
public:
  DeferredLightingProxy() : GraphNode("defLight", "DeferredLightingProxy") {
    // lets create the plugs
    Plug geometryBuffer;
    geometryBuffer.plugValue = 0;
    geometryBuffer.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
    geometryBuffer.nodePtr = this;
    geometryBuffer.name = "geometry";
    registerPlug(geometryBuffer);

    Plug normalBuffer;
    normalBuffer.plugValue = 0;
    normalBuffer.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
    normalBuffer.nodePtr = this;
    normalBuffer.name = "normal";
    registerPlug(normalBuffer);

    Plug specularBuffer;
    specularBuffer.plugValue = 0;
    specularBuffer.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
    specularBuffer.nodePtr = this;
    specularBuffer.name = "specular";
    registerPlug(specularBuffer);

    Plug depthBuffer;
    depthBuffer.plugValue = 0;
    depthBuffer.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
    depthBuffer.nodePtr = this;
    depthBuffer.name = "depth";
    registerPlug(depthBuffer);

    Plug lightBuffer;
    lightBuffer.plugValue = 0;
    lightBuffer.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
    lightBuffer.nodePtr = this;
    lightBuffer.name = "lighting";
    registerPlug(lightBuffer);
  }
  virtual ~DeferredLightingProxy() = default;
};
/*

class SkyBoxPassProxy final : public SirEngine::GraphNode {
public:
  SkyBoxPassProxy() : GraphNode("finalBlit", "SkyBoxPassProxy") {
    // lets create the plugs
    Plug fullscreenPass;
    fullscreenPass.plugValue = 0;
    fullscreenPass.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
    fullscreenPass.nodePtr = this;
    fullscreenPass.name = "fullscreenPass";
    registerPlug(fullscreenPass);

    Plug depthBuffer;
    depthBuffer.plugValue = 0;
    depthBuffer.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
    depthBuffer.nodePtr = this;
    depthBuffer.name = "depth";
    registerPlug(depthBuffer);

    Plug buffer;
    buffer.plugValue = 0;
    buffer.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
    buffer.nodePtr = this;
    buffer.name = "buffer";
    registerPlug(buffer);
  }
  virtual ~SkyBoxPassProxy() = default;
};

class PostProcessStackProxy final : public SirEngine::GraphNode {
public:
  PostProcessStackProxy() : GraphNode("post", "PostProcessStackProxy") {
    // lets create the plugs
    Plug inTexture;
    inTexture.plugValue = 0;
    inTexture.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
    inTexture.nodePtr = this;
    inTexture.name = "inTexture";
    registerPlug(inTexture);

    Plug outTexture;
    outTexture.plugValue = 0;
    outTexture.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
    outTexture.nodePtr = this;
    outTexture.name = "outTexture";
    registerPlug(outTexture);
  }
  virtual ~PostProcessStackProxy() = default;
};

Graph *buildGraphOne() {
  SirEngine::Graph *graph = new Graph();

  graph = new Graph();
  auto assetNode = new AssetManagerNodeProxy();
  auto finalBlit = new FinalBlitNodeProxy();
  // auto simpleForward = new SimpleForward("simpleForward");
  auto postProcess = new PostProcessStackProxy();
  // auto gbufferPass = new GBufferPass("GBufferPass");
  auto gbufferPass = new GBufferPassPBRProxy();
  auto lighting = new DeferredLightingProxy();
  // auto sky = new ProceduralSkyBoxPass("Procedural Sky");
  auto sky = new SkyBoxPassProxy();

  postProcess->initialize();

  // temporary graph for testing
  graph->addNode(assetNode);
  graph->addNode(finalBlit);
  // graph->addNode(simpleForward);
  graph->addNode(gbufferPass);
  graph->addNode(lighting);
  graph->addNode(sky);
  graph->setFinalNode(finalBlit);

  graph->connectNodes(assetNode, "matrices", gbufferPass, "matrices");
  graph->connectNodes(assetNode, "meshes", gbufferPass, "meshes");
  graph->connectNodes(assetNode, "materials", gbufferPass, "materials");

  // auto bw = new DebugNode("debugBW");
  graph->addNode(postProcess);
  // graph->connectNodes(simpleForward, "outTexture",
  // postProcess,
  //                                    "inTexture");

  graph->connectNodes(gbufferPass, "geometry", lighting, "geometry");
  graph->connectNodes(gbufferPass, "normal", lighting, "normal");
  graph->connectNodes(gbufferPass, "specular", lighting, "specular");
  graph->connectNodes(gbufferPass, "depth", lighting, "depth");

  graph->connectNodes(lighting, "lighting", sky, "fullscreenPass");
  graph->connectNodes(gbufferPass, "depth", sky, "depth");

  graph->connectNodes(sky, "buffer", postProcess, "inTexture");

  graph->connectNodes(postProcess, "outTexture", finalBlit, "inTexture");

  graph->finalizeGraph();

  return graph;
}

int getIndexOfNodeOfType(const std::vector<GraphNode *> &nodes,
                         const char *type) {
  const std::string typeString = std::string(type);
  int counter = 0;
  for (auto *node : nodes) {
    if (node->getNodeType() == typeString) {
      return counter;
    }
        ++counter;
  }
  return -1;
}
*/
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

  GAssetManagerNodeProxy node(allocs);
  const char *name = node.getName();
  REQUIRE(strcmp(name, "AssetManagerNode") == 0);
}

TEST_CASE("check node plugs", "[graphics,graph]") {
  StringPool stringPool(1024);
  ThreeSizesPool allocator(1024);
  GraphAllocators allocs{&stringPool, &allocator};

  GAssetManagerNodeProxy node(allocs);
  GPlug *plug1 = node.getPlug(GAssetManagerNodeProxy::MESHES);
  GPlug *plug2 = node.getPlug(GAssetManagerNodeProxy::MATERIALS);
  GPlug *plug3 = node.getPlug(GAssetManagerNodeProxy::MATRICES);

  REQUIRE(strcmp(plug1->name, "meshes") == 0);
  REQUIRE(strcmp(plug2->name, "materials") == 0);
  REQUIRE(strcmp(plug3->name, "matrices") == 0);
}

TEST_CASE("check node plugs 2", "[graphics,graph]") {
  StringPool stringPool(1024);
  ThreeSizesPool allocator(1024);
  GraphAllocators allocs{&stringPool, &allocator};

  GDebugNodeProxy node(allocs);
  GPlug *plug1 = node.getPlug(GDebugNodeProxy::IN_TEXTURE);
  GPlug *plug2 = node.getPlug(GDebugNodeProxy::OUT_TEXTURE);

  REQUIRE(strcmp(plug1->name, "inTexture") == 0);
  REQUIRE(strcmp(plug2->name, "outTexture") == 0);
}

TEST_CASE("testing connection between two nodes", "[graphics,graph]") {
  StringPool stringPool(1024);
  ThreeSizesPool allocator(1024);
  GraphAllocators allocs{&stringPool, &allocator};

  GAssetManagerNodeProxy asset(allocs);
  GGBufferPassPBRProxy gbuffer(allocs);

  DependencyGraph graph;
  graph.addNode(&asset);
  graph.addNode(&gbuffer);
  graph.setFinalNode(&gbuffer);
  bool res = graph.connectNodes(&asset, GAssetManagerNodeProxy::MESHES,
                                &gbuffer, GGBufferPassPBRProxy::MESHES);

  bool isConnected1 = graph.isConnected(&asset, GAssetManagerNodeProxy::MESHES,
                                        &gbuffer, GGBufferPassPBRProxy::MESHES);
  bool isConnected2 = graph.isConnected(&gbuffer, GGBufferPassPBRProxy::MESHES,
                                        &asset, GAssetManagerNodeProxy::MESHES);
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

  GAssetManagerNodeProxy asset(allocs);
  GGBufferPassPBRProxy gbuffer(allocs);
  GFinalBlitNodeProxy blit(allocs);

  DependencyGraph graph;
  graph.addNode(&asset);
  graph.addNode(&gbuffer);
  graph.addNode(&blit);
  graph.setFinalNode(&gbuffer);
  bool res = graph.connectNodes(&asset, GAssetManagerNodeProxy::MESHES,
                                &gbuffer, GGBufferPassPBRProxy::MESHES);

  bool isConnected1 = graph.isConnected(&blit, GFinalBlitNodeProxy::IN_TEXTURE,
                                        &gbuffer, GGBufferPassPBRProxy::MESHES);
  bool isConnected2 = graph.isConnected(&blit, GFinalBlitNodeProxy::IN_TEXTURE,
                                        &gbuffer, GGBufferPassPBRProxy::MESHES);
  bool isConnected3 = graph.isConnected(&gbuffer, GGBufferPassPBRProxy::MESHES,
                                        &blit, GFinalBlitNodeProxy::IN_TEXTURE);
  bool isConnected4 = graph.isConnected(&gbuffer, GGBufferPassPBRProxy::MESHES,
                                        &blit, GFinalBlitNodeProxy::IN_TEXTURE);
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

  GAssetManagerNodeProxy asset(allocs);
  GGBufferPassPBRProxy gbuffer(allocs);
  GFinalBlitNodeProxy blit(allocs);

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

  GAssetManagerNodeProxy asset(allocs);
  GGBufferPassPBRProxy gbuffer(allocs);
  GFinalBlitNodeProxy blit(allocs);

  DependencyGraph graph;
  graph.addNode(&asset);
  graph.addNode(&gbuffer);
  graph.addNode(&blit);
  graph.setFinalNode(&blit);

  bool res = graph.connectNodes(&asset, GAssetManagerNodeProxy::MESHES,
                                &gbuffer, GGBufferPassPBRProxy::MESHES);
  REQUIRE(res == true);
  res = graph.connectNodes(&gbuffer, GGBufferPassPBRProxy::GEOMETRY_RT, &blit,
                           GFinalBlitNodeProxy::IN_TEXTURE);
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

  GAssetManagerNodeProxy asset(allocs);
  GGBufferPassPBRProxy gbuffer(allocs);
  GFinalBlitNodeProxy blit(allocs);

  DependencyGraph graph;
  graph.addNode(&asset);
  graph.addNode(&gbuffer);
  graph.addNode(&blit);
  graph.setFinalNode(&blit);

  bool res = graph.connectNodes(&asset, GAssetManagerNodeProxy::MESHES,
                                &gbuffer, GGBufferPassPBRProxy::MESHES);
  REQUIRE(res == true);
  res = graph.connectNodes(&gbuffer, GGBufferPassPBRProxy::GEOMETRY_RT, &blit,
                           GFinalBlitNodeProxy::IN_TEXTURE);
  REQUIRE(res == true);
  graph.finalizeGraph();

  int outSourceCount;
  const GPlug *outPlugs = asset.getOutputPlugs(outSourceCount);
  int inDestCount;
  const GPlug *inPlugs = gbuffer.getInputPlugs(inDestCount);
  asset.removeConnection(&outPlugs[PLUG_INDEX(GAssetManagerNodeProxy::MESHES)],
                         &inPlugs[PLUG_INDEX(GGBufferPassPBRProxy::MESHES)]);   
  REQUIRE(res == true);
  res = graph.isConnected(&asset, GAssetManagerNodeProxy::MESHES,
                                &gbuffer, GGBufferPassPBRProxy::MESHES);
  REQUIRE(res == false);

  res = graph.connectNodes(&outPlugs[PLUG_INDEX(GAssetManagerNodeProxy::MESHES)],
                         &inPlugs[PLUG_INDEX(GGBufferPassPBRProxy::MESHES)]);
  REQUIRE(res == true);
  res = graph.isConnected(&asset, GAssetManagerNodeProxy::MESHES,
                                &gbuffer, GGBufferPassPBRProxy::MESHES);
  REQUIRE(res == true);

	
}

TEST_CASE("remove node", "[graphics,graph]") {
  StringPool stringPool(1024);
  ThreeSizesPool allocator(1024);
  GraphAllocators allocs{&stringPool, &allocator};

  GAssetManagerNodeProxy asset(allocs);
  GGBufferPassPBRProxy gbuffer(allocs);
  GFinalBlitNodeProxy blit(allocs);

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
/*
TEST_CASE("Graph sorting", "[graphics,graph]") {
  Graph *graph = buildGraphOne();
  const std::vector<GraphNode *> &linearizedGraph =
graph->getLinearizedGraph(); int assetNodeIdx =
      getIndexOfNodeOfType(linearizedGraph, "AssetManagerNodeProxy");
  int finalBlitIdx =
      getIndexOfNodeOfType(linearizedGraph, "FinalBlitNodeProxy");
  int debugNodeIdx = getIndexOfNodeOfType(linearizedGraph, "DebugNodeProxy");
  int deferredLightIdx =
      getIndexOfNodeOfType(linearizedGraph, "DeferredLightingProxy");
  int gbufferPBRIdx =
      getIndexOfNodeOfType(linearizedGraph, "GBufferPassPBRProxy");
  int skyboxIdx = getIndexOfNodeOfType(linearizedGraph, "SkyBoxPassProxy");
  int postProcessIdx =
      getIndexOfNodeOfType(linearizedGraph, "PostProcessStackProxy");

  REQUIRE(linearizedGraph.size() == 6);

  // asserting node idx have been found
  REQUIRE(assetNodeIdx != -1);
  REQUIRE(finalBlitIdx != -1);
  REQUIRE(debugNodeIdx == -1);
  REQUIRE(deferredLightIdx != -1);
  REQUIRE(gbufferPBRIdx != -1);
  REQUIRE(skyboxIdx != -1);
  REQUIRE(postProcessIdx != -1);

  // Checking dependencies
  // Here we are not going to check for specific order, that might change,
what
  // we are really interested into is the fact that dependencies are
respected, so
  // we will just make sure that index count are lower for a dependency of a
node REQUIRE(assetNodeIdx < gbufferPBRIdx); REQUIRE(gbufferPBRIdx<
deferredLightIdx); REQUIRE(gbufferPBRIdx< skyboxIdx); REQUIRE(skyboxIdx<
postProcessIdx); REQUIRE(postProcessIdx< finalBlitIdx);

}
*/
