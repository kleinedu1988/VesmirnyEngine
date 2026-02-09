#include <iostream>
#include <vector>
#include <cstdint> // Pro přesné datové typy jako uint8_t

/**
 * STRUKTURA: Node (Uzel mřížky)
 * Reprezentuje nejmenší kvantum prostoru.
 * V tvé teorii uzel nemá paměť, jen aktuální stav.
 */
struct Node {
    // 0 = volný prostor (vakuum), 1 = aktivní informace (hmota)
    uint8_t state; 
    
    // density reprezentuje "zahuštění" mřížky v tomto bodě.
    // V budoucnu bude ovlivňovat, kolik taktů musí uzel "čekat", než provede přepis.
    float density; 
};

/**
 * FUNKCE: rewrite (Informační přepis)
 * Toto je srdce tvého vesmíru. Implementuje princip výměny 1:1.
 * Informace se nepohybuje "skrze" prostor, ale body si vymění stavy.
 */
void rewrite(Node& current, Node& neighbor) {
    // Pokud je cílový uzel volný (stav 0), proběhne výměna.
    // Hmota (1) se přesune vpřed, prázdnota (0) se vrátí dozadu.
    if (current.state == 1 && neighbor.state == 0) {
        neighbor.state = 1;
        current.state = 0;
        // Poznámka: Zde se zachovává zákon zachování informace. 
        // Počet jedniček v systému zůstává konstantní.
    }
}

/**
 * FUNKCE: tick (Planckův čas / Takt)
 * Představuje jeden nejmenší časový úsek vesmíru.
 * Během jednoho taktu proběhne v mřížce jedna vlna přepisů.
 */
void tick(std::vector<Node>& grid, int width, int height) {
    // Procházíme mřížku odzadu (zprava doleva), aby se nám 
    // informace v jednom taktu neposunula o víc než jeden uzel.
    // To simuluje rychlostní limit 'c'.
    for (int y = 0; y < height; ++y) {
        for (int x = width - 2; x >= 0; --x) {
            int idx = y * width + x;
            int nextIdx = y * width + (x + 1);
            
            // Aplikujeme pravidlo přepisu pro každý bod a jeho pravého souseda
            rewrite(grid[idx], grid[nextIdx]);
        }
    }
}

int main() {
    // 1. DEFINICE PROSTORU
    const int WIDTH = 20;  // Malá mřížka pro názornost v terminálu
    const int HEIGHT = 1;  // Jednorozměrný testovací "pruh" vesmíru
    
    // Alokace mřížky v RAM (naplníme ji vakuem se základní hustotou)
    std::vector<Node> grid(WIDTH * HEIGHT, {0, 1.0f});

    // 2. VLOŽENÍ INFORMACE (Tvoje "ruka" nebo částice)
    // Umístíme částici na start (index 0)
    grid[0].state = 1;

    std::cout << "--- START SIMULACE INFORMACNIHO VESMIRU ---" << std::endl;
    std::cout << "Legenda: [X] = Hmota (Infor.), [ ] = Volny prostor" << std::endl;

    // 3. SIMULACE BĚHU ČASU (15 taktů/Planckových časů)
    for (int t = 0; t < 15; ++t) {
        std::cout << "Takt " << t << ": ";
        
        // Vykreslení aktuálního stavu mřížky do terminálu
        for (int i = 0; i < WIDTH; ++i) {
            if (grid[i].state == 1) std::cout << "[X]";
            else std::cout << "[ ]";
        }
        
        std::cout << std::endl;

        // Provedení jednoho taktu vesmíru (přepis informací)
        tick(grid, WIDTH, HEIGHT);
    }

    std::cout << "--- KONEC SIMULACE ---" << std::endl;
    return 0;
}