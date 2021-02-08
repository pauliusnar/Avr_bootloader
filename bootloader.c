#define __AVR_ATmega328P__
#define F_CPU 16000000
#include "stdint.h"
#include "avr/io.h"
#include "avr/boot.h"
#include "avr/interrupt.h"
#include "avr/eeprom.h"
#include "avr/pgmspace.h"

#include "util/delay.h"


#define DEBUG_STRINGS

// static void Set_Led_PCB(char enable)
// {
//     DDRC |= 1 << PINC2;
//     if (enable)
//     {
//         PORTC |= 1 << PINC2;
//     }
//     else
//     {
//         PORTC &= ~(1 << PINC2);
//     }
// }

// static void Set_Led_Switch(char enable)
// {
//     DDRD |= 1 << PIND3;
//     if (enable)
//     {
//         PORTD |= 1 << PIND3;
//     }
//     else
//     {
//         PORTD &= ~(1 << PIND3);
//     }
// }

// static void Set_Led_Line(char enable)
// {
//     DDRD |= 1 << PIND5;
//     if (enable)
//     {
//         PORTD |= 1 << PIND5;
//     }
//     else
//     {
//         PORTD &= ~(1 << PIND5);
//     }
// }

// volatile inline static uint8_t Get_Two_Complement(uint16_t input)
// {
//     return ((input ^ 0xFF) + 1) & 0xFF;
// }

volatile static uint8_t Get_Ihex_Checksum(uint8_t *buffer, uint8_t length)
{
    uint16_t sum;
    length -= 1;

    do
    {
        sum += *(buffer + length);
    } while (length--);

    return Get_Two_Complement(sum);
}

// void boot_program_page(uint8_t *buf)
// {
//     uint8_t sreg;
//     uint16_t counter = 0;
//     uint8_t page = 0;

//     // Disable interrupts.
//     sreg = SREG;
//     cli();

//     while (counter <= storage_size)
//     {
//         // eeprom_busy_wait();
//         boot_page_erase(page);
//         boot_spm_busy_wait(); // Wait until the memory is erased.

//         for (uint16_t i = 0; i < SPM_PAGESIZE; i += 2)
//         {
//             // Set up little-endian word.
//             uint16_t w = *(buf++);
//             w += (*buf++) << 8;
//             counter += 2;

//             boot_page_fill(page + i, w);

//             if (counter >= storage_size)
//             {
//                 break;
//             }
//         }
//         boot_page_write(page); // Store buffer in flash page.
//         boot_spm_busy_wait();  // Wait until the memory is written.

//         // Reenable RWW-section again. We need this if we want to jump back
//         // to the application after bootloading.

//         boot_rww_enable();
//         page += SPM_PAGESIZE;
//         if (counter >= storage_size)
//         {
//             break;
//         }
//     }

//     // Re-enable interrupts (if they were ever enabled).

//     SREG = sreg;
// }

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
    // Set DE/RI pin

    /* Wait for empty transmit buffer */
    while (!(UCSR0A & (1 << UDRE0)))
        ;
    UDR0 = data;

    // Clear DE/RI pin
}

void USART_transmit_string(char *s)
{
#ifdef DEBUG_STRINGS
    // transmit character until NULL is reached
    while (*s > 0)
        Uart_Transmit(*s++);
#endif
}

void USART_transmit_num(uint8_t num)
{
    Uart_Transmit('0' + num);
}

typedef enum Commands
{
    COMMAND_GET_VERSION = 0,     // Responses with pagesize value // TODO
    COMMAND_GET_CHIP = 1,        // Responses with devices signature // TODO
    COMMAND_GET_MODULE_NAME = 2, // Responses with STPF or STPB or WIN1 or WIN2 or WIN3 // TODO

    COMMAND_WRITE_FLASH_PAGE = 3,
    COMMAND_READ_FLASH_PAGE = 4,      // TODO
    COMMAND_READ_FLASH_PAGE_SIZE = 5, // Responses with pagesize value (16+bits)

    COMMAND_WRITE_EEPROM_BYTE = 6,
    COMMAND_READ_EEPROM_BYTE = 7,

    COMMAND_READ_DEVICE_SIGNATURE = 8,
    COMMAND_GO_TO_APPLICATION = 200, // Run main application
};

typedef enum
{
    STATE_START = 0,
    STATE_POOL_REQUEST,  // Command, arg1, arg0
    STATE_PARSE_REQUEST, // Get data from the buffer
} State;

typedef enum
{
    RESPONSE_OK,
    RESPONSE_GOING_TO_APP,
    RESPONSE_RECEIVED_DATA,
    RESPONSE_FLASH_WRITTEN,
    RESPONSE_VALIDATION_FAILED,
    RESPONSE_ERR,
} Response;

typedef enum
{
    REQUEST_INDEX_COMMAND = 0,
    REQUEST_INDEX_ARGUMENT_5,
    REQUEST_INDEX_ARGUMENT_4,
    REQUEST_INDEX_ARGUMENT_3,
    REQUEST_INDEX_ARGUMENT_2,
    REQUEST_INDEX_ARGUMENT_1,
    REQUEST_INDEX_ARGUMENT_0,
} Request_index;

main()
{
    Init_Uart();

    State state = STATE_START;

    uint16_t page = 0;
    uint8_t checksum = 0;
    uint8_t rx_buffer[3];
    uint8_t rx_data[SPM_PAGESIZE];

    while (1)
    {
        switch (state)
        {

        case STATE_START:
        {
            USART_transmit_string("\r\nStart\r\n");
            state = STATE_POOL_REQUEST;
            break;
        }

        case STATE_POOL_REQUEST:
        {
            if (Uart_Receive() == 0x55) // Start byte
            {
                // USART_transmit_string("\r\n CMD=");
                rx_buffer[REQUEST_INDEX_COMMAND] = Uart_Receive();    // Command
                rx_buffer[REQUEST_INDEX_ARGUMENT_0] = Uart_Receive(); // Argument_0 - PAGE  3
                rx_buffer[REQUEST_INDEX_ARGUMENT_1] = Uart_Receive(); // Argument_1 - PAGE  2
                rx_buffer[REQUEST_INDEX_ARGUMENT_2] = Uart_Receive(); // Argument_2 - PAGE  1
                rx_buffer[REQUEST_INDEX_ARGUMENT_3] = Uart_Receive(); // Argument_3 - PAGE  0
                rx_buffer[REQUEST_INDEX_ARGUMENT_4] = Uart_Receive(); // Argument_4 - CRC16 1
                rx_buffer[REQUEST_INDEX_ARGUMENT_5] = Uart_Receive(); // Argument_5 - CRC16 0

                // USART_transmit_string("cm");
                state = STATE_PARSE_REQUEST;
            }
            break;
        }

        case STATE_PARSE_REQUEST:
        {
            switch (rx_buffer[REQUEST_INDEX_COMMAND]) // Command
            {

            case COMMAND_READ_FLASH_PAGE:
            {
                USART_transmit_string(" FlR=start");

                // Parse CRC16
                uint16_t crc16;
                crc16 = rx_buffer[REQUEST_INDEX_ARGUMENT_4] << 8;
                crc16 |= rx_buffer[REQUEST_INDEX_ARGUMENT_5];
                Uart_Transmit(crc16 >> 8);
                Uart_Transmit(crc16);

                // Parse page
                page = rx_buffer[REQUEST_INDEX_ARGUMENT_2] << 8;
                page |= rx_buffer[REQUEST_INDEX_ARGUMENT_3];
                Uart_Transmit(page >> 8);
                Uart_Transmit(page);

                Uart_Transmit(0x00);
                Uart_Transmit(0x00);
                Uart_Transmit(0x00);

                // Get page (0,128,256..)
                uint16_t got_address = page;
                while (page < (got_address + SPM_PAGESIZE))
                {
                    Uart_Transmit(pgm_read_byte(page++));
                    // boot_page_fill
                }

                // Do data validation
                // Calculate crc16
                // if not ok response
                // Uart_Transmit(0x55);
                // Uart_Transmit(0);
                // Uart_Transmit(RESPONSE_VALIDATION_FAILED);
                // Uart_Transmit(0xAA);
                // continue;


                USART_transmit_string(" Flr=ok");
                // Go back pooling requests
                state = STATE_POOL_REQUEST;
                break;
            }

            // Get CRC16, Uart_Transmit, data and write it to flash if it's valid
            case COMMAND_WRITE_FLASH_PAGE:
            {
                USART_transmit_string(" Fl=start");

                // Parse CRC16
                uint16_t crc16;
                crc16 = rx_buffer[REQUEST_INDEX_ARGUMENT_4] << 8;
                crc16 |= rx_buffer[REQUEST_INDEX_ARGUMENT_5];
                Uart_Transmit(crc16 >> 8);
                Uart_Transmit(crc16);

                // Parse page
                page = rx_buffer[REQUEST_INDEX_ARGUMENT_2] << 8;
                page |= rx_buffer[REQUEST_INDEX_ARGUMENT_3];
                Uart_Transmit(page >> 8);
                Uart_Transmit(page);

                // Get page (0,128,256..)
                for (uint16_t i = 0; i < SPM_PAGESIZE; i++)
                {
                    rx_data[i] = Uart_Receive();
                }

                // Send response
                // Uart_Transmit(RESPONSE_RECEIVED_DATA);
                // Uart_Transmit(RESPONSE_RECEIVED_DATA);
                // Uart_Transmit(RESPONSE_RECEIVED_DATA);

                // Do data validation
                // Calculate crc16
                // if not ok response
                // Uart_Transmit(0x55);
                // Uart_Transmit(0);
                // Uart_Transmit(RESPONSE_VALIDATION_FAILED);
                // Uart_Transmit(0xAA);
                // continue;

                // Write data to flash
                // Do erase before programming
                boot_page_erase(page);
                boot_spm_busy_wait(); // Wait until the memory is written.

                for (uint16_t i = 0; i < SPM_PAGESIZE; i += 2)
                {
                    uint16_t w = rx_data[i];
                    w |= rx_data[i + 1] << 8;
                    boot_page_fill(page + i, w);
                }

                boot_page_write(page); // Store buffer in flash page.
                boot_spm_busy_wait();  // Wait until the memory is written.
                boot_rww_enable();

                // Uart_Transmit(RESPONSE_FLASH_WRITTEN);
                // Uart_Transmit(RESPONSE_FLASH_WRITTEN);
                // Uart_Transmit(RESPONSE_FLASH_WRITTEN);

                USART_transmit_string(" Fl=ok");
                // Go back pooling requests
                state = STATE_POOL_REQUEST;
                break;
            }

            // Send flash pagesize to master
            case COMMAND_READ_FLASH_PAGE_SIZE:
            {
                // USART_transmit_string(" R_PG_SZ=");

                // Send pagesize to master [Startbyte MSB LSB Endbyte]
                Uart_Transmit(0x55);
                Uart_Transmit(SPM_PAGESIZE >> 8);
                Uart_Transmit(SPM_PAGESIZE);
                Uart_Transmit(0xAA);

                // Go back pooling requests
                state = STATE_POOL_REQUEST;
                break;
            }

            // Run main application
            case COMMAND_GO_TO_APPLICATION:
            {
                USART_transmit_string(" GO_APP=");

                Uart_Transmit(0x55);
                Uart_Transmit(RESPONSE_GOING_TO_APP);
                Uart_Transmit(RESPONSE_OK);
                Uart_Transmit(0xAA);

                asm("jmp 0");

                Uart_Transmit(RESPONSE_ERR);
                // Go back pooling requests
                state = STATE_POOL_REQUEST;
                break;
            }

            case COMMAND_WRITE_EEPROM_BYTE:
            {
                uint16_t address;
                address = rx_buffer[REQUEST_INDEX_ARGUMENT_2] << 8;
                address |= rx_buffer[REQUEST_INDEX_ARGUMENT_3];
                eeprom_busy_wait();
                eeprom_write_byte(address, rx_buffer[REQUEST_INDEX_ARGUMENT_5]);
                eeprom_busy_wait();

                Uart_Transmit(0x55);
                Uart_Transmit(rx_buffer[REQUEST_INDEX_ARGUMENT_5]);
                Uart_Transmit(RESPONSE_OK);
                Uart_Transmit(0xAA);

                // Go back pooling requests
                state = STATE_POOL_REQUEST;
                break;
            }

            case COMMAND_READ_EEPROM_BYTE:
            {
                uint16_t address;
                address = rx_buffer[REQUEST_INDEX_ARGUMENT_2] << 8;
                address |= rx_buffer[REQUEST_INDEX_ARGUMENT_3];

                eeprom_busy_wait();
                uint8_t r = eeprom_read_byte(address);

                Uart_Transmit(0x55);
                Uart_Transmit(r);
                Uart_Transmit(RESPONSE_OK);
                Uart_Transmit(0xAA);

                // Go back pooling requests
                state = STATE_POOL_REQUEST;
                break;
            }

            case COMMAND_READ_DEVICE_SIGNATURE:
            {
                Uart_Transmit(0x55);
                Uart_Transmit(boot_signature_byte_get(0x00)); // Device signature 1
                Uart_Transmit(boot_signature_byte_get(0x02)); // Device signature 2
                Uart_Transmit(boot_signature_byte_get(0x04)); // Device signature 3
                // Uart_Transmit(boot_signature_byte_get(0x01)); // RC calibration value
                // Uart_Transmit(boot_signature_byte_get(0x02)); // Temperature sensor offset
                // Uart_Transmit(boot_signature_byte_get(0x03)); // Temperature sensor gain

                Uart_Transmit(RESPONSE_OK);
                Uart_Transmit(0xAA);
                
                // Go back pooling requests
                state = STATE_POOL_REQUEST;
                break;
            }

            default:
                break;
            }
            break;
            // break;
        }

        default:
        {
            USART_transmit_string("Error State\r\n");
            break;
        }
        }
    }
}