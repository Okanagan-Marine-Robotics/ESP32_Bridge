import serial
import argparse
from seaport import SeaPort

# import gui libraries
import tkinter as tk
import threading
from tkinter import ttk


def received(data):
    print(f"Data received: {data}")
    # check if the data contains tasks key
    if "tasks" in data:
        print("Tasks received, updating water level...")
        get_water_level(data)  # call the function to handle water level display

    
# create a new function to handle get_water_level command
# this should open a new window and display the task names and their memory usage
def get_water_level(data):
    # open a new window
    water_level_window = tk.Toplevel()
    water_level_window.title("Water Level")
    water_level_window.geometry("1000x600")
    label = tk.Label(water_level_window, text="Task Memory Usage")
    label.pack(pady=10)
    columns = ("Task Name", "Memory Remaining (bytes)")
    tree = ttk.Treeview(
        water_level_window,
        columns=columns,
        show="headings",
        height=10,
        style="Custom.Treeview"
    )
    tree.heading("Task Name", text="Task Name")
    tree.heading("Memory Remaining (bytes)", text="Memory Remaining (bytes)")
    tree.column("Task Name", anchor=tk.W, width=240, minwidth=200, stretch=True)
    tree.column("Memory Remaining (bytes)", anchor=tk.CENTER, width=180, minwidth=150, stretch=True)

    # Enable sorting by clicking the column header
    def treeview_sort_column(tv, col, reverse):
        l = [(tv.set(k, col), k) for k in tv.get_children('')]
        # Convert to int for sorting if sorting the memory column
        if col == "Memory Remaining (bytes)":
            l.sort(key=lambda t: int(t[0]), reverse=reverse)
        else:
            l.sort(reverse=reverse)
        for index, (val, k) in enumerate(l):
            tv.move(k, '', index)
        # Reverse sort next time
        tv.heading(col, command=lambda: treeview_sort_column(tv, col, not reverse))

    # Attach sorting to the memory column
    tree.heading("Memory Remaining (bytes)", text="Memory Remaining (bytes)", command=lambda: treeview_sort_column(tree, "Memory Remaining (bytes)", False))

    # Add striped row tags for better readability
    for idx, task in enumerate(data.get("tasks", [])):
        tag = "evenrow" if idx % 2 == 0 else "oddrow"
        tree.insert("", tk.END, values=(task['name'], task['stack_high_water_mark']), tags=(tag,))

    tree.pack(expand=True, fill=tk.BOTH, padx=10, pady=10)

    # Style for grid lines and padding
    style = ttk.Style()
    style.configure("Custom.Treeview.Heading", font=("Arial", 12, "bold"))
    style.configure("Custom.Treeview", font=("Arial", 11), rowheight=28, borderwidth=1)
    style.map("Custom.Treeview", background=[("selected", "#ececec")])
    style.layout("Custom.Treeview", [
        ('Treeview.treearea', {'sticky': 'nswe'})
    ])
    style.configure("Custom.Treeview", bordercolor="#cccccc", relief="solid")
    style.configure("Custom.Treeview", highlightthickness=1)
    style.configure("Custom.Treeview", borderwidth=1)
    style.configure("Custom.Treeview", background="#f9f9f9", fieldbackground="#f9f9f9")
    style.configure("Custom.Treeview", foreground="#222222")
    style.configure("Custom.Treeview", rowheight=28)
    style.map("Custom.Treeview", background=[('selected', '#d1eaff')])

    # Add striped rows
    style.configure("Treeview", rowheight=28)
    tree.tag_configure("evenrow", background="#f0f0f0")
    tree.tag_configure("oddrow", background="#ffffff")

    def update_striped_rows():
        for idx, item in enumerate(tree.get_children()):
            tag = "evenrow" if idx % 2 == 0 else "oddrow"
            tree.item(item, tags=(tag,))

    # Patch sorting to update stripes after sort
    def treeview_sort_column(tv, col, reverse):
        l = [(tv.set(k, col), k) for k in tv.get_children('')]
        if col == "Memory Remaining (bytes)":
            l.sort(key=lambda t: int(t[0]), reverse=reverse)
        else:
            l.sort(reverse=reverse)
        for index, (val, k) in enumerate(l):
            tv.move(k, '', index)
        tv.heading(col, command=lambda: treeview_sort_column(tv, col, not reverse))
        update_striped_rows()

    # Attach sorting to the memory column
    tree.heading("Memory Remaining (bytes)", text="Memory Remaining (bytes)", command=lambda: treeview_sort_column(tree, "Memory Remaining (bytes)", False))

    update_striped_rows()

    close_button = tk.Button(water_level_window, text="Close", command=water_level_window.destroy)
    close_button.pack(pady=10)
       

def main():
    parser = argparse.ArgumentParser(description="Serial port tester")
    parser.add_argument("serial_port", help="Serial port to open (e.g., /dev/ttyUSB0 or COM3)")
    parser.add_argument("baudrate", type=int, help="Baud rate (e.g., 115200)")
    args = parser.parse_args()
    
    ser = serial.Serial(args.serial_port, args.baudrate, timeout=1)
    seaport = SeaPort(ser)
    seaport.subscribe(254, lambda data: received(data), debug=True)

    # create a simple GUI to allow for a user to send commands
    root = tk.Tk()
    root.title("Serial Port Tester")
    root.geometry("700x1200")
    label = tk.Label(root, text="Press Ctrl+C to exit")
    label.pack(pady=20)
    frame = tk.Frame(root)
    frame.pack(pady=10)

    print(f"Sending to motor: {motor_index}, value: {value}")

    button_ping = tk.Button(frame, text="Send Ping", command=lambda: seaport.publish(254, {"cmd": "ping"}))
    button_ping.pack(side=tk.LEFT, padx=5)

    button_mem = tk.Button(frame, text="Get Task Memory Usage", command=lambda: seaport.publish(254, {"cmd": "get_water_level"}))
    button_mem.pack(side=tk.LEFT, padx=5)

    # add a slider that sends signals to the serial port when moved
    sliders = []
    for i in range(8):
        slider = tk.Scale(root, from_=-1, to=1, orient=tk.HORIZONTAL, label=f"Motor Speed {i+1}", resolution=0.01, length=600, width=30)
        slider.pack(pady=5)
        sliders.append(slider)

    def on_slider_change_factory(motor_index):
        def on_slider_change(value):
            seaport.publish(1, {str(motor_index) : float(value)})
            print(f"Motor {motor_index + 1} speed set to: {value}")
        return on_slider_change

    for idx, slider in enumerate(sliders):
        slider.config(command=on_slider_change_factory(idx))    


    def on_close():
        print("Closing application...")
        seaport.stop()
        root.destroy()
        exit(0)

    seaport.start()

    root.protocol("WM_DELETE_WINDOW", on_close)
    root.mainloop()

if __name__ == "__main__":
    main()

