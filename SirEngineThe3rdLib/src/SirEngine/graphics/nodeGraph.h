#pragma once
#include "SirEngine/core.h"
#include "SirEngine/globals.h"
#include "SirEngine/memory/resizableVector.h"
#include "SirEngine/runtimeString.h"
#include <cassert>
#include <cstdint>
#include <string>
#include <unordered_map>

namespace SirEngine {

enum PlugFlags {
  PLUG_INPUT = 1,
  PLUG_OUTPUT = 2,
  PLUG_GPU_BUFFER = 4,
  PLUG_TEXTURE = 8,
  PLUG_CPU_BUFFER = 16,
  PLUG_MESHES = 32
};

class GraphNode;
struct Plug final {
  uint32_t plugValue;
  uint32_t flags;
  GraphNode *nodePtr = nullptr;
  std::string name;
  uint32_t index = 0;
};

class SIR_ENGINE_API GraphNode {
public:
  // interface
  GraphNode(const std::string &name, const std::string &type)
      : nodeName(name), m_nodeType(type){};
  virtual ~GraphNode() = default;
  void addConnection(const std::string &thisNodePlugName, Plug *otherPlug);
  void removeConnection(const std::string &thisNodePlugName,
                        const Plug *otherPlug);
  virtual void compute(){};
  virtual void initialize(){};
  virtual void clear(){};
  // un-named parameters are screenWidth and screenHeight
  // removing the names just to avoid huge spam;
  virtual void onResizeEvent(int, int){};

  // getters
  inline Plug *getInputPlug(const std::string &name) {
    const size_t plugCount = m_inputPlugs.size();
    for (size_t i = 0; i < plugCount; ++i) {
      if (m_inputPlugs[i].name == name) {
        return &m_inputPlugs[i];
      }
    }
    assert(0 && "plug not found");
    return nullptr;
  }
  inline const std::vector<Plug> &getInputPlugs() const { return m_inputPlugs; }
  inline const std::vector<Plug> &getOutputPlugs() const {
    return m_outputPlugs;
  }
  inline Plug *getOutputPlug(const std::string &name) {
    const size_t outPlugsSize = m_outputPlugs.size();
    for (size_t i = 0; i < outPlugsSize; ++i) {
      if (m_outputPlugs[i].name == name) {
        return &m_outputPlugs[i];
      }
    }
    assert(0 && "plug not found");
    return nullptr;
  }

  inline const std::string &getNodeName() const { return nodeName; }
  inline const std::string &getNodeType() const { return m_nodeType; }
  inline uint32_t getInputCount() const {
    return static_cast<uint32_t>(m_inputPlugs.size());
  }
  inline uint32_t getOutputCount() const {
    return static_cast<uint32_t>(m_outputPlugs.size());
  }
  inline const std::vector<Plug *> *getPlugConnections(const Plug *plug) const {
    for (auto &conn : m_connections) {
      if (conn.first->name == plug->name) {
        return &conn.second;
      }
    }
    return nullptr;
  }
  // node index is mostly used to give a unique id in the graph to the node
  // useful for when we want to access a node by knwing the index or when
  // we are building the graphics representation
  inline void setNodeIndex(const uint32_t idx) { nodeIdx = idx; }
  inline uint32_t getNodeIdx() const { return nodeIdx; }

protected:
  void registerPlug(Plug plug);
  inline bool isFlag(const Plug &plug, const PlugFlags flag) const {
    return (plug.flags & flag) > 0;
  }

protected:
  std::unordered_map<const Plug *, std::vector<Plug *>> m_connections;
  std::vector<Plug> m_inputPlugs;
  std::vector<Plug> m_outputPlugs;
  const std::string nodeName;
  const std::string m_nodeType;
  uint32_t inCounter = 0;
  uint32_t outCounter = 0;
  uint32_t nodeIdx = 0;
};

class SIR_ENGINE_API Graph final {
public:
  Graph() = default;
  ~Graph() = default;

  void addNode(GraphNode *node) {
    node->setNodeIndex(m_nodeCounter++);
    m_nodes[node->getNodeName()] = node;
  }
  inline GraphNode *findNodeOfType(const std::string &type) {
    const size_t nodesCount = m_linearizedGraph.size();
    for (size_t i = 0; i < nodesCount; ++i) {
      if (m_linearizedGraph[i]->getNodeType() == type) {
        return m_linearizedGraph[i];
      }
    }
    return nullptr;
  }
  void removeDebugNode(GraphNode *debugNode) {
    // disconnect input
    assert(debugNode->getNodeType() == "FramePassDebugNode");
    const std::vector<Plug> &inPlugs = debugNode->getInputPlugs();
    assert(inPlugs.size() == 1);
    const Plug &inPlug = inPlugs[0];
    const std::vector<Plug *> *inConnections =
        debugNode->getPlugConnections(&inPlug);
    assert((*inConnections).size() == 1);
    const Plug *inConnectionPlug = (*inConnections)[0];
    debugNode->removeConnection(inPlug.name, inConnectionPlug);
    inConnectionPlug->nodePtr->removeConnection(inConnectionPlug->name,
                                                &inPlug);

    // disconnect output
    const std::vector<Plug> &outPlugs = debugNode->getOutputPlugs();
    assert(outPlugs.size() == 1);
    const Plug &outPlug = outPlugs[0];
    const std::vector<Plug *> *outConnections =
        debugNode->getPlugConnections(&outPlug);
    assert((*outConnections).size() == 1);
    const Plug *outConnectionPlug = (*outConnections)[0];
    debugNode->removeConnection(outPlug.name, outConnectionPlug);
    outConnectionPlug->nodePtr->removeConnection(outConnectionPlug->name,
                                                 &outPlug);

    // now lets connect the two sides
    connectNodes(inConnectionPlug->nodePtr, inConnectionPlug->name.c_str(),
                 outConnectionPlug->nodePtr, outConnectionPlug->name.c_str());

    m_nodes.erase(m_nodes.find(debugNode->getNodeName()));
    delete debugNode;
  }
  void addDebugNode(GraphNode *debugNode) {
    assert(debugNode->getNodeType() == "FramePassDebugNode");
    assert(finalNode != nullptr);

    // disconnect input
    const std::vector<Plug> &inPlugs = finalNode->getInputPlugs();
    assert(inPlugs.size() == 1);
    const Plug &inPlug = inPlugs[0];
    const std::vector<Plug *> *inConnections =
        finalNode->getPlugConnections(&inPlug);
    assert((*inConnections).size() == 1);
    const Plug *inConnectionPlug = (*inConnections)[0];
    finalNode->removeConnection(inPlug.name, inConnectionPlug);
    inConnectionPlug->nodePtr->removeConnection(inConnectionPlug->name,
                                                &inPlug);

    // no output to disconnect, final node has no output
    // now lets connect the two sides
    connectNodes(inConnectionPlug->nodePtr, inConnectionPlug->name.c_str(),
                 debugNode, debugNode->getInputPlugs()[0].name.c_str());

    connectNodes(debugNode, debugNode->getOutputPlugs()[0].name.c_str(),
                 finalNode, inPlug.name.c_str());

    // we need to re-compact the indices of the graph.
    std::unordered_map<std::string, GraphNode *> tempNodes = m_nodes;
    m_nodes.clear();
    m_nodeCounter = 0;
    for (auto node : tempNodes) {
      addNode(node.second);
    }
    addNode(debugNode);
  }

  void connectNodes(GraphNode *source, const char *sourcePlugName,
                    GraphNode *destination, const char *destinationPlugName);

  inline const std::vector<GraphNode *> &getLinearizedGraph() const {
    return m_linearizedGraph;
  };

  const GraphNode *getFinalNode() const { return finalNode; }
  inline void setFinalNode(GraphNode *node) { finalNode = node; }
  void finalizeGraph();
  void compute();
  void onResizeEvent(int screenWidth, int screenHeight) {
    for (auto node : m_linearizedGraph) {
      node->onResizeEvent(screenWidth, screenHeight);
    }
  };

private:
  std::unordered_map<std::string, GraphNode *> m_nodes;
  GraphNode *finalNode = nullptr;
  uint32_t m_nodeCounter = 0;
  std::vector<GraphNode *> m_linearizedGraph;
};

struct GraphAllocators {
  StringPool *stringPool;
  ThreeSizesPool *allocator;
};

// new node graph
class GNode;
struct GPlug final {
  GNode *nodePtr = nullptr;
  const char *name;
  uint32_t plugValue;
  uint32_t flags;
};

class SIR_ENGINE_API GNode {
// creates a mask for the first 31 bits, masking last one out and extracts
#define PLUG_INDEX(x) (((1u << 31u) - 1u) & (x))
#define INPUT_PLUG_CODE(x) ((1u << 31u) | (x))
#define OUTPUT_PLUG_CODE(x) x
#define IS_INPUT_PLUG(x) ((1u << 31u) & (x))

public:
  // interface
  GNode(const char *name, const char *type, const GraphAllocators &allocs)
      : m_allocs(allocs) {
    m_nodeName = m_allocs.stringPool->allocatePersistent(name);
    m_nodeType = m_allocs.stringPool->allocatePersistent(type);
  };
  virtual ~GNode() {
    m_allocs.stringPool->free(m_nodeName);
    m_allocs.stringPool->free(m_nodeType);
  };

  virtual void compute() {}
  virtual void initialize(){};
  virtual void clear(){};
  // un-named parameters are screenWidth and screenHeight
  // removing the names just to avoid huge spam;
  virtual void onResizeEvent(int, int){};

  inline const char *getName() const { return m_nodeName; }
  inline const char *getType() const { return m_nodeType; }
  inline GPlug *getPlug(const int index) {
    const bool isInput = IS_INPUT_PLUG(index);
    const int plugIndex = PLUG_INDEX(index);
    const int plugCount = isInput ? m_inputPlugsCount : m_outputPlugsCount;
    const PlugFlags flag =
        isInput ? PlugFlags::PLUG_INPUT : PlugFlags::PLUG_OUTPUT;
    GPlug *plugs = isInput ? m_inputPlugs : m_outputPlugs;

    assert(plugIndex < plugCount);
    const bool flagCorrect = isFlag(plugs[plugIndex], flag);
    assert(flagCorrect);
    return &plugs[plugIndex];
  }

  bool connect(int sourcePlugId, GNode *destinationNode, int destinationPlugId);
  inline void setNodeIndex(const uint32_t index) { m_nodeIdx = index; }

  bool isConnected(const int sourceId, GNode *destinationNode,
                   const int destinationPlugId) const {
    const bool isInput = IS_INPUT_PLUG(sourceId);
    const int plugIndex = PLUG_INDEX(sourceId);

    const int plugCount = isInput ? m_inputPlugsCount : m_outputPlugsCount;
    assert(plugIndex < plugCount);

    // fetch the connections and iterate over them
    ResizableVector<GPlug *> **connections =
        isInput ? m_inConnections : m_outConnections;

    GPlug *destinationPlug = destinationNode->getPlug(destinationPlugId);

    // TODO might be worth change this to test all of them and return
    // instead to have an extra check inside
    ResizableVector<GPlug *> *connectionList = connections[plugIndex];
    const int connectionCount = connectionList->size();
    for (int i = 0; i < connectionCount; ++i) {
      if (connectionList->getConstRef(i) == destinationPlug) {
        return true;
      }
    }
    return false;
  }

  inline bool isOfType(const char *type) const {
    return strcmp(m_nodeType, type) == 0;
  }
  inline uint32_t getNodeIdx() const { return m_nodeIdx; }
  const GPlug *getInputPlugs(int &count) const {
    count = m_inputPlugsCount;
    return m_inputPlugs;
  }
  const GPlug *getOutputPlugs(int &count) const {
    count = m_outputPlugsCount;
    return m_outputPlugs;
  }

  // TODO make this friend?
  const ResizableVector<GPlug *> *getPlugConnections(const GPlug *plug) const {
    const bool isInput = isFlag(*plug, PlugFlags::PLUG_INPUT);
    const int plugIdx = findPlugIndexFromInstance(plug);
    const int plugCount = isInput ? m_inputPlugsCount : m_outputPlugsCount;
    assert(plugIdx != -1);
    assert(plugIdx < plugCount);
    ResizableVector<GPlug *> **connections =
        isInput ? m_inConnections : m_outConnections;
    return connections[plugIdx];
  }

protected:
  inline bool isFlag(const GPlug &plug, const PlugFlags flag) const {
    return (plug.flags & flag) > 0;
  }
  void defaultInitializePlugsAndConnections(
      int inputCount, int outputCount,
      int reserve = DEFAULT_PLUG_CONNECTION_ALLOCATION) {

    const int totalCount = inputCount + outputCount;
    auto *plugs = static_cast<GPlug *>(
        m_allocs.allocator->allocate(sizeof(GPlug) * totalCount));

    m_inputPlugs = inputCount != 0 ? plugs : nullptr;
    m_inputPlugsCount = inputCount;
    m_outputPlugs = outputCount != 0 ?plugs + inputCount: nullptr;
    m_outputPlugsCount = outputCount;

    defaultInitializeConnectionPool(inputCount, outputCount, reserve);
  }

  void defaultInitializeConnectionPool(
      int inputCount, int outputCount,
      int reserve = DEFAULT_PLUG_CONNECTION_ALLOCATION);

  int findPlugIndexFromInstance(const GPlug *plug) const {
    const bool isInput = isFlag(*plug, PlugFlags::PLUG_INPUT);
    const int count = isInput ? m_inputPlugsCount : m_outputPlugsCount;
    const GPlug *plugs = isInput ? m_inputPlugs : m_outputPlugs;
    for (int i = 0; i < count; ++i) {
      if (&plugs[i] == plug) {
        return i;
      }
    }
    return -1;
  }

protected:
  const GraphAllocators &m_allocs;
  const char *m_nodeName;
  const char *m_nodeType;
  GPlug *m_inputPlugs = nullptr;
  GPlug *m_outputPlugs = nullptr;
  uint32_t m_inputPlugsCount = 0;
  uint32_t m_outputPlugsCount = 0;
  uint32_t m_nodeIdx = 0;
  static const int DEFAULT_PLUG_CONNECTION_ALLOCATION = 3;
  ResizableVector<GPlug *> **m_inConnections;
  ResizableVector<GPlug *> **m_outConnections;
};

class SIR_ENGINE_API DependencyGraph final {
public:
  DependencyGraph()
      : m_nodes(GRAPH_DEFAULT_RESERVE_SIZE),
        m_linearizedGraph(GRAPH_DEFAULT_RESERVE_SIZE) {}
  ~DependencyGraph() = default;

  inline void addNode(GNode *node) {
    node->setNodeIndex(m_nodeCounter++);
    m_nodes.pushBack(node);
  }
  inline const GNode *findNodeOfType(const char *type) const {

    const size_t nodesCount = m_nodes.size();
    for (size_t i = 0; i < nodesCount; ++i) {
      const bool isType = m_nodes.getConstRef(i)->isOfType(type);
      if (isType) {
        return m_nodes.getConstRef(i);
      }
    }
    return nullptr;
  }
  /*
  void removeDebugNode(GraphNode *debugNode) {
    // disconnect input
    assert(debugNode->getNodeType() == "FramePassDebugNode");
    const std::vector<Plug> &inPlugs = debugNode->getInputPlugs();
    assert(inPlugs.size() == 1);
    const Plug &inPlug = inPlugs[0];
    const std::vector<Plug *> *inConnections =
        debugNode->getPlugConnections(&inPlug);
    assert((*inConnections).size() == 1);
    const Plug *inConnectionPlug = (*inConnections)[0];
    debugNode->removeConnection(inPlug.name, inConnectionPlug);
    inConnectionPlug->nodePtr->removeConnection(inConnectionPlug->name,
                                                &inPlug);

    // disconnect output
    const std::vector<Plug> &outPlugs = debugNode->getOutputPlugs();
    assert(outPlugs.size() == 1);
    const Plug &outPlug = outPlugs[0];
    const std::vector<Plug *> *outConnections =
        debugNode->getPlugConnections(&outPlug);
    assert((*outConnections).size() == 1);
    const Plug *outConnectionPlug = (*outConnections)[0];
    debugNode->removeConnection(outPlug.name, outConnectionPlug);
    outConnectionPlug->nodePtr->removeConnection(outConnectionPlug->name,
                                                 &outPlug);

    // now lets connect the two sides
    connectNodes(inConnectionPlug->nodePtr, inConnectionPlug->name.c_str(),
                 outConnectionPlug->nodePtr, outConnectionPlug->name.c_str());

    m_nodes.erase(m_nodes.find(debugNode->getNodeName()));
    delete debugNode;
  }
  void addDebugNode(GraphNode *debugNode) {
    assert(debugNode->getNodeType() == "FramePassDebugNode");
    assert(finalNode != nullptr);

    // disconnect input
    const std::vector<Plug> &inPlugs = finalNode->getInputPlugs();
    assert(inPlugs.size() == 1);
    const Plug &inPlug = inPlugs[0];
    const std::vector<Plug *> *inConnections =
        finalNode->getPlugConnections(&inPlug);
    assert((*inConnections).size() == 1);
    const Plug *inConnectionPlug = (*inConnections)[0];
    finalNode->removeConnection(inPlug.name, inConnectionPlug);
    inConnectionPlug->nodePtr->removeConnection(inConnectionPlug->name,
                                                &inPlug);

    // no output to disconnect, final node has no output
    // now lets connect the two sides
    connectNodes(inConnectionPlug->nodePtr, inConnectionPlug->name.c_str(),
                 debugNode, debugNode->getInputPlugs()[0].name.c_str());

    connectNodes(debugNode, debugNode->getOutputPlugs()[0].name.c_str(),
                 finalNode, inPlug.name.c_str());

    // we need to re-compact the indices of the graph.
    std::unordered_map<std::string, GraphNode *> tempNodes = m_nodes;
    m_nodes.clear();
    m_nodeCounter = 0;
    for (auto node : tempNodes) {
      addNode(node.second);
    }
    addNode(debugNode);
  }
  */

  inline bool connectNodes(GNode *sourceNode, const int sourceId,
                           GNode *destinationNode, const int destinationId) {
    const bool sourceConnect =
        sourceNode->connect(sourceId, destinationNode, destinationId);
    const bool destConnect =
        destinationNode->connect(destinationId, sourceNode, sourceId);
    return sourceConnect & destConnect;
  }

  inline const ResizableVector<GNode *> &getLinearizedGraph() const {
    return m_linearizedGraph;
  }

  static bool isConnected(GNode *sourceNode, const int sourceId,
                          GNode *destinationNode, const int destinationId) {
    const bool sourceConnected =
        sourceNode->isConnected(sourceId, destinationNode, destinationId);
    const bool destinationConnected =
        destinationNode->isConnected(destinationId, sourceNode, sourceId);
    return sourceConnected & destinationConnected;
  }

  [[nodiscard]] const GNode *getFinalNode() const { return finalNode; }
  inline void setFinalNode(GNode *node) { finalNode = node; }
  void finalizeGraph();
  void compute();
  inline uint32_t nodeCount() const { return m_nodes.size(); }
  void onResizeEvent(int screenWidth, int screenHeight) {
    assert(0);
    /*
for (auto node : m_linearizedGraph) {
  node->onResizeEvent(screenWidth, screenHeight);
}
    */
  };

private:
  ResizableVector<GNode *> m_nodes;
  // std::unordered_map<std::string, GraphNode *> m_nodes;
  GNode *finalNode = nullptr;
  uint32_t m_nodeCounter = 0;
  ResizableVector<GNode *> m_linearizedGraph;
  static constexpr int GRAPH_DEFAULT_RESERVE_SIZE = 100;
};

} // namespace SirEngine
