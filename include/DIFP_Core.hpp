/**
 * @file DIFP_Core.hpp
 * @brief Jádro monolitické mřížkové architektury pro simulační rámec DIFP.
 * @version 3.1 (Research Grade)
 * @details Implementuje 64-bytově zarovnaný kontejner Structure of Arrays (SoA)
 *          optimalizovaný pro AVX-512 a efektivitu TLB. Obsahuje plnou implementaci
 *          Rule of Five pro prevenci invalidace interních ukazatelů.
 */

#ifndef DIFP_CORE_V3_HPP
#define DIFP_CORE_V3_HPP

#include <vector>
#include <cstdint>
#include <cmath>
#include <memory>
#include <immintrin.h>
#include <stdexcept>
#include <cstring>
#include <algorithm> // pro std::copy, std::fill
#include <utility>   // pro std::move

// AVX-512 vyžaduje zarovnání na 64 bytů pro optimální výkon (zmm registry)
constexpr size_t AVX_WIDTH_BYTES = 64;

/**
 * @class DIFPGrid
 * @brief Šablonová třída spravující fyzikální pole v jednom souvislém bloku paměti.
 * @tparam Real Typ s plovoucí řádovou čárkou (double nebo float).
 */
template <typename Real = double> 
class DIFPGrid {
private:
    // Jediný vlastník všech fyzikálních dat.
    // Použití std::vector zajišťuje RAII (automatickou správu paměti).
    std::vector<Real> raw_memory;
    
    // Bitově pakované stavové pole (1 bit na buňku pro stavy jako "is_solid", "active", atd.)
    std::vector<uint64_t> state_bits;

    /**
     * @brief Přepočítá interní ukazatele na základě aktuální adresy raw_memory.
     * @details Musí být volána po každé operaci, která mění adresu dat vektoru
     *          (konstrukce, kopírování, přesun, realokace).
     */
    void rebind_pointers() {
        if (raw_memory.empty()) {
            potential = mass = vx = vy = friction = pressure = nullptr;
            return;
        }

        // Získání ukazatele na začátek dat vektoru
        void* ptr = raw_memory.data();
        size_t space = raw_memory.size() * sizeof(Real);
        
        // Zarovnání prvního ukazatele na nejbližší 64-bytovou hranici.
        // std::align posune 'ptr' vpřed na zarovnanou adresu a zmenší 'space'.
        // To je důvod, proč alokujeme extra rezervu (reserve_elements).
        void* aligned_void = std::align(AVX_WIDTH_BYTES, sizeof(Real), ptr, space);
        
        // Bezpečnostní kontrola (teoreticky by neměla nastat díky rezervě)
        if (!aligned_void) {
            throw std::runtime_error("Critical Failure: Unable to align DIFPGrid memory to 64 bytes.");
        }

        Real* aligned_start = static_cast<Real*>(aligned_void);

        // "Krájení salámu": Nastavení ukazatelů na offsety v monolitickém bloku.
        // Používáme padded_size, aby každé pole začínalo na zarovnané hranici.
        potential = aligned_start;
        mass      = potential + padded_size;
        vx        = mass      + padded_size;
        vy        = vx        + padded_size;
        friction  = vy        + padded_size;
        pressure  = friction  + padded_size;
    }

public:
    size_t width;
    size_t height;
    size_t active_size; // w * h (skutečný počet prvků)
    size_t padded_size; // Velikost zarovnaná nahoru na násobek šířky SIMD

    // --- Veřejné fyzikální ukazatele (Read-only pointer values, mutable data) ---
    // Klíčové slovo __restrict je slib kompilátoru, že se tyto ukazatele nepřekrývají (aliasing),
    // což umožňuje agresivní auto-vektorizaci smyček.
    Real* __restrict potential = nullptr;
    Real* __restrict mass = nullptr;
    Real* __restrict vx = nullptr;
    Real* __restrict vy = nullptr;
    Real* __restrict friction = nullptr;
    Real* __restrict pressure = nullptr;

    /**
     * @brief Hlavní konstruktor. Alokuje paměť s paddingem a zarovnáním.
     */
    DIFPGrid(size_t w, size_t h) : width(w), height(h), active_size(w * h) {
        // Počet prvků, které se vejdou do jednoho SIMD registru
        // (např. 64 / 8 = 8 double prvků pro AVX-512)
        constexpr size_t SIMD_ELEMENTS = AVX_WIDTH_BYTES / sizeof(Real);
        
        // Zarovnání velikosti nahoru na nejbližší násobek SIMD šířky.
        // Bitová magie: (n + m - 1) & ~(m - 1)
        padded_size = (active_size + SIMD_ELEMENTS - 1) & ~(SIMD_ELEMENTS - 1);

        // Celková alokace: 6 polí * padded_size
        size_t total_elements = padded_size * 6;
        
        // Rezerva pro posun na 64-bytovou hranici (std::align)
        size_t reserve_elements = AVX_WIDTH_BYTES / sizeof(Real); 
        
        // Fyzická alokace paměti (inicializována na 0)
        raw_memory.resize(total_elements + reserve_elements, Real(0));
        
        // KRITICKÉ: Nastavení ukazatelů
        rebind_pointers();

        // Inicializace fyzikálních konstant (příklad)
        // Používáme std::fill pro bezpečnější inicializaci
        // Inicializujeme až do padded_size, abychom předešli NaN v padding zóně
        if (mass) std::fill(mass, mass + padded_size, Real(1.0));
        if (friction) std::fill(friction, friction + padded_size, Real(0.1));

        // Alokace bitového pole
        size_t bit_vector_size = (active_size + 63) / 64;
        state_bits.resize(bit_vector_size, 0);
    }

    // --- IMPLEMENTACE RULE OF FIVE (Bezpečnost paměti) ---

    // 1. Destruktor
    // Defaultní je v pořádku, std::vector se postará o uvolnění paměti.
    // Virtuální není potřeba, pokud neplánujeme dědičnost (v HPC se dědičnosti často vyhýbáme).
    ~DIFPGrid() = default; 

    // 2. Kopírovací konstruktor (Copy Constructor)
    DIFPGrid(const DIFPGrid& other) 
        : raw_memory(other.raw_memory), // Hluboká kopie datového vektoru
          state_bits(other.state_bits),
          width(other.width), height(other.height), 
          active_size(other.active_size), padded_size(other.padded_size) 
    {
        // KRITICKÉ: Nasměrovat moje ukazatele do MOJÍ nové paměti.
        // Bez tohoto by this->potential ukazoval do other.raw_memory!
        rebind_pointers(); 
    }

    // 3. Přesouvací konstruktor (Move Constructor)
    DIFPGrid(DIFPGrid&& other) noexcept 
        : raw_memory(std::move(other.raw_memory)), // Ukradne buffer vektoru (rychlé, žádná kopie)
          state_bits(std::move(other.state_bits)),
          width(other.width), height(other.height), 
          active_size(other.active_size), padded_size(other.padded_size)
    {
        // I po přesunu musíme nastavit ukazatele, protože raw_memory se přesunula
        // do 'this', ale 'this->potential' je zatím neinicializovaný.
        rebind_pointers();
        
        // Vyčistit 'other', aby jeho destruktor nebo použití nezpůsobilo problémy.
        // Není to striktně nutné pro bezpečnost (vektor je prázdný), ale je to slušnost.
        other.potential = nullptr;
        other.mass = nullptr;
        other.vx = nullptr;
        other.vy = nullptr;
        other.friction = nullptr;
        other.pressure = nullptr;
    }

    // 4. Kopírovací operátor přiřazení (Copy Assignment)
    DIFPGrid& operator=(const DIFPGrid& other) {
        if (this!= &other) {
            // Standardní přiřazení vektoru (hluboká kopie)
            raw_memory = other.raw_memory;
            state_bits = other.state_bits;
            width = other.width;
            height = other.height;
            active_size = other.active_size;
            padded_size = other.padded_size;
            
            // Obnovení vnitřní struktury ukazatelů
            rebind_pointers();
        }
        return *this;
    }

    // 5. Přesouvací operátor přiřazení (Move Assignment)
    DIFPGrid& operator=(DIFPGrid&& other) noexcept {
        if (this!= &other) {
            // Standardní přesun vektoru (ukradení bufferu)
            raw_memory = std::move(other.raw_memory);
            state_bits = std::move(other.state_bits);
            width = other.width;
            height = other.height;
            active_size = other.active_size;
            padded_size = other.padded_size;
            
            // Obnovení vnitřní struktury ukazatelů
            rebind_pointers();
            
            // Vyčištění oběti
            other.potential = nullptr;
            //... a další pointery
        }
        return *this;
    }

    // --- Metody a Helpy ---

    [[nodiscard]] size_t get_compute_size() const { return padded_size; }

    // Bitová manipulace pro stavy (např. is_solid, is_fluid)
    [[nodiscard]] inline bool get_state(size_t idx) const {
        return (state_bits[idx >> 6] >> (idx & 63)) & 1ULL;
    }

    inline void set_state(size_t idx, bool val) {
        if (val) state_bits[idx >> 6] |= (1ULL << (idx & 63));
        else     state_bits[idx >> 6] &= ~(1ULL << (idx & 63));
    }
};

#endif // DIFP_CORE_V3_HPP