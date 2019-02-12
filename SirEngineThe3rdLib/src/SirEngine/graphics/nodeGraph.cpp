#include "SirEngine/graphics/nodeGraph.h"
#include <cassert>
#include <queue>
#include <unordered_set>

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
void Graph::finalizeGraph() {
  // for the time being we will linearize the graph
  m_linearizedGraph.reserve(m_nodeCounter);

  std::unordered_set<uint32_t> visitedNodes;
  std::queue<GraphNode *> queue1;
  std::queue<GraphNode *> queue2;
  std::queue<GraphNode *> *currentQueue = &queue1;
  std::queue<GraphNode *> *nextQueue = &queue2;

  currentQueue->push(finalNode);
  bool go = true;
  while (go) {
    // this is the counter telling us for each recursion which node in the loop
    // we are processing, used for auto-layout
    while (!currentQueue->empty()) {
      // here we get first the current node from the queue
      // and build a node position, a structure with all the necessary
      // data for then being able to render the nodes.

      GraphNode *curr = currentQueue->front();
      // first we check whether or not the already processed the node
      if (visitedNodes.find(curr->getNodeIdx()) == visitedNodes.end()) {
        visitedNodes.insert(curr->getNodeIdx());
        m_linearizedGraph.push_back(curr);
        // lets process all the inputs, by accessing the other plug and getting
        // the parent node, the node will be added to the queue to be processed
        // in the next round

        const std::vector<Plug> &inPlugs = curr->getInputPlugs();
        for (size_t i = 0; i < inPlugs.size(); ++i) {
          // get the connections
          const std::vector<Plug *> *conns =
              curr->getPlugConnections(&inPlugs[i]);
          // if not empty we iterate all of them and extract the node at the
          // other end side
          if (conns != nullptr) {
            for (auto &conn : (*conns)) {
              nextQueue->push(conn->nodePtr);
            }
          }
        }
      }
      currentQueue->pop();
    }

    go = !nextQueue->empty();
    std::swap(currentQueue, nextQueue);
  }

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
