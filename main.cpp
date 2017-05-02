#include "pointers.h"

int main() {
    auto x = std::make_pair<int, int>(7, 8);
    UniquePtr<std::pair<int, int>> my_ptr(&(x));
    auto y = my_ptr.get();
    std::cout << y->first << ' ' << y->second << ' ';
    SharedPtr<int> ma(new int(6));
    std::cout << *(ma.get()) << ' ';
    SharedPtr<int> mx(ma);
    std::cout << mx.use_count() << ' ';
    return 0;
}