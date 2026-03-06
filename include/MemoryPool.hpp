#pragma once
#include <memory>
#include <new>
#include <type_traits>

template <std::size_t BlockSize>
struct Arena
{
	alignas(std::max_align_t) std::byte buffer[BlockSize];
	void* current{ buffer };
	std::size_t space_left{ BlockSize };

	template<typename T, typename... Args>
	T* allocate(Args&&... args)
	{
		static_assert(std::is_trivially_destructible_v<T>, "Arena allocator only supports trivially destructible types.");

		if (std::align(alignof(T), sizeof(T), current, space_left))
		{
			T* obj = new (current) T(std::forward<Args>(args)...);
			current = static_cast<std::byte*>(current) + sizeof(T);
			space_left -= sizeof(T);
			return obj;
		}
		throw std::bad_alloc();
		//return nullptr;
	}

	void reset() noexcept
	{
		current = buffer;
		space_left = BlockSize;
	}
};

template<typename T, std::size_t PoolSize = 1024>
class TypedPoolAllocator
{
	struct Block
	{
		Block* next;
	};

	static constexpr std::size_t BlockSize = std::max(sizeof(T), sizeof(Block));
	static constexpr std::size_t BlockAligment = std::max(alignof(T), alignof(Block));

	alignas(BlockAligment) std::byte buffer[BlockSize * PoolSize];
	Block* head{ nullptr };

public:
	TypedPoolAllocator()
	{
		for (std::size_t i = 0; i < PoolSize; ++i)
		{
			auto* node = new (buffer + i * BlockSize) Block{head};
			head = node;
		}
	}

	template<typename... Args>
	T* allocate(Args&&... args)
	{
		if (!head) throw std::bad_alloc();
		
		Block* free_block = head;
		head = head->next;

		return new (free_block) T(std::forward<Args>(args)...);
	}

	void deallocate(T* ptr)
	{
		ptr->~T();

		Block* node = new (ptr) Block{ head };
		head = node;
	}
};

template<std::size_t BlockSize, std::size_t PoolSize>
class SizeClassPool
{
private:
	static constexpr std::size_t Alignment = std::min(alignof(std::max_align_t), BlockSize);
	union Block
	{
		Block* next;
		alignas(Alignment) std::byte data[BlockSize];
	};

	static_assert(BlockSize >= sizeof(Block*), "BlockSize must fit a 64-bit pointer.");

	Block buffer[PoolSize];
	Block* head{ nullptr };
public:
	SizeClassPool()
	{
		for (std::size_t i = 0; i < PoolSize; ++i)
		{
			auto* node = new (&buffer[i]) Block{ head };
			head = node;
		}
	}

	void* allocate()
	{
		if (!head) throw std::bad_alloc();

		Block* free_block = head;
		head = head->next;

		return free_block->data;
	}

	void deallocate(void* ptr)
	{
		Block* node = new (ptr) Block{ head };
		head = node;
	}
};

template<std::size_t PoolSize>
class MemoryPool
{
private:
	SizeClassPool<8, PoolSize> pool_8;
	SizeClassPool<16, PoolSize> pool_16;
	SizeClassPool<64, PoolSize> pool_64;
	SizeClassPool<256, PoolSize> pool_256;

public:
	void* allocate(std::size_t size)
	{
		if (size <= 8) return pool_8.allocate();
		if (size <= 16) return pool_16.allocate();
		if (size <= 64) return pool_64.allocate();
		if (size <= 256) return pool_256.allocate();
		//else throw std::bad_alloc();

		return ::operator new(size);
	}

	void deallocate(void* ptr, std::size_t size)
	{
		if (size <= 8) pool_8.deallocate(ptr);
		else if (size <= 16) pool_16.deallocate(ptr);
		else if (size <= 64) pool_64.deallocate(ptr);
		else if (size <= 256) pool_256.deallocate(ptr);
		else ::operator delete(ptr);
	}
};