#include "SirEngine/globals.h"
#include <cstdint>

namespace SirEngine {
namespace globals {
uint32_t SCREEN_WIDTH = 1280;
uint32_t SCREEN_HEIGHT = 720;
Camera3DPivot *MAIN_CAMERA = nullptr;
GameClock GAME_CLOCK{};
uint64_t LAST_FRAME_TIME_NS =0;
uint32_t TOTAL_NUMBER_OF_FRAMES =0;
uint64_t CURRENT_FRAME = 0;
ConstantBufferManager* CONSTANT_BUFFER_MANAGER = nullptr; 
TextureManager* TEXTURE_MANAGER = nullptr;
AssetManager* ASSET_MANAGER = nullptr;
Application* APPLICATION =nullptr;

} // namespace Globals
} // namespace SirEngine
