cmake_minimum_required(VERSION 3.10)
project(CFRPokerEngine)

# Use C++17 standard for modern features
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories so the compiler can find your .h files
include_directories(include)

# Create the executable from your source files
add_executable(cfr_poker 
    src/main.cpp 
    src/game.cpp 
    src/cfr.cpp
    src/mccfr.cpp  # <--- Added our new Monte Carlo engine!
)