// include/task.hpp
#pragma once
#include <cstddef>
#include <type_traits>
#include <new>
#include <utility>
#include <cstdlib>

namespace cortex 
{

class Task 
{
    static constexpr size_t BUFFER_SIZE = 128;
    alignas(64) std::byte buffer_[BUFFER_SIZE];
    void* ptr_ = nullptr;
    void (*invoke_)(void*) = nullptr;
    void (*destroy_)(void*) = nullptr;
    void (*move_)(void*, void*) = nullptr;

public:
    Task() = default;

    template<typename F>
    explicit Task(F&& f) 
    {
        using T = std::decay_t<F>;
        
        if constexpr (sizeof(T) <= BUFFER_SIZE) 
        {
            new (buffer_) T(std::forward<F>(f));
            ptr_ = buffer_;
            destroy_ = [](void* self) { 
                static_cast<T*>(self)->~T(); 
            };
            move_ = [](void* dst, void* src) 
            {
                new (dst) T(std::move(*static_cast<T*>(src)));
                static_cast<T*>(src)->~T();
            };
            } 
        else 
        {
            ptr_ = new T(std::forward<F>(f));
            destroy_ = [](void* self) {
                static_cast<T*>(self)->~T();
                ::operator delete(self);
        };
            move_ = nullptr;
        }

        invoke_ = [](void* self) 
        {
             (*static_cast<T*>(self))(); 
        };
    }

    ~Task() 
    {
        if (ptr_) 
            destroy_(ptr_);
    }

    Task(Task&& other) noexcept 
        : invoke_(other.invoke_), destroy_(other.destroy_), move_(other.move_) 
        {
        
            if (other.ptr_ == nullptr)
             return;

            if (other.ptr_ == other.buffer_) 
            {
                move_(buffer_, other.buffer_);
                ptr_ = buffer_;
            } 
            else 
            {
                ptr_ = other.ptr_;
            }

        other.ptr_ = nullptr;
        other.invoke_ = nullptr;
        other.destroy_ = nullptr;
        other.move_ = nullptr;
    }

    Task& operator=(Task&& other) noexcept 
    {
        if (this != &other) 
        {
            this->~Task();
            new (this) Task(std::move(other));
        }
        return *this;
    }

    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    void operator()() const 
    {
        if (ptr_) invoke_(ptr_);
    }

    explicit operator bool() const 
    { 
        
        return ptr_ != nullptr; 
    }
};

} 