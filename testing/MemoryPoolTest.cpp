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

struct TrivialObject {
	int x;
	char c;

    TrivialObject(int a, char b) : x(a), c(b) {}
};

void ArenaAllocator_AllocatesObjects() {
    Arena<128> arena;

    auto* p1 = arena.allocate<TrivialObject>(10, '2');

    assert(p1 != nullptr && "Arena failed to allocate!");
    assert(p1->x == 10 && p1->y == 20 && "Arena failed to forward constructor arguments!");

    std::cout << "[SUCCESS] ArenaAllocator passed allocation and forwarding tests.\n";
}

void ArenaAllocator_RespectsAlignment() {
    Arena<128> arena;

    char* c = arena.allocate<char>('A');
    double* d = arena.allocate<double>(3.14159);

    auto d_addr = reinterpret_cast<std::uintptr_t>(d);
    assert(d_addr % alignof(double) == 0 && "Arena failed to align the double to an 8-byte boundary!");
    std::cout << "[SUCCESS] ArenaAllocator passed strict hardware alignment tests.\n";
}


void ArenaAllocator_HandlesExhaustion() {
    Arena<16> arena;
    arena.allocate<int64_t>(1);
    arena.allocate<int64_t>(2);

    try {
        arena.allocate<int64_t>(3);
        assert(false && "Arena did not throw std::bad_alloc on pool exhaustion!");
    }
    catch (const std::bad_alloc&) {
        std::cout << "[SUCCESS] ArenaAllocator correctly threw on pool exhaustion.\n";
    }
}

void ArenaAllocator_ResetsSuccessfully() {
    Arena<16> arena;
    arena.allocate<int64_t>(1);
    arena.allocate<int64_t>(2);

    arena.reset();
    int64_t* p = arena.allocate<int64_t>(99);

    assert(p != nullptr && *p == 99 && "Arena failed to allocate after reset!");
    std::cout << "[SUCCESS] ArenaAllocator passed nuclear reset tests.\n";
}

void ArenaAllocator_MoveSemantics() {
    Arena<128> arena1;
    int* p1 = arena1.allocate<int>(42);

    Arena<128> arena2(std::move(arena1));
    assert(arena1.buffer == nullptr && "Arena1 buffer was not nullified during move construction!");
    int* p2 = arena2.allocate<int>(99);
    assert(*p2 == 99 && "Arena2 failed to allocate after taking ownership!");
    Arena<128> arena3;
    arena3.allocate<int>(1);
    arena3 = std::move(arena2);
    assert(arena2.buffer == nullptr && "Arena2 buffer was not nullified during move assignment!");
    int* p3 = arena3.allocate<int>(100);
    assert(*p3 == 100 && "Arena3 failed to allocate after move assignment!");

    std::cout << "[SUCCESS] ArenaAllocator passed move semantics test.\n";
}

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

void TypedPoolAllocator_MoveSemantics() {
    TypedPoolAllocator<TestObject, 5> pool1;
    TestObject* obj1 = pool1.allocate();
    TypedPoolAllocator<TestObject, 5> pool2(std::move(pool1));
    TestObject* obj2 = pool2.allocate();

    assert(obj2 != nullptr && "TypedPool failed to transfer free-list head during move!");

    pool2.deallocate(obj1);
    pool2.deallocate(obj2);
    std::cout << "[SUCCESS] TypedPoolAllocator passed free-list ownership transfers.\n";
}

void UntypedMemoryPool_AllocatesDifferentSizedElements() {
    MemoryPool<100> pool;

    void* ptr_8 = pool.allocate(4);
    assert(ptr_8 != nullptr && "UntypedPool failed 8-byte routing!");
    void* ptr_64 = pool.allocate(48);
    assert(ptr_64 != nullptr && "UntypedPool failed 64-byte routing!");
    void* ptr_256 = pool.allocate(200);
    assert(ptr_256 != nullptr && "UntypedPool failed 256-byte routing!");

    std::cout << "[SUCCESS] Untyped MemoryPool allocates different sized elements\n";
}

void UntypedMemoryPool_MemoryIsWritableAndAligned() {
    MemoryPool<3> pool;
    void* ptr_64 = pool.allocate(48);

    // If alignment was broken, doing SIMD or writing 64-bit doubles here would crash.
    double* d_ptr = new (ptr_64) double(3.14159);
    assert(*d_ptr == 3.14159 && "UntypedPool returned corrupted memory!");

    std::cout << "[SUCCESS] Untyped MemoryPool returns writable and aligned memory\n";
}

void UntypedMemoryPool_HandlesPoolExhaustion() {
	MemoryPool<1> pool;

	void* ptr1 = pool.allocate(8);
	assert(ptr1 != nullptr && "UntypedPool failed to allocate first block!");

    try {
		void* ptr2 = pool.allocate(8);
		assert(false && "UntypedPool did not throw on pool exhaustion!");
	}
    catch (const std::bad_alloc&) {
		std::cout << "[SUCCESS] Untyped MemoryPool correctly threw on pool exhaustion.\n";
	}
}

void UntypedMemoryPool_FallbackToOSAllocation() {
	MemoryPool<1> pool;

    void* ptr_os = pool.allocate(1024);
    assert(ptr_os != nullptr && "UntypedPool failed OS fallback routing! (nullptr)");

	std::cout << "[SUCCESS] Untyped MemoryPool correctly falls back to OS allocation.\n";
}

void UntypedMemoryPool_DeallocatesMemory() {
    MemoryPool<100> pool;
    void* ptr_8 = pool.allocate(4);
    void* ptr_64 = pool.allocate(48);
    void* ptr_256 = pool.allocate(200);
    void* ptr_os = pool.allocate(1024);

    pool.deallocate(ptr_8, 4);
    pool.deallocate(ptr_64, 48);
    pool.deallocate(ptr_256, 200);
    pool.deallocate(ptr_os, 1024);

    std::cout << "[SUCCESS] Untyped MemoryPool allocates different sized elements\n";
}

void UntypedMemoryPool_MoveSemantics() {
    MemoryPool<100> pool1;
    void* ptr1 = pool1.allocate(24);
    MemoryPool<100> pool2(std::move(pool1));

    void* ptr2 = pool2.allocate(64);

    assert(ptr2 != nullptr && "UntypedPool failed to allocate after move!");
    pool2.deallocate(ptr1, 24);
    pool2.deallocate(ptr2, 64);
    std::cout << "[SUCCESS] Untyped MemoryPool passed composite move semantics.\n";
}


int main() {
    ArenaAllocator_AllocatesObjects();
	ArenaAllocator_RespectsAlignment();
	ArenaAllocator_HandlesExhaustion();
	ArenaAllocator_ResetsSuccessfully();
    ArenaAllocator_MoveSemantics();

	TypedPoolAllocator_AllocatesObjects();
	TypedPoolAllocator_DeallocatesObjects();
	TypedPoolAllocator_HandlesPoolExhaustion();
    TypedPoolAllocator_MoveSemantics();

    UntypedMemoryPool_AllocatesDifferentSizedElements();
    UntypedMemoryPool_MemoryIsWritableAndAligned();
    UntypedMemoryPool_HandlesPoolExhaustion();
    UntypedMemoryPool_FallbackToOSAllocation();
    UntypedMemoryPool_DeallocatesMemory();
    UntypedMemoryPool_MoveSemantics();

    return 0;
}