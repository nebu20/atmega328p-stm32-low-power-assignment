#include "atmega328p_lowpower.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>

#define LED_PIN     PB0
#define BUZZER_PIN  PB1
#define BUTTON_PIN  PD2

#define LCD_RS      PC0
#define LCD_E       PC1
#define LCD_D4      PC2
#define LCD_D5      PC3
#define LCD_D6      PC4
#define LCD_D7      PC5

static volatile uint32_t system_ticks = 0;
static volatile uint8_t wake_flag = 0;

/* -------------------------------------------------------------------------
 * Timer1 Compare Match A ISR (System Millisecond Tick)
 * ------------------------------------------------------------------------- */
ISR(TIMER1_COMPA_vect)
{
    system_ticks++;
}

/* -------------------------------------------------------------------------
 * External Interrupt INT0 ISR (Push Button Low-Level Wake-up)
 * ------------------------------------------------------------------------- */
ISR(INT0_vect)
{
    wake_flag = 1;
    // Disable INT0 inside ISR to prevent continuous re-triggering while PD2 remains LOW
    EIMSK &= ~(1 << INT0);
}

/* -------------------------------------------------------------------------
 * Timer1 Initialization (CTC Mode, Prescaler 64, 1 ms compare match @ 16 MHz)
 * ------------------------------------------------------------------------- */
static void timer1_init(void)
{
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1  = 0;

    // Configure Timer1 for CTC mode (WGM12 = 1 in TCCR1B)
    TCCR1B |= (1 << WGM12);

    // OCR1A calculation for 1 ms interval at 16 MHz with prescaler 64:
    // (16,000,000 Hz / (64 * 1000 Hz)) - 1 = 249
    OCR1A = 249;

    // Set Prescaler 64 (CS11 = 1, CS10 = 1 in TCCR1B)
    TCCR1B |= (1 << CS11) | (1 << CS10);

    // Enable Timer1 Output Compare Match A Interrupt
    TIMSK1 |= (1 << OCIE1A);
}

/* -------------------------------------------------------------------------
 * GPIO Pin Configuration
 * ------------------------------------------------------------------------- */
static void gpio_init(void)
{
    // Set PB0 (LED) and PB1 (Buzzer) as outputs
    DDRB |= (1 << LED_PIN) | (1 << BUZZER_PIN);
    PORTB &= ~((1 << LED_PIN) | (1 << BUZZER_PIN));

    // Set PD2 (Button / INT0) as input with internal pull-up enabled
    DDRD &= ~(1 << BUTTON_PIN);
    PORTD |= (1 << BUTTON_PIN);

    // Set PC0..PC5 (LCD RS, E, D4..D7) as outputs
    DDRC |= (1 << LCD_RS) | (1 << LCD_E) |
            (1 << LCD_D4) | (1 << LCD_D5) | (1 << LCD_D6) | (1 << LCD_D7);
    PORTC &= ~((1 << LCD_RS) | (1 << LCD_E) |
             (1 << LCD_D4) | (1 << LCD_D5) | (1 << LCD_D6) | (1 << LCD_D7));
}

/* -------------------------------------------------------------------------
 * INT0 External Interrupt Configuration (Low-Level Triggering)
 * ------------------------------------------------------------------------- */
static void int0_init(void)
{
    // Configure INT0 for low-level triggering (ISC01 = 0, ISC00 = 0 in EICRA)
    EICRA &= ~((1 << ISC01) | (1 << ISC00));

    // Clear any pending INT0 interrupt flag
    EIFR |= (1 << INTF0);

    // Enable INT0 external interrupt
    EIMSK |= (1 << INT0);
}

/* -------------------------------------------------------------------------
 * 4-Bit HD44780 LCD Low-Level Drivers (Port C: PC0-PC5)
 * ------------------------------------------------------------------------- */
static void lcd_enable_pulse(void)
{
    PORTC |= (1 << LCD_E);
    _delay_us(1);
    PORTC &= ~(1 << LCD_E);
    _delay_us(50);
}

static void lcd_send_nibble(uint8_t nibble)
{
    PORTC &= ~((1 << LCD_D4) | (1 << LCD_D5) | (1 << LCD_D6) | (1 << LCD_D7));

    if (nibble & 0x01) PORTC |= (1 << LCD_D4);
    if (nibble & 0x02) PORTC |= (1 << LCD_D5);
    if (nibble & 0x04) PORTC |= (1 << LCD_D6);
    if (nibble & 0x08) PORTC |= (1 << LCD_D7);

    lcd_enable_pulse();
}

static void lcd_command(uint8_t cmd)
{
    PORTC &= ~(1 << LCD_RS); // RS = 0 for Command
    lcd_send_nibble(cmd >> 4);
    lcd_send_nibble(cmd & 0x0F);
    if (cmd == 0x01 || cmd == 0x02) {
        _delay_ms(2);
    }
}

static void lcd_data(uint8_t data)
{
    PORTC |= (1 << LCD_RS); // RS = 1 for Data
    lcd_send_nibble(data >> 4);
    lcd_send_nibble(data & 0x0F);
}

void lcd_init(void)
{
    _delay_ms(50);

    PORTC &= ~(1 << LCD_RS);
    PORTC &= ~(1 << LCD_E);

    // HD44780 4-bit initialization sequence
    lcd_send_nibble(0x03);
    _delay_ms(5);
    lcd_send_nibble(0x03);
    _delay_us(150);
    lcd_send_nibble(0x03);
    _delay_us(150);

    // Set to 4-bit mode
    lcd_send_nibble(0x02);
    _delay_us(150);

    // Function set: 4-bit mode, 2 lines, 5x8 font
    lcd_command(0x28);
    // Display ON, Cursor OFF, Blink OFF
    lcd_command(0x0C);
    // Clear display
    lcd_command(0x01);
    // Entry mode set: Increment cursor
    lcd_command(0x06);
}

void lcd_clear(void)
{
    lcd_command(0x01);
}

void lcd_set_cursor(uint8_t row, uint8_t column)
{
    uint8_t address = (row == 0) ? 0x00 : 0x40;
    address += column;
    lcd_command(0x80 | address);
}

void lcd_print(const char *text)
{
    while (*text) {
        lcd_data((uint8_t)*text++);
    }
}

/* -------------------------------------------------------------------------
 * Public Library API Implementation
 * ------------------------------------------------------------------------- */
void system_init(void)
{
    gpio_init();
    lcd_init();
    timer1_init();
    int0_init();

    sei(); // Enable global interrupts
}

void led_on(void)
{
    PORTB |= (1 << LED_PIN);
}

void led_off(void)
{
    PORTB &= ~(1 << LED_PIN);
}

void led_toggle(void)
{
    PORTB ^= (1 << LED_PIN);
}

void buzzer_on(void)
{
    PORTB |= (1 << BUZZER_PIN);
}

void buzzer_off(void)
{
    PORTB &= ~(1 << BUZZER_PIN);
}

uint8_t button_pressed(void)
{
    return !(PIND & (1 << BUTTON_PIN));
}

void ext_int0_enable(void)
{
    EIFR |= (1 << INTF0);
    EIMSK |= (1 << INT0);
}

void ext_int0_disable(void)
{
    EIMSK &= ~(1 << INT0);
}

void enter_power_down(void)
{
    // Re-enable INT0 before entering sleep
    ext_int0_enable();

    // Select Power-Down sleep mode (SM2:SM0 = 010 in SMCR)
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);

    // Set SE bit, execute SLEEP instruction, clear SE bit on wake
    sleep_enable();
    sleep_cpu();
    sleep_disable();

    // Disable INT0 right after waking to prevent continuous level-triggering
    ext_int0_disable();
}

uint8_t wake_event(void)
{
    return wake_flag;
}

void clear_wake_event(void)
{
    wake_flag = 0;
}

uint32_t timer_ms(void)
{
    uint32_t ticks;
    cli();
    ticks = system_ticks;
    sei();
    return ticks;
}

uint8_t timer_elapsed(uint32_t last_tick, uint32_t interval)
{
    return ((timer_ms() - last_tick) >= interval);
}