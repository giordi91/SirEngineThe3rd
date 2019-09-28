#include "SirEngine/memory/resizableVector.h"
#include "SirEngine/memory/threeSizesPool.h"
#include "catch/catch.hpp"

TEST_CASE("Vector reserve size", "[memory]") {

  SirEngine::ResizableVector<float> vec(10);
  REQUIRE(vec.size() == 0);
  REQUIRE(vec.reservedSize() == 10);
}

TEST_CASE("Vector reserve size allocator", "[memory]") {

  SirEngine::ThreeSizesPool pool(200,64,256);
  SirEngine::ResizableVector<float> vec(10, &pool);
  REQUIRE(vec.size() == 0);
  REQUIRE(vec.reservedSize() == 10);
}

TEST_CASE("Vector add elements", "[memory]") {

  SirEngine::ResizableVector<float> vec(10);
  vec.pushBack(1.0f);
  vec.pushBack(2.0f);
  vec.pushBack(3.0f);
  vec.pushBack(4.0f);

  REQUIRE(vec.size() == 4);
  REQUIRE(vec.reservedSize() == 10);
  REQUIRE(vec[0] == 1.0f);
  REQUIRE(vec[1] == 2.0f);
  REQUIRE(vec[2] == 3.0f);
  REQUIRE(vec[3] == 4.0f);
  REQUIRE(vec.getConstRef(0) == 1.0f);
  REQUIRE(vec.getConstRef(1) == 2.0f);
  REQUIRE(vec.getConstRef(2) == 3.0f);
  REQUIRE(vec.getConstRef(3) == 4.0f);
}

TEST_CASE("Vector add elements allocator", "[memory]") {

  SirEngine::ThreeSizesPool pool(200,64,256);
  SirEngine::ResizableVector<float> vec(10, &pool);
  vec.pushBack(1.0f);
  vec.pushBack(2.0f);
  vec.pushBack(3.0f);
  vec.pushBack(4.0f);

  REQUIRE(vec.size() == 4);
  REQUIRE(vec.reservedSize() == 10);
  REQUIRE(vec[0] == 1.0f);
  REQUIRE(vec[1] == 2.0f);
  REQUIRE(vec[2] == 3.0f);
  REQUIRE(vec[3] == 4.0f);
  REQUIRE(vec.getConstRef(0) == 1.0f);
  REQUIRE(vec.getConstRef(1) == 2.0f);
  REQUIRE(vec.getConstRef(2) == 3.0f);
  REQUIRE(vec.getConstRef(3) == 4.0f);
}

TEST_CASE("Vector add elements internal resize", "[memory]") {

  SirEngine::ResizableVector<float> vec(10);
  vec.pushBack(1.0f);
  vec.pushBack(2.0f);
  vec.pushBack(3.0f);
  vec.pushBack(4.0f);
  vec.pushBack(5.0f);
  vec.pushBack(6.0f);
  vec.pushBack(7.0f);

  REQUIRE(vec.size() == 7);
  REQUIRE(vec.reservedSize() == 10);
  REQUIRE(vec[0] == 1.0f);
  REQUIRE(vec[1] == 2.0f);
  REQUIRE(vec[2] == 3.0f);
  REQUIRE(vec[3] == 4.0f);
  REQUIRE(vec[4] == 5.0f);
  REQUIRE(vec[5] == 6.0f);
  REQUIRE(vec[6] == 7.0f);
  REQUIRE(vec.getConstRef(0) == 1.0f);
  REQUIRE(vec.getConstRef(1) == 2.0f);
  REQUIRE(vec.getConstRef(2) == 3.0f);
  REQUIRE(vec.getConstRef(3) == 4.0f);
  REQUIRE(vec.getConstRef(4) == 5.0f);
  REQUIRE(vec.getConstRef(5) == 6.0f);
  REQUIRE(vec.getConstRef(6) == 7.0f);

  // force re-allocating internally
  vec.pushBack(8.0f);
  vec.pushBack(9.0f);
  vec.pushBack(10.0f);
  vec.pushBack(11.0f);
  REQUIRE(vec.size() == 11);
  REQUIRE(vec.reservedSize() == 20);
  REQUIRE(vec[0] == 1.0f);
  REQUIRE(vec[1] == 2.0f);
  REQUIRE(vec[2] == 3.0f);
  REQUIRE(vec[3] == 4.0f);
  REQUIRE(vec[4] == 5.0f);
  REQUIRE(vec[5] == 6.0f);
  REQUIRE(vec[6] == 7.0f);
  REQUIRE(vec[7] == 8.0f);
  REQUIRE(vec[8] == 9.0f);
  REQUIRE(vec[9] == 10.0f);
  REQUIRE(vec[10] == 11.0f);
  REQUIRE(vec.getConstRef(0) == 1.0f);
  REQUIRE(vec.getConstRef(1) == 2.0f);
  REQUIRE(vec.getConstRef(2) == 3.0f);
  REQUIRE(vec.getConstRef(3) == 4.0f);
  REQUIRE(vec.getConstRef(4) == 5.0f);
  REQUIRE(vec.getConstRef(5) == 6.0f);
  REQUIRE(vec.getConstRef(6) == 7.0f);
  REQUIRE(vec.getConstRef(7) == 8.0f);
  REQUIRE(vec.getConstRef(8) == 9.0f);
  REQUIRE(vec.getConstRef(9) == 10.0f);
  REQUIRE(vec.getConstRef(10) == 11.0f);
}

TEST_CASE("Vector add elements internal resize allocator", "[memory]") {

  SirEngine::ThreeSizesPool pool(200,64,256);
  SirEngine::ResizableVector<float> vec(10, &pool);
  vec.pushBack(1.0f);
  vec.pushBack(2.0f);
  vec.pushBack(3.0f);
  vec.pushBack(4.0f);
  vec.pushBack(5.0f);
  vec.pushBack(6.0f);
  vec.pushBack(7.0f);

  REQUIRE(vec.size() == 7);
  REQUIRE(vec.reservedSize() == 10);
  REQUIRE(vec[0] == 1.0f);
  REQUIRE(vec[1] == 2.0f);
  REQUIRE(vec[2] == 3.0f);
  REQUIRE(vec[3] == 4.0f);
  REQUIRE(vec[4] == 5.0f);
  REQUIRE(vec[5] == 6.0f);
  REQUIRE(vec[6] == 7.0f);
  REQUIRE(vec.getConstRef(0) == 1.0f);
  REQUIRE(vec.getConstRef(1) == 2.0f);
  REQUIRE(vec.getConstRef(2) == 3.0f);
  REQUIRE(vec.getConstRef(3) == 4.0f);
  REQUIRE(vec.getConstRef(4) == 5.0f);
  REQUIRE(vec.getConstRef(5) == 6.0f);
  REQUIRE(vec.getConstRef(6) == 7.0f);

  // force re-allocating internally
  vec.pushBack(8.0f);
  vec.pushBack(9.0f);
  vec.pushBack(10.0f);
  vec.pushBack(11.0f);
  REQUIRE(vec.size() == 11);
  REQUIRE(vec.reservedSize() == 20);
  REQUIRE(vec[0] == 1.0f);
  REQUIRE(vec[1] == 2.0f);
  REQUIRE(vec[2] == 3.0f);
  REQUIRE(vec[3] == 4.0f);
  REQUIRE(vec[4] == 5.0f);
  REQUIRE(vec[5] == 6.0f);
  REQUIRE(vec[6] == 7.0f);
  REQUIRE(vec[7] == 8.0f);
  REQUIRE(vec[8] == 9.0f);
  REQUIRE(vec[9] == 10.0f);
  REQUIRE(vec[10] == 11.0f);
  REQUIRE(vec.getConstRef(0) == 1.0f);
  REQUIRE(vec.getConstRef(1) == 2.0f);
  REQUIRE(vec.getConstRef(2) == 3.0f);
  REQUIRE(vec.getConstRef(3) == 4.0f);
  REQUIRE(vec.getConstRef(4) == 5.0f);
  REQUIRE(vec.getConstRef(5) == 6.0f);
  REQUIRE(vec.getConstRef(6) == 7.0f);
  REQUIRE(vec.getConstRef(7) == 8.0f);
  REQUIRE(vec.getConstRef(8) == 9.0f);
  REQUIRE(vec.getConstRef(9) == 10.0f);
  REQUIRE(vec.getConstRef(10) == 11.0f);
}

TEST_CASE("Vector resize", "[memory]") {

  SirEngine::ResizableVector<float> vec(10);
  vec.pushBack(1.0f);
  vec.pushBack(2.0f);
  vec.pushBack(3.0f);
  vec.pushBack(4.0f);
  vec.pushBack(5.0f);
  vec.pushBack(6.0f);
  vec.pushBack(7.0f);

  vec.resize(20);
  REQUIRE(vec[0] == 1.0f);
  REQUIRE(vec[1] == 2.0f);
  REQUIRE(vec[2] == 3.0f);
  REQUIRE(vec[3] == 4.0f);
  REQUIRE(vec[4] == 5.0f);
  REQUIRE(vec[5] == 6.0f);
  REQUIRE(vec[6] == 7.0f);
  REQUIRE(vec.size() == 20);
  REQUIRE(vec.reservedSize() == 40);

  vec.resize(5);
  REQUIRE(vec[0] == 1.0f);
  REQUIRE(vec[1] == 2.0f);
  REQUIRE(vec[2] == 3.0f);
  REQUIRE(vec[3] == 4.0f);
  REQUIRE(vec[4] == 5.0f);
  REQUIRE(vec.size() == 5);
  REQUIRE(vec.reservedSize() == 40);
}

TEST_CASE("Vector resize allocator", "[memory]") {

  SirEngine::ThreeSizesPool pool(400,64,256);
  SirEngine::ResizableVector<float> vec(10, &pool);
  vec.pushBack(1.0f);
  vec.pushBack(2.0f);
  vec.pushBack(3.0f);
  vec.pushBack(4.0f);
  vec.pushBack(5.0f);
  vec.pushBack(6.0f);
  vec.pushBack(7.0f);

  vec.resize(20);
  REQUIRE(vec[0] == 1.0f);
  REQUIRE(vec[1] == 2.0f);
  REQUIRE(vec[2] == 3.0f);
  REQUIRE(vec[3] == 4.0f);
  REQUIRE(vec[4] == 5.0f);
  REQUIRE(vec[5] == 6.0f);
  REQUIRE(vec[6] == 7.0f);
  REQUIRE(vec.size() == 20);
  REQUIRE(vec.reservedSize() == 40);

  vec.resize(5);
  REQUIRE(vec[0] == 1.0f);
  REQUIRE(vec[1] == 2.0f);
  REQUIRE(vec[2] == 3.0f);
  REQUIRE(vec[3] == 4.0f);
  REQUIRE(vec[4] == 5.0f);
  REQUIRE(vec.size() == 5);
  REQUIRE(vec.reservedSize() == 40);
}

TEST_CASE("Vector resize with no initialization", "[memory]"){

  SirEngine::ResizableVector<float> vec;
  REQUIRE(vec.size() == 0);
  REQUIRE(vec.reservedSize() == 0);
  vec.resize(10);
  REQUIRE(vec.size() == 10);
  REQUIRE(vec.reservedSize() == 20);

}

TEST_CASE("Vector resize with no initialization allocator", "[memory]"){

  SirEngine::ThreeSizesPool pool(200);
  SirEngine::ResizableVector<float> vec;
  REQUIRE(vec.size() == 0);
  REQUIRE(vec.reservedSize() == 0);
  vec.resize(10);
  REQUIRE(vec.size() == 10);
  REQUIRE(vec.reservedSize() == 20);

}
TEST_CASE("Vector remove by patching", "[memory]") {

  SirEngine::ThreeSizesPool pool(400,64,256);
  SirEngine::ResizableVector<float> vec(10, &pool);
  vec.pushBack(1.0f);
  vec.pushBack(2.0f);
  vec.pushBack(3.0f);
  vec.pushBack(4.0f);
  vec.pushBack(5.0f);
  vec.pushBack(6.0f);
  vec.pushBack(7.0f);

  REQUIRE(vec.size() == 7);
  float value = vec.removeByPatchingFromLast(4); //1 2 3 4 7 6
  REQUIRE(vec.size() == 6);
  REQUIRE(value == 5.0f);
  REQUIRE(vec[4]== 7.0f);

  value = vec.removeByPatchingFromLast(0);//6 2 3 4 7 
  REQUIRE(vec.size() == 5);
  REQUIRE(value == 1.0f);
  REQUIRE(vec[0]== 6.0f);

  value = vec.removeByPatchingFromLast(1);//6 7 3 4  
  REQUIRE(vec.size() == 4);
  REQUIRE(value == 2.0f);
  REQUIRE(vec[1]== 7.0f);

  value = vec.removeByPatchingFromLast(3);//6 7 3  
  REQUIRE(vec.size() == 3);
  REQUIRE(value == 4.0f);
  REQUIRE(vec[2]== 3.0f);

  value = vec.removeByPatchingFromLast(1);//6 3  
  REQUIRE(vec.size() == 2);
  REQUIRE(value == 7.0f);
  REQUIRE(vec[0]== 6.0f);
  REQUIRE(vec[1]== 3.0f);

  value = vec.removeByPatchingFromLast(1);//6   
  REQUIRE(vec.size() == 1);
  REQUIRE(value == 3.0f);
  REQUIRE(vec[0]== 6.0f);

  value = vec.removeByPatchingFromLast(0);// empty   
  REQUIRE(vec.size() == 0);
  REQUIRE(value == 6.0f);

}
