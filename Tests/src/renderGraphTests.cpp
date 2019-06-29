#include "SirEngine/graphics/nodeGraph.h"
#include "catch/catch.hpp"

using namespace SirEngine;
// defining proxy nodes
class AssetManagerNodeProxy final : public SirEngine::GraphNode {
public:
  AssetManagerNodeProxy()
      : GraphNode("AssetManagerNode", "AssetManagerNodeProxy") {
    // lets create the plugs
    Plug matrices;
    matrices.plugValue = 0;
    matrices.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_CPU_BUFFER;
    matrices.nodePtr = this;
    matrices.name = "matrices";
    registerPlug(matrices);

    Plug meshes;
    meshes.plugValue = 0;
    meshes.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_MESHES;
    meshes.nodePtr = this;
    meshes.name = "meshes";
    registerPlug(meshes);

    Plug materials;
    materials.plugValue = 0;
    materials.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_CPU_BUFFER;
    materials.nodePtr = this;
    materials.name = "materials";
    registerPlug(materials);
  }
  virtual ~AssetManagerNodeProxy() = default;
};
class DebugNodeProxy final : public SirEngine::GraphNode {
public:
  DebugNodeProxy() : GraphNode("DEBUG", "DebugNode") {
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
  virtual ~DebugNodeProxy() = default;
};
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

class FinalBlitNodeProxy final : public SirEngine::GraphNode {
public:
  FinalBlitNodeProxy() : GraphNode("finalBlit", "FinalBlitNodeProxy") {
    // lets create the plugs
    Plug inTexture;
    inTexture.plugValue = 0;
    inTexture.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
    inTexture.nodePtr = this;
    inTexture.name = "inTexture";
    registerPlug(inTexture);
  }
  virtual ~FinalBlitNodeProxy() = default;
};

class GBufferPassProxy final : public SirEngine::GraphNode {
public:
  GBufferPassProxy() : GraphNode("finalBlit", "GBufferPassProxy") {
    Plug geometryBuffer;
    geometryBuffer.plugValue = 0;
    geometryBuffer.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
    geometryBuffer.nodePtr = this;
    geometryBuffer.name = "geometry";
    registerPlug(geometryBuffer);

    Plug normalBuffer;
    normalBuffer.plugValue = 0;
    normalBuffer.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
    normalBuffer.nodePtr = this;
    normalBuffer.name = "normal";
    registerPlug(normalBuffer);

    Plug specularBuffer;
    specularBuffer.plugValue = 0;
    specularBuffer.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
    specularBuffer.nodePtr = this;
    specularBuffer.name = "specular";
    registerPlug(specularBuffer);

    Plug depthBuffer;
    depthBuffer.plugValue = 0;
    depthBuffer.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
    depthBuffer.nodePtr = this;
    depthBuffer.name = "depth";
    registerPlug(depthBuffer);

    // lets create the plugs
    Plug matrices;
    matrices.plugValue = 0;
    matrices.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_CPU_BUFFER;
    matrices.nodePtr = this;
    matrices.name = "matrices";
    registerPlug(matrices);

    Plug meshes;
    meshes.plugValue = 0;
    meshes.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_MESHES;
    meshes.nodePtr = this;
    meshes.name = "meshes";
    registerPlug(meshes);

    Plug materials;
    materials.plugValue = 0;
    materials.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_CPU_BUFFER;
    materials.nodePtr = this;
    materials.name = "materials";
    registerPlug(materials);
  }
  virtual ~GBufferPassProxy() = default;
};

class GBufferPassPBRProxy final : public SirEngine::GraphNode {
public:
  GBufferPassPBRProxy() : GraphNode("finalBlit", "GBufferPassPBRProxy") {
    Plug geometryBuffer;
    geometryBuffer.plugValue = 0;
    geometryBuffer.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
    geometryBuffer.nodePtr = this;
    geometryBuffer.name = "geometry";
    registerPlug(geometryBuffer);

    Plug normalBuffer;
    normalBuffer.plugValue = 0;
    normalBuffer.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
    normalBuffer.nodePtr = this;
    normalBuffer.name = "normal";
    registerPlug(normalBuffer);

    Plug specularBuffer;
    specularBuffer.plugValue = 0;
    specularBuffer.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
    specularBuffer.nodePtr = this;
    specularBuffer.name = "specular";
    registerPlug(specularBuffer);

    Plug depthBuffer;
    depthBuffer.plugValue = 0;
    depthBuffer.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
    depthBuffer.nodePtr = this;
    depthBuffer.name = "depth";
    registerPlug(depthBuffer);

    // lets create the plugs
    Plug matrices;
    matrices.plugValue = 0;
    matrices.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_CPU_BUFFER;
    matrices.nodePtr = this;
    matrices.name = "matrices";
    registerPlug(matrices);

    Plug meshes;
    meshes.plugValue = 0;
    meshes.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_MESHES;
    meshes.nodePtr = this;
    meshes.name = "meshes";
    registerPlug(meshes);

    Plug materials;
    materials.plugValue = 0;
    materials.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_CPU_BUFFER;
    materials.nodePtr = this;
    materials.name = "materials";
    registerPlug(materials);
  }
  virtual ~GBufferPassPBRProxy() = default;
};

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

TEST_CASE("Graph creation", "[graphics,graph]") {
  Graph *graph = buildGraphOne();
  REQUIRE(graph->getFinalNode() != nullptr);
}

TEST_CASE("Graph sorting", "[graphics,graph]") {
  Graph *graph = buildGraphOne();
  const std::vector<GraphNode *> &linearizedGraph = graph->getLinearizedGraph();
  int assetNodeIdx =
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
  // Here we are not going to check for specific order, that might change, what
  // we are really interested into is the fact that dependencies are respected, so
  // we will just make sure that index count are lower for a dependency of a node
  REQUIRE(assetNodeIdx < gbufferPBRIdx);
  REQUIRE(gbufferPBRIdx< deferredLightIdx);
  REQUIRE(gbufferPBRIdx< skyboxIdx);
  REQUIRE(skyboxIdx< postProcessIdx);
  REQUIRE(postProcessIdx< finalBlitIdx);

}
