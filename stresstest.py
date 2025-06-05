import serial
import argparse
import seaport as sp
import time
import math


def received(data):
    print(f"Data received: {data}")

def main():
    parser = argparse.ArgumentParser(description="Serial port tester")
    parser.add_argument("serial_port", help="Serial port to open (e.g., /dev/ttyUSB0 or COM3)")
    parser.add_argument("baudrate", type=int, help="Baud rate (e.g., 115200)")
    args = parser.parse_args()
    
    ser = serial.Serial(args.serial_port, args.baudrate, timeout=1)

    seaport = sp.SeaPort(ser)
    
    seaport.subscribe(254, lambda data: received(data))
    seaport.start()

    print("Press Ctrl+C to exit")
    
    motors = [0.0] * 8
    t = 0.0
    
    iter = 0
    while True:
        # Lerp motors between -1 and 1 using a sine wave
        # for i in range(8):
        #     motors[i] = math.sin(t + i)  # smoothly oscillates between -1 and 1
        # seaport.publish(1, {str(i): motors[i] for i in range(8)})
        # t += 0.01
    
        seaport.publish(254, {'cmd': 'ping'})
        
        # # print out hello every 100 loops
        # iter = iter + 1
        # if (iter % 100 == 0):
        #     seaport.publish(254, {'cmd': "get_water_level"})
        


if __name__ == "__main__":
    main()

