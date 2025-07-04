import serial
import argparse
import seaport as sp
import time
import math
import timeit


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
    def loop_body():
        nonlocal t
        for i in range(8):
            motors[i] = math.sin(t + i)
        # seaport.publish(1, {str(i): motors[i] for i in range(8)})
        seaport.publish(254, {'cmd': 'ping'})
        t += 0.02
        # time.sleep(0.001)

    # Benchmark the loop over 100 iterations
    start_time = time.time()
    for _ in range(1000):
        loop_body()
    end_time = time.time()
    print(f"1000 iterations took {end_time - start_time:.6f} seconds")

    # Continue with the infinite loop if needed
    # while True:
    #     loop_body()
    
        # seaport.publish(254, {'cmd': 'ping'})
        
        # # # print out hello every 100 loops
        # iter = iter + 1
        # if (iter % 100 == 0):
        #     seaport.publish(254, {'cmd': "get_heap_info"})
        


if __name__ == "__main__":
    main()

