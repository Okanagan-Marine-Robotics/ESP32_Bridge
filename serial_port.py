import serial
import argparse
from seaport import Publisher, Subscriber


# print the location of seaport.py on the filesystem
import os
import importlib.util
spec = importlib.util.find_spec("seaport")
if spec and spec.origin:
    print(f"seaport.py is located at: {spec.origin}")
else:
    print("seaport module not found.")

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
    subscriber.subscribe(254, lambda data: received(data), debug=True)

    # create a simple GUI to allow for a user to send commands
    root = tk.Tk()
    root.title("Serial Port Tester")
    root.geometry("700x1200")
    label = tk.Label(root, text="Press Ctrl+C to exit")
    label.pack(pady=20)
    button = tk.Button(root, text="Send Ping", command=lambda: publisher.publish(254, {"command": "ping"}))
    button.pack(pady=10)

    # add a slider that sends signals to the serial port when moved
    sliders = []
    for i in range(8):
        slider = tk.Scale(root, from_=-1, to=1, orient=tk.HORIZONTAL, label=f"Motor Speed {i+1}", resolution=0.01, length=600, width=30)
        slider.pack(pady=5)
        sliders.append(slider)

    def on_slider_change_factory(motor_index):
        def on_slider_change(value):
            publisher.publish(1, {str(motor_index) : float(value)})
            print(f"Motor {motor_index + 1} speed set to: {value}")
        return on_slider_change

    for idx, slider in enumerate(sliders):
        slider.config(command=on_slider_change_factory(idx))    


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

