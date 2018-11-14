#include <iostream>
#include <string>
#include "2darray.h"

int main() {
    std::cout << "Starting main()" << std::endl;

    ds::Array2D<int> array2d(10,10);

    std::cout << "Hello World!" << std::endl;

    std::cout << "Ending main()" << std::endl;
    return 0;
}
