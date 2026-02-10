#ifndef DIFP_RK4_SOLVER_HPP
#define DIFP_RK4_SOLVER_HPP

#include "DIFP_Core.hpp"
#include <vector>

class RK4Solver {
private:
    // Dočasné mřížky pro mezikroky RK4 (alokují se jen jednou při resize)
    // k1..k4 ukládají derivace (dx/dt)
    DIFPGrid<double> k1, k2, k3, k4;
    
    // Mřížka pro průběžný stav (state + dt*k)
    DIFPGrid<double> temp_state;

    // Zjistí, zda je potřeba realokovat buffery
    void ensure_buffers(const DIFPGrid<double>& main_grid);

    // Jádro fyzikálního výpočtu: d_out = f(t, state_in)
    // Toto je "stencil" operace, která počítá síly a toky
    void compute_physics_derivatives(const DIFPGrid<double>& state_in, DIFPGrid<double>& d_out);

    // Pomocná metoda pro akumulaci: result = state + scale * k
    void accumulate_step(const DIFPGrid<double>& state, const DIFPGrid<double>& k, 
                         double scale, DIFPGrid<double>& result);

public:
    RK4Solver() : k1(0,0), k2(0,0), k3(0,0), k4(0,0), temp_state(0,0) {}

    // Hlavní metoda, kterou volá smyčka simulace
    void step(DIFPGrid<double>& grid, double dt);
};

#endif // DIFP_RK4_SOLVER_HPP