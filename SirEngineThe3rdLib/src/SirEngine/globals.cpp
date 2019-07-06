#include "SirEngine/globals.h"
#include <cstdint>
#include <d3d12.h>

namespace SirEngine::globals {
uint32_t SCREEN_WIDTH = 1280;
uint32_t SCREEN_HEIGHT = 720;
Camera3DPivot *MAIN_CAMERA = nullptr;
GameClock GAME_CLOCK{};
uint64_t LAST_FRAME_TIME_NS = 0;
uint32_t TOTAL_NUMBER_OF_FRAMES = 0;
uint64_t CURRENT_FRAME = 0;
extern float MIN_DEPTH = 1.0f;
extern float MAX_DEPTH = 0.0f;
ConstantBufferManager *CONSTANT_BUFFER_MANAGER = nullptr;
BufferManager *BUFFER_MANAGER = nullptr;
TextureManager *TEXTURE_MANAGER = nullptr;
AssetManager *ASSET_MANAGER = nullptr;
Application *APPLICATION = nullptr;
RenderingContext *RENDERING_CONTEX = nullptr;
DebugFrameData *DEBUG_FRAME_DATA = nullptr;
std::string DATA_SOURCE_PATH;
std::string START_SCENE_PATH;

D3D12_VIEWPORT CURRECURRENT_VIEWPORT =
    D3D12_VIEWPORT{0,
                   0,
                   static_cast<float>(SCREEN_WIDTH),
                   static_cast<float>(SCREEN_HEIGHT),
                   MIN_DEPTH,
                   MAX_DEPTH};

} // namespace SirEngine::globals
