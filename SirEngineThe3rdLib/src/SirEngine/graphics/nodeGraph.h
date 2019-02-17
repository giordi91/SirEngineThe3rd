#pragma once
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

class GraphNode {
public:
  // interface
  GraphNode(const std::string &name, const std::string &type)
      : nodeName(name), m_nodeType(type){};
  virtual ~GraphNode() = default;
  void addConnection(const std::string &thisNodePlugName, Plug *otherPlug);
  void removeConnection(const std::string &thisPlugNode, const Plug *otherPlug);
  virtual void compute(){};
  virtual void initialize(){};
  virtual void clear(){};
  virtual void resize(int screenWidth, int screenHeight){};

  // getters
  inline Plug *getInputPlug(const std::string &name) {
    size_t plugCount = m_inputPlugs.size();
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
    for (int i = 0; i < m_outputPlugs.size(); ++i) {
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

class Graph final {
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
    assert(debugNode->getNodeType() == "DebugNode");
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
    assert(debugNode->getNodeType() == "DebugNode");
    GraphNode *finalBlit = findNodeOfType("FinalBlit");
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

  const GraphNode *getFinalNode() const { return finalNode; }
  inline void setFinalNode(GraphNode *node) { finalNode = node; }
  void finalizeGraph();
  void compute();
  void resize(int screenWidth, int screenHeight) {
    for (auto node : m_linearizedGraph) {
      node->resize(screenWidth, screenHeight);
    }
  };

private:
  std::unordered_map<std::string, GraphNode *> m_nodes;
  GraphNode *finalNode = nullptr;
  uint32_t m_nodeCounter = 0;
  std::vector<GraphNode *> m_linearizedGraph;
};

} // namespace SirEngine
