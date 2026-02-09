# Projekt: Informační Vesmír (Simulátor mřížky)

Tento projekt je experimentální implementací teorie digitální fyziky, kde časoprostor není prázdno, ale diskrétní informační síť.

## Základní axiomy teorie:
1. **Diskrétní prostor:** Vesmír tvoří síť uzlů (Node).
2. **Pohyb = Přepis:** Hmota se nepohybuje skrz prostor, ale přepisuje svůj stav mezi sousedními uzly.
3. **Gravitace = Latence:** Hustota informace v mřížce způsobuje latenci (zpoždění) v taktu přepisování.
4. **Planckův čas:** Nejkratší možný interval mezi dvěma přepisy v mřížce.

## Jak sestavit projekt:
- **Kompilátor:** g++ (součást MSYS2/MinGW)
- **Sestavení:** `Ctrl+Shift+B` ve VS Code (využívá `tasks.json`)
- **Optimalizace:** `-O3 -march=native` pro maximální výkon simulace.

## Aktuální stav:
- Definována struktura uzlu (5 bajtů).
- Implementována alokace 2D mřížky v RAM.