#include "SirEngine/graphics/nodeGraph.h"
#include <cassert>

namespace SirEngine {
void GraphNode::addConnection(const std::string &thisNodePlugName,
                              Plug *otherPlug) {
  Plug *thisPlug = nullptr;

  for (int i = 0; i < m_inputPlugs.size(); ++i) {
    if (m_inputPlugs[i].name == thisNodePlugName) {
      m_connections[&m_inputPlugs[i]].push_back(otherPlug);
      return;
    }
  }
  for (int i = 0; i < m_outputPlugs.size(); ++i) {
    if (m_outputPlugs[i].name == thisNodePlugName) {
      m_connections[&m_outputPlugs[i]].push_back(otherPlug);
      return;
    }
  }
  assert(thisPlug != nullptr);
} // namespace SirEngine

void GraphNode::registerPlug(Plug plug) {
  if (isFlag(plug, PlugFlags::INPUT)) {
    plug.index = inCounter++;
    m_inputPlugs.push_back(plug);
  } else if (isFlag(plug, PlugFlags::OUTPUT)) {
    plug.index = outCounter++;
    m_outputPlugs.push_back(plug);
  } else {
    assert(0 && "unsupported plug type");
  }
}

TestNode::TestNode(const std::string& nodeName) : GraphNode(nodeName) {
  Plug inPlug;
  inPlug.plugValue = 0;
  inPlug.flags = PlugFlags::INPUT | PlugFlags::TEXTURE;
  inPlug.nodePtr = this;
  inPlug.name = "inputTest";
  registerPlug(inPlug);
  Plug inPlug2;
  inPlug2.plugValue = 0;
  inPlug2.flags = PlugFlags::INPUT | PlugFlags::TEXTURE;
  inPlug2.nodePtr = this;
  inPlug2.name = "inputTest2";
  registerPlug(inPlug2);

  Plug outPlug;
  outPlug.plugValue = 0;
  outPlug.flags = PlugFlags::OUTPUT | PlugFlags::TEXTURE;
  outPlug.nodePtr = this;
  outPlug.name = "outTest";
  registerPlug(outPlug);
}

Foo::Foo(const std::string& nodeName) : GraphNode(nodeName) {
  Plug inPlug;
  inPlug.plugValue = 0;
  inPlug.flags = PlugFlags::INPUT | PlugFlags::GPU_BUFFER;
  inPlug.nodePtr = this;
  inPlug.name = "in";
  registerPlug(inPlug);

  Plug outPlug;
  outPlug.plugValue = 0;
  outPlug.flags = PlugFlags::OUTPUT | PlugFlags::TEXTURE;
  outPlug.nodePtr = this;
  outPlug.name = "texOut1";
  registerPlug(outPlug);

  Plug outPlug2;
  outPlug2.plugValue = 0;
  outPlug2.flags = PlugFlags::OUTPUT | PlugFlags::TEXTURE;
  outPlug2.nodePtr = this;
  outPlug2.name = "texOut2";
  registerPlug(outPlug2);
}
} // namespace SirEngine
