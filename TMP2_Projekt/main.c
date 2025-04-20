#include "MKL05Z4.h"
#include "lcd1602.h"
#include "buttons.h"
#include "frdm_bsp.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

volatile uint32_t timer_ms = 0; // Globalny licznik czasu
volatile uint16_t phase_accumulator = 0;  // Akumulator fazy
volatile uint16_t phase_increment = 64; // Krok fazy (reguluje czestotliwosc)

// Inicjalizuje timer SysTick do generowania przerwan co 1 ms
void SysTick_Init(void) {
    SysTick_Config(SystemCoreClock / 1000);
}

// Przerwanie SysTick zwieksza globalny licznik czasu
void SysTick_Handler(void) {
    timer_ms++;
}

// Funkcja wprowadza opoznienie na okreslona liczbe milisekund
void Delay_SysTick_ms(uint32_t ms) {
    uint32_t start_time = timer_ms; // Pobranie aktualnej wartosci czasu
    while ((timer_ms - start_time) < ms) { // Oczekiwanie na uplyw ms
        __WFI(); // Przejscie w tryb oczekiwania na przerwanie
    }
}

// Generuje sygnal dzwiekowy startu gry za pomoca prostego DDS (fala prostokatna)
void PlayStartSignal(void) {
    uint16_t duration_ms = 50;  // Czas trwania dźwięku w milisekundach
    uint32_t start_time = timer_ms;

    while ((timer_ms - start_time) < duration_ms) {
        phase_accumulator += phase_increment;  // Aktualizuj faze
        if (phase_accumulator & 0x8000) {      // Sprawdź MSB (generacja fali prostokatnej)
            PTB->PSOR = MASK(8);               // Stan wysoki
        } else {
            PTB->PCOR = MASK(8);               // Stan niski
        }
    }

    PTB->PCOR = MASK(8);  // Wylacz po zakonczeniu
}


// Resetuje wyniki graczy i wyswietla komunikat "Restart gry"
void RestartGame(uint8_t *score_p1, uint8_t *score_p2) {
    *score_p1 = 0;
    *score_p2 = 0;
    LCD1602_ClearAll();
    LCD1602_Print("Restart gry...");
    Delay_SysTick_ms(2000);
}

int main(void) {
    uint8_t score_p1 = 0, score_p2 = 0; // Wyniki graczy
    char display[17]; // Bufor do wyswietlania komunikatów na LCD
    const uint32_t reaction_timeout_ms = 5000; // Limit czasu reakcji (ms)

    // Inicjalizacja modulow
    LCD1602_Init();
    LCD1602_Backlight(TRUE);
    Buttons_Init();
    SysTick_Init();

    // Konfiguracja pinu glosnika
    SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;
    PORTB->PCR[8] = PORT_PCR_MUX(1);
    PTB->PDDR |= MASK(8);

    while (1) {
        for (int round = 1; round <= 3; round++) { 
            uint32_t reaction_time_ms = 0; // Czas reakcji aktualnej rundy
            uint8_t winner = 0;            // Zmienna do przechowywania zwyciezcy
            bool falstart = false;         // Flaga falstartu

            // Komunikat "Gotowi..."
            LCD1602_ClearAll();
            LCD1602_Print("Gotowi...");
            Delay_SysTick_ms(2000);

            // Losowy czas oczekiwania na START
            uint32_t delay_before_start = 1000 + (rand() % 3000);
            LCD1602_ClearAll();
            LCD1602_Print("Czekaj na START!");

            timer_ms = 0;
            while (timer_ms < delay_before_start) {
                // Sprawdzanie falstartu przez graczy
                if (IsButtonPressed(1)) {
                    LCD1602_ClearAll();
                    LCD1602_Print("Falstart P1!");
                    score_p2++;
                    Delay_SysTick_ms(2000);
                    falstart = true;
                    break;
                }
                if (IsButtonPressed(2)) {
                    LCD1602_ClearAll();
                    LCD1602_Print("Falstart P2!");
                    score_p1++;
                    Delay_SysTick_ms(2000);
                    falstart = true;
                    break;
                }
            }

            // Jezeli falstart, przechodzimy do kolejnej rundy lub restart gry
            if (falstart) {
                LCD1602_ClearAll();
                LCD1602_Print("Dalej?");
                while (!IsButtonPressed(3)) {
                    if (IsButtonPressed(4)) {
                        RestartGame(&score_p1, &score_p2);
                        round = 1;
                        break;
                    }
                }
                continue;
            }

            // Komunikat "START!" i uruchomienie odliczania
            LCD1602_ClearAll();
            LCD1602_Print("START!");
            PlayStartSignal();

            timer_ms = 0;
            while (timer_ms < reaction_timeout_ms) {
                if (IsButtonPressed(1)) { // Gracz 1 nacisnal przycisk
                    reaction_time_ms = timer_ms;
                    winner = 1;
                    break;
                }
                if (IsButtonPressed(2)) { // Gracz 2 nacisnal przycisk
                    reaction_time_ms = timer_ms;
                    winner = 2;
                    break;
                }
            }

            // Brak reakcji w limicie czasu
            if (winner == 0) {
                LCD1602_ClearAll();
                LCD1602_Print("Brak reakcji");
                Delay_SysTick_ms(2000);
                LCD1602_ClearAll();
                LCD1602_Print("Dalej?");
                while (!IsButtonPressed(3)) {
                    if (IsButtonPressed(4)) {
                        RestartGame(&score_p1, &score_p2);
                        round = 1;
                        break;
                    }
                }
                continue;
            }

            // Wyswietlenie czasu reakcji
            LCD1602_ClearAll();
            if (winner == 1) {
                score_p1++;
                sprintf(display, "P1: %ums", reaction_time_ms);
            } else {
                score_p2++;
                sprintf(display, "P2: %ums", reaction_time_ms);
            }
            LCD1602_Print(display);
            Delay_SysTick_ms(2000);

            // Oczekiwanie na rozpoczecie kolejnej rundy
            LCD1602_ClearAll();
            LCD1602_Print("Dalej?");
            while (!IsButtonPressed(3)) {
                if (IsButtonPressed(4)) {
                    RestartGame(&score_p1, &score_p2);
                    round = 1;
                    break;
                }
            }
        }

        // Wyswietlenie wyniku po 3 rundach
        LCD1602_ClearAll();
        sprintf(display, "P1:%u P2:%u", score_p1, score_p2);
        LCD1602_Print(display);
        Delay_SysTick_ms(3000);

        // Ogloszenie zwyciezcy
        LCD1602_ClearAll();
        if (score_p1 > score_p2) {
            LCD1602_Print("Wygrywa P1!");
        } else if (score_p1 < score_p2) {
            LCD1602_Print("Wygrywa P2!");
        } else {
            LCD1602_Print("Remis!");
        }
        Delay_SysTick_ms(3000);

        // Oczekiwanie na restart gry
        LCD1602_ClearAll();
        LCD1602_Print("Restart?");
        while (!IsButtonPressed(4)) {}
        RestartGame(&score_p1, &score_p2);
    }
}
