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
  GraphNode(const std::string &name) : nodeName(name){};
  virtual ~GraphNode() = default;
  void addConnection(const std::string &thisNodePlugName, Plug *otherPlug);
  virtual void compute(){};
  virtual void initialize(){};

  // getters
  inline Plug *getInputPlug(const std::string &name) {
    for (int i = 0; i < m_inputPlugs.size(); ++i) {
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

  inline const char *getNodeName() const { return nodeName.c_str(); }
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

  void connectNodes(GraphNode *source, const char *sourcePlugName,
                    GraphNode *destination, const char *destinationPlugName);

  const GraphNode *getFinalNode() const { return finalNode; }
  inline void setFinalNode(GraphNode *node) { finalNode = node; }
  void finalizeGraph();
  void compute();

private:
  std::unordered_map<std::string, GraphNode *> m_nodes;
  GraphNode *finalNode = nullptr;
  uint32_t m_nodeCounter = 0;
  std::vector<GraphNode*> m_linearizedGraph;
};

} // namespace SirEngine
