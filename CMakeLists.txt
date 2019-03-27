cmake_minimum_required(VERSION 3.10)
project(playground)

set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 

set(CMAKE_CXX_STANDARD 17)

find_package(websocketpp CONFIG REQUIRED)

#find_package(nlohmann CONFIG REQUIRED)

find_package(Boost 1.45.0 COMPONENTS filesystem regex)

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS}) 
    add_executable(playground main.cpp)
    target_link_libraries(playground ${Boost_LIBRARIES})
endif()

