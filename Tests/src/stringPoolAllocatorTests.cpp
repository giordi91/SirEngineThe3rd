#include "SirEngine/memory/stringPool.h"
#include "catch/catch.hpp"

TEST_CASE("String pool basic alloc 1 static", "[memory]") {
  SirEngine::StringPool alloc(2 << 16);
  const char *original = "hello world";
  const char *mem = alloc.allocatePersistent(original);
  REQUIRE(strcmp(original, mem) == 0);
}

TEST_CASE("String pool basic alloc 2 static", "[memory]") {
  SirEngine::StringPool alloc(2 << 16);
  const wchar_t *original = L"hello world";
  const wchar_t *mem = alloc.allocatePersistent(original);
  REQUIRE(wcscmp(original, mem) == 0);
}

TEST_CASE("String pool basic alloc 3 static", "[memory]") {
  SirEngine::StringPool alloc(2 << 16);
  const char *original = "hello world";
  const char *original2 =
      "we want to test multiple allocations, also with a possible big "
      "allocation, the problem is i am not sure how long I need to write to "
      "make a big allocation and I am too lazy to do the math so I am just "
      "going to write for a while";
  const char *original3 = "third string, this should be a cool one";
  const char *original4 = "what even a fourth??";
  const char *original5 = "short";
  const char *mem = alloc.allocatePersistent(original);
  const char *mem2 = alloc.allocatePersistent(original2);
  const char *mem3 = alloc.allocatePersistent(original3);
  const char *mem4 = alloc.allocatePersistent(original4);
  const char *mem5 = alloc.allocatePersistent(original5);
  REQUIRE(strcmp(original, mem) == 0);
  REQUIRE(strcmp(original2, mem2) == 0);
  REQUIRE(strcmp(original3, mem3) == 0);
  REQUIRE(strcmp(original4, mem4) == 0);
  REQUIRE(strcmp(original5, mem5) == 0);
}

TEST_CASE("String pool basic alloc 4 static", "[memory]") {
  SirEngine::StringPool alloc(2 << 16);
  const wchar_t *original = L"hello world";
  const wchar_t *original2 =
      L"we want to test multiple allocations, also with a possible big "
      "allocation, the problem is i am not sure how long I need to write to "
      "make a big allocation and I am too lazy to do the math so I am just "
      "going to write for a while";
  const wchar_t *original3 = L"third string, this should be a cool one";
  const wchar_t *original4 = L"what even a fourth??";
  const wchar_t *original5 = L"short";
  const wchar_t *mem = alloc.allocatePersistent(original);
  const wchar_t *mem2 = alloc.allocatePersistent(original2);
  const wchar_t *mem3 = alloc.allocatePersistent(original3);
  const wchar_t *mem4 = alloc.allocatePersistent(original4);
  const wchar_t *mem5 = alloc.allocatePersistent(original5);
  REQUIRE(wcscmp(original, mem) == 0);
  REQUIRE(wcscmp(original2, mem2) == 0);
  REQUIRE(wcscmp(original3, mem3) == 0);
  REQUIRE(wcscmp(original4, mem4) == 0);
  REQUIRE(wcscmp(original5, mem5) == 0);
}

TEST_CASE("String pool basic alloc 1 frame", "[memory]") {
  SirEngine::StringPool alloc(2 << 16);
  const char *original = "hello world";
  const char *mem = alloc.allocateFrame(original);
  REQUIRE(strcmp(original, mem) == 0);
}

TEST_CASE("String pool basic alloc 2 frame", "[memory]") {
  SirEngine::StringPool alloc(2 << 16);
  const wchar_t *original = L"hello world";
  const wchar_t *mem = alloc.allocateFrame(original);
  REQUIRE(wcscmp(original, mem) == 0);
}

TEST_CASE("String pool basic alloc 3 frame", "[memory]") {
  SirEngine::StringPool alloc(2 << 16);
  const char *original = "hello world";
  const char *original2 =
      "we want to test multiple allocations, also with a possible big "
      "allocation, the problem is i am not sure how long I need to write to "
      "make a big allocation and I am too lazy to do the math so I am just "
      "going to write for a while";
  const char *original3 = "third string, this should be a cool one";
  const char *original4 = "what even a fourth??";
  const char *original5 = "short";
  const char *mem = alloc.allocateFrame(original);
  const char *mem2 = alloc.allocateFrame(original2);
  const char *mem3 = alloc.allocateFrame(original3);
  const char *mem4 = alloc.allocateFrame(original4);
  const char *mem5 = alloc.allocateFrame(original5);
  REQUIRE(strcmp(original, mem) == 0);
  REQUIRE(strcmp(original2, mem2) == 0);
  REQUIRE(strcmp(original3, mem3) == 0);
  REQUIRE(strcmp(original4, mem4) == 0);
  REQUIRE(strcmp(original5, mem5) == 0);
}

TEST_CASE("String pool basic alloc 4 frame", "[memory]") {
  SirEngine::StringPool alloc(2 << 16);
  const wchar_t *original = L"hello world";
  const wchar_t *original2 =
      L"we want to test multiple allocations, also with a possible big "
      "allocation, the problem is i am not sure how long I need to write to "
      "make a big allocation and I am too lazy to do the math so I am just "
      "going to write for a while";
  const wchar_t *original3 = L"third string, this should be a cool one";
  const wchar_t *original4 = L"what even a fourth??";
  const wchar_t *original5 = L"short";
  const wchar_t *mem = alloc.allocateFrame(original);
  const wchar_t *mem2 = alloc.allocateFrame(original2);
  const wchar_t *mem3 = alloc.allocateFrame(original3);
  const wchar_t *mem4 = alloc.allocateFrame(original4);
  const wchar_t *mem5 = alloc.allocateFrame(original5);
  REQUIRE(wcscmp(original, mem) == 0);
  REQUIRE(wcscmp(original2, mem2) == 0);
  REQUIRE(wcscmp(original3, mem3) == 0);
  REQUIRE(wcscmp(original4, mem4) == 0);
  REQUIRE(wcscmp(original5, mem5) == 0);
}

TEST_CASE("String pool basic mixed alloc static", "[memory]") {
  SirEngine::StringPool alloc(2 << 16);
  const char *original = "hello world plus something";
  const char *original2 = "hello world";
  const char *original3 = "hello world two";
  const char *original4 = "hello world two two";
  const char *original5 =
      "hello world plus something but much longer than anything else";
  const char *original6 = "short";

  const char *mem = alloc.allocatePersistent(original);
  const char *mem2 = alloc.allocatePersistent(original2);

  alloc.free(mem);
  const char *mem3 = alloc.allocatePersistent(original3);
  // checking allocation should have been reused
  REQUIRE(mem == mem3);
  // checking mem2 has not been overridden or something
  REQUIRE(strcmp(mem2, original2) == 0);

  alloc.free(mem2);
  const char *mem4 = alloc.allocatePersistent(original4);

  // allocation should be too big and mem2 should not be recycled
  REQUIRE(mem4 != mem2);

  const char *mem5 = alloc.allocatePersistent(original5);
  // just doing a big alloc
  REQUIRE(strcmp(original5, mem5) == 0);

  const char *mem6 = alloc.allocatePersistent(original6);
  // mem 6 is short so should reuse mem2
  REQUIRE(mem6 == mem2);
}

TEST_CASE("String pool basic mixed alloc static 2", "[memory]") {
  SirEngine::StringPool alloc(2 << 16);
  const wchar_t *original = L"hello world plus something";
  const wchar_t *original2 = L"hello world";
  const wchar_t *original3 = L"hello world two";
  const wchar_t *original4 = L"hello world two two";
  const wchar_t *original5 =
      L"hello world plus something but much longer than anything else";
  const wchar_t *original6 = L"short";

  const wchar_t *mem = alloc.allocatePersistent(original);
  const wchar_t *mem2 = alloc.allocatePersistent(original2);

  alloc.free(mem);
  const wchar_t *mem3 = alloc.allocatePersistent(original3);
  // checking allocation should have been reused
  REQUIRE(mem == mem3);
  // checking mem2 has not been overridden or something
  REQUIRE(wcscmp(mem2, original2) == 0);

  alloc.free(mem2);
  const wchar_t *mem4 = alloc.allocatePersistent(original4);

  // allocation should be too big and mem2 should not be recycled
  REQUIRE(mem4 != mem2);

  const wchar_t *mem5 = alloc.allocatePersistent(original5);
  // just doing a big alloc
  REQUIRE(wcscmp(original5, mem5) == 0);

  const wchar_t *mem6 = alloc.allocatePersistent(original6);
  // mem 6 is short so should reuse mem2
  REQUIRE(mem6 == mem2);
}

TEST_CASE("String pool basic concatenation 1", "[memory]") {
  SirEngine::StringPool alloc(2 << 16);
  const char *original = "hello";
  const char *original2 = "world";
  const char *original3 = "hell";
  const char *original4 = "o world";
  const char *joiner1 = " ";
  const char *compare = "hello world";
  auto t = strlen(compare);

  // testing with joiner all non in pool
  const char *res1 = alloc.concatenatePersistent(original, original2, joiner1);
  REQUIRE(strcmp(res1, compare) == 0);

  // tesing without joiner all non in pool
  const char *res2 = alloc.concatenatePersistent(original3, original4);
  REQUIRE(strcmp(res2, compare) == 0);

  // now testing with some mixed in pool allocations
  const char *mem1 = alloc.allocatePersistent(original);
  const char *mem2 = alloc.allocatePersistent(original2);
  const char *res3 = alloc.concatenatePersistent(mem1, mem2, joiner1);
  REQUIRE(strcmp(res3, compare) == 0);

#if SE_DEBUG
  REQUIRE(strcmp(mem1, original) == 0);
  REQUIRE(strcmp(mem2, original2) == 0);
#endif

  // testing same as before but request deallocation of first
  const char *res4 = alloc.concatenatePersistent(
      mem1, mem2, joiner1,
      SirEngine::STRING_MANIPULATION_FLAGS::FREE_FIRST_AFTER_OPERATION);
  REQUIRE(strcmp(res4, compare) == 0);
  REQUIRE(strcmp(mem1, original) != 0);

  // realloc mem1
  mem1 = alloc.allocatePersistent(original);

  // freeing both first and second
  const char *res5 = alloc.concatenatePersistent(
      mem1, mem2, joiner1,
      SirEngine::STRING_MANIPULATION_FLAGS::FREE_FIRST_AFTER_OPERATION |
          SirEngine::STRING_MANIPULATION_FLAGS::FREE_SECOND_AFTER_OPERATION);
  REQUIRE(strcmp(res5, compare) == 0);
  REQUIRE(strcmp(mem1, original) != 0);
  REQUIRE(strcmp(mem2, original2) != 0);

  // realloc mem1 and mem2
  mem1 = alloc.allocatePersistent(original);
  mem2 = alloc.allocatePersistent(original2);
  const char *res6 = alloc.concatenatePersistent(
      mem1, mem2, joiner1,
      SirEngine::STRING_MANIPULATION_FLAGS::FREE_FIRST_AFTER_OPERATION |
          SirEngine::STRING_MANIPULATION_FLAGS::FREE_SECOND_AFTER_OPERATION |
          SirEngine::STRING_MANIPULATION_FLAGS::FREE_JOINER_AFTER_OPERATION);
  // nothing should happen since the joiner is not in the pool
  REQUIRE(strcmp(res6, compare) == 0);
  REQUIRE(strcmp(mem1, original) != 0);
  REQUIRE(strcmp(mem2, original2) != 0);

  // alloc and free everything
  mem1 = alloc.allocatePersistent(original);
  mem2 = alloc.allocatePersistent(original2);
  const char *joiner2 = alloc.allocatePersistent(joiner1);
  const char *res7 = alloc.concatenatePersistent(
      mem1, mem2, joiner2,
      SirEngine::STRING_MANIPULATION_FLAGS::FREE_FIRST_AFTER_OPERATION |
          SirEngine::STRING_MANIPULATION_FLAGS::FREE_SECOND_AFTER_OPERATION |
          SirEngine::STRING_MANIPULATION_FLAGS::FREE_JOINER_AFTER_OPERATION);
  // nothing should happen since the joiner is not in the pool
  REQUIRE(strcmp(res7, compare) == 0);
  REQUIRE(strcmp(mem1, original) != 0);
  REQUIRE(strcmp(mem2, original2) != 0);
  REQUIRE(strcmp(joiner2, joiner1) != 0);
}

TEST_CASE("String pool basic concatenation 2", "[memory]") {
  SirEngine::StringPool alloc(2 << 16);
  const wchar_t *original = L"hello";
  const wchar_t *original2 = L"world";
  const wchar_t *original3 = L"hell";
  const wchar_t *original4 = L"o world";
  const wchar_t *joiner1 = L" ";
  const wchar_t *compare = L"hello world";

  // testing with joiner all non in pool
  const wchar_t *res1 =
      alloc.concatenatePersistentWide(original, original2, joiner1);
  REQUIRE(wcscmp(res1, compare) == 0);

  // tesing without joiner all non in pool
  const wchar_t *res2 = alloc.concatenatePersistentWide(original3, original4);
  REQUIRE(wcscmp(res2, compare) == 0);

  // now testing with some mixed in pool allocations
  const wchar_t *mem1 = alloc.allocatePersistent(original);
  const wchar_t *mem2 = alloc.allocatePersistent(original2);
  const wchar_t *res3 = alloc.concatenatePersistentWide(mem1, mem2, joiner1);
  REQUIRE(wcscmp(res3, compare) == 0);

#if SE_DEBUG
  REQUIRE(wcscmp(mem1, original) == 0);
  REQUIRE(wcscmp(mem2, original2) == 0);
#endif

  // testing same as before but request deallocation of first
  const wchar_t *res4 = alloc.concatenatePersistentWide(
      mem1, mem2, joiner1,
      SirEngine::STRING_MANIPULATION_FLAGS::FREE_FIRST_AFTER_OPERATION);
  REQUIRE(wcscmp(res4, compare) == 0);
  REQUIRE(wcscmp(mem1, original) != 0);

  // realloc mem1
  mem1 = alloc.allocatePersistent(original);

  // freeing both first and second
  const wchar_t *res5 = alloc.concatenatePersistentWide(
      mem1, mem2, joiner1,
      SirEngine::STRING_MANIPULATION_FLAGS::FREE_FIRST_AFTER_OPERATION |
          SirEngine::STRING_MANIPULATION_FLAGS::FREE_SECOND_AFTER_OPERATION);
  REQUIRE(wcscmp(res5, compare) == 0);
  REQUIRE(wcscmp(mem1, original) != 0);
  REQUIRE(wcscmp(mem2, original2) != 0);

  // realloc mem1 and mem2
  mem1 = alloc.allocatePersistent(original);
  mem2 = alloc.allocatePersistent(original2);
  const wchar_t *res6 = alloc.concatenatePersistentWide(
      mem1, mem2, joiner1,
      SirEngine::STRING_MANIPULATION_FLAGS::FREE_FIRST_AFTER_OPERATION |
          SirEngine::STRING_MANIPULATION_FLAGS::FREE_SECOND_AFTER_OPERATION |
          SirEngine::STRING_MANIPULATION_FLAGS::FREE_JOINER_AFTER_OPERATION);
  // nothing should happen since the joiner is not in the pool
  REQUIRE(wcscmp(res6, compare) == 0);
  REQUIRE(wcscmp(mem1, original) != 0);
  REQUIRE(wcscmp(mem2, original2) != 0);

  // alloc and free everything
  mem1 = alloc.allocatePersistent(original);
  mem2 = alloc.allocatePersistent(original2);
  const wchar_t *joiner2 = alloc.allocatePersistent(joiner1);
  const wchar_t *res7 = alloc.concatenatePersistentWide(
      mem1, mem2, joiner2,
      SirEngine::STRING_MANIPULATION_FLAGS::FREE_FIRST_AFTER_OPERATION |
          SirEngine::STRING_MANIPULATION_FLAGS::FREE_SECOND_AFTER_OPERATION |
          SirEngine::STRING_MANIPULATION_FLAGS::FREE_JOINER_AFTER_OPERATION);
  // nothing should happen since the joiner is not in the pool
  REQUIRE(wcscmp(res7, compare) == 0);
  REQUIRE(wcscmp(mem1, original) != 0);
  REQUIRE(wcscmp(mem2, original2) != 0);
  REQUIRE(wcscmp(joiner2, joiner1) != 0);
}

TEST_CASE("String pool basic convertion 1", "[memory]") {
  SirEngine::StringPool alloc(2 << 16);
  const wchar_t *original = L"hello";
  const wchar_t *compareWide = L"hello world";
  const char *compareFull = "hello world";
  const char *compare1 = "hello";

  // testing with joiner all non in pool
  const char *res1 = alloc.convert(original);
  REQUIRE(strcmp(res1, compare1) == 0);

  // testing with joiner all non in pool
  const char *res2 = alloc.convert(compareWide);
  REQUIRE(strcmp(res2, compareFull) == 0);

  // lets do it now with allocation and release flag
  // testing with joiner all non in pool
  auto *originalAlloc = alloc.allocatePersistent(compareWide);
  const char *res3 = alloc.convert(
      originalAlloc,
      SirEngine::STRING_MANIPULATION_FLAGS::FREE_FIRST_AFTER_OPERATION);
  REQUIRE(strcmp(res3, compareFull) == 0);
  REQUIRE(wcscmp(originalAlloc, compareWide) != 0);
}

TEST_CASE("String pool basic convertion 2", "[memory]") {
  SirEngine::StringPool alloc(2 << 16);
  const char *original = "shall we convert this to a wide char";
  const wchar_t *compareWide = L"shall we convert this to a wide char";

  // testing with joiner all non in pool
  const wchar_t *res1 = alloc.convertWide(original);
  REQUIRE(wcscmp(res1, compareWide) == 0);

  // lets do it now with allocation and release flag
  // testing with joiner all non in pool
  auto *originalAlloc = alloc.allocatePersistent(original);
  const wchar_t *res2 = alloc.convertWide(
      originalAlloc,
      SirEngine::STRING_MANIPULATION_FLAGS::FREE_FIRST_AFTER_OPERATION);
  REQUIRE(wcscmp(res2, compareWide) == 0);
  REQUIRE(strcmp(originalAlloc, original) != 0);
}


TEST_CASE("String pool basic convertion 3", "[memory]") {
  SirEngine::StringPool alloc(2 << 16);
  const char *hello = "hello";
  const char *world = "world";
  const char *joiner1 = " ";
  const char *result = "hello world";

  const wchar_t *helloW = L"hello";
  const wchar_t *worldW = L"world";
  const wchar_t *joiner1W = L" ";
  const wchar_t *resultW = L"hello world";

  //c c c
  const char *res1 = alloc.concatenatePersistent(hello, world, joiner1);
  REQUIRE(strcmp(result, res1) == 0);

  //w c c
  const char *res2 = alloc.concatenatePersistent(helloW, world, joiner1);
  REQUIRE(strcmp(result, res2) == 0);

  //c w c
  const char *res3 = alloc.concatenatePersistent(hello, worldW, joiner1);
  REQUIRE(strcmp(result, res3) == 0);

  //c c w 
  const char *res4 = alloc.concatenatePersistent(hello, world, joiner1W);
  REQUIRE(strcmp(result, res4) == 0);

  //w w c 
  const char *res5 = alloc.concatenatePersistent(helloW, worldW, joiner1);
  REQUIRE(strcmp(result, res5) == 0);

  //c w w 
  const char *res6 = alloc.concatenatePersistent(hello, worldW, joiner1W);
  REQUIRE(strcmp(result, res6) == 0);

  //w c w 
  const char *res7 = alloc.concatenatePersistent(helloW, world, joiner1W);
  REQUIRE(strcmp(result, res7) == 0);

  //w w w 
  const char *res8 = alloc.concatenatePersistent(helloW, worldW, joiner1W);
  REQUIRE(strcmp(result, res8) == 0);
}

TEST_CASE("String pool basic convertion 4", "[memory]") {
  SirEngine::StringPool alloc(2 << 16);
  const char *hello = "hello";
  const char *world = "world";
  const char *joiner1 = " ";
  const char *result = "hello world";

  const wchar_t *helloW = L"hello";
  const wchar_t *worldW = L"world";
  const wchar_t *joiner1W = L" ";
  const wchar_t *resultW = L"hello world";

  //c c c
  const wchar_t *res1 = alloc.concatenatePersistentWide(hello, world, joiner1);
  REQUIRE(wcscmp(resultW, res1) == 0);

  //w c c
  const wchar_t *res2 = alloc.concatenatePersistentWide(helloW, world, joiner1);
  REQUIRE(wcscmp(resultW, res2) == 0);

  //c w c
  const wchar_t *res3 = alloc.concatenatePersistentWide(hello, worldW, joiner1);
  REQUIRE(wcscmp(resultW, res3) == 0);

  //c c w 
  const wchar_t *res4 = alloc.concatenatePersistentWide(hello, world, joiner1W);
  REQUIRE(wcscmp(resultW, res4) == 0);

  //w w c 
  const wchar_t *res5 = alloc.concatenatePersistentWide(helloW, worldW, joiner1);
  REQUIRE(wcscmp(resultW, res5) == 0);

  //c w w 
  const wchar_t *res6 = alloc.concatenatePersistentWide(hello, worldW, joiner1W);
  REQUIRE(wcscmp(resultW, res6) == 0);

  //w c w 
  const wchar_t *res7 = alloc.concatenatePersistentWide(helloW, world, joiner1W);
  REQUIRE(wcscmp(resultW, res7) == 0);

  //w w w 
  const wchar_t *res8 = alloc.concatenatePersistentWide(helloW, worldW, joiner1W);
  REQUIRE(wcscmp(resultW, res8) == 0);
}

// test allocation in pool but greater that min size allocation

TEST_CASE("String pool frame convertion 1", "[memory]") {
  SirEngine::StringPool alloc(2 << 16);
  const wchar_t *original = L"hello";
  const wchar_t *compareWide = L"hello world";
  const char *compareFull = "hello world";
  const char *compare1 = "hello";

  // testing with joiner all non in pool
  const char *res1 = alloc.convertFrame(original);
  REQUIRE(strcmp(res1, compare1) == 0);

  // testing with joiner all non in pool
  const char *res2 = alloc.convertFrame(compareWide);
  REQUIRE(strcmp(res2, compareFull) == 0);

  // lets do it now with allocation and release flag
  // testing with joiner all non in pool
  auto *originalAlloc = alloc.allocatePersistent(compareWide);
  const char *res3 = alloc.convertFrame(
      originalAlloc);
  REQUIRE(strcmp(res3, compareFull) == 0);
}

TEST_CASE("String pool frame convertion 2", "[memory]") {
  SirEngine::StringPool alloc(2 << 16);
  const char *original = "shall we convert this to a wide char";
  const wchar_t *compareWide = L"shall we convert this to a wide char";

  // testing with joiner all non in pool
  const wchar_t *res1 = alloc.convertFrameWide(original);
  REQUIRE(wcscmp(res1, compareWide) == 0);

  // lets do it now with allocation and release flag
  // testing with joiner all non in pool
  auto *originalAlloc = alloc.allocatePersistent(original);
  const wchar_t *res2 = alloc.convertFrameWide(
      originalAlloc);
  REQUIRE(wcscmp(res2, compareWide) == 0);
}
