#include <iostream>
#include <string>
#include "2darray.h"
#include <typeinfo>
#include <vector> 

class Test
{
    // Access specifier
    public:

    // Data Members
    std::string geekname;

    // Member Functions()
    void printname()
    {
       std::cout << "Test is: " << geekname << std::endl;
    }

    Test() {
        std::cout << "Test Constructor" << std::endl;
    }

    ~Test() {
        std::cout << "Test Deonstructor" << std::endl;
    }

    private:

    // Data Members
    std::string name;
};

int main() {
   std::cout << "Starting main()" << std::endl;

    

   return 0;
} 
