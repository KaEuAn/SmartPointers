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
        if (!this)
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
    UniquePtr(UniquePtr&& other) {
        pointer = other.release();
    }
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
        pointer = nullptr;
        return answer;
    }
    void reset(T_pointer ptr) {
        std::swap(ptr, pointer);
        if (ptr != pointer && ptr)
            delete ptr;
    }



};

template<typename T>
class WeakPtr;


template<typename T>
class SharedPtrLink {
    typedef typename std::remove_reference<T>::type type;
    typedef type* T_pointer;

    T_pointer pointer;
    size_t* count;

public:
    explicit SharedPtrLink(): pointer(nullptr), count(nullptr) {}
    explicit SharedPtrLink(const T_pointer& p) : pointer(p) {
        count = new size_t(1);
    }
    explicit SharedPtrLink(T_pointer&& p) : pointer(std::move(p)) {
        count = new size_t(1);
        p = nullptr;
    }
    SharedPtrLink(const SharedPtrLink& other) : pointer(other.pointer), count(other.count) {
        ++*(count);
    }

    SharedPtrLink(SharedPtrLink&& other) : pointer(std::move(other.pointer)), count(std::move(other.count)) {
        other.pointer = nullptr;
        other.count = nullptr;
    }

    ~SharedPtrLink() {
        remove();
    }
    SharedPtrLink operator= (SharedPtrLink&& other) {
        if (&other == this)
            return this;
        remove();
        count = other.count;
        other.count = nullptr;
        pointer = other.pointer;
        other.pointer = nullptr;
        return *this;
    }
    SharedPtrLink operator= (const SharedPtrLink& other) {
        if (&other == this)
            return this;
        remove();
        count = other.count;
        pointer = other.pointer;
        ++*count;
        return *this;
    }

    SharedPtrLink operator= (const T_pointer& p) {
        remove();
        pointer = new T_pointer(p);
        count = new size_t(1);
    }

    SharedPtrLink operator= (T&& other) {
        remove();
        pointer = std::move(other);
        count = new size_t(1);
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
            count = nullptr;
            pointer = nullptr;
        }
    }

    T_pointer get() const {
        return pointer;
    }

    size_t use_count() const {
        return *count;
    }
    friend class WeakPtr<T>;

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
        link = nullptr;
    }
public:
    SharedPtr() {
        link = new SharedPtrLink<T>();
    }
    explicit SharedPtr(const T_pointer& p) {
        link = new SharedPtrLink<T>(p);
    }
    explicit SharedPtr(T_pointer&& p) {
        link = new SharedPtrLink<T>(std::move(p));
    }

    SharedPtr(const SharedPtr& other) {
        link = new SharedPtrLink<T>(*(other.link));
    }

    SharedPtr(SharedPtr&& other) {
        link = new SharedPtrLink<T>(std::move(*(other.link)));
    }

    explicit SharedPtr(const WeakPtr<T>& other) {
        link = new SharedPtrLink<T>(*(other->link));
    }
    ~SharedPtr() {
        delete link;
    }
    SharedPtr operator= (T&& other) {
        if (&other == this)
            return this;
        link.operator=(std::move(other));
        return *this;
    }
    SharedPtr operator= (const T& other) {
        if (&other == this)
            return this;
        link.operator=(other);
        return *this;
    }

    SharedPtr operator= (const SharedPtr& other) {
        link->operator=(other.link);
        return *this;
    }

    SharedPtr operator= (const SharedPtr&& other) {
        link->operator=(std::move(other.link));
        return *this;
    }

    size_t use_count() const {
        return link->use_count();
    }

    void swap(SharedPtr& other) {
        std::swap(link, other.link);
    }

    void reset(T_pointer p) {
        SharedPtr(p).swap(*this);
    }
    void reset() {
        SharedPtr().swap(*this);
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
    friend class WeakPtr<T>;
};

template<typename T>
class WeakPtr {
    SharedPtrLink<T>* link;

    void remove() {
        if (!this)
            return;
        ++(link->count);
        delete link;
        link = nullptr;
    }

public:
    WeakPtr() : link(nullptr) {}
    WeakPtr(const SharedPtr<T>& x) {
        link->operator=(x.link);
        --*(link->count);
    }
    WeakPtr operator= (const SharedPtrLink<T>& x) {
        remove();
        link->operator=(x.link);
        return *this;
    }
    WeakPtr(const WeakPtr& other) {
        link = new SharedPtrLink<T>(*(other.link));
        --*(link->count);
    }
    WeakPtr(WeakPtr&& other) {
        link = other.link;
        other.link = nullptr;
    }
    WeakPtr& operator= (const WeakPtr& other) {
        if (&other == this)
            return this;
        remove();
        link = new SharedPtrLink<T>(*(other.link));
        --(link->count);
        return *this;
    }
    WeakPtr& operator= (WeakPtr&& other) {
        if (&other == this)
            return this;
        remove();
        link = other.link;
        other.link = nullptr;
    }
    ~WeakPtr() {
        remove();
    }
    size_t use_count() const {
        return link->use_count();
    }
    bool expired() const {
        return link->use_count() == 0;
    }
    SharedPtr<T>& lock() {
        return SharedPtr<T>(*this);
    }

    void swap(T& other) {
        std::swap(link, other.link);
    }
    void reset() {
        WeakPtr().swap(*this);
    }
};