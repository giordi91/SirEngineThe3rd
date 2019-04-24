#pragma once
#include "SirEngine/core.h"

namespace SirEngine {
// the main idea of this class is to be used for tooling where you might want
// to run graphics and or compute without spawning a window and maybe without
// the whole game editor event but a much simpler one off code execution. An
// example might be generation of mips or convolution of irradiance maps.
class SIR_ENGINE_API HeadlessClient {
public:
  HeadlessClient();
  virtual ~HeadlessClient();
  // this is an alias for the work that was done at the beginning and end
  // of a frame, after this we are able to fetch the commandlist and use it.
  void beginWork();
  // whatever work was created will now been dispatched, the same way a frame
  // was dispatched, this will be without locks. It is save to begin and end
  // multiple times same as it is safe in the normal render loop.
  void endWork();

  //makes sure all the work is done.
  void flushAllOperation();

};
} // namespace SirEngine
