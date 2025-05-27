import serial
import argparse
from python_library.pubsub import Subscriber, Publisher
import time

def serial_gen(port, baudrate):
    ser = serial.Serial(port, baudrate, timeout=1)
    try:
        while True:
            if ser.in_waiting:
                yield ser.read(ser.in_waiting)
    finally:
        ser.close()

def ping(data, publisher):
    print(f"Pong received: {data}")
    time.sleep(1)  # Simulate some processing delay
    print("Sending ping back")
    publisher.publish(1, {"data": "ping"})


def main():
    parser = argparse.ArgumentParser(description="Serial port tester")
    parser.add_argument("serial_port", help="Serial port to open (e.g., /dev/ttyUSB0 or COM3)")
    parser.add_argument("baudrate", type=int, help="Baud rate (e.g., 115200)")
    args = parser.parse_args()

    publisher = Publisher(serial.Serial(args.serial_port, args.baudrate, timeout=1))

    data_generator = serial_gen(args.serial_port, args.baudrate)
    subscriber = Subscriber(data_generator)
    subscriber.subscribe(1, lambda data: ping(data, publisher))

    publisher.publish(1, {"data": "ping"})

    while True:
        try:
            subscriber.feed()


        except KeyboardInterrupt:
            print("Exiting...")
            break
        except Exception as e:
            print(f"Error in Subscriber run loop: {e}")
            continue

if __name__ == "__main__":
    main()

