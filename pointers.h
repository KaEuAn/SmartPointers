#include <iostream>
#include <memory>
#include <utility>

template<typename T>
class UniquePtr {
    typedef typename std::remove_reference<T>::type type;
    typedef type* T_pointer;

    T_pointer pointer;

    void remove() {
        if (!this)
            return;
        delete pointer;
    }

public:
    explicit UniquePtr() : pointer(nullptr) {}
    explicit UniquePtr(T_pointer&& p) : pointer(std::move(p))  {
        p = nullptr;
    }
    explicit UniquePtr(const T_pointer& p) : pointer(p)  {}
    explicit UniquePtr(UniquePtr&& other) : pointer(std::move(other.pointer)) {
        other.pointer = nullptr;
    }
    ~UniquePtr() {
        remove();
    }
    const UniquePtr operator= (UniquePtr& other) = delete;
    const UniquePtr operator= (UniquePtr&& other) {
        if(this == &other)
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
        T_pointer answer = pointer;
        remove();
        return answer;
    }
    void reset(T_pointer ptr) {
        std::swap(ptr, pointer);
        if (ptr != pointer)
            delete ptr;
    }



};

template<typename T>
class SharedPtrLink {
    typedef typename std::remove_reference<T>::type type;
    typedef type* T_pointer;

    T_pointer pointer;
    uint32_t* count;

public:
    explicit SharedPtrLink(): pointer(nullptr), count(nullptr) {}
    explicit SharedPtrLink(const T_pointer& p) : pointer(p) {
        count = new uint32_t(1);
    }
    explicit SharedPtrLink(T_pointer&& p) : pointer(std::move(p)) {
        count = new uint32_t(1);
        p = nullptr;
    }
    explicit SharedPtrLink(const SharedPtrLink& other) : pointer(other.pointer), count(other.count) {
        ++*(count);
    }
    explicit SharedPtrLink(SharedPtrLink&& other) : pointer(std::move(other.pointer)), count(std::move(other.count)) {
        other.pointer = nullptr;
        other.count = nullptr;
    }
    ~SharedPtrLink() {
        remove();
    }
    SharedPtrLink operator= (SharedPtrLink&& other) {
        remove();
        count = other.count;
        other.count = nullptr;
        pointer = other.pointer;
        other.pointer = nullptr;
        return *this;
    }
    SharedPtrLink operator= (const SharedPtrLink& other) {
        remove();
        count = other.count;
        pointer = other.pointer;
        ++*count;
        return *this;
    }

    void swap(SharedPtrLink& other) {
        std::swap(pointer, other.pointer);
        std::swap(count, other.count);
    }


    void remove() {
        if (!this)
            return;
        --*(count);
        if (count == 0) {
            delete count;
            delete pointer;
        }
    }

    T_pointer get() const {
        return pointer;
    }

    uint32_t use_count() const {
        return *count;
    }

};

template<typename T>
class SharedPtr {
    typedef typename std::remove_reference<T>::type type;
    typedef type* T_pointer;

    SharedPtrLink<T>* link;

    void remove() {
        if (!this)
            return;
        link.remove();
    }
public:
    explicit SharedPtr(const T_pointer& p) {
        link = new SharedPtrLink<T>(p);
    }
    explicit SharedPtr(T_pointer&& p) {
        link = new SharedPtrLink<T>(std::move(p));
    }
    explicit SharedPtr(const SharedPtr& other) {
        link = new SharedPtrLink<T>(*(other.link));
    }
    explicit SharedPtr(SharedPtr&& other) {
        link = new SharedPtrLink<T>(std::move(*(other.link)));
    }
    ~SharedPtr() {
        delete link;
    }
    SharedPtr operator= (T&& other) {
        link.operator=(std::move(other));
        return *this;
    }
    SharedPtr operator= (const T& other) {
        link.operator=(other);
        return *this;
    }

    const uint32_t use_count() const {
        return link->use_count();
    }

    void swap(SharedPtr& other) {
        std::swap(link, other.link);
    }

    void reset(T_pointer p) {
        SharedPtr<T> sh(p);
        swap(sh);
        sh.remove();
    }

    T_pointer get() const {
        return link->get();
    }
    T_pointer operator-> () const {
        return get();
    }
    type& operator*() const {
        return *get();
    }
};

