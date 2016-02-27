#ifndef STGEN_POOL_ALLOC_H
#define STGEN_POOL_ALLOC_H

#include <utility>
#include <stdexcept>
#include <new>
#include <memory>
#include <stdlib.h>
#include <cstdint>
#include <cstring>
#include <cassert>

/*
 * XXX NOT THREAD-SAFE
 * FIXME needs further validation
 *
 * Based on:
 * 'Fast Efficient Fixed-Size Memory Pool'
 * Ben Kenwright, Newcastle University
 * Newcastle, UK
 */

#define POOL_MAX_BLOCKS (UINT32_MAX-1)
#define POOL_INIT_BLOCKS (512) //fudge number

template <typename T>
class BlockPoolAllocator
{
public:
	using value_type = T;
	using pointer = T*;
	using const_pointer = const T*;
	using reference = T&;
	using const_reference = const T&;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;

	template <typename U>
	struct rebind { using other = BlockPoolAllocator<U>; };

	BlockPoolAllocator()
	{
		if (sizeof(value_type) < sizeof(uint32_t))
			throw std::length_error("BlockPoolAllocator only valid for sizeof(T) > sizeof(uint32_t)");

		pool = static_cast<pointer>(malloc(POOL_INIT_BLOCKS*sizeof(value_type)));
		num_free_blocks = POOL_INIT_BLOCKS;
		total_blocks = POOL_INIT_BLOCKS;
		num_initialized = 0;
		head = 0;

		if (pool == nullptr)
			throw std::runtime_error("Error initializing BlockPoolAllocator");
	}

	~BlockPoolAllocator() { free(pool); }

	/* XXX not expecting these to be used */
	BlockPoolAllocator(const BlockPoolAllocator& other) { this = other; }
	BlockPoolAllocator& operator=(const BlockPoolAllocator&) = delete;

	// obtains the address of an object
	pointer address(reference r) const { return &r; };
	const_pointer address(const_reference r) const { return &r; };

	/* BlockPoolAllocators have state, and cannot be compared */
	bool operator!=(const BlockPoolAllocator& other) { return !(*this == other); }
	bool operator==(const BlockPoolAllocator&) { return false; }


	size_type max_size() const noexcept  
	{
		return (static_cast<size_t>(0) - static_cast<size_t>(1)) / sizeof(BlockPoolAllocator);
	}

	pointer allocate(size_type n, std::allocator<void>::const_pointer = 0)
		throw(std::bad_alloc, std::length_error)
	{
		if (n == 0) return nullptr;

		if (n > 1) throw std::length_error("Cannot alloc more than one block");

		if (0 == num_free_blocks)
			resize();
		
		uint32_t pool_idx = head;
	
		if (head < num_initialized)
		{
			head = *reinterpret_cast<uint32_t*>(pool + pool_idx);
		}
		else 
		{
			++head;
			++num_initialized;
		}
	
		--num_free_blocks;
		return pool + pool_idx;
	}

	void deallocate(pointer p, size_type n)
	{
		if (p == nullptr) return;

		if (n > 1) throw std::length_error("Cannot dealloc more than one block");

		assert(p < pool + total_blocks);

		*reinterpret_cast<uint32_t*>(p) = head;
		head = (p-pool);
		++num_free_blocks;
	}

	template <class U, class... Args>
	void construct(U* p, Args&&... args) 
	{
	    void* const cvp = static_cast<void*>(p);
	
	    ::new(cvp) U(std::forward<Args>(args)...);
	};
	
	template <class U>
	void destroy(U* p) 
	{ 
	    p->~U(); 
	};

private:
	void resize()
	{
		uint32_t prev_num_blocks = total_blocks;
		uint32_t new_num_blocks = 2*prev_num_blocks;

		if ( POOL_MAX_BLOCKS < new_num_blocks )
			throw std::runtime_error("Error expanding block pool");

		pool = static_cast<pointer>(realloc(pool, new_num_blocks*sizeof(value_type)));

		if (pool == nullptr)
			throw std::runtime_error("Error expanding block pool");

		num_free_blocks = new_num_blocks - prev_num_blocks;
		total_blocks = new_num_blocks;
	}

	uint32_t head;
	uint32_t num_free_blocks;
	uint32_t num_initialized;
	uint32_t total_blocks;
	pointer pool;
};

#endif
