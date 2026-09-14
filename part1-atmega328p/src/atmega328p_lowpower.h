#ifndef ATMEGA328P_LOWPOWER_H
#define ATMEGA328P_LOWPOWER_H

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <stdint.h>

// System Initialization
void system_init(void);

// Peripherals (LED & Buzzer)
void led_on(void);
void led_off(void);
void led_toggle(void);
void buzzer_on(void);
void buzzer_off(void);

// Button input
uint8_t button_pressed(void);

// 16x2 LCD (4-bit mode on PC0-PC5)
void lcd_init(void);
void lcd_clear(void);
void lcd_set_cursor(uint8_t row, uint8_t column);
void lcd_print(const char *text);

// Low Power & INT0 External Interrupt
void ext_int0_enable(void);
void ext_int0_disable(void);
void enter_power_down(void);
uint8_t wake_event(void);
void clear_wake_event(void);

// Timer1 Tick Counter (1 ms tick)
uint32_t timer_ms(void);
uint8_t timer_elapsed(uint32_t last_tick, uint32_t interval);

#endif