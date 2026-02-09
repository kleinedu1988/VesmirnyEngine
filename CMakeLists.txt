cmake_minimum_required(VERSION 3.10)
project(InformatickaMrizka)

# Nastavení C++ na moderní standard 20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Agresivní optimalizace (jako u Formule 1)
if(MSVC)
    add_compile_options(/O2 /arch:AVX2)
else()
    add_compile_options(-O3 -march=native)
endif()

# Definice výsledného programu
add_executable(Vesmir main.cpp)