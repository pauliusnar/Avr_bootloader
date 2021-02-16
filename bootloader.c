#define __AVR_ATmega328P__
#define F_CPU 16000000
#include "stdint.h"
#include "avr/io.h"
#include "avr/boot.h"
#include "avr/interrupt.h"
#include "avr/eeprom.h"
#include "avr/pgmspace.h"

#include "util/delay.h"
// #define USE_2X 0
// #define BAUD 9600
// #include "util/setbaud.h"

#define START_BYTE 0x55
#define STOP_BYTE 0xAA

#include "config.h"

uint16_t CRC16(const uint8_t *nData, uint16_t wLength)
{
    static const uint16_t wCRCTable[] =
        {
            0X0000, 0XC0C1, 0XC181, 0X0140, 0XC301, 0X03C0, 0X0280, 0XC241,
            0XC601, 0X06C0, 0X0780, 0XC741, 0X0500, 0XC5C1, 0XC481, 0X0440,
            0XCC01, 0X0CC0, 0X0D80, 0XCD41, 0X0F00, 0XCFC1, 0XCE81, 0X0E40,
            0X0A00, 0XCAC1, 0XCB81, 0X0B40, 0XC901, 0X09C0, 0X0880, 0XC841,
            0XD801, 0X18C0, 0X1980, 0XD941, 0X1B00, 0XDBC1, 0XDA81, 0X1A40,
            0X1E00, 0XDEC1, 0XDF81, 0X1F40, 0XDD01, 0X1DC0, 0X1C80, 0XDC41,
            0X1400, 0XD4C1, 0XD581, 0X1540, 0XD701, 0X17C0, 0X1680, 0XD641,
            0XD201, 0X12C0, 0X1380, 0XD341, 0X1100, 0XD1C1, 0XD081, 0X1040,
            0XF001, 0X30C0, 0X3180, 0XF141, 0X3300, 0XF3C1, 0XF281, 0X3240,
            0X3600, 0XF6C1, 0XF781, 0X3740, 0XF501, 0X35C0, 0X3480, 0XF441,
            0X3C00, 0XFCC1, 0XFD81, 0X3D40, 0XFF01, 0X3FC0, 0X3E80, 0XFE41,
            0XFA01, 0X3AC0, 0X3B80, 0XFB41, 0X3900, 0XF9C1, 0XF881, 0X3840,
            0X2800, 0XE8C1, 0XE981, 0X2940, 0XEB01, 0X2BC0, 0X2A80, 0XEA41,
            0XEE01, 0X2EC0, 0X2F80, 0XEF41, 0X2D00, 0XEDC1, 0XEC81, 0X2C40,
            0XE401, 0X24C0, 0X2580, 0XE541, 0X2700, 0XE7C1, 0XE681, 0X2640,
            0X2200, 0XE2C1, 0XE381, 0X2340, 0XE101, 0X21C0, 0X2080, 0XE041,
            0XA001, 0X60C0, 0X6180, 0XA141, 0X6300, 0XA3C1, 0XA281, 0X6240,
            0X6600, 0XA6C1, 0XA781, 0X6740, 0XA501, 0X65C0, 0X6480, 0XA441,
            0X6C00, 0XACC1, 0XAD81, 0X6D40, 0XAF01, 0X6FC0, 0X6E80, 0XAE41,
            0XAA01, 0X6AC0, 0X6B80, 0XAB41, 0X6900, 0XA9C1, 0XA881, 0X6840,
            0X7800, 0XB8C1, 0XB981, 0X7940, 0XBB01, 0X7BC0, 0X7A80, 0XBA41,
            0XBE01, 0X7EC0, 0X7F80, 0XBF41, 0X7D00, 0XBDC1, 0XBC81, 0X7C40,
            0XB401, 0X74C0, 0X7580, 0XB541, 0X7700, 0XB7C1, 0XB681, 0X7640,
            0X7200, 0XB2C1, 0XB381, 0X7340, 0XB101, 0X71C0, 0X7080, 0XB041,
            0X5000, 0X90C1, 0X9181, 0X5140, 0X9301, 0X53C0, 0X5280, 0X9241,
            0X9601, 0X56C0, 0X5780, 0X9741, 0X5500, 0X95C1, 0X9481, 0X5440,
            0X9C01, 0X5CC0, 0X5D80, 0X9D41, 0X5F00, 0X9FC1, 0X9E81, 0X5E40,
            0X5A00, 0X9AC1, 0X9B81, 0X5B40, 0X9901, 0X59C0, 0X5880, 0X9841,
            0X8801, 0X48C0, 0X4980, 0X8941, 0X4B00, 0X8BC1, 0X8A81, 0X4A40,
            0X4E00, 0X8EC1, 0X8F81, 0X4F40, 0X8D01, 0X4DC0, 0X4C80, 0X8C41,
            0X4400, 0X84C1, 0X8581, 0X4540, 0X8701, 0X47C0, 0X4680, 0X8641,
            0X8201, 0X42C0, 0X4380, 0X8341, 0X4100, 0X81C1, 0X8081, 0X4040};

    uint8_t nTemp;
    uint16_t wCRCWord = 0xFFFF;

    while (wLength--)
    {
        nTemp = *nData++ ^ wCRCWord;
        wCRCWord >>= 8;
        wCRCWord ^= wCRCTable[nTemp];
    }
    return wCRCWord;
}

static void Init_Uart()
{

#define BAUD 9600
#include <util/setbaud.h>
    UBRR0H = UBRRH_VALUE;
    UBRR0L = UBRRL_VALUE;
#if USE_2X
    UCSR0A |= (1 << U2X0);
#else
    UCSR0A &= ~(1 << U2X0);
#endif

    UCSR0A |= (1 << UDRE0); // Clear transmit buffer

    UCSR0B = (1 << TXEN0);  // Enable transmitter
    UCSR0B |= (1 << RXEN0); // Enable receiver

    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // Set frame: 8data, 1 stop
}

static uint8_t Uart_Receive(void)
{
    /* Wait for data to be received */
    while (!(UCSR0A & (1 << RXC0)))
        ;
    /* Get and return received data from buffer */
    return UDR0;
}

static void Uart_Transmit(unsigned char data)
{
    // Set DE/RI pin

    /* Wait for empty transmit buffer */
    while (!(UCSR0A & (1 << UDRE0)))
        ;
    UDR0 = data;

    // Clear DE/RI pin
}

static void Uart_Transmit_buffer(uint8_t *data, uint16_t length)
{
    // Set DE/RI pin
    // Light up LED

    for (uint16_t i = 0; i < length; i++)
    {
        /* Wait for empty transmit buffer */
        while (!(UCSR0A & (1 << UDRE0)))
            ;
        UDR0 = *(data + i);
    }

    // Clear DE/RI pin
}

static void USART_transmit_string(char *s)
{
    // transmit character until NULL is reached
    while (*s > 0)
        Uart_Transmit(*s++);
}

static void Send_Response(uint8_t response, uint8_t resp_val)
{
    Uart_Transmit(START_BYTE); // Send start byte
    Uart_Transmit(resp_val);   // Send send response's value
    Uart_Transmit(response);   // Send response
    Uart_Transmit(STOP_BYTE);  // Send Stop byte
}

static void Send_Response_2(uint8_t response, uint8_t arg3, uint8_t arg2, uint8_t arg1, uint8_t arg0)
{
    Uart_Transmit(START_BYTE); // Send start byte
    Uart_Transmit(response);   // Send response
    Uart_Transmit(arg3);       // Send argument
    Uart_Transmit(arg2);       // Send argument
    Uart_Transmit(arg1);       // Send argument
    Uart_Transmit(arg0);       // Send argument
    Uart_Transmit(STOP_BYTE);  // Send Stop byte

    // uint8_t tx[7] = {START_BYTE,response,arg3,arg2,arg1,arg0,STOP_BYTE};
    // Uart_Transmit_buffer(&tx,7);
}

typedef enum Commands
{
    COMMAND_WRITE_FLASH_PAGE = 3,
    COMMAND_READ_FLASH_PAGE = 4,
    COMMAND_READ_FLASH_PAGE_SIZE = 5,

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
    RESPONSE_ERR,
    RESPONSE_BOOTLOADER_STARTED,
    RESPONSE_VALIDATION_FAILED,
    RESPONSE_FLASH_WRITTEN,
    RESPONSE_FLASH_READ,
    RESPONSE_GOING_TO_APP,
    RESPONSE_READING_EEPROM,
    RESPONSE_WRITING_EEPROM,
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

typedef enum
{
    RESPONSE_INDEX_RESPONSE = 0,
    RESPONSE_INDEX_ARGUMENT_3,
    RESPONSE_INDEX_ARGUMENT_2,
    RESPONSE_INDEX_ARGUMENT_1,
    RESPONSE_INDEX_ARGUMENT_0,
} Response_index;

main()
{
    uint8_t sreg = SREG;
    uint16_t page = 0;
    uint8_t checksum = 0;
    uint8_t rx_buffer[3];
    uint8_t rx_data[SPM_PAGESIZE];

    State state = STATE_START;

    cli();
    Init_Uart();

    Send_Response_2(0, 1, 2, 3, 4);

    while (1)
    {
        switch (state)
        {

        case STATE_START:
        {
            Send_Response(RESPONSE_BOOTLOADER_STARTED, 0);
            state = STATE_POOL_REQUEST;
            break;
        }

        case STATE_POOL_REQUEST:
        {
            if (Uart_Receive() == START_BYTE) // Start byte
            {
                rx_buffer[REQUEST_INDEX_COMMAND] = Uart_Receive();    // Command
                rx_buffer[REQUEST_INDEX_ARGUMENT_0] = Uart_Receive(); // Argument_0 - PAGE  3
                rx_buffer[REQUEST_INDEX_ARGUMENT_1] = Uart_Receive(); // Argument_1 - PAGE  2
                rx_buffer[REQUEST_INDEX_ARGUMENT_2] = Uart_Receive(); // Argument_2 - PAGE  1
                rx_buffer[REQUEST_INDEX_ARGUMENT_3] = Uart_Receive(); // Argument_3 - PAGE  0
                rx_buffer[REQUEST_INDEX_ARGUMENT_4] = Uart_Receive(); // Argument_4 - CRC16 1
                rx_buffer[REQUEST_INDEX_ARGUMENT_5] = Uart_Receive(); // Argument_5 - CRC16 0
                // TODO : STOP BYTE

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
                Send_Response(RESPONSE_FLASH_READ, 0);

                // Parse CRC16
                uint16_t crc16;
                crc16 = rx_buffer[REQUEST_INDEX_ARGUMENT_4] << 8;
                crc16 |= rx_buffer[REQUEST_INDEX_ARGUMENT_5];

                // Parse page
                page = rx_buffer[REQUEST_INDEX_ARGUMENT_2] << 8;
                page |= rx_buffer[REQUEST_INDEX_ARGUMENT_3];

                uint16_t i = 0;
                for (uint16_t addr = page; addr < (page + SPM_PAGESIZE); addr++)
                {
                    rx_buffer[i] = pgm_read_byte(addr);
                    Uart_Transmit(rx_buffer[i]);
                    i++;
                }

                uint16_t crc16_calc = CRC16(&rx_data, SPM_PAGESIZE);
                Uart_Transmit(crc16_calc >> 8);
                Uart_Transmit(crc16_calc);

                // Go back pooling requests
                state = STATE_POOL_REQUEST;
                break;
            }

            // Get CRC16, Uart_Transmit, data and write it to flash if it's valid
            case COMMAND_WRITE_FLASH_PAGE:
            {
                Send_Response(RESPONSE_OK, 0);

                // Parse CRC16
                uint16_t crc16_rx;
                crc16_rx = rx_buffer[REQUEST_INDEX_ARGUMENT_4] << 8;
                crc16_rx |= rx_buffer[REQUEST_INDEX_ARGUMENT_5];

                // Parse page
                page = rx_buffer[REQUEST_INDEX_ARGUMENT_2] << 8;
                page |= rx_buffer[REQUEST_INDEX_ARGUMENT_3];
                // Uart_Transmit(page >> 8);
                // Uart_Transmit(page);

                // Receive flash page's data through communication
                for (uint16_t i = 0; i < SPM_PAGESIZE; i++)
                {
                    rx_data[i] = Uart_Receive();
                }

                // Do data validation
                volatile uint16_t crc16_calc = CRC16(&rx_data, SPM_PAGESIZE);

                // Check if received data is valid
                if (crc16_calc != crc16_rx)
                {
                    Send_Response(RESPONSE_VALIDATION_FAILED, 0); // Send when data is received but failed to validate
                    state = STATE_POOL_REQUEST;
                    break;
                }

                // Write data to flash
                boot_page_erase(page); // Do erase before programming
                boot_spm_busy_wait();  // Wait until the memory is written.

                for (uint16_t i = 0; i < SPM_PAGESIZE; i += 2)
                {
                    uint16_t w = (rx_data[i]) | (rx_data[i + 1] << 8);
                    boot_page_fill(page + i, w);
                }

                boot_page_write(page); // Store buffer in flash page.
                boot_spm_busy_wait();  // Wait until the memory is written.
                boot_rww_enable();

                Send_Response(RESPONSE_FLASH_WRITTEN, 0);

                // Go back pooling requests
                state = STATE_POOL_REQUEST;
                break;
            }

            // Send flash pagesize to master
            case COMMAND_READ_FLASH_PAGE_SIZE: //
            {
                // Send pagesize to master [Startbyte MSB LSB Endbyte]
                Uart_Transmit(START_BYTE);
                Uart_Transmit(SPM_PAGESIZE >> 8);
                Uart_Transmit(SPM_PAGESIZE);
                Uart_Transmit(STOP_BYTE);

                // Go back pooling requests
                state = STATE_POOL_REQUEST;
                break;
            }

            // Run main application
            case COMMAND_GO_TO_APPLICATION:
            {
                Send_Response(RESPONSE_GOING_TO_APP, 0);

                SREG = sreg;

                asm("jmp 0");

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

                Send_Response(RESPONSE_WRITING_EEPROM, 0);

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
                uint8_t read = eeprom_read_byte(address);
                Send_Response(RESPONSE_READING_EEPROM, read);

                // Go back pooling requests
                state = STATE_POOL_REQUEST;
                break;
            }

            case COMMAND_READ_DEVICE_SIGNATURE:
            {
                Uart_Transmit(START_BYTE);
                Uart_Transmit(boot_signature_byte_get(0x00)); // Device signature 1
                Uart_Transmit(boot_signature_byte_get(0x02)); // Device signature 2
                Uart_Transmit(boot_signature_byte_get(0x04)); // Device signature 3
                // Uart_Transmit(boot_signature_byte_get(0x01)); // RC calibration value
                // Uart_Transmit(boot_signature_byte_get(0x02)); // Temperature sensor offset
                // Uart_Transmit(boot_signature_byte_get(0x03)); // Temperature sensor gain
                Uart_Transmit(STOP_BYTE);

                // Go back pooling requests
                state = STATE_POOL_REQUEST;
                break;
            }

            default:
                Send_Response(RESPONSE_ERR, RESPONSE_ERR);
                break;
            }
            break;
            // break;
        }

        default:
        {
            Send_Response(RESPONSE_ERR, 0xee);
            break;
        }
        }
    }
}