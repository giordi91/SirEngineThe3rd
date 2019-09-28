#include "SirEngine/graphics/nodeGraph.h"
#include <cassert>
#include <queue>
#include <unordered_set>

namespace SirEngine {
void GraphNode::addConnection(const std::string &thisNodePlugName,
                              Plug *otherPlug) {
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
  assert(0);
} // namespace SirEngine

void GraphNode::removeConnection(const std::string &thisNodePlugName,
                                 const Plug *otherPlug) {
  for (int i = 0; i < m_inputPlugs.size(); ++i) {
    if (m_inputPlugs[i].name == thisNodePlugName) {
      auto found = m_connections.find(&m_inputPlugs[i]);
      if (found == m_connections.end()) {
        assert(0 && "cannot disconnect two plugs are not connected");
        return;
      }
      size_t connectionCount = found->second.size();
      for (size_t c = 0; c < connectionCount; ++c) {
        if (found->second[c] == otherPlug) {
          found->second.erase(found->second.begin() + c);
          if (found->second.size() == 0) {
            m_connections.erase(m_connections.find(&m_inputPlugs[i]));
          }
          return;
        }
        assert(0 && "could not find other node plug as a connection");
      }
      return;
    }
  }
  for (int i = 0; i < m_outputPlugs.size(); ++i) {
    if (m_outputPlugs[i].name == thisNodePlugName) {
      auto found = m_connections.find(&m_outputPlugs[i]);
      if (found == m_connections.end()) {
        assert(0 && "cannot disconnect two plugs are not connected");
        return;
      }
      size_t connectionCount = found->second.size();
      for (size_t c = 0; c < connectionCount; ++c) {
        if (found->second[c] == otherPlug) {
          found->second.erase(found->second.begin() + c);
          if (found->second.size() == 0) {
            m_connections.erase(m_connections.find(&m_outputPlugs[i]));
          }
          return;
        }
        assert(0 && "could not find other node plug as a connection");
      }
      return;
    }
  }
}

void GraphNode::registerPlug(Plug plug) {
  if (isFlag(plug, PlugFlags::PLUG_INPUT)) {
    plug.index = inCounter++;
    m_inputPlugs.push_back(plug);
  } else if (isFlag(plug, PlugFlags::PLUG_OUTPUT)) {
    plug.index = outCounter++;
    m_outputPlugs.push_back(plug);
  } else {
    assert(0 && "unsupported plug type");
  }
}

void Graph::connectNodes(GraphNode *source, const char *sourcePlugName,
                         GraphNode *destination,
                         const char *destinationPlugName) {
  destination->addConnection(destinationPlugName,
                             source->getOutputPlug(sourcePlugName));
  source->addConnection(sourcePlugName,
                        destination->getInputPlug(destinationPlugName));
}

void recurseNode(GraphNode *currentNode, std::vector<GraphNode *> &queue,
                 std::unordered_set<uint32_t> &visitedNodes) {

  // first we check whether or not the already processed the node
  if (visitedNodes.find(currentNode->getNodeIdx()) == visitedNodes.end()) {
    visitedNodes.insert(currentNode->getNodeIdx());

    queue.push_back(currentNode);
    // let us now recurse depth first
    const std::vector<Plug> &inPlugs = currentNode->getInputPlugs();
    for (size_t i = 0; i < inPlugs.size(); ++i) {
      // get the connections
      const std::vector<Plug *> *conns =
          currentNode->getPlugConnections(&inPlugs[i]);
      // if not empty we iterate all of them and extract the node at the
      // other end side
      if (conns != nullptr) {
        for (auto &conn : (*conns)) {
          recurseNode(conn->nodePtr, queue, visitedNodes);
        }
      }
    }
  }
}

void Graph::finalizeGraph() {
  // for the time being we will linearize the graph
  m_linearizedGraph.clear();
  m_linearizedGraph.reserve(m_nodeCounter);
  for (auto &node : m_nodes) {
    node.second->clear();
  }

  std::unordered_set<uint32_t> visitedNodes;
  // using recursion, graph should be small, and we check against cycles
  // should not be any risk of overflow.
  recurseNode(finalNode, m_linearizedGraph, visitedNodes);

  // just need to flip the vector
  std::reverse(m_linearizedGraph.begin(), m_linearizedGraph.end());

  for (auto *node : m_linearizedGraph) {
    node->initialize();
  }
}
void Graph::compute() {

  for (auto *node : m_linearizedGraph) {
    node->compute();
  }
}

bool GNode::connect(const int sourcePlugId, GNode *destinationNode,
                    const int destinationPlugId) {
  const bool isInput = IS_INPUT_PLUG(sourcePlugId);
  GPlug *destinationPlug = destinationNode->getPlug(destinationPlugId);

  // making sure the in->out or out->in condition is met
  const PlugFlags requiredDestinationFlag =
      isInput ? PlugFlags::PLUG_OUTPUT : PlugFlags::PLUG_INPUT;
  assert(isFlag(*destinationPlug, requiredDestinationFlag));

  // if it is then we are good to go!
  GPlug *sourcePlug = getPlug(sourcePlugId);
  const int sourceIndex = PLUG_INDEX(sourcePlugId);

  ResizableVector<const GPlug *> **plugPtr =
      isInput ? m_inConnections : m_outConnections;
  plugPtr[sourceIndex]->pushBack(destinationPlug);
  return true;
}
bool GNode::connect(const GPlug *sourcePlug, const GPlug *destPlug) {
  // making sure this plug belongs to this node
  assert(sourcePlug->nodePtr == this);

  // making sure the in->out or out->in condition is met
  const bool isInput = isFlag(*sourcePlug, PlugFlags::PLUG_INPUT);

  // making sure the in->out or out->in condition is met
  const PlugFlags requiredDestinationFlag =
      isInput ? PlugFlags::PLUG_OUTPUT : PlugFlags::PLUG_INPUT;
  assert(isFlag(*destPlug, requiredDestinationFlag));

  const int sourceIndex = findPlugIndexFromInstance(sourcePlug);

  ResizableVector<const GPlug *> **plugPtr =
      isInput ? m_inConnections : m_outConnections;
  // temp fix
  plugPtr[sourceIndex]->pushBack(destPlug);
  return true;
}

bool GNode::removeConnection(const GPlug *sourcePlug, const GPlug *destPlug) {

  // making sure this plug belongs to this node
  assert(sourcePlug->nodePtr == this);

  // making sure the in->out or out->in condition is met
  const bool isInput = isFlag(*sourcePlug, PlugFlags::PLUG_INPUT);
  const PlugFlags requiredDestinationFlag =
      isInput ? PlugFlags::PLUG_OUTPUT : PlugFlags::PLUG_INPUT;
  assert(isFlag(*destPlug, requiredDestinationFlag));

  // first we find the index of the plug, this will allow us to
  // to find the connections vector
  int srcIndex = findPlugIndexFromInstance(sourcePlug);
  ResizableVector<const GPlug *> **connections =
      isInput ? m_inConnections : m_outConnections;
  int count = isInput ? m_inputPlugsCount : m_outputPlugsCount;
  assert(srcIndex < count);

  // now that we have the connections we need to iterate all of them
  // to see if any matches
  ResizableVector<const GPlug *> *connectionList = connections[srcIndex];
  const int connectionCount = connectionList->size();
  int foundIndex = -1;
  for (int i = 0; i < connectionCount; ++i) {
    if (connectionList->getConstRef(i) == destPlug) {
      foundIndex = i;
      break;
    }
  }
  // at this point we know if we have a connection and which index it is
  if (foundIndex == -1) {
    assert(0 && "plugs are not connected");
    return false;
  }
  // clearing the connection and patching the hole in the vector
  connectionList->removeByPatchingFromLast(foundIndex);

  // connections successfully removed we can return
  return true;
}

void GNode::defaultInitializeConnectionPool(const int inputCount,
                                            const int outputCount,
                                            int reserve) {
  auto *connections = static_cast<ResizableVector<const GPlug *> **>(
      m_allocs.allocator->allocate(sizeof(GPlug) * (inputCount + outputCount)));
  // for each connection we need to allocate a vector
  m_inConnections = connections;
  m_outConnections = connections + inputCount;
  for (int i = 0; i < inputCount; ++i) {
    m_inConnections[i] =
        new ResizableVector<const GPlug *>(DEFAULT_PLUG_CONNECTION_ALLOCATION);
  }
  for (int i = 0; i < outputCount; ++i) {
    m_outConnections[i] =
        new ResizableVector<const GPlug *>(DEFAULT_PLUG_CONNECTION_ALLOCATION);
  }
}
void recurseNode(GNode *currentNode, ResizableVector<GNode *> &queue,
                 std::unordered_set<uint32_t> &visitedNodes) {

  // first we check whether or not the already processed the node
  if (visitedNodes.find(currentNode->getNodeIdx()) == visitedNodes.end()) {
    visitedNodes.insert(currentNode->getNodeIdx());

    queue.pushBack(currentNode);
    // let us now recurse depth first
    int inCount;
    const GPlug *inPlugs = currentNode->getInputPlugs(inCount);
    for (size_t i = 0; i < inCount; ++i) {
      // get the connections
      const ResizableVector<const GPlug *> *conns =
          currentNode->getPlugConnections(&inPlugs[i]);
      // if not empty we iterate all of them and extract the node at the
      // other end side
      const int connsCount = conns->size();
      if (connsCount != 0) {
        for (int c = 0; c < connsCount; ++c) {
          recurseNode(conns->getConstRef(c)->nodePtr, queue, visitedNodes);
        }
      }
    }
  }
}

bool DependencyGraph::removeNode(GNode *node) {
  int nodeCount = m_nodes.size();
  for (int i = 0; i < nodeCount; ++i) {
    if (m_nodes[i] == node) {
      m_nodes.removeByPatchingFromLast(i);
      return true;
    }
  }
  return false;
}

void DependencyGraph::finalizeGraph() {
  // for the time being we will linearize the graph
  m_linearizedGraph.clear();
  // m_linearizedGraph.reserve(m_nodeCounter);
  const int count = m_nodes.size();

  for (int i = 0; i < count; ++i) {
    m_nodes[i]->clear();
  }
  // for (auto &node : m_nodes) {
  //  node.second->clear();
  //}

  std::unordered_set<uint32_t> visitedNodes;
  // using recursion, graph should be small, and we check against cycles
  // should not be any risk of overflow.
  recurseNode(finalNode, m_linearizedGraph, visitedNodes);

  // std::vector<GNode *> temp;
  // for (int i = 0; i < linearCount; ++i) {
  //  temp.push_back(m_linearizedGraph[i]);
  //}

  // std::reverse(temp.begin(), temp.end());
  // just need to flip the vector
  const int linearCount = m_linearizedGraph.size();
  for (int low = 0, high = linearCount - 1; low < high; ++low, --high) {
    GNode *temp = m_linearizedGraph[low];
    m_linearizedGraph[low] = m_linearizedGraph[high];
    m_linearizedGraph[high] = temp;
  }

  for (int i = 0; i < linearCount; ++i) {
    m_linearizedGraph[i]->initialize();
  }
}

void DependencyGraph::compute() {
  const int count = m_linearizedGraph.size();
  for (int i = 0; i < count; ++i) {
    m_linearizedGraph[i]->compute();
  }
}
} // namespace SirEngine
