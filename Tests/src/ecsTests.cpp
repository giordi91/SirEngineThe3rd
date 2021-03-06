#include "SirEngine/ecs/ecs.h"
#include "catch/catch.hpp"

struct Position {
  float x, y, z, w;
};
struct Health {
  float hp;
};
struct Dummy {
  int a, b;
  float c;
  uint16_t d, e;
};

using SirEngine::ecs::Archetype;
using SirEngine::ecs::Entity;
using SirEngine::ecs::EntityId;
using SirEngine::ecs::Registry;

TEST_CASE("basic", "[core,ecs]") {
  Registry registry;
  Position p{0, 1, 2, 3};
  Health h{100};

  EntityId id = registry.createEntity(p, h);
  REQUIRE(registry.hasComponent<Position>(id));
  REQUIRE(registry.hasComponent<Health>(id));
  REQUIRE(!registry.hasComponent<Dummy>(id));

  const Position& outPos = registry.getComponent<Position>(id);
  const Health& outHealth = registry.getComponent<Health>(id);

  REQUIRE(outPos.x == Approx(p.x));
  REQUIRE(outPos.y == Approx(p.y));
  REQUIRE(outPos.z == Approx(p.z));
  REQUIRE(outHealth.hp == Approx(h.hp));
}

TEST_CASE("arch query", "[core,ecs]") {
  Position p{0, 1, 2, 3};
  Health h{100};
  Archetype arch;
  size_t entityGlobalIndex = 0;
  arch.create(entityGlobalIndex, p, h);

  REQUIRE(arch.hasComponent<Position>());
  REQUIRE(arch.hasComponent<Health>());
  REQUIRE(!arch.hasComponent<Dummy>());
  REQUIRE(arch.hasComponentFromHash(SirEngine::ecs::MultiHash<Position>::hash));
  REQUIRE(arch.hasComponentFromHash(SirEngine::ecs::MultiHash<Health>::hash));
  REQUIRE(!arch.hasComponentFromHash(SirEngine::ecs::MultiHash<Dummy>::hash));
  REQUIRE(arch.hasComponents<Position, Health>());
  REQUIRE(!arch.hasComponents<Position, Health, Dummy>());
  REQUIRE(!arch.hasComponents<Position, Dummy>());
  REQUIRE(!arch.hasComponents<Dummy>());
  REQUIRE(!arch.hasComponents<Health, Dummy>());
  REQUIRE(!arch.hasComponents<Dummy, Health>());
  REQUIRE(arch.hasComponents<Health, Position>());
  REQUIRE(
      arch.hasComponentFromHashes({SirEngine::ecs::MultiHash<Position>::hash,
                                   SirEngine::ecs::MultiHash<Health>::hash}));
  REQUIRE(
      !arch.hasComponentFromHashes({SirEngine::ecs::MultiHash<Position>::hash,
                                    SirEngine::ecs::MultiHash<Health>::hash,
                                    SirEngine::ecs::MultiHash<Dummy>::hash}));
  REQUIRE(
      !arch.hasComponentFromHashes({SirEngine::ecs::MultiHash<Position>::hash,
                                    SirEngine::ecs::MultiHash<Dummy>::hash}));
  REQUIRE(
      !arch.hasComponentFromHashes({SirEngine::ecs::MultiHash<Dummy>::hash}));
  REQUIRE(
      !arch.hasComponentFromHashes({SirEngine::ecs::MultiHash<Health>::hash,
                                    SirEngine::ecs::MultiHash<Dummy>::hash}));
  REQUIRE(
      arch.hasComponentFromHashes({SirEngine::ecs::MultiHash<Health>::hash,
                                   SirEngine::ecs::MultiHash<Position>::hash}));
}

TEST_CASE("Basic query", "[core,ecs]") {
  Registry registry;
  EntityId eid{};

  for (int i = 0; i < 20; ++i) {
    if ((i % 2) == 0) {
      Position p{0, static_cast<float>(i), 0};
      Health h{static_cast<float>(i)};
      auto currentEntity = registry.createEntity(p, h);
      if (i == 4) {
        eid = currentEntity;
      }
    } else {
      Health h = {static_cast<float>(i)};
      registry.createEntity(h);
    }
  }

  std::vector<std::tuple<size_t, Position*, Health*>> query;
  registry.populateComponentQuery(query);
  REQUIRE(query.size() == 1);
  REQUIRE(std::get<0>(query[0]) == 10);

  const Position* p = std::get<1>(query[0]);
  const Health* h = std::get<2>(query[0]);
  size_t count = std::get<0>(query[0]);
  for (size_t i = 0; i < count; ++i) {
    const Position& currP = p[i];
    const Health& currH = h[i];
    REQUIRE(currP.x == Approx(0.0f));
    REQUIRE(currP.y == Approx(i * 2));
    REQUIRE(currP.z == Approx(0.0f));

    REQUIRE(currH.hp == Approx(i * 2));
  }

  // testing look up by entity, we saved an id during creation and we want to
  // look it up
  Position posCmp = registry.getComponent<Position>(eid);
  REQUIRE(posCmp.x == Approx(0.0f));
  REQUIRE(posCmp.y == Approx(4));
  REQUIRE(posCmp.z == Approx(0.0f));

  REQUIRE(registry.hasComponent<Position>(eid));
  REQUIRE(registry.hasComponent<Health>(eid));
  REQUIRE(!registry.hasComponent<Dummy>(eid));
}

TEST_CASE("Check on not existing component", "[core,ecs]") {
  Registry registry;
  Position p{0, 1, 2, 3};
  Health h{100};
  EntityId eid = registry.createEntity(p, h);
  REQUIRE(!registry.hasComponent<Dummy>(eid));
}

TEST_CASE("Entity growth over limit", "[core,ecs]") {
  const int capacity = Archetype::INITIAL_SIZE;
  const int toIterate = capacity * 2000;
  Registry registry;
  EntityId eid{};
  for (int i = 0; i < toIterate; ++i) {
    EntityId id = registry.createEntity(
        Position{0, static_cast<float>(i), static_cast<float>(i), 99});
    if (i == (capacity + 1)) {
      eid = id;
    }
  }
  Position posCmp = registry.getComponent<Position>(eid);
  REQUIRE(posCmp.x == Approx(0.0f));
  REQUIRE(posCmp.y == Approx(capacity + 1));
  REQUIRE(posCmp.z == Approx(capacity + 1));
}

TEST_CASE("Add component to entity", "[core,ecs]") {
  Registry registry;
  Position p{0, 1, 16, 32};
  Health h{100};
  Dummy d{0, 1, 3.0f, 5, 6};

  EntityId eid = registry.createEntity(p);
  std::vector<std::tuple<size_t, Position*>> query1;
  registry.populateComponentQuery(query1);

  // Testing we only get one archetype and one item in it
  REQUIRE(query1.size() == 1);
  const Position* p0 = std::get<1>(query1[0]);
  REQUIRE(std::get<0>(query1[0]) == 1);
  REQUIRE(p0[0].x == Approx(0));
  REQUIRE(p0[0].y == Approx(1));
  REQUIRE(p0[0].z == Approx(16));
  REQUIRE(p0[0].w == Approx(32));

  // let us add a health component
  registry.addComponent(eid, h);
  REQUIRE(registry.hasComponent<Health>(eid));

  // similarly now we query and check that only get one archetype that has
  // both components
  std::vector<std::tuple<size_t, Position*, Health*>> query2;
  registry.populateComponentQuery(query2);
  REQUIRE(query2.size() == 1);
  const Position* p2 = std::get<1>(query2[0]);
  const Health* h2 = std::get<2>(query2[0]);
  REQUIRE(std::get<0>(query2[0]) == 1);
  REQUIRE(p2[0].x == Approx(0));
  REQUIRE(p2[0].y == Approx(1));
  REQUIRE(p2[0].z == Approx(16));
  REQUIRE(p2[0].w == Approx(32));
  REQUIRE(h2[0].hp == Approx(100));

  // let check that the the look up by entity is still valid
  Position movedPos = registry.getComponent<Position>(eid);
  Health movedHealth = registry.getComponent<Health>(eid);
  REQUIRE(movedPos.x == Approx(0));
  REQUIRE(movedPos.y == Approx(1));
  REQUIRE(movedPos.z == Approx(16));
  REQUIRE(movedPos.w == Approx(32));
  REQUIRE(movedHealth.hp == Approx(100));

  // we need to make sure the old query for position only,returns one, since
  // the entity got moved.
  registry.populateComponentQuery(query1);
  REQUIRE(query1.size() == 1);

  // adding only two positions entities
  p.x = 30;
  EntityId eid2 = registry.createEntity(p);
  p.x = 40;
  EntityId eid3 = registry.createEntity(p);
  std::vector<std::tuple<size_t, Position*>> query3;
  registry.populateComponentQuery(query3);

  // now we check that two archetypes actually match the query
  REQUIRE(query3.size() == 2);
  const Position* p3 = std::get<1>(query3[0]);
  REQUIRE(std::get<0>(query3[0]) == 2);
  REQUIRE(p3[0].x == Approx(30));
  REQUIRE(p3[1].x == Approx(40));

  // next we check archetype 1, the first returned which is the one with
  // position and health
  const Position* p3_1 = std::get<1>(query3[1]);
  REQUIRE(p3_1[0].z == Approx(16));
  REQUIRE(registry.hasComponent<Dummy>(eid) == false);
  REQUIRE(registry.hasComponent<Dummy>(eid3) == false);

  // next we add a dummy component to one of the position only entity, the
  // last one created
  registry.addComponent(eid3, d);

  std::vector<std::tuple<size_t, Position*>> query4;
  registry.populateComponentQuery(query4);
  // we now expect 3 archetypes (position), (position,health),(position,dummy)
  REQUIRE(query4.size() == 3);
  const Position* p4 = std::get<1>(query4[0]);
  REQUIRE(std::get<0>(query4[0]) == 1);
  // it should only have one entity in it the one with position 30
  REQUIRE(p4[0].x == Approx(30));

  // next we check archetype 1, the first returned which is the one with
  // position and health
  REQUIRE(std::get<1>(query4[1])[0].z == Approx(16));

  // finally  we check the new archetype
  Position* q4_2pos = std::get<1>(query4[2]);
  REQUIRE(q4_2pos[0].x == Approx(40));

  // also checking that the original entity did not have a dummy, buy the one
  // we actually added the component to does have it
  REQUIRE(registry.hasComponent<Dummy>(eid) == false);
  REQUIRE(registry.hasComponent<Dummy>(eid3) == true);

  std::vector<std::tuple<size_t, Position*, Dummy*>> query5;
  registry.populateComponentQuery(query5);
  REQUIRE(query5.size() == 1);
  REQUIRE(std::get<0>(query5[0]) == 1);
  REQUIRE(std::get<1>(query5[0])[0].x == Approx(40));
  REQUIRE(std::get<2>(query5[0])[0].a == 0);
  REQUIRE(std::get<2>(query5[0])[0].b == 1);
  REQUIRE(std::get<2>(query5[0])[0].c == Approx(3.0f));
  REQUIRE(std::get<2>(query5[0])[0].d == 5);
  REQUIRE(std::get<2>(query5[0])[0].e == 6);

  // let us perform the look up by entityId and check of parameters for all 3
  // entities
  movedPos = registry.getComponent<Position>(eid);
  movedHealth = registry.getComponent<Health>(eid);
  REQUIRE(movedPos.x == Approx(0));
  REQUIRE(movedPos.y == Approx(1));
  REQUIRE(movedPos.z == Approx(16));
  REQUIRE(movedPos.w == Approx(32));
  REQUIRE(movedHealth.hp == Approx(100));

  movedPos = registry.getComponent<Position>(eid2);
  REQUIRE(movedPos.x == Approx(30));
  REQUIRE(movedPos.y == Approx(1));
  REQUIRE(movedPos.z == Approx(16));
  REQUIRE(movedPos.w == Approx(32));

  movedPos = registry.getComponent<Position>(eid3);
  Dummy movedDummy = registry.getComponent<Dummy>(eid3);
  REQUIRE(movedPos.x == Approx(40));
  REQUIRE(movedPos.y == Approx(1));
  REQUIRE(movedPos.z == Approx(16));
  REQUIRE(movedPos.w == Approx(32));

  REQUIRE(movedDummy.a == 0);
  REQUIRE(movedDummy.b == 1);
  REQUIRE(movedDummy.c == Approx(3.0f));
  REQUIRE(movedDummy.d == 5);
  REQUIRE(movedDummy.e == 6);
}

TEST_CASE("Delete component to entity", "[core,ecs]") {
  Registry registry;
  Position p{0, 1, 16, 32};
  Health h{100};

  EntityId eid = registry.createEntity(p, h);
  std::vector<std::tuple<size_t, Position*, Health*>> query1;
  registry.populateComponentQuery(query1);

  // testing that we only get one archetype and one item in it
  REQUIRE(query1.size() == 1);
  const Position* p0 = std::get<1>(query1[0]);
  REQUIRE(std::get<0>(query1[0]) == 1);
  REQUIRE(p0[0].x == Approx(0));
  REQUIRE(p0[0].y == Approx(1));
  REQUIRE(p0[0].z == Approx(16));
  REQUIRE(p0[0].w == Approx(32));

  // now we delete a component
  registry.removeComponent<Health>(eid);
  std::vector<std::tuple<size_t, Position*, Health*>> query2;
  registry.populateComponentQuery(query2);
  REQUIRE(query2.size() == 0);

  std::vector<std::tuple<size_t, Position*>> query3;
  registry.populateComponentQuery(query3);
  REQUIRE(query3.size() == 1);
}

TEST_CASE("Add component to existing archetype", "[core,ecs]") {
  Registry registry;
  Position p{0, 1, 16, 32};
  Health h{100};

  // first we create an entity with both position and health
  EntityId eid = registry.createEntity(p, h);
  std::vector<std::tuple<size_t, Position*, Health*>> query1;
  registry.populateComponentQuery(query1);

  REQUIRE(query1.size() == 1);
  REQUIRE(std::get<0>(query1[0]) == 1);

  // next we create an entity with only position
  p.x = 100;
  EntityId eid2 = registry.createEntity(p);
  std::vector<std::tuple<size_t, Position*>> query2;
  registry.populateComponentQuery(query2);
  // at this point we should have two archetype returned in the query
  // and each archetype has one entity in it
  REQUIRE(query2.size() == 2);
  REQUIRE(std::get<0>(query2[0]) == 1);

  // now we wish to add a health component to our entity with just position
  h.hp = 900;
  registry.addComponent(eid2, h);
  // now we should still have one archetype, a new one should not be created
  std::vector<std::tuple<size_t, Position*, Health*>> query3;
  registry.populateComponentQuery(query3);
  REQUIRE(query3.size() == 1);
  // and the current archetype should have two entities with position and
  // health

  REQUIRE(std::get<0>(query3[0]) == 2);

  // we also want to query we get the right entity components
  const Position* p3 = std::get<1>(query3[0]);
  const Health* h3 = std::get<2>(query3[0]);
  REQUIRE(p3[0].x == Approx(0));
  REQUIRE(p3[0].y == Approx(1));
  REQUIRE(p3[0].z == Approx(16));
  REQUIRE(p3[0].w == Approx(32));
  REQUIRE(p3[1].x == Approx(100));
  REQUIRE(p3[1].y == Approx(1));
  REQUIRE(p3[1].z == Approx(16));
  REQUIRE(p3[1].w == Approx(32));

  REQUIRE(h3[0].hp == Approx(100));
  REQUIRE(h3[1].hp == Approx(900));

  // the query for position only should return just one archetype
  // because the other archetype is empty
  std::vector<std::tuple<size_t, Position*>> query4;
  registry.populateComponentQuery(query4);
  REQUIRE(query4.size() == 1);
}

TEST_CASE("Preserve entity tracking on entity deletion", "[core,ecs]") {
  Registry registry;
  Position p{0, 1, 2, 3};
  Health h{100};
  Position p2{4, 5, 6, 7};
  Health h2{900};
  EntityId eid = registry.createEntity(p, h);
  EntityId eid2 = registry.createEntity(p2, h2);
  const Entity& e2 = registry.getEntity(eid2);

  // we are about to remove a component, this will move the
  // the entity from one archetype to the other, so we are going
  // to extract the entity index from the entity before and after
  // the component remove, it should change
  const uint32_t e2idxPre = e2.localIndex;
  registry.removeComponent<Position>(eid);
  const uint32_t e2idxPost = e2.localIndex;
  REQUIRE(e2idxPost != e2idxPre);

  Position pPost = registry.getComponent<Position>(eid2);
  REQUIRE(pPost.x == Approx(p2.x));
  REQUIRE(pPost.y == Approx(p2.y));
  REQUIRE(pPost.z == Approx(p2.z));
  REQUIRE(pPost.w == Approx(p2.w));
}

TEST_CASE("Entity with no components", "[core,ecs]") {
  Registry registry;
  Position p{0, 1, 2, 3};
  EntityId eid = registry.createEntity(p);
  registry.removeComponent<Position>(eid);
  bool result = registry.hasComponent<Position>(eid);
  REQUIRE(result == false);
}

TEST_CASE("Delete entity", "[core,ecs]") {
  Registry registry;
  Position p{0, 1, 2, 3};
  EntityId eid = registry.createEntity(p);
  registry.deleteEntity(eid);
  REQUIRE(!registry.isEntityValid(eid));
}

TEST_CASE("Add - Delete entities", "[core,ecs]") {
  Registry registry;
  Position p{0, 1, 2, 3};
  EntityId eid1 = registry.createEntity(p);
  registry.deleteEntity(eid1);
  std::vector<std::tuple<size_t, Position*>> query1;
  registry.populateComponentQuery(query1);
  REQUIRE(query1.empty());
  p.x = 10;
  EntityId eid2 = registry.createEntity(p);
  REQUIRE(!registry.isEntityValid(eid1));
  REQUIRE(registry.isEntityValid(eid2));
  std::vector<std::tuple<size_t, Position*>> query2;
  registry.populateComponentQuery(query2);
  REQUIRE(query2.size() == 1);
  REQUIRE(registry.getEntity(eid2).version == 2);
}

TEST_CASE("Multiple deletes", "[core,ecs]") {
  Registry registry;
  Position p{0, 1, 2, 3};
  EntityId eid1 = registry.createEntity(p);
  p.x = 10;
  EntityId eid2 = registry.createEntity(p);
  p.x = 20;
  EntityId eid3 = registry.createEntity(p);
  Entity e3 = registry.getEntity(eid3);

  // we start by removing the first entity, should get
  // eid3 to move in the archetype
  registry.deleteEntity(eid2);
  std::vector<std::tuple<size_t, Position*>> query;
  registry.populateComponentQuery(query);
  REQUIRE(query.size() == 1);
  REQUIRE(std::get<0>(query[0]) == 2);
  e3 = registry.getEntity(eid3);
  // e3 got moved in the archetype
  REQUIRE(e3.localIndex == 1);
  Position p3 = registry.getComponent<Position>(eid3);
  REQUIRE(p3.x == Approx(20));

  // delete now entity1 , so entity3 should be moved again
  registry.deleteEntity(eid1);
  registry.populateComponentQuery(query);
  REQUIRE(query.size() == 1);
  REQUIRE(std::get<0>(query[0]) == 1);
  e3 = registry.getEntity(eid3);
  // e3 got moved in the archetype
  REQUIRE(e3.localIndex == 0);
  p3 = registry.getComponent<Position>(eid3);
  REQUIRE(p3.x == Approx(20));
}

TEST_CASE("Three components creation", "[core,ecs]") {
  Registry registry;
  Position p{0, 1, 2, 3};
  Health h{100};
  Dummy d{10, 20};
  Position p2{4, 5, 6, 7};
  Health h2{900};
  Dummy d2{90, 91};
  EntityId eid = registry.createEntity(p, h, d);
  EntityId eid2 = registry.createEntity(p2, h2, d2);

  std::vector<std::tuple<size_t, Position*, Dummy*>> query;
  registry.populateComponentQuery(query);
  REQUIRE(query.size() == 1);
  REQUIRE(std::get<0>(query[0]) == 2);
  // just checking the components
  Position* posPtr = std::get<1>(query[0]);
  REQUIRE(posPtr[0].x == Approx(0));
  REQUIRE(posPtr[1].x == Approx(4));

  Dummy* dummyPtr = std::get<2>(query[0]);
  REQUIRE(dummyPtr[0].a == Approx(10));
  REQUIRE(dummyPtr[1].a == Approx(90));

  std::vector<std::tuple<size_t, Position*, Health*, Dummy*>> query2;
  registry.populateComponentQuery(query2);
  REQUIRE(query2.size() == 1);
  REQUIRE(std::get<0>(query2[0]) == 2);

  posPtr = std::get<1>(query2[0]);
  REQUIRE(posPtr[0].x == Approx(0));
  REQUIRE(posPtr[1].x == Approx(4));

  Health* hPtr = std::get<2>(query2[0]);
  REQUIRE(hPtr[0].hp == Approx(100));
  REQUIRE(hPtr[1].hp == Approx(900));

  dummyPtr = std::get<3>(query2[0]);
  REQUIRE(dummyPtr[0].a == Approx(10));
  REQUIRE(dummyPtr[1].a == Approx(90));
}

TEST_CASE("add remove component multiple times", "[core,ecs]") {
  Registry registry;
  Position p{0, 1, 2, 3};
  Health h{100};

  EntityId eid = registry.createEntity(p);
  for (int i = 0; i < 10; ++i) {
    h.hp = static_cast<float>(i);
    registry.addComponent(eid, h);
    registry.removeComponent<Health>(eid);
  }

  registry.addComponent(eid, h);
  Health cmp = registry.getComponent<Health>(eid);
  REQUIRE(cmp.hp == Approx(9));
}

TEST_CASE("add remove component multiple times single component",
          "[core,ecs]") {
  Registry registry;
  Health h{100};

  EntityId eid = registry.createEntity(h);
  for (int i = 0; i < 10; ++i) {
    registry.removeComponent<Health>(eid);
    h.hp = static_cast<float>(i);
    registry.addComponent(eid, h);
  }

  Health cmp = registry.getComponent<Health>(eid);
  REQUIRE(cmp.hp == Approx(9));
}

TEST_CASE("add remove component multiple times single component - 2",
          "[core,ecs]") {
  Registry registry;
  Position p{0, 1, 2, 3};
  Health h{100};

  EntityId eid = registry.createEntity(h, p);
  for (int i = 0; i < 10; ++i) {
    registry.removeComponent<Health>(eid);
    registry.removeComponent<Position>(eid);
    h.hp = static_cast<float>(i);
    p.x = static_cast<float>(i);
    registry.addComponent(eid, h);
    registry.addComponent(eid, p);
  }

  Health cmpH = registry.getComponent<Health>(eid);
  Position cmpP = registry.getComponent<Position>(eid);
  REQUIRE(cmpH.hp == Approx(9));
  REQUIRE(cmpP.x == Approx(9));
}

TEST_CASE("add remove component multiple times single component - 3",
          "[core,ecs]") {
  Registry registry;
  Position p{0, 1, 2, 3};
  Health h{100};
  Dummy d{};

  EntityId eid = registry.createEntity(h, p, d);
  for (int i = 0; i < 10; ++i) {
    registry.removeComponent<Health>(eid);
    registry.removeComponent<Position>(eid);
    registry.removeComponent<Dummy>(eid);
    h.hp = static_cast<float>(i);
    p.x = static_cast<float>(i);
    d.a = i;
    registry.addComponent(eid, h);
    registry.addComponent(eid, p);
    registry.addComponent(eid, d);
  }

  Health cmpH = registry.getComponent<Health>(eid);
  Position cmpP = registry.getComponent<Position>(eid);
  Dummy cmpD = registry.getComponent<Dummy>(eid);
  REQUIRE(cmpH.hp == Approx(9));
  REQUIRE(cmpP.x == Approx(9));
  REQUIRE(cmpD.a == Approx(9));
}

TEST_CASE("delete empty entity", "[core,ecs]") {
  Registry registry;
  Position p{0, 1, 2, 3};

  EntityId eid = registry.createEntity(p);
  registry.removeComponent<Position>(eid);
  registry.deleteEntity(eid);
}
