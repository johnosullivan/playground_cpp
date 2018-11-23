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

    ds::Array2D<int> array2d(10,10);

    std::cout << "Hello World!" << std::endl;


    Test test;

    test.geekname = "s";

    test.printname();


    test.~Test();

    std::cout << "Ending main()" << std::endl;

    int  var = 20;   // actual variable declaration.
   int  *ip;        // pointer variable 

   ip = &var;       // store address of var in pointer variable

   std::cout << "Value of var variable: ";
   std::cout << var << std::endl;

   // print the address stored in ip pointer variable
   std::cout << "Address stored in ip variable: ";
   std::cout << ip << std::endl;

   *ip = 40;

   // access the value at the address available in pointer
   std::cout << "Value of *ip variable: ";
   std::cout << *ip << std::endl;

   int x = 25;

   int *p = &x;

   std::cout << *p << std::endl;

   //std::cout << typeid(*p).name() << std::endl;

   std::vector<int> vect; 

   vect.push_back(1);
   vect.push_back(2);
   vect.push_back(3);
   vect.push_back(4);

   std::cout << vect.size() << std::endl;
   std::cout << vect.max_size() << std::endl;

   return 0;
} 
