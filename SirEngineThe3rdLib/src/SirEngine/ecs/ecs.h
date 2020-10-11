#pragma once
#include <stdint.h>

#include <cassert>
#include <iostream>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace SirEngine::ecs {
namespace CompileHash {
static constexpr unsigned int crc_table[256] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
    0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
    0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
    0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
    0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
    0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
    0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
    0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
    0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
    0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
    0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
    0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
    0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
    0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
    0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
    0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
    0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
    0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
    0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
    0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
    0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d};
template <size_t idx>
constexpr uint32_t combine_crc32(const char* str, uint32_t part) {
  return (part >> 8) ^ crc_table[(part ^ str[idx]) & 0x000000FF];
}

template <size_t idx>
constexpr uint32_t crc32(const char* str) {
  return combine_crc32<idx>(str, crc32<idx - 1>(str));
}

// This is the stop-recursion function
template <>
constexpr uint32_t crc32<size_t(-1)>(const char*) {
  return 0xFFFFFFFF;
}

template <size_t len>
constexpr uint32_t hash_fn(const char* str) {
  return CompileHash::crc32<len - 2>(str) ^ 0xFFFFFFFF;
}
}  // namespace CompileHash

#if defined(_MSC_VER)
#define GENERATOR_PRETTY_FUNCTION __FUNCSIG__
#elif defined(__clang__) || (defined(__GNUC__) || defined(__GNUG__))
#define GENERATOR_PRETTY_FUNCTION __PRETTY_FUNCTION__
#endif

constexpr size_t hash_combine(size_t seed1, size_t seed2) {
  seed1 ^= seed2 + 0x9e3779b9 + (seed1 << 6) + (seed1 >> 2);
  return seed1;
}

// a const expr version of string len, used for compile time hash
constexpr int strl(const char* str) {
  int counter = 0;
  while (str[counter] != '\0') {
    ++counter;
  }
  return counter;
}

// this is part of the backbone of the ecs, this allows us to
// hash N types at compile time
template <class T, class... Types>
struct MultiHash;

// this is the final case of the recursion, or simply a single type
template <class T>
struct MultiHash<T> {
  // here we need a second TYPE passed in to change the signature of
  // the function and get us a different function name
  template <class FN_TYPE>
  static constexpr const char* fnType() {
    return GENERATOR_PRETTY_FUNCTION;
  }
  static constexpr size_t hash =
      CompileHash::hash_fn<strl(fnType<T>())>(fnType<T>());
};

template <class T, class... TYPES>
struct MultiHash {
  template <class... FN_TYPES>
  static constexpr const char* fnType() {
    return GENERATOR_PRETTY_FUNCTION;
  }
  // here we recursively hash one type at the time and combine it with the hash
  // combine function
  static constexpr size_t hash =
      hash_combine(CompileHash::hash_fn<strl(fnType<T>())>(fnType<T>()),
                   MultiHash<TYPES...>::hash);
};
// this is some meta-template magic to find the index of a type in a list of
// types
namespace TypeToIndexMeta {
// getting integral constant for index of a template type
template <typename...>
struct index;

// found it
template <typename T, typename... R>
struct index<T, T, R...> : std::integral_constant<size_t, 0> {};

// still looking
template <typename T, typename F, typename... R>
struct index<T, F, R...>
    : std::integral_constant<size_t, 1 + index<T, R...>::value> {};
}  // namespace TypeToIndexMeta







	
// This is a poor man metadata about a component type. It is enough information
// to move components around
struct ComponentTypeInfo {
  // this is the size of the component not the number of elements in the
  // components the data type size.
  size_t componentDataTypeSize;
  // hash generated for the type, coming from the MultiHash template class
  size_t hash;
};

// this strut represent a component, the component can only hold pod data to
// make our life easier if in the future we need to change that we can register
// constructor destructor lambda functions
struct Component {
  void* data;
  ComponentTypeInfo info;
};

// when we move components from one entity to another archetype we have other
// entities moved around to patch holes in memory we use this entity move result
// to inform the registry of internal movements that happened inside the
// archetype
struct EntityMoveResult {
  static constexpr int64_t VOID_REQUEST = -1;
  int64_t entityGlobalIndex = VOID_REQUEST;
  // the index of the entity that was moved
  uint32_t sourceIdx;
  // the index where the entity has been moved to
  uint32_t destIdx;
};

// this is an actual entity, the entity is nothing more than tracking
// information to where the actual data is.
struct Entity {
  // this is where inside the archetype the entity is
  uint32_t localIndex;
  // points to the index in the registry archetype array, this allows us to do
  // a quick look up of the archetype
  uint16_t archetypeIndex;
  // the version is used to avoid issues where entities get recycled and you
  // might have an old entity that points to a valid location just because the
  // entity has been reused but refers to a wrong entity, the version lets you
  // disambiguate against that. Every time an entity is used the version gets
  // bumped
  uint16_t version;
};
struct EntityId {
  uint32_t index;
  uint16_t version;
  // left for potential extra metadata later on
  uint16_t _padding;
};


struct Archetype {
  // how many components to allocate on createtion
  static constexpr uint32_t INITIAL_SIZE = 10;
  uint32_t componentCount = 0;
  uint32_t entityCount = 0;
  // this is how many components can fit in our buffer allocation.It is not
  // a size in bytes but a size in elements.
  uint32_t bufferElementCount = 0;
  // the id of the archetype, this is the result of hashing all the types of
  // components it hosts
  size_t hash = 0;
  Component* m_components = nullptr;
  // this array hosts the indices to which each entity refers to, this is used
  // for several bookkeeping, like inform the registry that an entity has been
  // moved internally
  size_t* m_entitieIndexes = nullptr;
  std::unordered_map<size_t, size_t> lookup;

  template <typename... TYPES>
  void create() {
    bufferElementCount = INITIAL_SIZE;
    m_components = new Component[sizeof...(TYPES)]{
        Component{new char[INITIAL_SIZE * sizeof(TYPES)],
                  {sizeof(TYPES), MultiHash<TYPES>::hash}}...};
    m_entitieIndexes = new size_t[INITIAL_SIZE];
    lookup = std::unordered_map<size_t, size_t>{
        {MultiHash<TYPES>::hash,
         TypeToIndexMeta::index<TYPES, TYPES...>::value}...};
    componentCount = sizeof...(TYPES);
    hash = MultiHash<TYPES...>::hash;
  }

  template <typename... TYPES>
  void create(const size_t entityGlobalIndex, TYPES... types) {
    create<TYPES...>();
    (write(types, 0), ...);
    m_entitieIndexes[0] = entityGlobalIndex;
  }
  void createFromComponents(Component* cmps, const size_t size) {
    hash = 0;
    for (size_t i = 0; i < size; ++i) {
      lookup[cmps[i].info.hash] = i;
      hash = hash_combine(hash, cmps[i].info.hash);
    }
    bufferElementCount = INITIAL_SIZE;
    m_components = cmps;
    m_entitieIndexes = new size_t[INITIAL_SIZE];
    entityCount = 0;
    componentCount = static_cast<uint32_t>(size);
  }

  template <typename T>
  void write(T cmp, uint32_t idx) {
    auto cmpId = getComponentIndex<T>();
    assert(cmpId != -1);
    assert(static_cast<uint32_t>(cmpId) < componentCount);
    assert(idx < bufferElementCount);
    static_cast<T*>(m_components[cmpId].data)[idx] = cmp;
  }

  // component getters
  template <typename T>
  [[nodiscard]] int getComponentIndex() const {
    return getComponentIndex(MultiHash<T>::hash);
  }

  [[nodiscard]] int getComponentIndex(const size_t cmpId) const {
    auto found = lookup.find(cmpId);
    return found != lookup.end() ? static_cast<int>(found->second) : -1;
  }

  template <typename T>
  [[nodiscard]] bool hasComponent() const {
    return hasComponent(MultiHash<T>::hash);
  }

  [[nodiscard]] bool hasComponent(const size_t cmpId) const {
    return getComponentIndex(cmpId) != -1;
  }

  template <typename... Types>
  [[nodiscard]] bool hasComponents() const {
    return (hasComponent<Types>() & ...);
  }
  [[nodiscard]] bool hasComponents(const std::vector<size_t>& ids) const {
    size_t size = ids.size();
    bool result = true;
    for (size_t i = 0; i < size; ++i) {
      result &= hasComponent(ids[i]);
    }
    return result;
  }

  template <typename T>
  Component* getComponent() {
    auto idx = getComponentIndex<T>();
    return getComponent(idx);
  }

  [[nodiscard]] Component* getComponent(const size_t cmpId) const {
    assert(cmpId < componentCount);
    return &(m_components[cmpId]);
  }

  template <typename... Types>
  size_t addEntity(size_t eid, Types... toAdd) {
    // let us do the size check
    resizeIfNeeded();

    (write(toAdd, entityCount), ...);
    m_entitieIndexes[entityCount] = eid;
    return entityCount++;
  }
  void resizeIfNeeded() {
    if (entityCount >= bufferElementCount) {
      resize();
    }
  }

  void resize() {
    uint32_t newSize = bufferElementCount * 2;
    for (uint32_t i = 0; i < componentCount; ++i) {
      Component& cmp = m_components[i];
      // todo change this for an allocator
      size_t toAlloc = newSize * cmp.info.componentDataTypeSize;
      char* newMemory = new char[toAlloc];
      memcpy(newMemory, cmp.data,
             bufferElementCount * cmp.info.componentDataTypeSize);
      delete[] static_cast<char*>(cmp.data);
      cmp.data = newMemory;
    }
    auto* newEntities = new size_t[newSize];
    memcpy(newEntities, m_entitieIndexes, bufferElementCount * sizeof(size_t));
    delete[] m_entitieIndexes;
    m_entitieIndexes = newEntities;

    bufferElementCount = newSize;
  }

  [[nodiscard]] EntityMoveResult deleteEntity(const Entity e) {
    uint32_t destIdx = e.localIndex;
    uint32_t sourceIdx = entityCount - 1;
    size_t eid = m_entitieIndexes[sourceIdx];
    // if the entity is the last one we do not process it
    if (destIdx != sourceIdx) {
      assert(destIdx < sourceIdx);
      for (uint32_t i = 0; i < componentCount; ++i) {
        Component& cmp = m_components[i];
        memcpy(static_cast<char*>(cmp.data) +
                   destIdx * cmp.info.componentDataTypeSize,
               static_cast<char*>(cmp.data) +
                   sourceIdx * cmp.info.componentDataTypeSize,
               cmp.info.componentDataTypeSize);
      }
    }

    // we have one less entity now
    --entityCount;
    m_entitieIndexes[destIdx] = m_entitieIndexes[sourceIdx];
    return EntityMoveResult{
        destIdx == sourceIdx ? -1 : static_cast<int64_t>(eid), sourceIdx,
        destIdx};
  }

  template <typename T>
  EntityMoveResult move(Archetype* source, T cmp, Entity& e) {
    auto& sourceIds = source->lookup;
    resizeIfNeeded();
    // using structured bindings to expand the tuple returned by the hash map
    // TODO optimize this no need to iterate over a hash map, store the hash in
    // the component and iterate component list
    for (const auto& [componentHash, componentIndex] : sourceIds) {
      Component* sourceCmp = source->getComponent(componentIndex);
      auto destIdx = getComponentIndex(componentHash);
      assert(destIdx != -1);
      Component* destCmp = getComponent(destIdx);
      // perform the copy
      assert(destCmp->info.componentDataTypeSize ==
             sourceCmp->info.componentDataTypeSize);
      auto* destPtr = static_cast<char*>(destCmp->data);
      size_t destOffset = entityCount * destCmp->info.componentDataTypeSize;
      auto* srcPtr = static_cast<char*>(sourceCmp->data);
      size_t srcOffset = e.localIndex * sourceCmp->info.componentDataTypeSize;
      memcpy(destPtr + destOffset, srcPtr + srcOffset,
             sourceCmp->info.componentDataTypeSize);
    }
    // next we need to add the new component
    write(cmp, entityCount);

    // we need to remove the old entity from the source component
    // to do so we copy the last entity entity to the hole and return that such
    // entity has been moved
    EntityMoveResult r = source->deleteEntity(e);
    // now we need to update the entity
    e.localIndex = entityCount;
    // updating the entity count
    ++entityCount;
    return r;
  }
  EntityMoveResult move(Archetype* source, Entity& e) {
    auto& sourceIds = lookup;
    resizeIfNeeded();
    // using structured bindings to expand the tuple returned by the hash map
    // TODO optimize this no need to iterate over a hash map, store the hash in
    // the component and iterate component list
    for (const auto& [componentHash, componentIndex] : sourceIds) {
      auto srcIdx = source->getComponentIndex(componentHash);
      assert(srcIdx != -1);
      Component* sourceCmp = source->getComponent(srcIdx);
      Component* destCmp = getComponent(componentIndex);
      // perform the copy
      assert(destCmp->info.componentDataTypeSize ==
             sourceCmp->info.componentDataTypeSize);
      auto* destPtr = static_cast<char*>(destCmp->data);
      size_t destOffset = entityCount * destCmp->info.componentDataTypeSize;
      auto* srcPtr = static_cast<char*>(sourceCmp->data);
      size_t srcOffset = e.localIndex * sourceCmp->info.componentDataTypeSize;
      memcpy(destPtr + destOffset, srcPtr + srcOffset,
             sourceCmp->info.componentDataTypeSize);
    }

    // we need to remove the old entity from the source component
    // to do so we copy the last entity entity to the hole and return that such
    // entity has been moved
    EntityMoveResult r = source->deleteEntity(e);
    // now we need to update the entity
    e.localIndex = entityCount;
    // updating the entity count
    ++entityCount;
    return r;
  }
};

class Registry {
 public:
  template <typename... Types>
  EntityId addEntity(Types... types) {
    // make sure the types exists
    ensureTypeInfos<Types...>();
    // let us find an archetype
    size_t id = MultiHash<Types...>::hash;
    Archetype* arch = findArchetype<Types...>();

    size_t eid = getNewEntityId();
    size_t localIndex = arch->addEntity(eid, types...);

    Entity& e = m_entities[eid];
    e.archetypeIndex = m_archetypeToIndex[id];
    e.localIndex = static_cast<uint32_t>(localIndex);
    return {static_cast<uint32_t>(eid), e.version};
  }
  template <typename... Types>
  void ensureTypeInfos() {
    (ensureTypeInfo<Types>(), ...);
  }

  template <typename T>
  void ensureTypeInfo() {
    size_t id = MultiHash<T>::hash;
    auto found = m_componentTypeInfo.find(id);
    if (found == m_componentTypeInfo.end()) {
      // we need to add the type info
      m_componentTypeInfo[id] = {sizeof(T), MultiHash<T>::hash};
    }
  }

  template <typename T>
  bool hasComponent(const EntityId eid) {
    Entity e = m_entities[eid.index];
    return m_archetypes[e.archetypeIndex]->getComponentIndex<T>() != -1;
  }

  template <typename T>
  [[nodiscard]] const T& getComponent(const EntityId eid) const {
    return getComponent<T>(eid);
  }

  template <typename T>
  T& getComponent(const EntityId eid) {
    assert(hasComponent<T>(eid));
    const Entity e = m_entities[eid.index];
    Component* cmp = m_archetypes[e.archetypeIndex]->getComponent<T>();
    auto* data = static_cast<T*>(cmp->data);
    return data[e.localIndex];
  }

  template <typename... QTypes>
  void query(std::vector<std::tuple<size_t, QTypes...>>& q) {
    q.clear();
    size_t size = m_archetypes.size();
    for (size_t i = 0; i < size; ++i) {
      Archetype* arch = m_archetypes[i];
      bool result =
          arch->hasComponents<typename std::remove_pointer<QTypes>::type...>();
      result &= (arch->entityCount != 0);
      if (result) {
        auto componentCount = arch->entityCount;
        std::tuple<size_t, QTypes...> tup{
            componentCount,
            (static_cast<QTypes>(
                arch->getComponent<typename std::remove_pointer<QTypes>::type>()
                    ->data))...};
        q.emplace_back(tup);
      }
    }
  }


  void deleteEntity(const EntityId eid) {
    assert(eid.index < m_entities.size());
    Entity& e = m_entities[eid.index];
    auto* arch = m_archetypes[e.archetypeIndex];

    EntityMoveResult moveResult = arch->deleteEntity(e);
    if (moveResult.entityGlobalIndex != -1) {
      // update the moved entity
      Entity& movedEntity = m_entities[moveResult.entityGlobalIndex];
      movedEntity.localIndex = moveResult.destIdx;
    }
    m_freeEntities.push_back(eid.index);
    e.archetypeIndex = INVALID_ARCHETYPE;
  }

  [[nodiscard]] bool isEntityValid(const EntityId eid) const {
    assert(eid.index < m_entities.size());
    const Entity& e = m_entities[eid.index];
    return (e.archetypeIndex != INVALID_ARCHETYPE) & (eid.version == e.version);
  }

  template <typename T>
  void removeComponent(const EntityId eid) {
    ensureTypeInfo<T>();
    assert(hasComponent<T>(eid));

    Entity& e = m_entities[eid.index];
    auto* arch = m_archetypes[e.archetypeIndex];

    const auto& ids = arch->lookup;
    Archetype* next = nullptr;
    uint16_t nextIdx = INVALID_ARCHETYPE;
    uint16_t counter = 0;
    // let us build the list of components we need
    scratchIds.clear();
    for (const auto& [cmpId, cmpIdx] : ids) {
      if (cmpId != MultiHash<T>::hash) {
        scratchIds.push_back(cmpId);
      }
    }
    assert(scratchIds.size() == (arch->lookup.size() - 1));

    for (auto* a : m_archetypes) {
      if ((a == arch) | (a->componentCount != scratchIds.size())) {
        ++counter;
        continue;
      }
      bool found = true;

      // iterating the current components
      for (const auto currId : scratchIds) {
        found &= (a->getComponentIndex(currId) == -1) ? false : true;
      }
      if (found) {
        next = a;
        nextIdx = counter;
        break;
      }
      ++counter;
    }
    if (next == nullptr) {
      next = createArchetypeFromIds(scratchIds);
      auto found = m_archetypeToIndex.find(next->hash);
      assert(found != m_archetypeToIndex.end());
      nextIdx = found->second;
    }
    assert(nextIdx != -1);
    EntityMoveResult moveResult = next->move(arch, e);
    if (moveResult.entityGlobalIndex != -1) {
      // update the moved entity
      Entity& movedEntity = m_entities[moveResult.entityGlobalIndex];
      movedEntity.localIndex = moveResult.destIdx;
    }

    // updated archetype
    e.archetypeIndex = nextIdx;
  }

  template <typename T>
  void addComponent(const EntityId eid, T cmp) {
    ensureTypeInfo<T>();
    Entity& e = m_entities[eid.index];
    auto* arch = m_archetypes[e.archetypeIndex];
    const auto& ids = arch->lookup;
    Archetype* next = nullptr;
    uint16_t nextIdx = INVALID_ARCHETYPE;
    uint16_t counter = 0;
    scratchIds.clear();
    for (const auto& [cmpId, cmpIdx] : ids) {
      scratchIds.push_back(cmpId);
    }
    scratchIds.push_back(MultiHash<T>::hash);

    for (auto* a : m_archetypes) {
      if (a == arch) {
        ++counter;
        continue;
      }
      bool found = true;

      // iterating the current components
      for (const auto currId : scratchIds) {
        found &= (a->getComponentIndex(currId) == -1) ? false : true;
      }
      if (found) {
        next = a;
        nextIdx = counter;
        break;
      }
      ++counter;
    }
    if (next == nullptr) {
      next = createArchetypeFromIds(scratchIds);
      auto found = m_archetypeToIndex.find(next->hash);
      assert(found != m_archetypeToIndex.end());
      nextIdx = found->second;
    }
    assert(nextIdx != INVALID_ARCHETYPE);

    EntityMoveResult moveResult = next->move(arch, cmp, e);
    if (moveResult.entityGlobalIndex != -1) {
      // update the moved entity
      Entity& movedEntity = m_entities[moveResult.entityGlobalIndex];
      movedEntity.localIndex = moveResult.destIdx;
    }

    // updated archetype
    e.archetypeIndex = nextIdx;
  }

  Entity& getEntity(const EntityId eid) {
    assert(eid.index < m_entities.size());
    return m_entities[eid.index];
  }

 private:
  template <typename... TYPES>
  Archetype* findArchetype(const bool createIfMissing = true) {
    const size_t id = MultiHash<TYPES...>::hash;
    auto found = m_archetypeToIndex.find(id);
    if (found != m_archetypeToIndex.end()) {
      return m_archetypes[found->second];
    }
    if (createIfMissing) {
      m_archetypeToIndex[id] = static_cast<uint16_t>(m_archetypes.size());
      auto* arch = new Archetype();
      arch->create<TYPES...>();
      m_archetypes.emplace_back(arch);
      return arch;
    }
    return nullptr;
  }


  static Component createComponent(const ComponentTypeInfo& info) {
    return Component{
        new char[info.componentDataTypeSize * Archetype::INITIAL_SIZE],
        {info.componentDataTypeSize, info.hash}};
  }

  Archetype* createArchetypeFromIds(const std::vector<size_t>& ids) {
    // allocating memory for the component array
    auto* cmps = new Component[ids.size()];
    int counter = 0;
    for (const auto cmpId : ids) {
      auto found = m_componentTypeInfo.find(cmpId);
      assert(found != m_componentTypeInfo.end());
      const ComponentTypeInfo& info = found->second;
      cmps[counter++] = createComponent(info);
    }
    auto* arch = new Archetype();
    arch->createFromComponents(cmps, ids.size());
    m_archetypeToIndex[arch->hash] = static_cast<uint16_t>(m_archetypes.size());
    m_archetypes.push_back(arch);
    return arch;
  }

  size_t getNewEntityId() {
    if (m_freeEntities.empty()) {
      size_t toReturn = m_entities.size();
      m_entities.emplace_back(
          Entity{0, INVALID_ARCHETYPE, static_cast<uint16_t>(1)});
      return toReturn;
    }
    size_t toReturn = m_freeEntities[m_freeEntities.size() - 1];
    assert(m_entities[toReturn].archetypeIndex == INVALID_ARCHETYPE);
    ++m_entities[toReturn].version;
    m_freeEntities.pop_back();
    return toReturn;
  }

 private:
  static constexpr uint16_t INVALID_ARCHETYPE = static_cast<uint16_t>(-1);

  std::vector<size_t> scratchIds;
  std::vector<Archetype*> m_archetypes;
  std::vector<Entity> m_entities;
  std::unordered_map<size_t, uint16_t> m_archetypeToIndex;
  std::unordered_map<size_t, ComponentTypeInfo> m_componentTypeInfo;
  std::vector<size_t> m_freeEntities;
};
}  // namespace SirEngine::ecs