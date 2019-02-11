#include "SirEngine/graphics/nodes/simpleForward.h"

namespace SirEngine {
SimpleForward::SimpleForward(const char *name) : GraphNode(name) {
  // lets create the plugs
  Plug outTexture;
  outTexture.plugValue = 0;
  outTexture.flags = PlugFlags::OUTPUT | PlugFlags::TEXTURE;
  outTexture.nodePtr = this;
  outTexture.name = "outTexture";
  registerPlug(outTexture);

  // lets create the plugs
  Plug matrices;
  matrices.plugValue = 0;
  matrices.flags = PlugFlags::INPUT | PlugFlags::CPU_BUFFER;
  matrices.nodePtr = this;
  matrices.name = "matrices";
  registerPlug(matrices);

  Plug meshes;
  meshes.plugValue = 0;
  meshes.flags = PlugFlags::INPUT | PlugFlags::MESHES;
  meshes.nodePtr = this;
  meshes.name = "meshes";
  registerPlug(meshes);

  Plug materials;
  materials.plugValue = 0;
  materials.flags = PlugFlags::INPUT | PlugFlags::CPU_BUFFER;
  materials.nodePtr = this;
  materials.name = "materials";
  registerPlug(materials);
}
} // namespace SirEngine
