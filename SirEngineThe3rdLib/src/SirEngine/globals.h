#pragma once
#include "SirEngine/handle.h"
#include "clock.h"
#include "core.h"

namespace SirEngine {

struct EngineConfig;
class MaterialManager;
class AssetManager;
class ConstantBufferManager;
class Application;
class Camera3DPivot;
class TextureManager;
class RenderingContext;
class BufferManager;
class AnimationManager;
class SkinClusterManager;
class ScriptingContext;
class Input;
class MeshManager;
class PSOManager;
class RootSignatureManager;
class DependencyGraph;
class DebugRenderer;

class StringPool;
class StackAllocator;
class ThreeSizesPool;

using PersistantAllocatorType = ThreeSizesPool;
namespace graphics
{
	class ShaderManager;
}

namespace globals {

struct DebugFrameData {
  TextureHandle geometryBuffer;
  TextureHandle normalBuffer;
  TextureHandle specularBuffer;
  TextureHandle gbufferDepth;
  TextureHandle directionalShadow;
};

extern Camera3DPivot *MAIN_CAMERA;
extern Clock<std::chrono::nanoseconds> GAME_CLOCK;
extern uint64_t LAST_FRAME_TIME_NS;
extern uint32_t TOTAL_NUMBER_OF_FRAMES;
extern uint32_t CURRENT_FRAME;
extern float MIN_DEPTH;
extern float MAX_DEPTH;
// TODO change this to not be a define and be driven by engine config
// also needs to be in the same place for both Vulkan ad dx12
#define FRAME_BUFFERS_COUNT 2

// managers
extern ConstantBufferManager *CONSTANT_BUFFER_MANAGER;
extern BufferManager *BUFFER_MANAGER;
extern TextureManager *TEXTURE_MANAGER;
extern AssetManager *ASSET_MANAGER;
extern Application *APPLICATION;
extern RenderingContext *RENDERING_CONTEXT;
extern DebugFrameData *DEBUG_FRAME_DATA;
extern SIR_ENGINE_API AnimationManager *ANIMATION_MANAGER;
extern SkinClusterManager *SKIN_MANAGER;
extern ScriptingContext *SCRIPTING_CONTEXT;
extern Input *INPUT;
extern MeshManager *MESH_MANAGER;
extern MaterialManager *MATERIAL_MANAGER;
extern PSOManager *PSO_MANAGER;
extern RootSignatureManager *ROOT_SIGNATURE_MANAGER;
extern DependencyGraph *RENDERING_GRAPH;
extern DebugRenderer *DEBUG_RENDERER;
extern graphics::ShaderManager *SHADER_MANAGER;

// generic allocators
extern SIR_ENGINE_API StringPool *STRING_POOL;
extern SIR_ENGINE_API StackAllocator *FRAME_ALLOCATOR;
extern SIR_ENGINE_API ThreeSizesPool *PERSISTENT_ALLOCATOR;

// config
extern EngineConfig *ENGINE_CONFIG;

}  // namespace globals
}  // namespace SirEngine
