cmake_minimum_required(VERSION 3.20)

# ZDE: Nastavení verze na 1.0.0
project(DIFP_Simulation VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Detekce a povolení AVX-512 optimalizací
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # -march=native: Povolí instrukce dostupné na tomto stroji (včetně AVX-512)
    # -O3: Agresivní optimalizace smyček
    # -Wall -Wextra: Varování pro udržení kvality kódu
    add_compile_options(-march=native -O3 -mprefer-vector-width=512 -Wall -Wextra)
endif()

# Zahrnutí složky include (aby fungovalo #include "solvers/rk4_solver.hpp")
include_directories(include)

# Definice spustitelného souboru
add_executable(difp_sim 
    src/main.cpp 
    src/solvers/rk4_solver.cpp
)