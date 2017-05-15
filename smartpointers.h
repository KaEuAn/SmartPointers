#include <iostream>
#include <memory>
#include <utility>
#include <type_traits>


template<typename T>
class UniquePtr {
    typedef typename std::remove_reference<T>::type type;
    typedef type* T_pointer;

    T_pointer pointer;

    void remove() {
        if (!pointer)
            return;
        delete pointer;
        pointer = nullptr;
    }

public:
    explicit UniquePtr() : pointer(nullptr) {}
    explicit UniquePtr(T_pointer&& p) : pointer(std::move(p))  {
        p = nullptr;
    }
    UniquePtr(UniquePtr& other) = delete;
    explicit UniquePtr(const T_pointer& p) : pointer(p)  {}
    UniquePtr(UniquePtr&& other) : pointer(other.release()) {}
    ~UniquePtr() {
        remove();
    }
    UniquePtr& operator= (UniquePtr& other) = delete;
    UniquePtr& operator= (UniquePtr&& other) {
        if (this == &other)
            return *this;
        reset(other.release());
        return *this;
    }


    T_pointer operator-> () const {
        return get();
    }
    type& operator*() const {
        return *get();
    }
    T_pointer get() const {
        return pointer;
    }


    void swap (UniquePtr& other) {
        std::swap(pointer, other.pointer);
    }

    T_pointer release() {
        if (!pointer)
            return nullptr;
        T_pointer answer = new T(*pointer);
        delete pointer;
        pointer = nullptr;
        return answer;
    }
    void reset(T_pointer ptr) {
        std::swap(ptr, pointer);
        if (ptr != pointer)
            delete ptr;
    }



};

template<typename T>
class WeakPtr;


template<typename T>
class SharedPtr {
    typedef typename std::remove_reference<T>::type type;
    typedef type* T_pointer;

    T_pointer pointer;
    size_t* count;

public:
    SharedPtr(const WeakPtr<T>& other) : pointer(other.pointer), count(other.count) {
        ++(*count);
    }

    SharedPtr(): pointer(nullptr), count(nullptr) {}

    explicit SharedPtr(const T_pointer& p) : pointer(p) {
        count = new size_t(1);
    }
    explicit SharedPtr(T_pointer&& p) : pointer(std::move(p)) {
        count = new size_t(1);
        p = nullptr;
    }

    SharedPtr(const SharedPtr& other) : pointer(other.pointer), count(other.count) {
        ++*(count);
    }
    SharedPtr(SharedPtr&& other) : pointer(std::move(other.pointer)), count(std::move(other.count)) {
        other.pointer = nullptr;
        other.count = nullptr;
    }

    ~SharedPtr() {
        remove();
    }

    SharedPtr operator= (SharedPtr&& other) {
        if (&other == this)
            return *this;
        remove();
        count = other.count;
        other.count = nullptr;
        pointer = other.pointer;
        other.pointer = nullptr;
        return *this;
    }
    SharedPtr operator= (const SharedPtr& other) {
        if (&other == this)
            return *this;
        remove();
        count = other.count;
        pointer = other.pointer;
        if (count)
            ++*count;
        return *this;
    }

    SharedPtr operator= (const T_pointer& p) {
        remove();
        pointer = new T_pointer(p);
        count = new size_t(1);
    }
    SharedPtr operator= (T&& other) {
        remove();
        pointer = std::move(other);
        count = new size_t(1);
    }

    void swap(SharedPtr& other) {
        std::swap(pointer, other.pointer);
        std::swap(count, other.count);
    }

    void remove() {
        if (!count)
            return;
        --*(count);
        if (*(count) == 0) {
            delete count;
            delete pointer;
            count = nullptr;
            pointer = nullptr;
        }
    }
    void reset() {
        SharedPtr().swap(*this);
    }
    void reset(T_pointer p) {
        SharedPtr(p).swap(*this);
    }

    T_pointer get() const {
        return pointer;
    }
    T_pointer operator-> () const {
        return get();
    }
    type& operator*() const {
        return *get();
    }

    size_t use_count() const {
        return *count;
    }
    friend class WeakPtr<T>;

};


template<typename T>
class WeakPtr {
    typedef typename std::remove_reference<T>::type type;
    typedef type* T_pointer;

    T_pointer pointer;
    size_t* count;

    void remove() {
        if (!this)
            return;
        if (*count == 0 and !pointer) {
            delete pointer;
            delete count;
            count = nullptr;
            pointer = nullptr;
        }
    }

public:
    template<typename _T>
    friend class SharedPtr;

    WeakPtr() : pointer(nullptr), count(nullptr) {}
    WeakPtr(const SharedPtr<T>& other) : pointer(other.pointer), count(other.count) {}
    WeakPtr operator= (const SharedPtr<T>& other) {
        remove();
        pointer = other.pointer;
        count = other.count;
        return *this;
    }
    WeakPtr(const WeakPtr& other) : pointer(other.pointer), count(other.count) {}
    WeakPtr(WeakPtr&& other) : pointer(std::move(other.pointer)), count(std::move(other.count)) {
        other.count = nullptr;
        other.pointer = nullptr;
    }
    WeakPtr& operator= (const WeakPtr& other) {
        if (&other == this)
            return this;
        remove();
        pointer = other.pointer;
        count = other.count;
        return *this;
    }
    WeakPtr& operator= (WeakPtr&& other) {
        if (&other == this)
            return *this;
        remove();
        pointer = std::move(other.pointer);
        count = std::move(other.count);
        other.count = nullptr;
        other.pointer = nullptr;
        return *this;
    }
    ~WeakPtr() {
        remove();
    }
    size_t use_count() const {
        if (!count)
            return 0;
        return *count;
    }
    bool expired() const {
        return *count == 0;
    }
    SharedPtr<T> lock() const {
        return SharedPtr<T>(*this);
    }

    void swap(T& other) {
        std::swap(pointer, other);
    }
    void reset() {
        WeakPtr().swap(*this);
    }
};