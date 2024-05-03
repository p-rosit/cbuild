#include <iostream>
#include "max.hpp"
#include "test.hpp"


int main(int argc, char** argv) {
    std::cout << max<int>(5, double_num(3)) << std::endl;
    std::cout << "Hello, world" << std::endl;
}
