#include "SirEngine/core.h"
struct ComponentDescription
{
    uint32_t  count;
    uint32_t  componentSize;
    size_t componentHash;
};

struct Component
{
    void* data;
    size_t count;
    size_t componentSize;

};

struct Entity
{
    size_t archetypeId;
    uint32_t localIndex;
    uint32_t version;
};
struct EntityId
{
    uint32_t index;
    uint32_t version;
};

/*
struct Archetype 
{
    uint32_t componentCount =0; 
    size_t id =0;
    Component* m_components = nullptr;
    std::unordered_map<size_t,size_t> lookup;
    static constexpr uint32_t INITIAL_SIZE = 10;

    //getting integral constant for index of a template type
    template <typename... >
    struct index;
    
    // found it
    template <typename T, typename... R>
    struct index<T, T, R...>
    : std::integral_constant<size_t, 0>
    { };
    
    // still looking
    template <typename T, typename F, typename... R>
    struct index<T, F, R...>
    : std::integral_constant<size_t, 1 + index<T,R...>::value>
    { };


    template<typename ... Types> 
    void create(){
        m_components = new Component[sizeof...(Types)]{Component{new Types[INITIAL_SIZE],1,sizeof(Types)}...};
        lookup = std::unordered_map<size_t,size_t>{{MultiHash<Types>::hash,index<Types,Types...>::value }...};
        componentCount = sizeof...(Types);
        id = MultiHash<Types...>::hash;
    }

    template<typename ... Types> 
    void create(Types ... types){
        create<Types...>();
        (write(types,0),...);
    }
    size_t create(const std::vector<size_t>& ids)
    {
        componentCount = ids.size();
        m_components = new Component[componentCount]{};
        for(int i =0; i < componentCount;++i)
        {

        }

    }

    template<typename T>
    void write(T first , uint32_t index){
        auto cmpId= getComponentIndex<T>();
        assert(index != -1);
        static_cast<T*>(m_components[cmpId].data)[index] = first;
    }

    //component getters
    template<typename T>
    int getComponentIndex() const 
    {
        return getComponentIndex(MultiHash<T>::hash);
    }
    int getComponentIndex(size_t id) const 
    {
        auto found = lookup.find(id);
        return found != lookup.end() ? static_cast<int>(found->second) : -1;
    }

     template <typename T>
     Component* getComponent()
     {
         auto idx = getComponentIndex<T>();
         return getComponent(idx);
     }

    Component* getComponent(size_t cmpId) 
    {
        assert(cmpId < componentCount);
        return &(m_components[cmpId]);
    }

    template<typename T,typename ...TypesToAdd>
    size_t addEntity(T toAdd, TypesToAdd... next)
    {
        Component* cmp = getComponent<T>();
        static_cast<T*>(cmp->data)[cmp->count] = toAdd;
        return addEntity(next... );
    }
    template<typename T>
    size_t addEntity(T toAdd)
    {
        Component* cmp = getComponent<T>();
        (static_cast<T*>(cmp->data)[cmp->count]) = toAdd;
        return cmp->count++;

    }

    template<typename T>
    void move(Archetype* source,T cmp, Entity& e)
    {
        auto& sourceIds = source->lookup;
        for(auto id : sourceIds)
        {
            const Component* dstCmp = getComponent(id.second);
            const Component* srcCmp = source->getComponent(id.second);
            uint32_t offset = dstCmp->componentSize * e.localIndex;
            memcpy((char*)dstCmp->data + offset,(char*)srcCmp->data + offset, dstCmp->componentSize);
            write(cmp,e.localIndex);
        }

    }
};


class Registry
{
    std::vector<size_t> scratchBuffer;
public:
    template<typename T,typename ...Types>
    EntityId addEntity(T first, Types... next)
    {
        //let us find an archetype
        size_t id = MultiHash<T,Types...>::hash;
        Archetype* arch  = findArchetype<T,Types...>(id);
        size_t localIndex = arch->addEntity(first,next...);

        int eid = m_entities.size();
        m_entities.emplace_back(Entity{id,static_cast<uint32_t>(localIndex),0});
        return {static_cast<uint32_t>(eid),0};
    }
    
    //template<typename ...Types>
    //void query(std::vector<std::tuple<Types..., size_t>>& ret)
    //{
    //    ret.clear();
    //    auto size = m_archetypes.size();
    //    for(int i =0; i < size;++i)
    //    {
    //        auto* arch = m_archetypes[i];
    //        bool result = arch->hasComponents<Types...>();
    //        if(result)
    //        {
    //            Component* cmp = arch->getComponent(MultiHash<Position>::hash);
    //            std::tuple<Types..., size_t> tup{*static_cast<Types*>(arch->getComponent(MultiHash<Types>::hash)->data)... ,0};
    //            ret.emplace_back(tup);
    //        }

    //    }
    //}

    template<typename T>
    void addComponent(EntityId eid , T cmp)
    {
        auto* arch = m_archetypes[m_entities[eid.index].archetypeId];
        const auto& ids = arch->lookup;
        Archetype * next = nullptr;
        for(auto* a : m_archetypes)
        {
            if(a == arch)continue;
            for(const auto&id : ids)
            {
                if(!a->getComponentIndex(id.first) != -1)
                {
                    break;
                }
            }
            //it also needs to have the new component
            if(!a->getComponentIndex(MultiHash<T>::hash) != -1)
            {
                continue;
            }

            next = a;
            break;
        }
        //if we are here and we have no archetype and we need to create a new one
        if(next == nullptr){
            scratchBuffer.clear();
            for(const auto& id : ids)
            {
                scratchBuffer.push_back(id.first);
                scratchBuffer.push_back(MultiHash<T>::hash);
                findArchetype(scratchBuffer);

            }

        }
        Entity e = m_entities[eid.index];
        next->move(arch,cmp,e);
    }

private:
    template<typename T,typename ...Types>
    Archetype* findArchetype(size_t id, bool createIfMissing=true)
    {
        auto found = m_archetypeToIndex.find(id);
        if(found != m_archetypeToIndex.end())
        {
            return m_archetypes[found->second];
        }
        if(createIfMissing)
        {
            m_archetypeToIndex[id] = m_archetypes.size();
            auto* arch = new Archetype();
            arch->create<T,Types...>();
            m_archetypes.emplace_back(arch);
            return arch;
        }
        return nullptr;
    }
    Archetype* findArchetype(const std::vector<size_t>& ids, bool createIfMissing=true)
    {
        Archetype* arch =nullptr;
        for(const auto& a : m_archetypes)
        {
            bool found = true;
            for(const auto& id : ids)
            {
                found &= (a->getComponentIndex(id) != -1);
            }
            if(found)
            {
                arch = a;break;
            }
        }
        if(arch == nullptr && createIfMissing)
        {
            auto* arch = new Archetype();
            arch->create(ids);
            m_archetypeToIndex[arch->id] = m_archetypes.size();
            m_archetypes.emplace_back(arch);
            return arch;
        }
        return nullptr;
    }

private:
    std::vector<Archetype*> m_archetypes;
    std::vector<Entity> m_entities;
    std::unordered_map<size_t,size_t>m_archetypeToIndex;
};

int main()
{
    Position p{0,1,2};
    Health h{99};
    Dummy d {10};

    Registry r;

    auto eid  = r.addEntity(p,h);
    r.addComponent(eid,d);
    return eid.index;

    
}
*/