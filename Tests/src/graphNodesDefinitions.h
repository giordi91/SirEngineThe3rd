
#include "SirEngine/graphics/nodeGraph.h"

using namespace SirEngine;
class LegacyAssetNode final : public SirEngine::GNode {
public:
  enum PLUGS {
    MATRICES = OUTPUT_PLUG_CODE(0),
    MESHES = OUTPUT_PLUG_CODE(1),
    MATERIALS = OUTPUT_PLUG_CODE(2),
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
  virtual ~LegacyAssetNode() = default;
};
class LegacyDebugNode final : public SirEngine::GNode {
public:
  enum PLUGS {
    IN_TEXTURE = INPUT_PLUG_CODE(0),
    OUT_TEXTURE = OUTPUT_PLUG_CODE(0),
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
  virtual ~LegacyDebugNode() = default;
};
class LegacyFinalBlitNode final : public SirEngine::GNode {
public:
  enum PLUGS { IN_TEXTURE = INPUT_PLUG_CODE(0), COUNT = 1 };
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

    GPlug &inTexture = m_inputPlugs[PLUG_INDEX(PLUGS::IN_TEXTURE)];
    inTexture.plugValue = 0;
    inTexture.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
    inTexture.nodePtr = this;
    inTexture.name = "inTexture";
  }
  virtual ~LegacyFinalBlitNode() = default;
};
class LegacyGBufferPassPBR final : public SirEngine::GNode {
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
  virtual ~LegacyGBufferPassPBR() = default;
};

class AssetManagerNode final : public GNode {
public:
  enum PLUGS { ASSET_STREAM = OUTPUT_PLUG_CODE(0), COUNT = 1 };

public:
  AssetManagerNode(GraphAllocators &allocators)
      : GNode("AssetManagerNode", "AssetManagerNode", allocators) {
    // lets create the plugs
    defaultInitializePlugsAndConnections(0, 1);

    GPlug &stream = m_outputPlugs[PLUG_INDEX(PLUGS::ASSET_STREAM)];
    stream.plugValue = 0;
    stream.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_CPU_BUFFER;
    stream.nodePtr = this;
    stream.name = "assetStream";
  }
};
class FinalBlitNode final : public GNode {
public:
  enum PLUGS { IN_TEXTURE = INPUT_PLUG_CODE(0), COUNT = 1 };

public:
  FinalBlitNode(GraphAllocators &allocators)
      : GNode("FinalBlit", "FinalBlit", allocators) {
    // lets create the plugs
    defaultInitializePlugsAndConnections(1, 0);

    GPlug &inTexture = m_inputPlugs[PLUG_INDEX(PLUGS::IN_TEXTURE)];
    inTexture.plugValue = 0;
    inTexture.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
    inTexture.nodePtr = this;
    inTexture.name = "inTexture";
  }

  virtual ~FinalBlitNode() = default;
};

class SimpleForward final : public GNode {
public:
  enum PLUGS {
    IN_TEXTURE = INPUT_PLUG_CODE(0),
    DEPTH_RT = INPUT_PLUG_CODE(1),
    ASSET_STREAM = INPUT_PLUG_CODE(2),
    OUT_TEXTURE = OUTPUT_PLUG_CODE(0),
    COUNT = 4
  };

public:
  explicit SimpleForward(GraphAllocators &allocators)
      : GNode("SimpleForward", "SimpleForward", allocators) {

    defaultInitializePlugsAndConnections(3, 1);
    // lets create the plugs
    GPlug &inTexture = m_inputPlugs[PLUG_INDEX(PLUGS::IN_TEXTURE)];
    inTexture.plugValue = 0;
    inTexture.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
    inTexture.nodePtr = this;
    inTexture.name = "inTexture";

    GPlug &depthBuffer = m_inputPlugs[PLUG_INDEX(PLUGS::DEPTH_RT)];
    depthBuffer.plugValue = 0;
    depthBuffer.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
    depthBuffer.nodePtr = this;
    depthBuffer.name = "depthTexture";

    // lets create the plugs
    GPlug &stream = m_inputPlugs[PLUG_INDEX(PLUGS::ASSET_STREAM)];
    stream.plugValue = 0;
    stream.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_CPU_BUFFER;
    stream.nodePtr = this;
    stream.name = "assetStream";

    GPlug &outTexture = m_outputPlugs[PLUG_INDEX(PLUGS::OUT_TEXTURE)];
    outTexture.plugValue = 0;
    outTexture.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
    outTexture.nodePtr = this;
    outTexture.name = "outTexture";
  };
  virtual ~SimpleForward() = default;
};

class PostProcessStack final : public GNode {
public:
  enum PLUGS {
    IN_TEXTURE = INPUT_PLUG_CODE(0),
    DEPTH_RT = INPUT_PLUG_CODE(1),
    OUT_TEXTURE = OUTPUT_PLUG_CODE(0),
    COUNT = 3
  };

public:
  PostProcessStack(GraphAllocators &allocators)
      : GNode("PostProcessStack", "PostProcessStack", allocators) {
    defaultInitializePlugsAndConnections(2, 1);
    // lets create the plugs
    GPlug &inTexture = m_inputPlugs[PLUG_INDEX(PLUGS::IN_TEXTURE)];
    inTexture.plugValue = 0;
    inTexture.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
    inTexture.nodePtr = this;
    inTexture.name = "inTexture";

    GPlug &depthTexture = m_inputPlugs[PLUG_INDEX(PLUGS::DEPTH_RT)];
    depthTexture.plugValue = 0;
    depthTexture.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
    depthTexture.nodePtr = this;
    depthTexture.name = "depthTexture";

    GPlug &outTexture = m_outputPlugs[PLUG_INDEX(PLUGS::OUT_TEXTURE)];
    outTexture.plugValue = 0;
    outTexture.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
    outTexture.nodePtr = this;
    outTexture.name = "outTexture";
  }
};

class GBufferPassPBR final : public GNode {
public:
  enum PLUGS {
    GEOMETRY_RT = OUTPUT_PLUG_CODE(0),
    NORMALS_RT = OUTPUT_PLUG_CODE(1),
    SPECULAR_RT = OUTPUT_PLUG_CODE(2),
    DEPTH_RT = OUTPUT_PLUG_CODE(3),
    ASSET_STREAM = INPUT_PLUG_CODE(0),
    COUNT = 5
  };

public:
  explicit GBufferPassPBR(GraphAllocators &allocators)
      : GNode("GBufferPassPBR", "GBufferPassPBR", allocators) {

    defaultInitializePlugsAndConnections(1, 4);
    // lets create the plugs
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
    GPlug &stream = m_inputPlugs[PLUG_INDEX(PLUGS::ASSET_STREAM)];
    stream.plugValue = 0;
    stream.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_CPU_BUFFER;
    stream.nodePtr = this;
    stream.name = "assetStream";
  }

  virtual ~GBufferPassPBR() = default;
};

class DeferredLightingPass : public GNode {
public:
  enum PLUGS {
    GEOMETRY_RT = INPUT_PLUG_CODE(0),
    NORMALS_RT = INPUT_PLUG_CODE(1),
    SPECULAR_RT = INPUT_PLUG_CODE(2),
    DEPTH_RT = INPUT_PLUG_CODE(3),
    DIRECTIONAL_SHADOW_RT = INPUT_PLUG_CODE(4),
    LIGHTING_RT = OUTPUT_PLUG_CODE(0),
    COUNT = 6
  };

public:
  DeferredLightingPass(GraphAllocators &allocators)
      : GNode("DeferredLightingPass", "DeferredLightingPass", allocators) {
    // init data
    defaultInitializePlugsAndConnections(5, 1);

    // lets create the plugs
    GPlug &geometryBuffer = m_inputPlugs[PLUG_INDEX(PLUGS::GEOMETRY_RT)];
    geometryBuffer.plugValue = 0;
    geometryBuffer.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
    geometryBuffer.nodePtr = this;
    geometryBuffer.name = "geometry";

    GPlug &normalBuffer = m_inputPlugs[PLUG_INDEX(PLUGS::NORMALS_RT)];
    normalBuffer.plugValue = 0;
    normalBuffer.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
    normalBuffer.nodePtr = this;
    normalBuffer.name = "normal";

    GPlug &specularBuffer = m_inputPlugs[PLUG_INDEX(PLUGS::SPECULAR_RT)];
    specularBuffer.plugValue = 0;
    specularBuffer.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
    specularBuffer.nodePtr = this;
    specularBuffer.name = "specular";

    GPlug &depthBuffer = m_inputPlugs[PLUG_INDEX(PLUGS::DEPTH_RT)];
    depthBuffer.plugValue = 0;
    depthBuffer.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
    depthBuffer.nodePtr = this;
    depthBuffer.name = "depth";

    GPlug &shadowBuffer =
        m_inputPlugs[PLUG_INDEX(PLUGS::DIRECTIONAL_SHADOW_RT)];
    shadowBuffer.plugValue = 0;
    shadowBuffer.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
    shadowBuffer.nodePtr = this;
    shadowBuffer.name = "shadow";

    GPlug &lightBuffer = m_outputPlugs[PLUG_INDEX(PLUGS::LIGHTING_RT)];
    lightBuffer.plugValue = 0;
    lightBuffer.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
    lightBuffer.nodePtr = this;
    lightBuffer.name = "lighting";
  }
  virtual ~DeferredLightingPass() { clear(); };
};

class SkyBoxPass : public GNode {
public:
  enum PLUGS {
    IN_TEXTURE = INPUT_PLUG_CODE(0),
    DEPTH = INPUT_PLUG_CODE(1),
    OUT_TEX = OUTPUT_PLUG_CODE(0),
    COUNT = 3
  };

public:
  SkyBoxPass(GraphAllocators &allocators)
      : GNode("SkyBoxPass", "SkyBoxPass", allocators) {

    defaultInitializePlugsAndConnections(2, 1);
    // lets create the plugs
    GPlug &fullscreenPass = m_inputPlugs[PLUG_INDEX(PLUGS::IN_TEXTURE)];
    fullscreenPass.plugValue = 0;
    fullscreenPass.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
    fullscreenPass.nodePtr = this;
    fullscreenPass.name = "fullscreenPass";

    GPlug &depthBuffer = m_inputPlugs[PLUG_INDEX(PLUGS::DEPTH)];
    depthBuffer.plugValue = 0;
    depthBuffer.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
    depthBuffer.nodePtr = this;
    depthBuffer.name = "depth";

    GPlug &buffer = m_outputPlugs[PLUG_INDEX(PLUGS::OUT_TEX)];
    buffer.plugValue = 0;
    buffer.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
    buffer.nodePtr = this;
    buffer.name = "buffer";
  }
  virtual ~SkyBoxPass() { clear(); };
};

class DebugDrawNode final : public GNode {
public:
  enum PLUGS {
    IN_TEXTURE = INPUT_PLUG_CODE(0),
    DEPTH_RT = INPUT_PLUG_CODE(1),
    OUT_TEXTURE = OUTPUT_PLUG_CODE(0),
    COUNT = 2
  };

public:
  explicit DebugDrawNode(GraphAllocators &allocators)
      : GNode("DebugDrawNode", "DebugDrawNode", allocators) {

    defaultInitializePlugsAndConnections(2, 1);
    // lets create the plugs
    GPlug &inTexture = m_inputPlugs[PLUG_INDEX(PLUGS::IN_TEXTURE)];
    inTexture.plugValue = 0;
    inTexture.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
    inTexture.nodePtr = this;
    inTexture.name = "inTexture";

    GPlug &inDepth = m_inputPlugs[PLUG_INDEX(PLUGS::DEPTH_RT)];
    inDepth.plugValue = 0;
    inDepth.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_TEXTURE;
    inDepth.nodePtr = this;
    inDepth.name = "depthTexture";

    GPlug &outTexture = m_outputPlugs[PLUG_INDEX(PLUGS::OUT_TEXTURE)];
    outTexture.plugValue = 0;
    outTexture.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
    outTexture.nodePtr = this;
    outTexture.name = "outTexture";
  }
  virtual ~DebugDrawNode() = default;
};

class ShadowPass final : public GNode {
public:
  enum PLUGS {
    DIRECTIONAL_SHADOW_RT = OUTPUT_PLUG_CODE(0),
    ASSET_STREAM = INPUT_PLUG_CODE(0),
    COUNT = 2
  };

public:
  explicit ShadowPass(GraphAllocators &allocators)
      : GNode("ShadowPass", "ShadowPass", allocators) {

    defaultInitializePlugsAndConnections(1, 4);
    // lets create the plugs
    GPlug &geometryBuffer =
        m_outputPlugs[PLUG_INDEX(PLUGS::DIRECTIONAL_SHADOW_RT)];
    geometryBuffer.plugValue = 0;
    geometryBuffer.flags = PlugFlags::PLUG_OUTPUT | PlugFlags::PLUG_TEXTURE;
    geometryBuffer.nodePtr = this;
    geometryBuffer.name = "shadowRT";

    // lets create the plugs
    GPlug &stream = m_inputPlugs[PLUG_INDEX(PLUGS::ASSET_STREAM)];
    stream.plugValue = 0;
    stream.flags = PlugFlags::PLUG_INPUT | PlugFlags::PLUG_CPU_BUFFER;
    stream.nodePtr = this;
    stream.name = "assetStream";
  }
  virtual ~ShadowPass() = default;
};
