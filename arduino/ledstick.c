#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <stddef.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <avr/pgmspace.h>
#include <stdarg.h>
#include <stdlib.h>

// Bit manipulation macros
#define sbi(a, b) ((a) |= 1 << (b))       //sets bit B in variable A
#define cbi(a, b) ((a) &= ~(1 << (b)))    //clears bit B in variable A
#define tbi(a, b) ((a) ^= 1 << (b))       //toggles bit B in variable A

#define BAUD 38400
#define UBBR (F_CPU / 16 / BAUD - 1)
#define TIMER0_INIT 0xF0 // 16 ticks before it overflows = 1us
#define DEBUG 0

// Time keeping
static volatile uint32_t g_time = 0;

// timer interrupt. 
ISR (TIMER1_OVF_vect)
{
    g_time++;
    TCNT0 = TIMER0_INIT;

    // TODO: run clock only when we have data to shift.
}

void serial_init(void)
{
    /*Set baud rate */ 
    UBRR0H = (unsigned char)(UBBR>>8); 
    UBRR0L = (unsigned char)UBBR; 
    /* Enable transmitter */ 
    UCSR0B = (1<<TXEN0) | (1<<RXEN0) | (1<<RXCIE0);
    /* Set frame format: 8data, 1stop bit */ 
    UCSR0C = (0<<USBS0)|(3<<UCSZ00); 
}

uint8_t serial_tx(uint8_t ch)
{
    while ( !( UCSR0A & (1<<UDRE0)) )
    {
        if (is_reset())
            return 0;
    }
    UDR0 = ch;
    return 1;
}

#define MAX 80 

// debugging printf function. Max MAX characters per line!!
void dprintf(const char *fmt, ...)
{
    va_list va;
    va_start (va, fmt);
    char buffer[MAX];
    char *ptr = buffer;
    vsnprintf(buffer, MAX, fmt, va);
    va_end (va);
    for(ptr = buffer; *ptr; ptr++)
    {
        if (*ptr == '\n') serial_tx('\r');
        serial_tx(*ptr);
    }
}

uint8_t serial_rx(void)
{
    while ( !(UCSR0A & (1<<RXC0))) 
          ;
        
    return UDR0;
}

void setup(void)
{
    // PD2 - clock
    // PD3 - signal
    DDRD |= (1<<PD2)|(1<<PD3);

    // Timer setup for clock 
    TCCR0B |= _BV(CS00); // clock / 16 = 1Mhz = 1us
    TIMSK0 |= (1<<TOIE0);
    TCNT0 = TIMER0_INIT;

    serial_init();
}

int main(void)
{
    uint32_t t;

    setup();

    for(;;)
    {
        cli();
        t= g_time;
        sei();

        if ((t% 1000000) == 0)
        {
            dprintf("%d\n", t / 1000000);
        }
    }
}
