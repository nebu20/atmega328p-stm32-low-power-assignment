#include "atmega328p_lowpower.h"
#include <util/delay.h>

#define ACTIVE_TIMEOUT_MS  5000  // 5 seconds inactivity timeout before sleep

int main(void)
{
    uint32_t last_activity_time = 0;

    // 1. Initialize system (GPIO, LCD, Timer1, INT0, enable global interrupts)
    system_init();

    // 2. Display startup header on 16x2 LCD
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_print("ATmega328P MCU");
    lcd_set_cursor(1, 0);
    lcd_print("State: ACTIVE");

    // Signal Active Mode with LED ON
    led_on();
    buzzer_off();
    _delay_ms(1000);

    last_activity_time = timer_ms();

    while (1)
    {
        // -----------------------------------------------------------------
        // State 1: Active Mode - Normal Operation & Inactivity Monitoring
        // -----------------------------------------------------------------
        led_on();

        // Update LCD with remaining active countdown
        uint32_t current_time = timer_ms();
        uint32_t elapsed = current_time - last_activity_time;

        if (elapsed < ACTIVE_TIMEOUT_MS)
        {
            uint32_t seconds_left = (ACTIVE_TIMEOUT_MS - elapsed) / 1000 + 1;
            lcd_set_cursor(1, 0);
            lcd_print("Active: ");
            lcd_print((seconds_left == 5) ? "5s..." :
                      (seconds_left == 4) ? "4s..." :
                      (seconds_left == 3) ? "3s..." :
                      (seconds_left == 2) ? "2s..." : "1s...");
        }

        // If button is pressed during active state, reset active timer
        if (button_pressed())
        {
            last_activity_time = timer_ms();
            lcd_set_cursor(1, 0);
            lcd_print("Button Pressed!");
            _delay_ms(200);
        }

        // Check if 5-second inactivity threshold has been reached by Timer1
        if (timer_elapsed(last_activity_time, ACTIVE_TIMEOUT_MS))
        {
            // -------------------------------------------------------------
            // State 2: Prepare for Low-Power Power-Down Sleep
            // -------------------------------------------------------------
            lcd_clear();
            lcd_set_cursor(0, 0);
            lcd_print("Inactivity Timed");
            lcd_set_cursor(1, 0);
            lcd_print("Entering Sleep..");
            _delay_ms(1500);

            lcd_clear();
            lcd_set_cursor(0, 0);
            lcd_print("MCU: POWER-DOWN");
            lcd_set_cursor(1, 0);
            lcd_print("Press SW1 (INT0)");

            // Turn off active indicators before sleep to maximize power saving
            led_off();
            buzzer_off();
            clear_wake_event();

            // -------------------------------------------------------------
            // State 3: Enter Power-Down Sleep Mode (Timer1 stops, MCU halts)
            // -------------------------------------------------------------
            // Re-enables INT0 (low-level), sets SMCR SM2:SM0=010, executes SLEEP
            enter_power_down();

            // -------------------------------------------------------------
            // State 4: Wake-Up Execution (Triggered by PD2 LOW on INT0)
            // -------------------------------------------------------------
            // Main oscillator restarts -> CPU resumes -> ISR(INT0_vect) executed
            if (wake_event())
            {
                // Restore active indicators upon wake
                led_on();
                buzzer_on();
                _delay_ms(150); // Short 150 ms beep on buzzer
                buzzer_off();

                lcd_clear();
                lcd_set_cursor(0, 0);
                lcd_print("WOKE UP! INT0");
                lcd_set_cursor(1, 0);
                lcd_print("Release Button..");

                // Wait for push button release (PD2 returns HIGH) before re-arming
                while (button_pressed())
                {
                    _delay_ms(10);
                }

                clear_wake_event();
                _delay_ms(500);

                lcd_clear();
                lcd_set_cursor(0, 0);
                lcd_print("ATmega328P MCU");
                lcd_set_cursor(1, 0);
                lcd_print("State: ACTIVE");

                // Reset inactivity timer for new active cycle
                last_activity_time = timer_ms();
            }
        }

        _delay_ms(100);
    }

    return 0;
}
