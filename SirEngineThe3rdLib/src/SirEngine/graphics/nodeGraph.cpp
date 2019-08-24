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
  //using recursion, graph should be small, and we check against cycles
  //should not be any risk of overflow.
  recurseNode(finalNode,m_linearizedGraph,visitedNodes);

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
} // namespace SirEngine
