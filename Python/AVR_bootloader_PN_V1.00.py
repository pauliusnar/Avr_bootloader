from termcolor import colored
import serial
import time
import sys
import os
import codecs
from datetime import datetime
import serial.tools.list_ports
import numpy as np
os.system('color')
# region COLORED TERMINAL


class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'


os.system('color')

# endregion

# region PACKET STATISTICS
Packet_good = 0
Packet_bad = 0
Packet_total = 0
Packet_total_session = 0
# endregion

# region FIND AND CONNECT
ports = serial.tools.list_ports.comports()
# print([port.name for port in ports])
portname = ""

for port in ports:
    # print(port.device)
    if(port.hwid == "USB VID:PID=0403:6001 SER=SWITCHA"):
        portname = port.device
        print("--Connected to a device " + port.device +
              " " + port.hwid + " " + port.manufacturer)

if(portname == ""):
    print("--Device not found")
    for port in ports:
        print("--Available devices: '" + port.device + "' '" +
              port.hwid + "' '" + port.manufacturer+"'")
    os.exit()

ser = serial.Serial()
ser.baudrate = 9600
ser.dsrdtr = False
ser.rtscts = False
ser.port = portname
ser.stopbits = serial.STOPBITS_ONE
ser.parity = serial.PARITY_NONE
ser.open()

if(ser.is_open == True):
    print("--Com port opened")
    ser.flushInput()
else:
    print("--Can't open COM port")


# endregion

# region PACKET VALIDATION
def CRC16_calculate(data):
    length = len(data)
    wCRCTable = [0X0000, 0XC0C1, 0XC181, 0X0140, 0XC301, 0X03C0, 0X0280, 0XC241, 0XC601, 0X06C0, 0X0780, 0XC741, 0X0500, 0XC5C1, 0XC481, 0X0440, 0XCC01, 0X0CC0, 0X0D80, 0XCD41, 0X0F00, 0XCFC1, 0XCE81, 0X0E40, 0X0A00, 0XCAC1, 0XCB81, 0X0B40, 0XC901, 0X09C0, 0X0880, 0XC841, 0XD801, 0X18C0, 0X1980, 0XD941, 0X1B00, 0XDBC1, 0XDA81, 0X1A40, 0X1E00, 0XDEC1, 0XDF81, 0X1F40, 0XDD01, 0X1DC0, 0X1C80, 0XDC41, 0X1400, 0XD4C1, 0XD581, 0X1540, 0XD701, 0X17C0, 0X1680, 0XD641, 0XD201, 0X12C0, 0X1380, 0XD341, 0X1100, 0XD1C1, 0XD081, 0X1040, 0XF001, 0X30C0, 0X3180, 0XF141, 0X3300, 0XF3C1, 0XF281, 0X3240, 0X3600, 0XF6C1, 0XF781, 0X3740, 0XF501, 0X35C0, 0X3480, 0XF441, 0X3C00, 0XFCC1, 0XFD81, 0X3D40, 0XFF01, 0X3FC0, 0X3E80, 0XFE41, 0XFA01, 0X3AC0, 0X3B80, 0XFB41, 0X3900, 0XF9C1, 0XF881, 0X3840, 0X2800, 0XE8C1, 0XE981, 0X2940, 0XEB01, 0X2BC0, 0X2A80, 0XEA41, 0XEE01, 0X2EC0, 0X2F80, 0XEF41, 0X2D00, 0XEDC1, 0XEC81, 0X2C40, 0XE401, 0X24C0, 0X2580, 0XE541, 0X2700, 0XE7C1, 0XE681, 0X2640, 0X2200, 0XE2C1, 0XE381, 0X2340, 0XE101, 0X21C0, 0X2080,
                 0XE041, 0XA001, 0X60C0, 0X6180, 0XA141, 0X6300, 0XA3C1, 0XA281, 0X6240, 0X6600, 0XA6C1, 0XA781, 0X6740, 0XA501, 0X65C0, 0X6480, 0XA441, 0X6C00, 0XACC1, 0XAD81, 0X6D40, 0XAF01, 0X6FC0, 0X6E80, 0XAE41, 0XAA01, 0X6AC0, 0X6B80, 0XAB41, 0X6900, 0XA9C1, 0XA881, 0X6840, 0X7800, 0XB8C1, 0XB981, 0X7940, 0XBB01, 0X7BC0, 0X7A80, 0XBA41, 0XBE01, 0X7EC0, 0X7F80, 0XBF41, 0X7D00, 0XBDC1, 0XBC81, 0X7C40, 0XB401, 0X74C0, 0X7580, 0XB541, 0X7700, 0XB7C1, 0XB681, 0X7640, 0X7200, 0XB2C1, 0XB381, 0X7340, 0XB101, 0X71C0, 0X7080, 0XB041, 0X5000, 0X90C1, 0X9181, 0X5140, 0X9301, 0X53C0, 0X5280, 0X9241, 0X9601, 0X56C0, 0X5780, 0X9741, 0X5500, 0X95C1, 0X9481, 0X5440, 0X9C01, 0X5CC0, 0X5D80, 0X9D41, 0X5F00, 0X9FC1, 0X9E81, 0X5E40, 0X5A00, 0X9AC1, 0X9B81, 0X5B40, 0X9901, 0X59C0, 0X5880, 0X9841, 0X8801, 0X48C0, 0X4980, 0X8941, 0X4B00, 0X8BC1, 0X8A81, 0X4A40, 0X4E00, 0X8EC1, 0X8F81, 0X4F40, 0X8D01, 0X4DC0, 0X4C80, 0X8C41, 0X4400, 0X84C1, 0X8581, 0X4540, 0X8701, 0X47C0, 0X4680, 0X8641, 0X8201, 0X42C0, 0X4380, 0X8341, 0X4100, 0X81C1, 0X8081, 0X4040]

    nTemp = 0
    wCRCWord = 0xFFFF

    for i in range(length):
        nTemp = np.uint8(data[i] ^ wCRCWord)
        wCRCWord = wCRCWord >> 8
        wCRCWord ^= wCRCTable[nTemp]
    return wCRCWord
# endregion


def Send_Command(Command, arg0, arg1, arg2, arg3, arg4, arg5):
    packet = bytearray()
    packet.append(STARTBYTE)  # Sync-start byte
    packet.append(Command)  # Command
    packet.append(arg0)  # Argument 0
    packet.append(arg1)  # Argument 1
    packet.append(arg2)  # Argument 2
    packet.append(arg3)  # Argument 3
    packet.append(arg4)  # Argument 4
    packet.append(arg5)  # Argument 5
    ser.write(packet)
    while (ser.out_waiting > 0):
        pass
    time.sleep(STANDART_SLEEP)


def Parse_Response(input):
    print("response")


def Unit_Test():
    test_ok = 0
    test_err = 0

    # ------ PAGESIZE
    # mystring = ''
    # if (data[2] == COMM_ADDRESS_PC):
    #     mystring = bcolors.WARNING
    # else:
    #     mystring = bcolors.OKBLUE

    # width = 15
    # mystring = mystring + '{0: >{width}}'.format(comment, width=width)
    # mystring = mystring + '{0: >{width}}'.format(str(data[2]), width=width)
    # mystring = mystring + '{0: >{width}}'.format(str(data[3]), width=width)
    # mystring = mystring + '{0: >{width}}'.format(str(data[4]), width=width)
    # mystring = mystring + '{0: >{width}}'.format(str(data[5]), width=width)
    # mystring = mystring + '{0: >{width}}'.format(str(data[6]), width=width)
    # mystring = mystring + '{0: >{width}}'.format(str(data[7]), width=width)
    # mystring = mystring + bcolors.ENDC

    testvar = Get_Page_Size(0)
    print("Pagesize :", end='')
    if(testvar == 128):
        print("OK")
        test_ok = test_ok + 1
    else:
        print("ERROR - " + str(testvar))
        test_err + 1


pagesize = 128
STARTBYTE = 0x55

COMMAND_GET_VERSION = 0
COMMAND_GET_CHIP = 1
COMMAND_GET_MODULE_NAME = 2
COMMAND_WRITE_FLASH_PAGE = 3
COMMAND_READ_FLASH_PAGE = 4
COMMAND_READ_FLASH_PAGE_SIZE = 5
COMMAND_WRITE_EEPROM_BYTE = 6
COMMAND_READ_EEPROM_BYTE = 7
COMMAND_READ_DEVICE_SIGNATURE = 8
COMMAND_GO_TO_APPLICATION = 200


STANDART_SLEEP = 0.5


def Get_Page_Size(echo=1):
    # print(ser.read_all())
    Send_Command(COMMAND_READ_FLASH_PAGE_SIZE, 0, 0, 0, 0, 0, 0)
    # print(ser.read_all())
    myvar = ser.read_all()
    if(len(myvar)>=2):
        result = myvar[2] + (myvar[1] << 8)
        if echo:
            print("-Request - Get page size = " + str(result) + "bytes")
    else:
        if echo:
            print("-Request - Get page size = FAIL")
        return -1
   
    return result


def Go_To_App(echo=1):

    Send_Command(COMMAND_GO_TO_APPLICATION, 0, 0, 0, 0, 0, 0)

    var = ser.read_all()
    print("-GO_TO_APP_RESPONSE =" + str(var))


def Write_To_Flash(page_address, input, echo=1):

    # Collect program data
    length = len(input)
    packet = bytearray()
    for i in range(0, length):
        packet.append(input[i])
    for i in range(length, pagesize):
        packet.append(0xFF)

    # Calculate CRC16
    CRC16 = CRC16_calculate(packet)

    # Send command
    Send_Command(COMMAND_WRITE_FLASH_PAGE, 0, 0, (page_address >> 8),
                 (page_address & 0xFF), (CRC16 >> 8), CRC16 & 0xFF)

    # Receive response
    myvar = ser.read_all()
    if echo:
        print("-Request - Write_To_Flash = " + str(myvar))


    # Send program data
    ser.write(packet)
    while (ser.out_waiting > 0):
        pass
    time.sleep(1)

    # Print response
    print("- Write_To_Flash response -",end=' ')
    print(" ".join(hex(n) for n in ser.read_all()))


def Read_Eeprom_Byte(address, echo=1):
    Send_Command(COMMAND_READ_EEPROM_BYTE, 0, 0,
                 (address >> 8), (address & 0xFF), 0, 0)
    var = ser.read_all()
    if(echo == 1):
        print("-Read eeprom response =" + str(var))


def Write_Eeprom_Byte(address, w_byte, echo=1):
    Send_Command(COMMAND_WRITE_EEPROM_BYTE, 0, 0,
                 (address >> 8), (address & 0xFF), 0, w_byte)
    var = ser.read_all()
    if(echo == 1):
        print("-Write eeprom response =" + str(var))


def Read_Flash_Page(address, echo=1):
    Send_Command(COMMAND_READ_FLASH_PAGE, 0, 0,
                 (address >> 8), (address & 0xFF), 0, 0)
    var = ser.read_all()
    if(echo == 1):
        print("-Read flash response =" + str(var))


def Read_Device_signature(echo=1):
    Send_Command(COMMAND_READ_DEVICE_SIGNATURE, 0, 0, 0, 0, 0, 0)
    var = ser.read_all()
    if(len(var) >=3):
        if(echo == 1):
            print("-COMMAND_READ_DEVICE_SIGNATURE response =" + str(var))
    else:
        if(echo == 1):
            print("-COMMAND_READ_DEVICE_SIGNATURE response = FAIL")
        return -1


def Program_Eeprom_Configuration(isProgrammed, ModuleName, FirmwareVersion, PowerLimit, ActuatorCount, CommunicationErrors):
    print("-Write device configuration to EEPROM")
    # Programmed byte
    Write_Eeprom_Byte(0x00, isProgrammed, 0)  # programmed
    print("-Programmed byte   OK")

    # Module name
    if(len(ModuleName) != 4):
        print("Module name parsing error")
        return -1
    for i in range(4):
        # mychar = ModuleName[i]
        number = int(np.fromstring(ModuleName[i], dtype=np.uint8))
        Write_Eeprom_Byte(i+1, number, 0)  # programmed
        print("-Debug module name =" + str(number))
    print("-Module name       OK")

    # Module version
    FirmwareVersion = (int)(FirmwareVersion * 1000)
    Firmware_High = (int)(FirmwareVersion - (int)(FirmwareVersion)) # x.100
    Firmware_Low = (int)(FirmwareVersion%1000)
    Write_Eeprom_Byte(5, Firmware_High, 0)
    Write_Eeprom_Byte(6, Firmware_Low, 0)
    print("-Version           OK")

    # Power limit set
    Write_Eeprom_Byte(7, PowerLimit >> 8, 0)
    Write_Eeprom_Byte(8, PowerLimit & 0xFF, 0)
    print("-Power limit       OK")

    # Actuator move count
    Write_Eeprom_Byte(0x0010, (ActuatorCount >> 24) & 0xFF, 0)
    Write_Eeprom_Byte(0x0011, (ActuatorCount >> 16) & 0xFF, 0)
    Write_Eeprom_Byte(0x0012, (ActuatorCount >> 8) & 0xFF, 0)
    Write_Eeprom_Byte(0x0013, (ActuatorCount >> 0) & 0xFF, 0)
    print("-Actuator count    OK")

    # Communication errors
    Write_Eeprom_Byte(0x0020, (ActuatorCount >> 24) & 0xFF, 0)
    Write_Eeprom_Byte(0x0021, (ActuatorCount >> 16) & 0xFF, 0)
    Write_Eeprom_Byte(0x0022, (ActuatorCount >> 8) & 0xFF, 0)
    Write_Eeprom_Byte(0x0023, (ActuatorCount >> 0) & 0xFF, 0)
    print("-Communication errors OK")


# Read program file
program_str = ""
with open("main.bin", "rb") as f:
    program_str = f.read()
    print("Program size = "+str(len(program_str)))

# Write to separate pages
page = list()
pages_total = (int)(len(program_str) / pagesize)
if((len(program_str) % pagesize != 0)):
    pages_total = pages_total + 1
for i in range(0, pages_total*pagesize, pagesize):
    page.append(program_str[i:i+pagesize])

# Print CRC values of different pages
# print("Calculating CRC for the data")
# print('{:>10} {:>30}'.format("Page", "CRC 16"))
crc16_values = list()
for i in range(0, pages_total):
    crc16_values.append(CRC16_calculate(page[i]))
    # print('{:>8} {:>32}'.format(i, hex(crc16_values[i])))


# Read initial buffer
time.sleep(STANDART_SLEEP)
print("- Starting buffer= [start] " + str(ser.read_all()))



# Write_Eeprom_Byte(0x100,0xAD,1)
# Read_Eeprom_Byte(0x100,1)


# Unit_Test()
# Write_Eeprom_Byte(0x020, 0xFF, 0)
# Write_Eeprom_Byte(0x021, 0xFF, 0)
# Write_Eeprom_Byte(0x022, 0xFF, 0)
# Write_Eeprom_Byte(0x023, 0xFF, 0)

# Program_Eeprom_Configuration(1, 'STPF', 0.100 , 255, 1, 0)
# Program_Eeprom_Configuration(1, 'STPB', 0.100 , 255, 1, 0)
# Program_Eeprom_Configuration(1, 'WIN1', 0.100 , 255, 1, 0)
# Program_Eeprom_Configuration(1, 'WIN2', 0.100 , 255, 1, 0)
# Program_Eeprom_Configuration(1, 'WIN3', 0.100 , 255, 1, 0)


Read_Device_signature(1)
Get_Page_Size(1)

for i in range(pages_total):
    time.sleep(STANDART_SLEEP)
    Write_To_Flash(i*pagesize, page[i], 1)

# Read_Flash_Page(0,1)


# Go_To_App()


# * END --------------------------------------------------------------------------------------------------------------------------

# mystring = ''
# if (data[2] == COMM_ADDRESS_PC):
#     mystring = bcolors.WARNING
# else:
#     mystring = bcolors.OKBLUE

# width = 15
# mystring = mystring + '{0: >{width}}'.format(comment, width=width)
# mystring = mystring + '{0: >{width}}'.format(str(data[2]), width=width)
# mystring = mystring + '{0: >{width}}'.format(str(data[3]), width=width)
# mystring = mystring + '{0: >{width}}'.format(str(data[4]), width=width)
# mystring = mystring + '{0: >{width}}'.format(str(data[5]), width=width)
# mystring = mystring + '{0: >{width}}'.format(str(data[6]), width=width)
# mystring = mystring + '{0: >{width}}'.format(str(data[7]), width=width)
# mystring = mystring + bcolors.ENDC
