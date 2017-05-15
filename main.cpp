#include <iostream>
#include <memory>
#include <vector>
#include <algorithm>
#include <cassert>

#include "smartpointers.h"



void test_unique_ptr() {
    {
        for (int i = 0; i < 1'000'000; ++i) {
            auto u = UniquePtr<long long>(new long long(5));
            auto uu = UniquePtr<long long>(new long long(10));
            u = std::move(uu);
            assert(*u == 10);
            assert(uu.get() == nullptr);
            auto uuu = std::move(uu);
            uuu = std::move(u);
        }
    }

    {
        for (int k = 0; k < 10; ++k) {
            std::vector<UniquePtr<int>> v;
            for (int i = 0; i < 100'000; ++i) {
                v.push_back(UniquePtr<int>(new int(i)));
            }
            std::reverse(v.begin(), v.end());
            assert(*v[20'000] == 79'999);
        }
    }

    auto p = UniquePtr<std::string>(new std::string("1234567890"));
    assert(p->length() == 10);
}

void test_shared_ptr() {
    using std::vector;

    auto first_ptr = SharedPtr<vector<int>>(new vector<int>(1'000'000));

    (*first_ptr)[0] = 1;

    vector<int>& vec = *first_ptr;
    auto second_ptr = SharedPtr<vector<int>>(new vector<int>(vec));

    (*second_ptr)[0] = 2;

    for (int i = 0; i < 1'000'000; ++i)
        first_ptr.swap(second_ptr);
    first_ptr->swap(*second_ptr);

    assert(first_ptr->front() == 2);
    assert(second_ptr->front() == 1);

    assert(first_ptr.use_count() == 1);
    assert(second_ptr.use_count() == 1);

    for (int i = 0; i < 10; ++i) {
        auto third_ptr = SharedPtr<vector<int>>(new vector<int>(vec));
        auto fourth_ptr = second_ptr;
        fourth_ptr.swap(third_ptr);
        assert(second_ptr.use_count() == 2);
    }

    assert(second_ptr.use_count() == 1);

    {
        vector<SharedPtr<vector<int>>> ptrs(10, SharedPtr<vector<int>>(first_ptr));
        for (int i = 0; i < 100'000; ++i) {
            ptrs.push_back(ptrs.back());
            ptrs.push_back(SharedPtr<vector<int>>(ptrs.back()));
        }
        assert(first_ptr.use_count() == 1 + 10 + 200'000);
    }

    first_ptr.reset(new vector<int>());
    second_ptr.reset();
    SharedPtr<vector<int>>().swap(first_ptr);

    assert(second_ptr.get() == nullptr);
    assert(second_ptr.get() == nullptr);

    for (int k = 0; k < 2; ++k) {
        vector<SharedPtr<int>> ptrs;
        for (int i = 0; i < 100'000; ++i) {
            int* p = new int(rand() % 99'999);
            ptrs.push_back(SharedPtr<int>(p));
        }
        std::sort(ptrs.begin(), ptrs.end(), [](auto&& x, auto&& y){return *x < *y;});
        for (int i = 0; i + 1 < 100'000; ++i) {
            assert(*(ptrs[i]) <= *(ptrs[i+1]));
        }
        while (!ptrs.empty()) {
            ptrs.pop_back();
        }
    }

}

template<typename T>
using SharedPtrForWeak = std::conditional_t<
        std::is_same<WeakPtr<T>, std::weak_ptr<T>>::value,
        std::shared_ptr<T>, SharedPtr<T>>;

struct Node;

/*union*/
struct Next {
    //bool isLast = false;
    SharedPtrForWeak<Node> shared;
    WeakPtr<Node> weak;
    Next(const SharedPtrForWeak<Node>& shared): shared(shared) {}
    Next(const WeakPtr<Node>& weak): weak(weak) {}
    /*~Next() {
        if (!isLast) {
            shared.~SharedPtr<Node>();
        } else {
            weak.~WeakPtr<Node>();
        }
    }*/
};

struct Node {
    int value;
    Next next;
    Node(int value): value(value), next(SharedPtrForWeak<Node>()) {}
    Node(int value, const SharedPtrForWeak<Node>& next): value(value), next(next) {}
    Node(int value, const WeakPtr<Node>& next): value(value), next(next) {}
    ~Node() {}
};

SharedPtrForWeak<Node> getCyclePtr(int cycleSize) {
    SharedPtrForWeak<Node> head(new Node(0));
    SharedPtrForWeak<Node> prev(head);
    for (int i = 1; i < cycleSize; ++i) {
        SharedPtrForWeak<Node> current(new Node(i));
        prev->next.shared = current;
        prev = current;
        // std::cout << prev.use_count() << '\n';
    }
    //prev->next.shared.~SharedPtr<Node>();
    //new (&prev->next.weak) WeakPtr<Node>(head);
    prev->next.weak = head;
    //prev->next.isLast = true;
    return head;
}

void test_weak_ptr() {
    auto sp = SharedPtrForWeak<int>(new int(23));
    WeakPtr<int> weak = sp;
    {
        auto shared = SharedPtrForWeak<int>(new int(42));
        weak = shared;
        assert(weak.use_count() == 1);
        assert(!weak.expired());
    }
    assert(weak.use_count() == 0);
    assert(weak.expired());

    weak = sp;
    auto wp = weak;
    assert(weak.use_count() == 1);
    assert(wp.use_count() == 1);
    auto wwp = std::move(weak);
    assert(weak.use_count() == 0);
    assert(wwp.use_count() == 1);

    auto ssp = wwp.lock();
    assert(sp.use_count() == 2);

    for (int i = 0; i < 1'000'000; ++i) {
        SharedPtrForWeak<Node> head = getCyclePtr(8);
        SharedPtrForWeak<Node> nextHead = head->next.shared;
        assert(nextHead.use_count() == 2);
        head.reset();
        assert(nextHead.use_count() == 1);
    }
}

int main() {
    int x;
    std::cin >> x;
    int y;
    switch (x) {
        case 0:
            test_unique_ptr();
            break;
        case 1:
            test_shared_ptr();
            break;
        case 2:
            test_weak_ptr();
            break;
    }
    std::cout << 0;
}