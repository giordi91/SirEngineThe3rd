#include "SirEngine/memory/stringPool.h"
#include "catch/catch.hpp"

TEST_CASE("String pool basic alloc 1 static", "[memory]") {
  SirEngine::StringPool alloc(2 << 16);
  const char *original = "hello world";
  const char *mem = alloc.allocateStatic(original);
  REQUIRE(strcmp(original, mem) == 0);
}

TEST_CASE("String pool basic alloc 2 static", "[memory]") {
  SirEngine::StringPool alloc(2 << 16);
  const wchar_t *original = L"hello world";
  const wchar_t *mem = alloc.allocateStatic(original);
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
  const char *mem = alloc.allocateStatic(original);
  const char *mem2 = alloc.allocateStatic(original2);
  const char *mem3 = alloc.allocateStatic(original3);
  const char *mem4 = alloc.allocateStatic(original4);
  const char *mem5 = alloc.allocateStatic(original5);
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
  const wchar_t *mem = alloc.allocateStatic(original);
  const wchar_t *mem2 = alloc.allocateStatic(original2);
  const wchar_t *mem3 = alloc.allocateStatic(original3);
  const wchar_t *mem4 = alloc.allocateStatic(original4);
  const wchar_t *mem5 = alloc.allocateStatic(original5);
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

  const char *mem = alloc.allocateStatic(original);
  const char *mem2 = alloc.allocateStatic(original2);

  alloc.free(mem);
  const char *mem3 = alloc.allocateStatic(original3);
  // checking allocation should have been reused
  REQUIRE(mem == mem3);
  // checking mem2 has not been overridden or something
  REQUIRE(strcmp(mem2, original2) == 0);

  alloc.free(mem2);
  const char *mem4 = alloc.allocateStatic(original4);

  // allocation should be too big and mem2 should not be recycled
  REQUIRE(mem4 != mem2);

  const char *mem5 = alloc.allocateStatic(original5);
  // just doing a big alloc
  REQUIRE(strcmp(original5, mem5) == 0);

  const char *mem6 = alloc.allocateStatic(original6);
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

  const wchar_t *mem = alloc.allocateStatic(original);
  const wchar_t *mem2 = alloc.allocateStatic(original2);

  alloc.free(mem);
  const wchar_t *mem3 = alloc.allocateStatic(original3);
  // checking allocation should have been reused
  REQUIRE(mem == mem3);
  // checking mem2 has not been overridden or something
  REQUIRE(wcscmp(mem2, original2) == 0);

  alloc.free(mem2);
  const wchar_t *mem4 = alloc.allocateStatic(original4);

  // allocation should be too big and mem2 should not be recycled
  REQUIRE(mem4 != mem2);

  const wchar_t *mem5 = alloc.allocateStatic(original5);
  // just doing a big alloc
  REQUIRE(wcscmp(original5, mem5) == 0);

  const wchar_t *mem6 = alloc.allocateStatic(original6);
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
  const char *res1 = alloc.concatenateStatic(original, original2, joiner1);
  REQUIRE(strcmp(res1, compare) == 0);

  // tesing without joiner all non in pool
  const char *res2 = alloc.concatenateStatic(original3, original4);
  REQUIRE(strcmp(res2, compare) == 0);

  // now testing with some mixed in pool allocations
  const char *mem1 = alloc.allocateStatic(original);
  const char *mem2 = alloc.allocateStatic(original2);
  int mem1Len = strlen(mem1);
  int mem2Len = strlen(mem2);
  const char *res3 = alloc.concatenateStatic(mem1, mem2, joiner1);
  REQUIRE(strcmp(res3, compare) == 0);

#if SE_DEBUG
  REQUIRE(strcmp(mem1, original) == 0);
  REQUIRE(strcmp(mem2, original2) == 0);
#endif

  // testing same as before but request deallocation of first
  const char *res4 = alloc.concatenateStatic(
      mem1, mem2, joiner1,
      SirEngine::STRING_MANIPULATION_FLAGS::FREE_FIRST_AFTER_OPERATION);
  REQUIRE(strcmp(res4, compare) == 0);
  REQUIRE(strcmp(mem1, original) != 0);

  // realloc mem1
  mem1 = alloc.allocateStatic(original);

  // freeing both first and second
  const char *res5 = alloc.concatenateStatic(
      mem1, mem2, joiner1,
      SirEngine::STRING_MANIPULATION_FLAGS::FREE_FIRST_AFTER_OPERATION |
          SirEngine::STRING_MANIPULATION_FLAGS::FREE_SECOND_AFTER_OPERATION);
  REQUIRE(strcmp(res5, compare) == 0);
  REQUIRE(strcmp(mem1, original) != 0);
  REQUIRE(strcmp(mem2, original2) != 0);

  // realloc mem1 and mem2
  mem1 = alloc.allocateStatic(original);
  mem2 = alloc.allocateStatic(original2);
  const char *res6 = alloc.concatenateStatic(
      mem1, mem2, joiner1,
      SirEngine::STRING_MANIPULATION_FLAGS::FREE_FIRST_AFTER_OPERATION |
          SirEngine::STRING_MANIPULATION_FLAGS::FREE_SECOND_AFTER_OPERATION|
          SirEngine::STRING_MANIPULATION_FLAGS::FREE_JOINER_AFTER_OPERATION);
  //nothing should happen since the joiner is not in the pool
  REQUIRE(strcmp(res6, compare) == 0);
  REQUIRE(strcmp(mem1, original) != 0);
  REQUIRE(strcmp(mem2, original2) != 0);

  // alloc and free everything 
  mem1 = alloc.allocateStatic(original);
  mem2 = alloc.allocateStatic(original2);
  const char* joiner2= alloc.allocateStatic(joiner1);
  const char *res7 = alloc.concatenateStatic(
      mem1, mem2, joiner2,
      SirEngine::STRING_MANIPULATION_FLAGS::FREE_FIRST_AFTER_OPERATION |
          SirEngine::STRING_MANIPULATION_FLAGS::FREE_SECOND_AFTER_OPERATION|
          SirEngine::STRING_MANIPULATION_FLAGS::FREE_JOINER_AFTER_OPERATION);
  //nothing should happen since the joiner is not in the pool
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

  /*
  // testing with joiner all non in pool
  const wchar_t *res1 = alloc.concatenateStatic(original, original2, joiner1);
  REQUIRE(wcscmp(res1, compare) == 0);

  // tesing without joiner all non in pool
  const wchar_t *res2 = alloc.concatenateStatic(original3, original4);
  REQUIRE(wcscmp(res2, compare) == 0);

  // now testing with some mixed in pool allocations
  const wchar_t *mem1 = alloc.allocateStatic(original);
  const wchar_t *mem2 = alloc.allocateStatic(original2);
  int mem1Len = strlen(mem1);
  int mem2Len = strlen(mem2);
  const wchar_t *res3 = alloc.concatenateStatic(mem1, mem2, joiner1);
  REQUIRE(wcscmp(res3, compare) == 0);

#if SE_DEBUG
  REQUIRE(wcscmp(mem1, original) == 0);
  REQUIRE(wcscmp(mem2, original2) == 0);
#endif

  // testing same as before but request deallocation of first
  const wchar_t *res4 = alloc.concatenateStatic(
      mem1, mem2, joiner1,
      SirEngine::STRING_MANIPULATION_FLAGS::FREE_FIRST_AFTER_OPERATION);
  REQUIRE(wcscmp(res4, compare) == 0);
  REQUIRE(wcscmp(mem1, original) != 0);

  // realloc mem1
  mem1 = alloc.allocateStatic(original);

  // freeing both first and second
  const wchar_t *res5 = alloc.concatenateStatic(
      mem1, mem2, joiner1,
      SirEngine::STRING_MANIPULATION_FLAGS::FREE_FIRST_AFTER_OPERATION |
          SirEngine::STRING_MANIPULATION_FLAGS::FREE_SECOND_AFTER_OPERATION);
  REQUIRE(wcscmp(res5, compare) == 0);
  REQUIRE(wcscmp(mem1, original) != 0);
  REQUIRE(wcscmp(mem2, original2) != 0);

  // realloc mem1 and mem2
  mem1 = alloc.allocateStatic(original);
  mem2 = alloc.allocateStatic(original2);
  const wchar_t *res6 = alloc.concatenateStatic(
      mem1, mem2, joiner1,
      SirEngine::STRING_MANIPULATION_FLAGS::FREE_FIRST_AFTER_OPERATION |
          SirEngine::STRING_MANIPULATION_FLAGS::FREE_SECOND_AFTER_OPERATION|
          SirEngine::STRING_MANIPULATION_FLAGS::FREE_JOINER_AFTER_OPERATION);
  //nothing should happen since the joiner is not in the pool
  REQUIRE(wcscmp(res6, compare) == 0);
  REQUIRE(wcscmp(mem1, original) != 0);
  REQUIRE(wcscmp(mem2, original2) != 0);

  // alloc and free everything 
  mem1 = alloc.allocateStatic(original);
  mem2 = alloc.allocateStatic(original2);
  const wchar_t* joiner2= alloc.allocateStatic(joiner1);
  const wchar_t *res7 = alloc.concatenateStatic(
      mem1, mem2, joiner2,
      SirEngine::STRING_MANIPULATION_FLAGS::FREE_FIRST_AFTER_OPERATION |
          SirEngine::STRING_MANIPULATION_FLAGS::FREE_SECOND_AFTER_OPERATION|
          SirEngine::STRING_MANIPULATION_FLAGS::FREE_JOINER_AFTER_OPERATION);
  //nothing should happen since the joiner is not in the pool
  REQUIRE(wcscmp(res7, compare) == 0);
  REQUIRE(wcscmp(mem1, original) != 0);
  REQUIRE(wcscmp(mem2, original2) != 0);
  REQUIRE(wcscmp(joiner2, joiner1) != 0);
  */
}

// test allocation in pool but greater that min size allocation
