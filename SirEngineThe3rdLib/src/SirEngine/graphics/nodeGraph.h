#pragma once
#include <cassert>
#include <cstdint>

#include "SirEngine/core.h"
#include "SirEngine/memory/cpu/resizableVector.h"
#include "SirEngine/runtimeString.h"

namespace SirEngine {

enum PLUG_FLAGS {
  PLUG_INPUT = 1,
  PLUG_OUTPUT = 2,
  PLUG_GPU_BUFFER = 4,
  PLUG_TEXTURE = 8,
  PLUG_CPU_BUFFER = 16,
  PLUG_MESHES = 32
};

struct GraphAllocators {
  StringPool *stringPool = nullptr;
  ThreeSizesPool *allocator = nullptr;
};

// forward declaring the node such that we can use it for defining the plug
class GNode;
struct GPlug final {
  GNode *nodePtr = nullptr;
  const char *name = nullptr;
  uint32_t plugValue = 0;
  uint32_t flags = 0;
};

class GNodeCallback {
 public:
  virtual ~GNodeCallback() = default;
  virtual void initialize(uint32_t id) = 0;
  virtual void initializeResolutionDepenantResources(uint32_t id) = 0;
  virtual void prePassRender(uint32_t id) = 0;
  virtual void passRender(uint32_t id, BindingTableHandle passTable) = 0;
  virtual void clear(uint32_t id) = 0;
  virtual void clearResolutionDepenantResources(uint32_t id) = 0;
};

// TODO make node not copyable assignable
class SIR_ENGINE_API GNode {
 public:
  GNode(const GNode &) = delete;
  GNode &operator=(const GNode &) = delete;
  GNode(GNode &&) = delete;
  GNode &operator=(GNode &&) = delete;

  // interface
  GNode(const char *name, const char *type, const GraphAllocators &allocs)
      : m_allocs(allocs), m_callbacks(4) {
    m_nodeName = m_allocs.stringPool->allocatePersistent(name);
    m_nodeType = m_allocs.stringPool->allocatePersistent(type);
  };
  virtual ~GNode() {
    m_allocs.stringPool->free(m_nodeName);
    m_allocs.stringPool->free(m_nodeType);
  }

  inline void setGeneration(const int generation) { m_generation = generation; }
  inline int getGeneration() const { return m_generation; }

  virtual void compute() {}
  virtual void initialize(CommandBufferHandle ) {}
  virtual void initializeResolutionDepenantResources(
      CommandBufferHandle ) {}
  virtual void clearResolutionDepenantResources() {}
  virtual void clear() { m_generation = -1; }
  virtual void populateNodePorts() {}

  // un-named parameters are screenWidth and screenHeight
  // removing the names just to avoid huge spam from compiler warning;
  virtual void onResizeEvent(int, int,CommandBufferHandle ) {}

  inline const char *getName() const { return m_nodeName; }
  inline const char *getType() const { return m_nodeType; }
  void addCallbackConfig(const uint32_t id, GNodeCallback *config) {
    m_callbacks.pushBack({config, id});
  };
  inline GPlug *getPlug(const int index) {
    const bool isInput = isInputPlug(index);
    const int plugIndex = getPlugIndex(index);
    const int plugCount = isInput ? m_inputPlugsCount : m_outputPlugsCount;
    const PLUG_FLAGS flag =
        isInput ? PLUG_FLAGS::PLUG_INPUT : PLUG_FLAGS::PLUG_OUTPUT;
    GPlug *plugs = isInput ? m_inputPlugs : m_outputPlugs;

    assert(plugIndex < plugCount);
    const bool flagCorrect = isFlag(plugs[plugIndex], flag);
    assert(flagCorrect);
    return &plugs[plugIndex];
  }

  bool connect(int sourcePlugId, GNode *destinationNode, int destinationPlugId);
  bool connect(const GPlug *sourcePlug, const GPlug *destPlug);
  bool removeConnection(const GPlug *sourcePlug, const GPlug *destPlug);

  inline void setNodeIndex(const uint32_t index) { m_nodeIdx = index; }

  int isConnected(const int sourceId, GNode *destinationNode,
                  const int destinationPlugId) const;

  int isConnected(const GPlug *sourcePlug, const GPlug *destinationPlug) const;

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
  inline uint32_t getInputCount() const { return m_inputPlugsCount; }
  inline uint32_t getOutputCount() const { return m_outputPlugsCount; }

  // TODO make this friend?
  const ResizableVector<const GPlug *> *getPlugConnections(
      const GPlug *plug) const {
    const bool isInput = isFlag(*plug, PLUG_FLAGS::PLUG_INPUT);
    const int plugIdx = findPlugIndexFromInstance(plug);
    const int plugCount = isInput ? m_inputPlugsCount : m_outputPlugsCount;
    assert(plugIdx != -1);
    assert(plugIdx < plugCount);
    ResizableVector<const GPlug *> **connections =
        isInput ? m_inConnections : m_outConnections;
    return connections[plugIdx];
  }

  int findPlugIndexFromInstance(const GPlug *plug) const {
    const bool isInput = isFlag(*plug, PLUG_FLAGS::PLUG_INPUT);
    const int count = isInput ? m_inputPlugsCount : m_outputPlugsCount;
    const GPlug *plugs = isInput ? m_inputPlugs : m_outputPlugs;
    for (int i = 0; i < count; ++i) {
      if (&plugs[i] == plug) {
        return i;
      }
    }
    return -1;
  }
  // plug functions
  // extract the first 31st bits such that to ignore the input/output flag bit
  static constexpr uint32_t getPlugIndex(const uint32_t x) {
    return (((1u << 31u) - 1u) & (x));
  }
  // sets the last bit of the 32bit value to 1, marking it an input
  static constexpr uint32_t inputPlugCode(const uint32_t x) {
    return ((1u << 31u) | (x));
  }
  // sets the last bit of the 32bit value to 0, marking it an output
  // as of now this does nothing, is pure visual for the programmer, might want
  // to force the last bit to be zero with an & ?
  static constexpr uint32_t outputPlugCode(const uint32_t x) {
    return (~(1u << 31u)) & x;
  }
  // extracting the last bit, if one is an input plug
  static constexpr uint32_t isInputPlug(const uint32_t x) {
    return (((1u << 31u) & (x)) > 0);
  }

 protected:
  inline bool isFlag(const GPlug &plug, const PLUG_FLAGS flag) const {
    return (plug.flags & flag) > 0;
  }
  void defaultInitializePlugsAndConnections(
      const int inputCount, const int outputCount,
      const int reserve = DEFAULT_PLUG_CONNECTION_ALLOCATION) {
    const int totalCount = inputCount + outputCount;
    auto *plugs = static_cast<GPlug *>(
        m_allocs.allocator->allocate(sizeof(GPlug) * totalCount));

    m_inputPlugs = inputCount != 0 ? plugs : nullptr;
    m_inputPlugsCount = inputCount;
    m_outputPlugs = outputCount != 0 ? plugs + inputCount : nullptr;
    m_outputPlugsCount = outputCount;

    defaultInitializeConnectionPool(inputCount, outputCount, reserve);
  }

  void defaultInitializeConnectionPool(
      int inputCount, int outputCount,
      int reserve = DEFAULT_PLUG_CONNECTION_ALLOCATION);

 protected:
  struct CallbackTracker {
    GNodeCallback *callback;
    uint32_t id;
  };

  const GraphAllocators &m_allocs;
  const char *m_nodeName;
  const char *m_nodeType;
  GPlug *m_inputPlugs = nullptr;
  GPlug *m_outputPlugs = nullptr;
  uint32_t m_inputPlugsCount = 0;
  uint32_t m_outputPlugsCount = 0;
  uint32_t m_nodeIdx = 0;
  int m_generation = -1;
  static const int DEFAULT_PLUG_CONNECTION_ALLOCATION = 3;
  ResizableVector<const GPlug *> **m_inConnections = nullptr;
  ResizableVector<const GPlug *> **m_outConnections = nullptr;
  ResizableVector<CallbackTracker> m_callbacks;
};

template <typename T>
inline T getInputConnection(ResizableVector<const GPlug *> **conns,
                            const int plugId) {
  const auto conn = conns[GNode::getPlugIndex(plugId)];

  assert(conn->size() != 0 && "no connections on given plug");
  assert(conn->size() == 1 && "too many input connections");
  const GPlug *source = (*conn)[0];
  const auto h = T{source->plugValue};
  assert(h.isHandleValid());
  return h;
}
template class SIR_ENGINE_API ResizableVector<GNode *>;
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

  inline GNode *findNodeOfType(const char *type) {
    const uint32_t nodesCount = m_nodes.size();
    for (uint32_t i = 0; i < nodesCount; ++i) {
      const bool isType = m_nodes.getConstRef(i)->isOfType(type);
      if (isType) {
        return m_nodes.getConstRef(i);
      }
    }
    return nullptr;
  }

  /* This function removes the node from the graph
   * NOTE this function won't cleanup the connections,
   * is a simple function to find the node and remove it, is up to the user
   * to clean up connections or, call the removeNodeAndConnections method (if
   * exists, if not you need to make it :D No check is done whether or not you
   * are removing the last node in the graph used for linearization, this is
   * still up for debate Ownership of the node is returned, this mean no clean
   * up on the node itself will be done
   */
  bool removeNode(GNode *node);

  static inline bool connectNodes(GNode *sourceNode, const int sourceId,
                                  GNode *destinationNode,
                                  const int destinationId) {
    const bool sourceConnect =
        sourceNode->connect(sourceId, destinationNode, destinationId);
    const bool destConnect =
        destinationNode->connect(destinationId, sourceNode, sourceId);
    return sourceConnect & destConnect;
  }
  static inline bool connectNodes(const GPlug *sourcePlug,
                                  const GPlug *destinationPlug) {
    const bool sourceConnect =
        sourcePlug->nodePtr->connect(sourcePlug, destinationPlug);
    const bool destConnect =
        destinationPlug->nodePtr->connect(destinationPlug, sourcePlug);
    return sourceConnect & destConnect;
  }

  inline const ResizableVector<GNode *> &getLinearizedGraph() const {
    return m_linearizedGraph;
  }

  static bool isConnected(GNode *sourceNode, const int sourceId,
                          GNode *destinationNode, const int destinationId) {
    const int sourceConnected =
        sourceNode->isConnected(sourceId, destinationNode, destinationId);
    const int destinationConnected =
        destinationNode->isConnected(destinationId, sourceNode, sourceId);
    return (sourceConnected != -1) & (destinationConnected != -1);
  }

  [[nodiscard]] GNode *getFinalNode() const { return finalNode; }
  inline void setFinalNode(GNode *node) { finalNode = node; }
  void finalizeGraph();
  void clear();
  void compute();
  inline uint32_t nodeCount() const { return m_nodes.size(); }
  void onResizeEvent(const int screenWidth, const int screenHeight,
                     CommandBufferHandle commandBuffer) {
    int count = m_linearizedGraph.size();
    for (int i = 0; i < count; ++i) {
      m_linearizedGraph[i]->onResizeEvent(screenWidth, screenHeight,
                                          commandBuffer);
    }
    for (int i = 0; i < count; ++i) {
      m_linearizedGraph[i]->populateNodePorts();
    }
  };

 private:
  ResizableVector<GNode *> m_nodes;
  GNode *finalNode = nullptr;
  uint32_t m_nodeCounter = 0;
  ResizableVector<GNode *> m_linearizedGraph;
  static constexpr int GRAPH_DEFAULT_RESERVE_SIZE = 100;
};

}  // namespace SirEngine
