Všechny změny v projektu DIFP Simulation budou dokumentovány v tomto souboru.

Formát je založen na Keep a Changelog, a tento projekt dodržuje(https://semver.org/spec/v2.0.0.html).

[1.0.0] - 2023-10-27
Přidáno

    Architektura Jádra (Core):

        Implementována třída DIFPGrid (v3.1 Monolith).

        Monolitická alokace paměti pro minimalizaci TLB misses.

        Automatické 64-bytové zarovnání pro AVX-512 (zmm registry).

        Padding dat (zarovnání velikosti řádků) pro eliminaci peel-loops.

    Bezpečnost Paměti:

        Plná implementace Rule of Five (kopírovací/přesouvací konstruktory).

        Mechanismus rebind_pointers() zabraňující invalidaci ukazatelů po realokaci.

    Numerické Solvery:

        RK4Solver: Implementace Runge-Kutta 4. řádu.

        Vektorizace smyček pomocí #pragma omp simd a klíčového slova __restrict.

        Interní správa "scratch" bufferů (k1-k4) pro eliminaci alokací v hlavní smyčce.

    Build Systém:

        CMake konfigurace s detekcí -march=native.

Změněno

    Refaktorován původní návrh (Structure of Vectors) na Monolithic Structure of Arrays.