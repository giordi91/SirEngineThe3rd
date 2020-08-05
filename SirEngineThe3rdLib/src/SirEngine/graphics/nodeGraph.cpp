#include "SirEngine/graphics/nodeGraph.h"
#include <cassert>
#include <queue>
#include <unordered_map>
#include <unordered_set>

namespace SirEngine {

bool GNode::connect(const int sourcePlugId, GNode *destinationNode,
                    const int destinationPlugId) {
  const bool isInput = isInputPlug(sourcePlugId);
  GPlug *destinationPlug = destinationNode->getPlug(destinationPlugId);

  // making sure the in->out or out->in condition is met
  const PLUG_FLAGS requiredDestinationFlag =
      isInput ? PLUG_FLAGS::PLUG_OUTPUT : PLUG_FLAGS::PLUG_INPUT;
  assert(isFlag(*destinationPlug, requiredDestinationFlag));

  // if it is then we are good to go!
  const int sourceIndex = getPlugIndex(sourcePlugId);

  ResizableVector<const GPlug *> **plugPtr =
      isInput ? m_inConnections : m_outConnections;
  plugPtr[sourceIndex]->pushBack(destinationPlug);
  return true;
}
bool GNode::connect(const GPlug *sourcePlug, const GPlug *destPlug) {
  // making sure this plug belongs to this node
  assert(sourcePlug->nodePtr == this);

  // making sure the in->out or out->in condition is met
  const bool isInput = isFlag(*sourcePlug, PLUG_FLAGS::PLUG_INPUT);

  // making sure the in->out or out->in condition is met
  const PLUG_FLAGS requiredDestinationFlag =
      isInput ? PLUG_FLAGS::PLUG_OUTPUT : PLUG_FLAGS::PLUG_INPUT;
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
  const bool isInput = isFlag(*sourcePlug, PLUG_FLAGS::PLUG_INPUT);
  const PLUG_FLAGS requiredDestinationFlag =
      isInput ? PLUG_FLAGS::PLUG_OUTPUT : PLUG_FLAGS::PLUG_INPUT;
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

int GNode::isConnected(const int sourceId, GNode* destinationNode, const int destinationPlugId) const
{
	const bool isInput = isInputPlug(sourceId);
	const int plugIndex = getPlugIndex(sourceId);

	const int plugCount = isInput ? m_inputPlugsCount : m_outputPlugsCount;
	assert(plugIndex < plugCount);

	// fetch the connections and iterate over them
	ResizableVector<const GPlug*>** connections =
		isInput ? m_inConnections : m_outConnections;

	GPlug* destinationPlug = destinationNode->getPlug(destinationPlugId);

	// TODO might be worth change this to test all of them and return
	// instead to have an extra check inside
	ResizableVector<const GPlug*>* connectionList = connections[plugIndex];
	const int connectionCount = connectionList->size();
	for (int i = 0; i < connectionCount; ++i)
	{
		if (connectionList->getConstRef(i) == destinationPlug)
		{
			return i;
		}
	}
	return -1;
}

int GNode::isConnected(const GPlug* sourcePlug, const GPlug* destinationPlug) const
{
	int srcIndex = findPlugIndexFromInstance(sourcePlug);
	const bool isInput = isFlag(*sourcePlug, PLUG_FLAGS::PLUG_INPUT);
	ResizableVector<const GPlug*>** connections =
		isInput ? m_inConnections : m_outConnections;
#if SE_DEBUG
	int count = isInput ? m_inputPlugsCount : m_outputPlugsCount;
	assert(srcIndex < count);
#endif
	ResizableVector<const GPlug*>* connectionList = connections[srcIndex];
	const int connectionCount = connectionList->size();
	for (int i = 0; i < connectionCount; ++i)
	{
		if (connectionList->getConstRef(i) == destinationPlug)
		{
			return i;
		}
	}
	return -1;
}

void GNode::defaultInitializeConnectionPool(const int inputCount,
                                            const int outputCount,
                                            const int reserve) {
  auto *connections = static_cast<ResizableVector<const GPlug *> **>(
      m_allocs.allocator->allocate(sizeof(GPlug) * (inputCount + outputCount)));
  // for each connection we need to allocate a vector
  m_inConnections = connections;
  m_outConnections = connections + inputCount;
  for (int i = 0; i < inputCount; ++i) {
    m_inConnections[i] = new ResizableVector<const GPlug *>(reserve);
  }
  for (int i = 0; i < outputCount; ++i) {
    m_outConnections[i] = new ResizableVector<const GPlug *>(reserve);
  }
}
void recurseNode(GNode *currentNode, ResizableVector<GNode *> &queue,
                 std::unordered_map<uint32_t, int> &visitedNodes,
                 int generation) {
  const auto found = visitedNodes.find(currentNode->getNodeIdx());
  // first we check whether or not the already processed the node
  if (found == visitedNodes.end()) {

    visitedNodes[currentNode->getNodeIdx()] = generation;

    queue.pushBack(currentNode);
    // let us now recurse depth first
    int inCount;
    const GPlug *inPlugs = currentNode->getInputPlugs(inCount);
    for (int i = 0; i < inCount; ++i) {
      // get the connections
      const ResizableVector<const GPlug *> *conns =
          currentNode->getPlugConnections(&inPlugs[i]);
      // if not empty we iterate all of them and extract the node at the
      // other end side
      const int connsCount = conns->size();
      if (connsCount != 0) {
        for (int c = 0; c < connsCount; ++c) {
          recurseNode(conns->getConstRef(c)->nodePtr, queue, visitedNodes,
                      generation + 1);
        }
      }
    }
  } else {
    // if we already found it we make sure to update the generation
    if (found->second < generation) {
      visitedNodes[currentNode->getNodeIdx()] = generation;
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

bool compareNodes(const GNode *&lhs, const GNode *&rhs) {
  return lhs->getGeneration() < rhs->getGeneration();
}

void DependencyGraph::finalizeGraph() {
  // for the time being we will linearize the graph
  m_linearizedGraph.clear();
  // m_linearizedGraph.reserve(m_nodeCounter);
  const int count = m_nodes.size();

  for (int i = 0; i < count; ++i) {
    m_nodes[i]->clear();
  }

  // node id, generation
  std::unordered_map<uint32_t, int> visitedNodes;
  // using recursion, graph should be small, and we check against cycles
  // should not be any risk of overflow.
  uint32_t generation = 0;
  recurseNode(finalNode, m_linearizedGraph, visitedNodes, generation);

  std::vector<GNode *> toSort;
  m_linearizedGraph.resize(m_nodes.size());
  for (int i = 0; i < count; ++i) {
    auto found = visitedNodes.find(m_nodes[i]->getNodeIdx());
    assert(found != visitedNodes.end());
    m_nodes[i]->setGeneration(found->second);
    toSort.push_back(m_nodes[i]);
    m_linearizedGraph[i] = m_nodes[i];
  }

  std::sort(toSort.begin(), toSort.end(), [](const auto &lhs, const auto &rhs) {
    return lhs->getGeneration() < rhs->getGeneration();
  });
  std::sort(m_linearizedGraph.data(),
            m_linearizedGraph.data() + m_linearizedGraph.size(),
            [](const auto &lhs, const auto &rhs) {
              return lhs->getGeneration() < rhs->getGeneration();
            });

  //std::reverse(toSort.begin(), toSort.end());
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

  //first we initialize all the nodes
  for (int i = 0; i < linearCount; ++i) {
    m_linearizedGraph[i]->initialize();
  }
  //next we populate all the ports, such that all the rendering resources
  //are known, also you have the hard guarantee that dependencies node
  //have their port already initialized thanks to the sort
  for (int i = 0; i < linearCount; ++i) {
    m_linearizedGraph[i]->populateNodePorts();
  }
}

void DependencyGraph::clear()
{
  const int count = m_linearizedGraph.size();
  for (int i = 0; i < count; ++i) {
    m_linearizedGraph[i]->clear();
    delete m_linearizedGraph[i];
  }
  m_linearizedGraph.clear();

}

void DependencyGraph::compute() {
  const int count = m_linearizedGraph.size();
  for (int i = 0; i < count; ++i) {
    m_linearizedGraph[i]->compute();
  }
}
} // namespace SirEngine
