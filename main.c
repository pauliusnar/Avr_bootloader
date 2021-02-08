#define __AVR_ATmega328P__
#include "avr/io.h"
#define F_CPU 16000000
#include "util/delay.h"

static void Set_Led_PCB(char enable)
{
    DDRC |= 1 << PINC2;
    if (enable)
    {
        PORTC |= 1 << PINC2;
    }
    else
    {
        PORTC &= ~(1 << PINC2);
    }
}


void Init_Uart()
{
// setting the baud rate  based on the datasheet
#define UART_BAUD 102
    UBRR0H = (unsigned char)(UART_BAUD >> 8); // 0x00 // TODO: change it to value
    UBRR0L = (unsigned char)UART_BAUD;        // 0x0C
    // enabling TX & RX
    UCSR0B = (1 << TXEN0) | (1 << RXEN0);
    UCSR0A = (1 << UDRE0);
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // Set frame: 8data, 1 stop
}
uint8_t Uart_Receive(void)
{
    /* Wait for data to be received */
    while (!(UCSR0A & (1 << RXC0)))
        ;
    /* Get and return received data from buffer */
    return UDR0;
}

void Uart_Transmit(unsigned char data)
{
    /* Wait for empty transmit buffer */
    while (!(UCSR0A & (1 << UDRE0)))
        ;
    /* Put data into buffer, sends the data */
    UDR0 = data;
}

void USART_transmit_string(char *s)
{
    // transmit character until NULL is reached
    while (*s > 0)
        Uart_Transmit(*s++);
}

int main()
{
    Init_Uart();
    Set_Led_PCB(1);
    USART_transmit_string("-Application-");
    

}