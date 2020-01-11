#include "SirEngine/globals.h"
#include "engineConfig.h"
#include <SirEngine/memory/stringPool.h>
#include <SirEngine/memory/threeSizesPool.h>
#include <cstdint>

namespace SirEngine::globals {
Camera3DPivot *MAIN_CAMERA = nullptr;
Clock<std::chrono::nanoseconds> GAME_CLOCK{};
uint64_t LAST_FRAME_TIME_NS = 0;
uint32_t TOTAL_NUMBER_OF_FRAMES = 0;
uint32_t CURRENT_FRAME = 0;
float MIN_DEPTH = 1.0f;
float MAX_DEPTH = 0.0f;
ConstantBufferManager *CONSTANT_BUFFER_MANAGER = nullptr;
BufferManager *BUFFER_MANAGER = nullptr;
TextureManager *TEXTURE_MANAGER = nullptr;
AssetManager *ASSET_MANAGER = nullptr;
Application *APPLICATION = nullptr;
RenderingContext *RENDERING_CONTEXT = nullptr;
DebugFrameData *DEBUG_FRAME_DATA = nullptr;
AnimationManager *ANIMATION_MANAGER = nullptr;
SkinClusterManager *SKIN_MANAGER = nullptr;
ScriptingContext *SCRIPTING_CONTEXT = nullptr;
Input *INPUT = nullptr;
MeshManager *MESH_MANAGER = nullptr;
MaterialManager *MATERIAL_MANAGER = nullptr;
PSOManager *PSO_MANAGER = nullptr;
RootSignatureManager *ROOT_SIGNATURE_MANAGER = nullptr;
DependencyGraph *RENDERING_GRAPH =nullptr;
DebugRenderer* DEBUG_RENDERER = nullptr;

// generic allocators
StringPool *STRING_POOL = nullptr;
StackAllocator *FRAME_ALLOCATOR = nullptr;
ThreeSizesPool *PERSISTENT_ALLOCATOR = nullptr;

EngineConfig *ENGINE_CONFIG = nullptr;

} // namespace SirEngine::globals
