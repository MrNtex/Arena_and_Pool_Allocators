#pragma once
#include <cstdlib>
#include <memory>
#include <utility>
#include <new>

template<typename T, typename... Args>
T* my_new(Args&&... args) {
    void* raw_memory = std::malloc(sizeof(T));
    if(!raw_memory) throw std::bad_alloc();

    T* ptr = static_cast<T*>(raw_memory);
    try
    {
        std::construct_at(ptr, std::forward<Args>(args)...);
        return ptr;
    } catch (...) {
        std::free(raw_memory);
        throw;
    }
}

template<typename T>
void my_delete(T* ptr) {
    if (!ptr) return;

    std::destroy_at(ptr);
    std::free(ptr);
}

template<typename T>
T* my_new_array(size_t n) {
    void* raw_memory = std::malloc(n*sizeof(T));
    if(!raw_memory) throw std::bad_alloc();

    T* ptr = static_cast<T*>(raw_memory);

    size_t i = 0;
    try 
    {
        for (; i < n; ++i)
        {
            std::construct_at(ptr+i);
        }
        return ptr;
    } catch (...)
    {
        for (size_t j=i; j-- > 0;)
        {
            std::destroy_at(ptr+i);
        }
        std::free(raw_memory);
        throw;
    }
    
}

template<typename T>
void my_delete_array(T* ptr, size_t n) {
    if (!ptr) return;

    for (size_t i = n; i-->0;)
    {
        std::destroy_at(ptr + (i));
    }
    std::free(ptr);
}
