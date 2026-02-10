#include "../include/DIFP_Core.hpp"
#include "rk4_solver.hpp"
#include <omp.h> // Pro #pragma omp simd
#include <cmath>

// Inicializace bufferů, pokud se změnila velikost simulace
void RK4Solver::ensure_buffers(const DIFPGrid<double>& grid) {
    if (k1.active_size!= grid.active_size) {
        // Využíváme move sémantiku pro efektivní realokaci
        k1 = DIFPGrid<double>(grid.width, grid.height);
        k2 = DIFPGrid<double>(grid.width, grid.height);
        k3 = DIFPGrid<double>(grid.width, grid.height);
        k4 = DIFPGrid<double>(grid.width, grid.height);
        temp_state = DIFPGrid<double>(grid.width, grid.height);
    }
}

// Fyzikální jádro (Kernel)
// Příklad: Jednoduchá vlnová rovnice s tlumením
void RK4Solver::compute_physics_derivatives(const DIFPGrid<double>& in, DIFPGrid<double>& out) {
    size_t N = in.get_compute_size(); // Zarovnaná velikost pro AVX

    // Načtení pointerů pro kompilátor (zaručujeme, že se nepřekrývají)
    const double* __restrict pot = in.potential;
    const double* __restrict vx  = in.vx;
    const double* __restrict vy  = in.vy;
    const double* __restrict mass = in.mass;
    const double* __restrict fric = in.friction;

    double* __restrict d_pot = out.potential;
    double* __restrict d_vx  = out.vx;
    double* __restrict d_vy  = out.vy;

    // Explicitní vektorizace smyčky
    // aligned: Říkáme kompilátoru, že všechny pointery začínají na 64-byte hranici
    #pragma omp simd aligned(pot, vx, vy, mass, fric, d_pot, d_vx, d_vy : 64)
    for (size_t i = 0; i < N; ++i) {
        // 1. Změna potenciálu (např. div(v))
        // Poznámka: Pro skutečnou derivaci (gradient) by zde byl přístup k sousedům (i-1, i+1).
        // Pro demonstraci vektorizace děláme lokální operaci.
        d_pot[i] = -(vx[i] + vy[i]); 

        // 2. Změna hybnosti (Newtonův zákon: F = ma -> a = F/m)
        // Síla je gradient potenciálu (zde zjednodušeno) - tření
        double force_x = -pot[i]; 
        double force_y = -pot[i];

        d_vx[i] = (force_x / mass[i]) - (fric[i] * vx[i]);
        d_vy[i] = (force_y / mass[i]) - (fric[i] * vy[i]);
    }
}

// Pomocná funkce pro Eulerův krok uvnitř RK4
void RK4Solver::accumulate_step(const DIFPGrid<double>& state, const DIFPGrid<double>& k, 
                                double scale, DIFPGrid<double>& result) {
    size_t N = state.get_compute_size();
    
    // Získáme ukazatele na raw data
    // Poznámka: V reálném kódu by se toto dalo napsat obecněji přes pole pointerů
    const double* __restrict s_pot = state.potential;
    const double* __restrict k_pot = k.potential;
    double* __restrict r_pot = result.potential;
    
    const double* __restrict s_vx = state.vx;
    const double* __restrict k_vx = k.vx;
    double* __restrict r_vx = result.vx;

    //... a tak dále pro všechna pole...

    #pragma omp simd aligned(s_pot, k_pot, r_pot, s_vx, k_vx, r_vx : 64)
    for (size_t i = 0; i < N; ++i) {
        r_pot[i] = s_pot[i] + scale * k_pot[i];
        r_vx[i]  = s_vx[i]  + scale * k_vx[i];
        //...
    }
}

// Hlavní krok RK4
void RK4Solver::step(DIFPGrid<double>& grid, double dt) {
    ensure_buffers(grid);

    // K1 = f(t, y)
    compute_physics_derivatives(grid, k1);

    // K2 = f(t + dt/2, y + dt/2 * k1)
    accumulate_step(grid, k1, dt * 0.5, temp_state); // temp = y + k1*dt/2
    compute_physics_derivatives(temp_state, k2);

    // K3 = f(t + dt/2, y + dt/2 * k2)
    accumulate_step(grid, k2, dt * 0.5, temp_state); // temp = y + k2*dt/2
    compute_physics_derivatives(temp_state, k3);

    // K4 = f(t + dt, y + dt * k3)
    accumulate_step(grid, k3, dt, temp_state);       // temp = y + k3*dt
    compute_physics_derivatives(temp_state, k4);

    // Finální integrace: y = y + (dt/6) * (k1 + 2*k2 + 2*k3 + k4)
    size_t N = grid.get_compute_size();
    double* __restrict pot = grid.potential;
    double* __restrict vx  = grid.vx;
    //... pointery pro k1..k4...
    
    double dt_6 = dt / 6.0;

    // Finální smyčka - kompilátor zde vygeneruje FMA instrukce (Fused Multiply-Add)
    #pragma omp simd aligned(pot, vx : 64) 
    for (size_t i = 0; i < N; ++i) {
        // Přímý přístup do pre-alokovaných mřížek k1..k4
        pot[i] += dt_6 * (k1.potential[i] + 2*k2.potential[i] + 2*k3.potential[i] + k4.potential[i]);
        vx[i]  += dt_6 * (k1.vx[i]        + 2*k2.vx[i]        + 2*k3.vx[i]        + k4.vx[i]);
        //... analogicky pro ostatní pole
    }
}