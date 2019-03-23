#include <iostream>
#include <string>
#include <typeinfo>
#include <vector> 
#include <optional>
#include <functional>

/*
 * Included the JSON for Modern C++ 
 * MIT: https://github.com/nlohmann/json
 */
#include "json.hpp"

//using namespace std;

void processRequest() {


}

std::optional<int> string_to_int(std::string rawValue, int base) {
    std::string::size_type size;
    try {
        return std::stoi(rawValue, &size, base);
    } catch (...) { }
    return { };
}

int main() {
    std::cout << "Starting main()" << std::endl;
    
    std::cout << string_to_int("127", 0).value() << std::endl;

    std::cout << string_to_int("127 North Ave", 0).value() << std::endl;

    std::cout << string_to_int("1111111", 2).value() << std::endl;

    std::cout << "Ending main()" << std::endl;
    return 0;
} 