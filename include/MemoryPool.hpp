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
