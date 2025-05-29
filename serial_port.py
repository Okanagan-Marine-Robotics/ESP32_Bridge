import serial
import argparse
from python_library.pubsub import Subscriber, Publisher
import time
# import gui libraries
import tkinter as tk
import threading

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
    # time.sleep(1)  # Simulate some processing delay
    print("Sending ping back")
    publisher.publish(1, {"data": "ping", "timestamp": time.time()})

def received(data):
    print(f"Data received: {data}")



def main():
    parser = argparse.ArgumentParser(description="Serial port tester")
    parser.add_argument("serial_port", help="Serial port to open (e.g., /dev/ttyUSB0 or COM3)")
    parser.add_argument("baudrate", type=int, help="Baud rate (e.g., 115200)")
    args = parser.parse_args()

    publisher = Publisher(serial.Serial(args.serial_port, args.baudrate, timeout=1))

    data_generator = serial_gen(args.serial_port, args.baudrate)
    subscriber = Subscriber(data_generator)
    subscriber.subscribe(1, lambda data: ping(data, publisher))
    subscriber.subscribe(2, lambda data: received(data))

    # create a simple GUI to allow for a user to send commands
    root = tk.Tk()
    root.title("Serial Port Tester")
    root.geometry("600x400")
    label = tk.Label(root, text="Press Ctrl+C to exit")
    label.pack(pady=20)
    button = tk.Button(root, text="Send Ping", command=lambda: publisher.publish(1, {"data": "ping"}))
    button.pack(pady=10)

    # add a slider that sends signals to the serial port when moved
    slider = tk.Scale(root, from_=0, to=100, orient=tk.HORIZONTAL, label="Motor Speed")
    slider.pack(pady=10)

    def on_slider_change(value):
        publisher.publish(3, {"speed": int(value)})
        print(f"Motor speed set to: {value}")

    slider.config(command=on_slider_change)

    


    def on_close():
        print("Closing application...")
        root.destroy()
        exit(0)

    def subscriber_loop():
        while True:
            try:
                subscriber.feed()
            except KeyboardInterrupt:
                print("Exiting subscriber thread...")
                break
            except Exception as e:
                print(f"Error in Subscriber run loop: {e}")
                continue

    subscriber_thread = threading.Thread(target=subscriber_loop, daemon=True)
    subscriber_thread.start()

    root.protocol("WM_DELETE_WINDOW", on_close)
    root.mainloop()

if __name__ == "__main__":
    main()

