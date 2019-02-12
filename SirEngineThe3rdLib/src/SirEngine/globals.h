#pragma once
#include "clock.h"

namespace SirEngine {
	class AssetManager;
	class ConstantBufferManager;
class Camera3DPivot;
class TextureManager;

namespace globals {

extern unsigned int SCREEN_WIDTH;
extern unsigned int SCREEN_HEIGHT;
extern Camera3DPivot *MAIN_CAMERA;
extern GameClock GAME_CLOCK;
extern uint64_t LAST_FRAME_TIME_NS;
extern uint32_t TOTAL_NUMBER_OF_FRAMES;
extern uint64_t CURRENT_FRAME;
#define FRAME_BUFFERS_COUNT 2

// managers
extern ConstantBufferManager *CONSTANT_BUFFER_MANAGER;
extern TextureManager* TEXTURE_MANAGER;
extern AssetManager* ASSET_MANAGER;

} // namespace globals
typedef unsigned char uchar;
} // namespace SirEngine
