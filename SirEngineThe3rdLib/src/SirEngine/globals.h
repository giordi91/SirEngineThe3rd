#pragma once
#include "SirEngine/handle.h"
#include "clock.h"
#include "core.h"

namespace SirEngine {

struct EngineConfig;
struct EngineFlags;
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
class CommandBufferManager;
class ImGuiManager;

class StringPool;
class StackAllocator;
class ThreeSizesPool;
class InteropData;

using PersistantAllocatorType = ThreeSizesPool;
namespace graphics {
class ShaderManager;
class LightManager;
class BindingTableManager;
}  // namespace graphics

namespace globals {

extern Camera3DPivot *MAIN_CAMERA;
extern Camera3DPivot *DEBUG_CAMERA;
extern Camera3DPivot *ACTIVE_CAMERA;
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
extern AnimationManager *ANIMATION_MANAGER;
extern SkinClusterManager *SKIN_MANAGER;
extern ScriptingContext *SCRIPTING_CONTEXT;
extern CommandBufferManager *COMMAND_BUFFER_MANAGER;
extern Input *INPUT;
extern MeshManager *MESH_MANAGER;
extern MaterialManager *MATERIAL_MANAGER;
extern PSOManager *PSO_MANAGER;
extern RootSignatureManager *ROOT_SIGNATURE_MANAGER;
extern DependencyGraph *RENDERING_GRAPH;
extern DebugRenderer *DEBUG_RENDERER;
extern graphics::ShaderManager *SHADER_MANAGER;
extern graphics::LightManager *LIGHT_MANAGER;
extern graphics::BindingTableManager *BINDING_TABLE_MANAGER;
extern InteropData *INTEROP_DATA;
extern ImGuiManager *IMGUI_MANAGER;
// TODO remove this, figure out a way to make imgui and the engine communicate
extern TextureHandle OFFSCREEN_BUFFER;

// generic allocators
extern StringPool *STRING_POOL;
extern StackAllocator *FRAME_ALLOCATOR;
extern ThreeSizesPool *PERSISTENT_ALLOCATOR;

// config
extern EngineConfig *ENGINE_CONFIG;
extern EngineFlags *ENGINE_FLAGS;

}  // namespace globals
}  // namespace SirEngine
