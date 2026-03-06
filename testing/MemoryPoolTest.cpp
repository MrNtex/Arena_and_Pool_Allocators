#include <iostream>
#include <cassert>
#include <vector>
#include "MemoryPool.hpp"


static int global_live_objects = 0;
struct TestObject {
    double weights[4];
    TestObject() { global_live_objects++; }
    ~TestObject() { global_live_objects--; }
};

void TypedPoolAllocator_AllocatesObjects() {
    TypedPoolAllocator<TestObject, 5> pool;

    TestObject* op1 = pool.allocate();
    TestObject* op2 = pool.allocate();
    assert(global_live_objects == 2 && "TypedPool failed to call constructor!");

    std::cout << "[SUCCESS] TypedPoolAllocator passed all lifecycle tests.\n";
}

void TypedPoolAllocator_DeallocatesObjects()
{
    TypedPoolAllocator<TestObject, 5> pool;

	TestObject* op1 = pool.allocate();
	TestObject* op2 = pool.allocate();
	pool.deallocate(op1);
	assert(global_live_objects == 1 && "TypedPool failed to call destructor on deallocation!");
	pool.deallocate(op2);
	assert(global_live_objects == 0 && "TypedPool failed to call destructor on deallocation!");

	std::cout << "[SUCCESS] TypedPoolAllocator passed all deallocation tests.\n";
}

void TypedPoolAllocator_HandlesPoolExhaustion() {
	TypedPoolAllocator<TestObject, 2> pool;

	TestObject* op1 = pool.allocate();
	TestObject* op2 = pool.allocate();

    try {
		TestObject* op3 = pool.allocate();
		assert(false && "TypedPool did not throw on pool exhaustion!");
    }
    catch (const std::bad_alloc&) {
		std::cout << "[SUCCESS] TypedPoolAllocator correctly threw on pool exhaustion.\n";
	}
}


int main() {
    
	TypedPoolAllocator_AllocatesObjects();
	TypedPoolAllocator_DeallocatesObjects();
	TypedPoolAllocator_HandlesPoolExhaustion();
    return 0;
}