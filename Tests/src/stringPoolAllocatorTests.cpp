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
