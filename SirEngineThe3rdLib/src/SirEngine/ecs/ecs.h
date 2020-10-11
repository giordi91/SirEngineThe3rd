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

// An archetype is a blueprint, it represents a specific configuration of
// components uniquely. This means there should never be two archetypes with the
// same exact set of components.
// An archetype holds the data for each component type, entity indices to know
// which entity the data belongs to
struct Archetype {
  // how many components to allocate on creation
  static constexpr uint32_t INITIAL_SIZE = 10;
  static constexpr int INVALID_COMPONENT_INDEX = -1;
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

  //---------------------------------------------
  // creation functionality
  //---------------------------------------------

  // creates an empty archetype with the given required components
  template <typename... TYPES>
  void create() {
    bufferElementCount = INITIAL_SIZE;
    m_components = new Component[sizeof...(TYPES)]{
        Component{new char[INITIAL_SIZE * sizeof(TYPES)],
                  {sizeof(TYPES), MultiHash<TYPES>::hash}}...};
    m_entitieIndexes = new size_t[INITIAL_SIZE];
    componentCount = sizeof...(TYPES);
    hash = MultiHash<TYPES...>::hash;
  }

  // this is a variation of the creation where you already have the components
  // to add
  template <typename... TYPES>
  void create(const size_t entityGlobalIndex, TYPES... types) {
    create<TYPES...>();
    (write(types, 0), ...);
    m_entitieIndexes[0] = entityGlobalIndex;
  }

  // create an archetype from a list of existing components and a size.
  // This happens when the registry needs to generate an archetype without
  // knowing any of the concrete types. This happens often when moving entities
  // between archetypes
  void createFromComponents(Component* cmps, const size_t size) {
    hash = 0;
    // lets compute the hash by hash-combining the components hashes
    for (size_t i = 0; i < size; ++i) {
      hash = hash_combine(hash, cmps[i].info.hash);
    }
    bufferElementCount = INITIAL_SIZE;
    m_components = cmps;
    m_entitieIndexes = new size_t[INITIAL_SIZE];
    entityCount = 0;
    componentCount = static_cast<uint32_t>(size);
  }

  //---------------------------------------------
  // component getters
  //---------------------------------------------
  template <typename T>
  [[nodiscard]] int getComponentIndex() const {
    return getComponentIndexFromHash(MultiHash<T>::hash);
  }

  [[nodiscard]] int getComponentIndexFromHash(const size_t cmpId) const {
    for (uint32_t i = 0; i < componentCount; ++i) {
      if (m_components[i].info.hash == cmpId) {
        return static_cast<int>(i);
      }
    }
    return INVALID_COMPONENT_INDEX;
  }

  template <typename T>
  Component* getComponent() {
    auto idx = getComponentIndex<T>();
    return getComponentFromIdx(idx);
  }

  [[nodiscard]] Component* getComponentFromIdx(const size_t cmpId) const {
    assert(cmpId < componentCount);
    return &(m_components[cmpId]);
  }

  //---------------------------------------------
  // component exist queries
  //---------------------------------------------
  template <typename T>
  [[nodiscard]] bool hasComponent() const {
    return hasComponentFromHash(MultiHash<T>::hash);
  }

  [[nodiscard]] bool hasComponentFromHash(const size_t cmpId) const {
    return getComponentIndexFromHash(cmpId) != INVALID_COMPONENT_INDEX;
  }

  template <typename... TYPES>
  [[nodiscard]] bool hasComponents() const {
    return (hasComponent<TYPES>() & ...);
  }
  [[nodiscard]] bool hasComponentFromHashes(
      const std::vector<size_t>& ids) const {
    size_t size = ids.size();
    bool result = true;
    for (size_t i = 0; i < size; ++i) {
      result &= hasComponentFromHash(ids[i]);
    }
    return result;
  }

  //---------------------------------------------
  // entity manipulation
  //---------------------------------------------
  // creates the sets of backing up components needed for the given
  // entity id
  template <typename... TYPES>
  size_t createEntity(const size_t eid, TYPES... toAdd) {
    // let us do the size check
    resizeIfNeeded();

    (write(toAdd, entityCount), ...);
    m_entitieIndexes[entityCount] = eid;
    return entityCount++;
  }

  [[nodiscard]] EntityMoveResult deleteEntity(const Entity e) {
    // in order to avoid a whole in the dense array we grab the last element
    // and patch the hole with it, to do so we compute the destination and
    // source index the destination being the hole to be fixed and the source
    // where the entity plugging the hole is coming from
    uint32_t destIdx = e.localIndex;
    uint32_t sourceIdx = entityCount - 1;
    size_t eid = m_entitieIndexes[sourceIdx];
    // if the entity is the last one we do not process it
    // when that happens the source and destination are the same
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
    // we know generate result that will allows the registry to react to the
    // entity being moved and update any internal bookkeeping it might have.
    return EntityMoveResult{destIdx == sourceIdx
                                ? EntityMoveResult::VOID_REQUEST
                                : static_cast<int64_t>(eid),
                            sourceIdx, destIdx};
  }

  // this function is in charge to migrate an entity from one archetype to
  // another and updating the bookkeeping in the progress
  // this function specifically moves the given entity plus adding a new
  // component
  template <typename T>
  EntityMoveResult move(Archetype* source, T cmp, Entity& e) {
    resizeIfNeeded();

    for (uint32_t i = 0; i < source->componentCount; ++i) {
      // first we find the corresponding components for both archetypes
      Component* sourceCmp = source->getComponentFromIdx(i);
      auto destIdx =
          getComponentIndexFromHash(source->m_components[i].info.hash);
      assert(destIdx != INVALID_COMPONENT_INDEX);
      Component* destCmp = getComponentFromIdx(destIdx);

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

  // this function is in charge to migrate an entity from one archetype to
  // another and updating the bookkeeping in the progress
  // this function specifically moves the given entity in the context of
  // component removal
  // as such the components copied are the one in the current archetype not in
  // the source
  EntityMoveResult move(Archetype* source, Entity& e) {
    resizeIfNeeded();

    for (uint32_t i = 0; i < componentCount; ++i) {
      // first we find the corresponding components for both archetypes
      auto srcIdx =
          source->getComponentIndexFromHash(m_components[i].info.hash);
      assert(srcIdx != INVALID_COMPONENT_INDEX);
      Component* sourceCmp = source->getComponentFromIdx(srcIdx);
      Component* destCmp = getComponentFromIdx(i);

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

 private:
  // writes a component of the given type in the correct array at the requested
  // index
  template <typename T>
  void write(T cmp, uint32_t idx) {
    int cmpId = getComponentIndex<T>();
    assert(cmpId != -1);
    assert(static_cast<uint32_t>(cmpId) < componentCount);
    assert(idx < bufferElementCount);
    static_cast<T*>(m_components[cmpId].data)[idx] = cmp;
  }

  inline void resizeIfNeeded() {
    if (entityCount >= bufferElementCount) {
      resize();
    }
  }

  // resizes the needed allocation for components and entities
  void resize() {
    // simple strategy of doubling allocation size
    uint32_t newSize = bufferElementCount * 2;

    // iterate all the components and allocate new memory
    for (uint32_t i = 0; i < componentCount; ++i) {
      Component& cmp = m_components[i];
      // TODO change this for an allocator
      size_t toAlloc = newSize * cmp.info.componentDataTypeSize;
      char* newMemory = new char[toAlloc];
      memcpy(newMemory, cmp.data,
             bufferElementCount * cmp.info.componentDataTypeSize);
      delete[] static_cast<char*>(cmp.data);
      cmp.data = newMemory;
    }

    // do the same but for the entities bookkeeping:
    auto* newEntities = new size_t[newSize];
    memcpy(newEntities, m_entitieIndexes, bufferElementCount * sizeof(size_t));
    delete[] m_entitieIndexes;
    m_entitieIndexes = newEntities;

    bufferElementCount = newSize;
  }
};

// The registry is the public face of the ecs. The user only interacts with the
// Registry only. The Archetype is completely hidden to the user altough exposed
// in this header due to the template craziness, the Archetype is not used by
// the user and only manipulated internally by the Register
class Registry {
 public:
  template <typename... TYPES>
  EntityId createEntity(TYPES... types) {
    // make sure the types exists in our bookkeeping
    ensureTypeInfos<TYPES...>();
    // let us find an archetype
    size_t id = MultiHash<TYPES...>::hash;
    // find a fitting archetype if required create a new one
    Archetype* arch = findArchetype<TYPES...>();

    // creating the entity
    size_t eid = getNewEntityId();
    size_t localIndex = arch->createEntity(eid, types...);

    // updating the entity content with the result of the new allocation
    Entity& e = m_entities[eid];
    e.archetypeIndex = m_archetypeToIndex[id];
    e.localIndex = static_cast<uint32_t>(localIndex);
    return {static_cast<uint32_t>(eid), e.version, 0};
  }
  void deleteEntity(const EntityId eid) {
    assert(eid.index < m_entities.size());
    Entity& e = m_entities[eid.index];
    assert(eid.version == e.version);
    auto* arch = m_archetypes[e.archetypeIndex];

    // deleting an entity is pretty straight forward, we ask the archetype to
    // get rid of it, once that is done we just need to invalidate the current
    // entity and market it for being able to be recycled
    EntityMoveResult moveResult = arch->deleteEntity(e);
    if (moveResult.entityGlobalIndex != EntityMoveResult::VOID_REQUEST) {
      Entity& movedEntity = m_entities[moveResult.entityGlobalIndex];
      movedEntity.localIndex = moveResult.destIdx;
    }
    // marking entity as free ready to be recycled
    m_freeEntities.push_back(eid.index);
    e.archetypeIndex = INVALID_ARCHETYPE;
  }

  template <typename T>
  bool hasComponent(const EntityId eid) {
    Entity e = m_entities[eid.index];
    assert(eid.version == e.version);
    return m_archetypes[e.archetypeIndex]->getComponentIndex<T>() !=
           Archetype::INVALID_COMPONENT_INDEX;
  }

  template <typename T>
  [[nodiscard]] const T& getComponent(const EntityId eid) const {
    return getComponent<T>(eid);
  }

  [[nodiscard]] bool isEntityValid(const EntityId eid) const {
    assert(eid.index < m_entities.size());
    const Entity& e = m_entities[eid.index];
    return (e.archetypeIndex != INVALID_ARCHETYPE) & (eid.version == e.version);
  }

  template <typename T>
  T& getComponent(const EntityId eid) {
    assert(hasComponent<T>(eid));
    const Entity e = m_entities[eid.index];
    assert(eid.version == e.version);
    Component* cmp = m_archetypes[e.archetypeIndex]->getComponent<T>();
    auto* data = static_cast<T*>(cmp->data);
    return data[e.localIndex];
  }

  // a query allows to find all the archetypes that match a specific component
  // setup. The result is a series of tuples, each tuple refers to a matching
  // archetype. The tuple will contain at first index a size, telling the user
  // how many entities are in that archetype, following we will have strongly
  // typed pointers to the requested components. The query is populated from a
  // provided one, this will allow the user to optimize the memory and avoid the
  // registry allocating new memory every time.
  template <typename... TYPES>
  void populateComponentQuery(
      std::vector<std::tuple<size_t, TYPES...>>& query) {
    query.clear();

    // iterating all the archetypes and find which one match
    // TODO there are interesting: ideas to try here, like using a graph of
    // connected archetype to quickly speed up the search instead of this brute
    // force one.
    // https://medium.com/@ajmmertens/building-an-ecs-2-archetypes-and-vectorization-fe21690805f9
    // of course it can be optimized but the growth will always be linear
    size_t size = m_archetypes.size();
    for (size_t i = 0; i < size; ++i) {
      Archetype* arch = m_archetypes[i];
      bool result =
          arch->hasComponents<typename std::remove_pointer<TYPES>::type...>();
      result &= (arch->entityCount != 0);
      if (result) {
        auto componentCount = arch->entityCount;
        std::tuple<size_t, TYPES...> tup{
            componentCount,
            (static_cast<TYPES>(
                arch->getComponent<typename std::remove_pointer<TYPES>::type>()
                    ->data))...};
        query.emplace_back(tup);
      }
    }
  }

  template <typename T>
  void removeComponent(const EntityId eid) {
    ensureTypeInfo<T>();
    assert(hasComponent<T>(eid));

    Entity& e = m_entities[eid.index];
    assert(eid.version == e.version);

    auto* arch = m_archetypes[e.archetypeIndex];

    // let us build the list of components we need
    scratchIds.clear();
    for (uint32_t i = 0; i < arch->componentCount; ++i) {
      auto hash = arch->m_components[i].info.hash;
      if (hash != MultiHash<T>::hash) {
        scratchIds.push_back(hash);
      }
    }

    assert(scratchIds.size() == (arch->componentCount - 1));
    uint16_t nextIdx = INVALID_ARCHETYPE;
    Archetype* next = findArchetypeFromIds(scratchIds, nextIdx);

    EntityMoveResult moveResult = next->move(arch, e);
    if (moveResult.entityGlobalIndex != -1) {
      // update the moved entity
      Entity& movedEntity = m_entities[moveResult.entityGlobalIndex];
      movedEntity.localIndex = moveResult.destIdx;
    }

    // updated archetype in entity
    e.archetypeIndex = nextIdx;
  }

  template <typename T>
  void addComponent(const EntityId eid, T cmp) {
    ensureTypeInfo<T>();
    Entity& e = m_entities[eid.index];
    assert(eid.version == e.version);

    auto* arch = m_archetypes[e.archetypeIndex];
    scratchIds.clear();

    // populate the list of required components
    for (uint32_t i = 0; i < arch->componentCount; ++i) {
      scratchIds.push_back(arch->m_components[i].info.hash);
    }
    scratchIds.push_back(MultiHash<T>::hash);

    uint16_t nextIdx = INVALID_ARCHETYPE;
    Archetype* next = findArchetypeFromIds(scratchIds, nextIdx);

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
    Entity& e = m_entities[eid.index];
    assert(eid.version == e.version);
    return e;
  }

  [[nodiscard]] const Entity& getEntity(const EntityId eid) const {
    assert(eid.index < m_entities.size());
    const Entity& e = m_entities[eid.index];
    assert(eid.version == e.version);
    return e;
  }

 private:
  // Every time we come across a concrete type, we generate the type info for
  // it, meaning the hash and the size of the type. Such that whenever we
  // encounter that type
  // without the concrete type we have enough information to deal with it
  template <typename... TYPES>
  void ensureTypeInfos() {
    (ensureTypeInfo<TYPES>(), ...);
  }

  template <typename T>
  void ensureTypeInfo() {
    size_t id = MultiHash<T>::hash;
    const auto found = m_componentTypeInfo.find(id);
    if (found == m_componentTypeInfo.end()) {
      // we need to add the type info
      m_componentTypeInfo[id] = {sizeof(T), MultiHash<T>::hash};
    }
  }

  // given an array of archetypes we are going to find a matching archetype, if
  // not potentially create one if requested
  Archetype* findArchetypeFromIds(const std::vector<size_t>& requestedIds,
                                  uint16_t& outIdx,
                                  const bool createIfMissing = true) {
    Archetype* next = nullptr;
    uint16_t nextIdx = INVALID_ARCHETYPE;
    uint16_t counter = 0;
    for (auto* a : m_archetypes) {
      if (a->componentCount != requestedIds.size()) {
        ++counter;
        continue;
      }
      bool found = true;

      // iterating the current components
      for (const auto currId : requestedIds) {
        found &= (a->getComponentIndexFromHash(currId) == -1) ? false : true;
      }
      if (found) {
        next = a;
        nextIdx = counter;
        break;
      }
      ++counter;
    }
    if ((next == nullptr) & createIfMissing) {
      next = createArchetypeFromIds(requestedIds);
      auto found = m_archetypeToIndex.find(next->hash);
      assert(found != m_archetypeToIndex.end());
      nextIdx = found->second;
    }
    assert(nextIdx != INVALID_ARCHETYPE);
    outIdx = nextIdx;
    return next;
  }

  // find an archetype given the concrete types
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

  // This function creates an archetype completely from type ids.
  Archetype* createArchetypeFromIds(const std::vector<size_t>& ids) {
    // allocating memory for the component array
    // memory will be of ownership of the archetypes
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
    // first we check whether or not we have a free entity in the free list
    if (m_freeEntities.empty()) {
      size_t toReturn = m_entities.size();
      m_entities.emplace_back(
          Entity{0, INVALID_ARCHETYPE,
                 static_cast<uint16_t>(ENTITY_STARTING_VERSION)});
      return toReturn;
    }
    // if here we recycle a free entity
    size_t toReturn = m_freeEntities[m_freeEntities.size() - 1];
    assert(m_entities[toReturn].archetypeIndex == INVALID_ARCHETYPE);
    // important: here the version gets updated this will invalidate any stale
    // EntityId out there
    ++m_entities[toReturn].version;
    m_freeEntities.pop_back();
    return toReturn;
  }

 private:
  static constexpr uint16_t INVALID_ARCHETYPE = static_cast<uint16_t>(-1);
  static constexpr uint16_t ENTITY_STARTING_VERSION = 1;

  std::vector<size_t> scratchIds;
  std::vector<Archetype*> m_archetypes;
  std::vector<Entity> m_entities;
  std::unordered_map<size_t, uint16_t> m_archetypeToIndex;
  std::unordered_map<size_t, ComponentTypeInfo> m_componentTypeInfo;
  std::vector<size_t> m_freeEntities;
};
}  // namespace SirEngine::ecs