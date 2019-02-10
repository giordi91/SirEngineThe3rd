#pragma once
#include <cassert>
#include <cstdint>
#include <string>
#include <unordered_map>

namespace SirEngine {

enum PlugFlags {
  INPUT = 1,
  OUTPUT = 2,
  GPU_BUFFER = 4,
  TEXTURE = 8,
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
  GraphNode(const std::string &name) : nodeName(name){};
  virtual ~GraphNode() = default;
  void addConnection(const std::string &thisNodePlugName, Plug *otherPlug);
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
  inline uint32_t getInputCount() const { return m_inputPlugs.size(); }
  inline uint32_t getOutputCount() const { return m_outputPlugs.size(); }
  inline const std::vector<Plug *> *getPlugConnections(const Plug *plug) const {
    for (auto &conn : m_connections) {
      if (conn.first->name == plug->name) {
        return &conn.second;
      }
    }
    return nullptr;
    // auto found = m_connections.find(plug);
    // if(found != m_connections.end())
    //{
    //    return found->second;
    //}
    // assert(0 && "could not find plug");

    // return m_connections[plug];
  }
  inline void setNodeIndex(uint32_t idx) { nodeIdx = idx; }
  inline uint32_t getNodeIdx() const{return nodeIdx;}

protected:
  void registerPlug(Plug plug);
  inline bool isFlag(Plug plug, PlugFlags flag) const {
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

class TestNode : public GraphNode {
public:
  TestNode(const std::string& nodeName);
  virtual ~TestNode() = default;
};

class Foo : public GraphNode {
public:
  Foo(const std::string& nodeName);
  virtual ~Foo() = default;
};

class Graph final {
public:
  Graph() = default;
  ~Graph() = default;

  void addNode( GraphNode *node) {
    node->setNodeIndex(m_nodeCounter++);
    m_nodes[node->getNodeName()] = node;
  }
  const GraphNode *getFinalNode() const { return finalNode; }
  inline void setFinalNode(GraphNode *node) { finalNode = node; }

private:
  std::unordered_map<std::string, GraphNode *> m_nodes;
  GraphNode *finalNode = nullptr;
  uint32_t m_nodeCounter = 0;
};

} // namespace SirEngine
