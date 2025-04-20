#include "MKL05Z4.h"
#include "buttons.h"

void Delay_SysTick_ms(uint32_t ms);

// Definicje przyciskow
#define S1_MASK (1 << 9)  // Przycisk Gracza 1 (S1)
#define S2_MASK (1 << 10) // Przycisk Gracza 2 (S2)
#define S3_MASK (1 << 11) // Przycisk Dalej
#define S4_MASK (1 << 12) // Przycisk Restart

// Konfiguracja pinow S1, S2, S3, S4 jako wejscia
void Buttons_Init(void) {
    SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK; // Wlacz zegar dla portu A
    PORTA->PCR[9] = PORT_PCR_MUX(1) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;  // Konfiguracja S1
    PORTA->PCR[10] = PORT_PCR_MUX(1) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK; // Konfiguracja S2
    PORTA->PCR[11] = PORT_PCR_MUX(1) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK; // Konfiguracja S3
    PORTA->PCR[12] = PORT_PCR_MUX(1) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK; // Konfiguracja S4
    PTA->PDDR &= ~(S1_MASK | S2_MASK | S3_MASK | S4_MASK); // Ustaw piny jako wejscia
}

// Sprawdzenie, czy dany przycisk jest wcisniety
uint8_t IsButtonPressed(uint8_t button) {
    uint8_t pressed = 0;


    uint32_t mask = 0;
    switch (button) {
        case 1: mask = S1_MASK; break; // S1
        case 2: mask = S2_MASK; break; // S2
        case 3: mask = S3_MASK; break; // S3
        case 4: mask = S4_MASK; break; // S4
        default: return 0;
    }

    // Sprawdz stan przycisku
    if (!(PTA->PDIR & mask)) { 
        Delay_SysTick_ms(20); // Debouncing - czekaj 20 ms
        if (!(PTA->PDIR & mask)) {
            pressed = 1;
        }
    }

    return pressed;
}