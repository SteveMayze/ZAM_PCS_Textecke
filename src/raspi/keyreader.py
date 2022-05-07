import readchar
import serial
from time import sleep

print("Starting keyreader...")
ascii = ["NULL", "SOH", "STX", "EXT", "EOT", "ENQ", "ACK", "BEL", "BS", "HT", "LF", "VT", "FF", "CR", "SO", "SI", "DLE", "DC1", "DC2", "DC3", "DC4", "NAK", "SYN", "ETB", "CAN", "EM", "SUB", "ESC", "FS", "GS", "RS", "US", ]
try:
    ser = serial.Serial("/dev/ttyS0", 9600)
    while True:
        ch = readchar.readchar()
        if ord(ch) < 32:
            disp_ch = ascii[ord(ch)]
        elif ord(ch) == 0x7F:
            disp_ch = "DEL"
        else:
            disp_ch = ch
            
        if ord(ch) == 0x03:
            print("Ending")
            break
        print(f"Sending {disp_ch} - {ord(ch):02X}")
        ser.write(ch.encode())
except KeyboardInterrupt:
    print("Ending")
    
    