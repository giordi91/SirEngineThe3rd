#pragma once
#include "SirEngine/handle.h"
#include "clock.h"
#include "core.h"

struct D3D12_VIEWPORT;
namespace SirEngine {
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

class StringPool;
class StackAllocator;
class ThreeSizesPool;

using PersistantAllocatorType = ThreeSizesPool;

namespace globals {

struct DebugFrameData {
  TextureHandle geometryBuffer;
  TextureHandle normalBuffer;
  TextureHandle specularBuffer;
  TextureHandle gbufferDepth;
};

extern unsigned int SCREEN_WIDTH;
extern unsigned int SCREEN_HEIGHT;
extern Camera3DPivot *MAIN_CAMERA;
extern GameClock GAME_CLOCK;
extern uint64_t LAST_FRAME_TIME_NS;
extern uint32_t TOTAL_NUMBER_OF_FRAMES;
extern uint64_t CURRENT_FRAME;
extern float MIN_DEPTH;
extern float MAX_DEPTH;
#define FRAME_BUFFERS_COUNT 2

// managers
extern ConstantBufferManager *CONSTANT_BUFFER_MANAGER;
extern BufferManager *BUFFER_MANAGER;
extern TextureManager *TEXTURE_MANAGER;
extern AssetManager *ASSET_MANAGER;
extern Application *APPLICATION;
extern RenderingContext *RENDERING_CONTEXT;
extern DebugFrameData *DEBUG_FRAME_DATA;
extern AnimationManager* ANIMATION_MANAGER;
extern SkinClusterManager* SKIN_MANAGER;
extern ScriptingContext* SCRIPTING_CONTEXT;

// generic allocators
extern SIR_ENGINE_API StringPool *STRING_POOL;
extern SIR_ENGINE_API StackAllocator *FRAME_ALLOCATOR;
extern SIR_ENGINE_API ThreeSizesPool *PERSISTENT_ALLOCATOR;

// config
extern const char* DATA_SOURCE_PATH;
extern const char* START_SCENE_PATH;
extern D3D12_VIEWPORT CURRENT_VIEWPORT;

}  // namespace globals
typedef unsigned char uchar;
}  // namespace SirEngine
