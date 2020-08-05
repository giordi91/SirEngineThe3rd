
#include "SirEngine/graphics/nodeGraph.h"

using namespace SirEngine;
class LegacyAssetNode final : public SirEngine::GNode {
public:
  enum PLUGS {
    MATRICES = outputPlugCode(0),
    MESHES = outputPlugCode(1),
    MATERIALS = outputPlugCode(2),
    COUNT = 3
  };

  explicit LegacyAssetNode(GraphAllocators &allocators)
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

    GPlug &matrices = m_outputPlugs[getPlugIndex(PLUGS::MATRICES)];
    matrices.plugValue = 0;
    matrices.flags = PLUG_FLAGS::PLUG_OUTPUT | PLUG_FLAGS::PLUG_CPU_BUFFER;
    matrices.nodePtr = this;
    matrices.name = "matrices";

    GPlug &meshes = m_outputPlugs[getPlugIndex(PLUGS::MESHES)];
    meshes.plugValue = 0;
    meshes.flags = PLUG_FLAGS::PLUG_OUTPUT | PLUG_FLAGS::PLUG_MESHES;
    meshes.nodePtr = this;
    meshes.name = "meshes";

    GPlug &materials = m_outputPlugs[getPlugIndex(PLUGS::MATERIALS)];
    materials.plugValue = 0;
    materials.flags = PLUG_FLAGS::PLUG_OUTPUT | PLUG_FLAGS::PLUG_CPU_BUFFER;
    materials.nodePtr = this;
    materials.name = "materials";
  }
  virtual ~LegacyAssetNode() = default;
};
class LegacyDebugNode final : public SirEngine::GNode {
public:
  enum PLUGS {
    IN_TEXTURE = inputPlugCode(0),
    OUT_TEXTURE = outputPlugCode(0),
    COUNT = 2
  };

  explicit LegacyDebugNode(GraphAllocators &allocs)
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

    GPlug &inTexture = m_inputPlugs[getPlugIndex(PLUGS::IN_TEXTURE)];
    inTexture.plugValue = 0;
    inTexture.flags = PLUG_FLAGS::PLUG_INPUT | PLUG_FLAGS::PLUG_TEXTURE;
    inTexture.nodePtr = this;
    inTexture.name = "inTexture";

    GPlug &outTexture = m_outputPlugs[getPlugIndex(PLUGS::OUT_TEXTURE)];
    outTexture.plugValue = 0;
    outTexture.flags = PLUG_FLAGS::PLUG_OUTPUT | PLUG_FLAGS::PLUG_TEXTURE;
    outTexture.nodePtr = this;
    outTexture.name = "outTexture";
  }
  virtual ~LegacyDebugNode() = default;
};
class LegacyFinalBlitNode final : public SirEngine::GNode {
public:
  enum PLUGS { IN_TEXTURE = inputPlugCode(0), COUNT = 1 };
  LegacyFinalBlitNode(GraphAllocators &allocs)
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

    GPlug &inTexture = m_inputPlugs[getPlugIndex(PLUGS::IN_TEXTURE)];
    inTexture.plugValue = 0;
    inTexture.flags = PLUG_FLAGS::PLUG_INPUT | PLUG_FLAGS::PLUG_TEXTURE;
    inTexture.nodePtr = this;
    inTexture.name = "inTexture";
  }
  virtual ~LegacyFinalBlitNode() = default;
};
class LegacyGBufferPassPBR final : public SirEngine::GNode {
public:
  enum PLUGS {
    GEOMETRY_RT = outputPlugCode(0),
    NORMALS_RT = outputPlugCode(1),
    SPECULAR_RT = outputPlugCode(2),
    DEPTH_RT = outputPlugCode(3),
    MATRICES = inputPlugCode(0),
    MESHES = inputPlugCode(1),
    MATERIALS = inputPlugCode(2),
    COUNT = 7
  };

  LegacyGBufferPassPBR(GraphAllocators &allocs)
      : GNode("finalBlit", "GBufferPassPBRProxy", allocs) {
    auto *plugs = static_cast<GPlug *>(
        allocs.allocator->allocate(sizeof(GPlug) * PLUGS::COUNT));

    m_inputPlugs = plugs;
    m_inputPlugsCount = 3;
    m_outputPlugs = plugs + 3;
    m_outputPlugsCount = 4;

    // setup connection pool
    defaultInitializeConnectionPool(3, 4);

    GPlug &geometryBuffer = m_outputPlugs[getPlugIndex(PLUGS::GEOMETRY_RT)];
    geometryBuffer.plugValue = 0;
    geometryBuffer.flags = PLUG_FLAGS::PLUG_OUTPUT | PLUG_FLAGS::PLUG_TEXTURE;
    geometryBuffer.nodePtr = this;
    geometryBuffer.name = "geometry";

    GPlug &normalBuffer = m_outputPlugs[getPlugIndex(PLUGS::NORMALS_RT)];
    normalBuffer.plugValue = 0;
    normalBuffer.flags = PLUG_FLAGS::PLUG_OUTPUT | PLUG_FLAGS::PLUG_TEXTURE;
    normalBuffer.nodePtr = this;
    normalBuffer.name = "normal";

    GPlug &specularBuffer = m_outputPlugs[getPlugIndex(PLUGS::SPECULAR_RT)];
    specularBuffer.plugValue = 0;
    specularBuffer.flags = PLUG_FLAGS::PLUG_OUTPUT | PLUG_FLAGS::PLUG_TEXTURE;
    specularBuffer.nodePtr = this;
    specularBuffer.name = "specular";

    GPlug &depthBuffer = m_outputPlugs[getPlugIndex(PLUGS::DEPTH_RT)];
    depthBuffer.plugValue = 0;
    depthBuffer.flags = PLUG_FLAGS::PLUG_OUTPUT | PLUG_FLAGS::PLUG_TEXTURE;
    depthBuffer.nodePtr = this;
    depthBuffer.name = "depth";

    // lets create the plugs
    GPlug &matrices = m_inputPlugs[getPlugIndex(PLUGS::MATRICES)];
    matrices.plugValue = 0;
    matrices.flags = PLUG_FLAGS::PLUG_INPUT | PLUG_FLAGS::PLUG_CPU_BUFFER;
    matrices.nodePtr = this;
    matrices.name = "matrices";

    GPlug &meshes = m_inputPlugs[getPlugIndex(PLUGS::MESHES)];
    meshes.plugValue = 0;
    meshes.flags = PLUG_FLAGS::PLUG_INPUT | PLUG_FLAGS::PLUG_MESHES;
    meshes.nodePtr = this;
    meshes.name = "meshes";

    GPlug &materials = m_inputPlugs[getPlugIndex(PLUGS::MATERIALS)];
    materials.plugValue = 0;
    materials.flags = PLUG_FLAGS::PLUG_INPUT | PLUG_FLAGS::PLUG_CPU_BUFFER;
    materials.nodePtr = this;
    materials.name = "materials";
  }
  virtual ~LegacyGBufferPassPBR() = default;
};

class AssetManagerNode final : public GNode {
public:
  enum PLUGS { ASSET_STREAM = outputPlugCode(0), COUNT = 1 };

public:
  AssetManagerNode(GraphAllocators &allocators)
      : GNode("AssetManagerNode", "AssetManagerNode", allocators) {
    // lets create the plugs
    defaultInitializePlugsAndConnections(0, 1);

    GPlug &stream = m_outputPlugs[getPlugIndex(PLUGS::ASSET_STREAM)];
    stream.plugValue = 0;
    stream.flags = PLUG_FLAGS::PLUG_OUTPUT | PLUG_FLAGS::PLUG_CPU_BUFFER;
    stream.nodePtr = this;
    stream.name = "assetStream";
  }
};
class FinalBlitNode final : public GNode {
public:
  enum PLUGS { IN_TEXTURE = inputPlugCode(0), COUNT = 1 };

public:
  FinalBlitNode(GraphAllocators &allocators)
      : GNode("FinalBlit", "FinalBlit", allocators) {
    // lets create the plugs
    defaultInitializePlugsAndConnections(1, 0);

    GPlug &inTexture = m_inputPlugs[getPlugIndex(PLUGS::IN_TEXTURE)];
    inTexture.plugValue = 0;
    inTexture.flags = PLUG_FLAGS::PLUG_INPUT | PLUG_FLAGS::PLUG_TEXTURE;
    inTexture.nodePtr = this;
    inTexture.name = "inTexture";
  }

  virtual ~FinalBlitNode() = default;
};

class SimpleForward final : public GNode {
public:
  enum PLUGS {
    IN_TEXTURE = inputPlugCode(0),
    DEPTH_RT = inputPlugCode(1),
    ASSET_STREAM = inputPlugCode(2),
    OUT_TEXTURE = outputPlugCode(0),
    COUNT = 4
  };

public:
  explicit SimpleForward(GraphAllocators &allocators)
      : GNode("SimpleForward", "SimpleForward", allocators) {

    defaultInitializePlugsAndConnections(3, 1);
    // lets create the plugs
    GPlug &inTexture = m_inputPlugs[getPlugIndex(PLUGS::IN_TEXTURE)];
    inTexture.plugValue = 0;
    inTexture.flags = PLUG_FLAGS::PLUG_INPUT | PLUG_FLAGS::PLUG_TEXTURE;
    inTexture.nodePtr = this;
    inTexture.name = "inTexture";

    GPlug &depthBuffer = m_inputPlugs[getPlugIndex(PLUGS::DEPTH_RT)];
    depthBuffer.plugValue = 0;
    depthBuffer.flags = PLUG_FLAGS::PLUG_INPUT | PLUG_FLAGS::PLUG_TEXTURE;
    depthBuffer.nodePtr = this;
    depthBuffer.name = "depthTexture";

    // lets create the plugs
    GPlug &stream = m_inputPlugs[getPlugIndex(PLUGS::ASSET_STREAM)];
    stream.plugValue = 0;
    stream.flags = PLUG_FLAGS::PLUG_INPUT | PLUG_FLAGS::PLUG_CPU_BUFFER;
    stream.nodePtr = this;
    stream.name = "assetStream";

    GPlug &outTexture = m_outputPlugs[getPlugIndex(PLUGS::OUT_TEXTURE)];
    outTexture.plugValue = 0;
    outTexture.flags = PLUG_FLAGS::PLUG_OUTPUT | PLUG_FLAGS::PLUG_TEXTURE;
    outTexture.nodePtr = this;
    outTexture.name = "outTexture";
  };
  virtual ~SimpleForward() = default;
};

class PostProcessStack final : public GNode {
public:
  enum PLUGS {
    IN_TEXTURE = inputPlugCode(0),
    DEPTH_RT = inputPlugCode(1),
    OUT_TEXTURE = outputPlugCode(0),
    COUNT = 3
  };

public:
  PostProcessStack(GraphAllocators &allocators)
      : GNode("PostProcessStack", "PostProcessStack", allocators) {
    defaultInitializePlugsAndConnections(2, 1);
    // lets create the plugs
    GPlug &inTexture = m_inputPlugs[getPlugIndex(PLUGS::IN_TEXTURE)];
    inTexture.plugValue = 0;
    inTexture.flags = PLUG_FLAGS::PLUG_INPUT | PLUG_FLAGS::PLUG_TEXTURE;
    inTexture.nodePtr = this;
    inTexture.name = "inTexture";

    GPlug &depthTexture = m_inputPlugs[getPlugIndex(PLUGS::DEPTH_RT)];
    depthTexture.plugValue = 0;
    depthTexture.flags = PLUG_FLAGS::PLUG_INPUT | PLUG_FLAGS::PLUG_TEXTURE;
    depthTexture.nodePtr = this;
    depthTexture.name = "depthTexture";

    GPlug &outTexture = m_outputPlugs[getPlugIndex(PLUGS::OUT_TEXTURE)];
    outTexture.plugValue = 0;
    outTexture.flags = PLUG_FLAGS::PLUG_OUTPUT | PLUG_FLAGS::PLUG_TEXTURE;
    outTexture.nodePtr = this;
    outTexture.name = "outTexture";
  }
};

class GBufferPassPBR final : public GNode {
public:
  enum PLUGS {
    GEOMETRY_RT = outputPlugCode(0),
    NORMALS_RT = outputPlugCode(1),
    SPECULAR_RT = outputPlugCode(2),
    DEPTH_RT = outputPlugCode(3),
    ASSET_STREAM = inputPlugCode(0),
    COUNT = 5
  };

public:
  explicit GBufferPassPBR(GraphAllocators &allocators)
      : GNode("GBufferPassPBR", "GBufferPassPBR", allocators) {

    defaultInitializePlugsAndConnections(1, 4);
    // lets create the plugs
    GPlug &geometryBuffer = m_outputPlugs[getPlugIndex(PLUGS::GEOMETRY_RT)];
    geometryBuffer.plugValue = 0;
    geometryBuffer.flags = PLUG_FLAGS::PLUG_OUTPUT | PLUG_FLAGS::PLUG_TEXTURE;
    geometryBuffer.nodePtr = this;
    geometryBuffer.name = "geometry";

    GPlug &normalBuffer = m_outputPlugs[getPlugIndex(PLUGS::NORMALS_RT)];
    normalBuffer.plugValue = 0;
    normalBuffer.flags = PLUG_FLAGS::PLUG_OUTPUT | PLUG_FLAGS::PLUG_TEXTURE;
    normalBuffer.nodePtr = this;
    normalBuffer.name = "normal";

    GPlug &specularBuffer = m_outputPlugs[getPlugIndex(PLUGS::SPECULAR_RT)];
    specularBuffer.plugValue = 0;
    specularBuffer.flags = PLUG_FLAGS::PLUG_OUTPUT | PLUG_FLAGS::PLUG_TEXTURE;
    specularBuffer.nodePtr = this;
    specularBuffer.name = "specular";

    GPlug &depthBuffer = m_outputPlugs[getPlugIndex(PLUGS::DEPTH_RT)];
    depthBuffer.plugValue = 0;
    depthBuffer.flags = PLUG_FLAGS::PLUG_OUTPUT | PLUG_FLAGS::PLUG_TEXTURE;
    depthBuffer.nodePtr = this;
    depthBuffer.name = "depth";

    // lets create the plugs
    GPlug &stream = m_inputPlugs[getPlugIndex(PLUGS::ASSET_STREAM)];
    stream.plugValue = 0;
    stream.flags = PLUG_FLAGS::PLUG_INPUT | PLUG_FLAGS::PLUG_CPU_BUFFER;
    stream.nodePtr = this;
    stream.name = "assetStream";
  }

  virtual ~GBufferPassPBR() = default;
};

class DeferredLightingPass : public GNode {
public:
  enum PLUGS {
    GEOMETRY_RT = inputPlugCode(0),
    NORMALS_RT = inputPlugCode(1),
    SPECULAR_RT = inputPlugCode(2),
    DEPTH_RT = inputPlugCode(3),
    DIRECTIONAL_SHADOW_RT = inputPlugCode(4),
    LIGHTING_RT = outputPlugCode(0),
    COUNT = 6
  };

public:
  DeferredLightingPass(GraphAllocators &allocators)
      : GNode("DeferredLightingPass", "DeferredLightingPass", allocators) {
    // init data
    defaultInitializePlugsAndConnections(5, 1);

    // lets create the plugs
    GPlug &geometryBuffer = m_inputPlugs[getPlugIndex(PLUGS::GEOMETRY_RT)];
    geometryBuffer.plugValue = 0;
    geometryBuffer.flags = PLUG_FLAGS::PLUG_INPUT | PLUG_FLAGS::PLUG_TEXTURE;
    geometryBuffer.nodePtr = this;
    geometryBuffer.name = "geometry";

    GPlug &normalBuffer = m_inputPlugs[getPlugIndex(PLUGS::NORMALS_RT)];
    normalBuffer.plugValue = 0;
    normalBuffer.flags = PLUG_FLAGS::PLUG_INPUT | PLUG_FLAGS::PLUG_TEXTURE;
    normalBuffer.nodePtr = this;
    normalBuffer.name = "normal";

    GPlug &specularBuffer = m_inputPlugs[getPlugIndex(PLUGS::SPECULAR_RT)];
    specularBuffer.plugValue = 0;
    specularBuffer.flags = PLUG_FLAGS::PLUG_INPUT | PLUG_FLAGS::PLUG_TEXTURE;
    specularBuffer.nodePtr = this;
    specularBuffer.name = "specular";

    GPlug &depthBuffer = m_inputPlugs[getPlugIndex(PLUGS::DEPTH_RT)];
    depthBuffer.plugValue = 0;
    depthBuffer.flags = PLUG_FLAGS::PLUG_INPUT | PLUG_FLAGS::PLUG_TEXTURE;
    depthBuffer.nodePtr = this;
    depthBuffer.name = "depth";

    GPlug &shadowBuffer =
        m_inputPlugs[getPlugIndex(PLUGS::DIRECTIONAL_SHADOW_RT)];
    shadowBuffer.plugValue = 0;
    shadowBuffer.flags = PLUG_FLAGS::PLUG_INPUT | PLUG_FLAGS::PLUG_TEXTURE;
    shadowBuffer.nodePtr = this;
    shadowBuffer.name = "shadow";

    GPlug &lightBuffer = m_outputPlugs[getPlugIndex(PLUGS::LIGHTING_RT)];
    lightBuffer.plugValue = 0;
    lightBuffer.flags = PLUG_FLAGS::PLUG_OUTPUT | PLUG_FLAGS::PLUG_TEXTURE;
    lightBuffer.nodePtr = this;
    lightBuffer.name = "lighting";
  }
  virtual ~DeferredLightingPass() { clear(); };
};

class SkyBoxPass : public GNode {
public:
  enum PLUGS {
    IN_TEXTURE = inputPlugCode(0),
    DEPTH = inputPlugCode(1),
    OUT_TEX = outputPlugCode(0),
    COUNT = 3
  };

public:
  SkyBoxPass(GraphAllocators &allocators)
      : GNode("SkyBoxPass", "SkyBoxPass", allocators) {

    defaultInitializePlugsAndConnections(2, 1);
    // lets create the plugs
    GPlug &fullscreenPass = m_inputPlugs[getPlugIndex(PLUGS::IN_TEXTURE)];
    fullscreenPass.plugValue = 0;
    fullscreenPass.flags = PLUG_FLAGS::PLUG_INPUT | PLUG_FLAGS::PLUG_TEXTURE;
    fullscreenPass.nodePtr = this;
    fullscreenPass.name = "fullscreenPass";

    GPlug &depthBuffer = m_inputPlugs[getPlugIndex(PLUGS::DEPTH)];
    depthBuffer.plugValue = 0;
    depthBuffer.flags = PLUG_FLAGS::PLUG_INPUT | PLUG_FLAGS::PLUG_TEXTURE;
    depthBuffer.nodePtr = this;
    depthBuffer.name = "depth";

    GPlug &buffer = m_outputPlugs[getPlugIndex(PLUGS::OUT_TEX)];
    buffer.plugValue = 0;
    buffer.flags = PLUG_FLAGS::PLUG_OUTPUT | PLUG_FLAGS::PLUG_TEXTURE;
    buffer.nodePtr = this;
    buffer.name = "buffer";
  }
  virtual ~SkyBoxPass() { clear(); };
};

class DebugDrawNode final : public GNode {
public:
  enum PLUGS {
    IN_TEXTURE = inputPlugCode(0),
    DEPTH_RT = inputPlugCode(1),
    OUT_TEXTURE = outputPlugCode(0),
    COUNT = 2
  };

public:
  explicit DebugDrawNode(GraphAllocators &allocators)
      : GNode("DebugDrawNode", "DebugDrawNode", allocators) {

    defaultInitializePlugsAndConnections(2, 1);
    // lets create the plugs
    GPlug &inTexture = m_inputPlugs[getPlugIndex(PLUGS::IN_TEXTURE)];
    inTexture.plugValue = 0;
    inTexture.flags = PLUG_FLAGS::PLUG_INPUT | PLUG_FLAGS::PLUG_TEXTURE;
    inTexture.nodePtr = this;
    inTexture.name = "inTexture";

    GPlug &inDepth = m_inputPlugs[getPlugIndex(PLUGS::DEPTH_RT)];
    inDepth.plugValue = 0;
    inDepth.flags = PLUG_FLAGS::PLUG_INPUT | PLUG_FLAGS::PLUG_TEXTURE;
    inDepth.nodePtr = this;
    inDepth.name = "depthTexture";

    GPlug &outTexture = m_outputPlugs[getPlugIndex(PLUGS::OUT_TEXTURE)];
    outTexture.plugValue = 0;
    outTexture.flags = PLUG_FLAGS::PLUG_OUTPUT | PLUG_FLAGS::PLUG_TEXTURE;
    outTexture.nodePtr = this;
    outTexture.name = "outTexture";
  }
  virtual ~DebugDrawNode() = default;
};

class ShadowPass final : public GNode {
public:
  enum PLUGS {
    DIRECTIONAL_SHADOW_RT = outputPlugCode(0),
    ASSET_STREAM = inputPlugCode(0),
    COUNT = 2
  };

public:
  explicit ShadowPass(GraphAllocators &allocators)
      : GNode("ShadowPass", "ShadowPass", allocators) {

    defaultInitializePlugsAndConnections(1, 4);
    // lets create the plugs
    GPlug &geometryBuffer =
        m_outputPlugs[getPlugIndex(PLUGS::DIRECTIONAL_SHADOW_RT)];
    geometryBuffer.plugValue = 0;
    geometryBuffer.flags = PLUG_FLAGS::PLUG_OUTPUT | PLUG_FLAGS::PLUG_TEXTURE;
    geometryBuffer.nodePtr = this;
    geometryBuffer.name = "shadowRT";

    // lets create the plugs
    GPlug &stream = m_inputPlugs[getPlugIndex(PLUGS::ASSET_STREAM)];
    stream.plugValue = 0;
    stream.flags = PLUG_FLAGS::PLUG_INPUT | PLUG_FLAGS::PLUG_CPU_BUFFER;
    stream.nodePtr = this;
    stream.name = "assetStream";
  }
  virtual ~ShadowPass() = default;
};
