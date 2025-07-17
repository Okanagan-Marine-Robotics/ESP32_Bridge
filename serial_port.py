import serial
import argparse
from seaport import SeaPort
import time
from collections import defaultdict, deque
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.figure import Figure
import matplotlib.dates as mdates
from datetime import datetime, timedelta
import matplotlib.animation as animation

# import gui libraries
import tkinter as tk
import threading
from tkinter import ttk


# Global storage for sensor data
sensor_data = defaultdict(lambda: {
    'temperature': deque(maxlen=100),
    'humidity': deque(maxlen=100),
    'pressure': deque(maxlen=100),
    'timestamps': deque(maxlen=100)
})

# Global variables for real-time plotting
realtime_windows = {}  # Track active realtime windows
update_interval = 1000  # Update interval in milliseconds

def received(data):
    print(f"Data received: {data}")
    # check if the data contains tasks key
    if "tasks" in data:
        print("Tasks received, updating water level...")
        get_water_level(data)  # call the function to handle water level display
        
    if "free_heap" in data:
        print("Heap info received, updating heap info...")
        get_heap_info(data)

def sensor_data_received(data):
    """Handle incoming sensor data from channel 2"""
    print(f"Sensor data received: {data}")
    
    # Expected format: {"address": 0, "temperature": 25.5, "humidity": 60.2, "pressure": 1013.25}
    if all(key in data for key in ["a", "t", "h", "p"]):
        address = data["a"]
        timestamp = datetime.now()
        
        # Store the data
        sensor_data[address]['temperature'].append(data["t"])
        sensor_data[address]['humidity'].append(data["h"])
        sensor_data[address]['pressure'].append(data["p"])
        sensor_data[address]['timestamps'].append(timestamp)
        
        print(f"Stored data for address {address}: T={data['t']}°C, H={data['h']}%, P={data['p']} Pa")
        
        # Update any active realtime windows
        update_realtime_displays()

def update_realtime_displays():
    """Update all active realtime display windows"""
    for window_id, window_data in realtime_windows.items():
        if window_data['active']:
            # Schedule update on the main thread
            if 'update_function' in window_data:
                window_data['root'].after_idle(window_data['update_function'])

class RealtimeChart:
    def __init__(self, parent_window):
        self.parent_window = parent_window
        self.fig = Figure(figsize=(12, 8), dpi=100)
        self.fig.suptitle("Real-time Sensor Data", fontsize=16)
        
        # Create subplots
        self.ax1 = self.fig.add_subplot(311)
        self.ax2 = self.fig.add_subplot(312)
        self.ax3 = self.fig.add_subplot(313)
        
        # Colors for different addresses
        self.colors = ['blue', 'red', 'green', 'orange', 'purple', 'brown', 'pink', 'gray']
        
        # Setup axes
        self.setup_axes()
        
        # Create canvas
        self.canvas = FigureCanvasTkAgg(self.fig, parent_window)
        self.canvas.draw()
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)
        
        # Start animation
        self.animation = animation.FuncAnimation(
            self.fig, self.update_plot, interval=1000, blit=False, cache_frame_data=False
        )
        
    def setup_axes(self):
        """Configure the axes"""
        self.ax1.set_ylabel('Temperature (°C)')
        self.ax1.grid(True, alpha=0.3)
        self.ax1.set_title('Temperature')
        
        self.ax2.set_ylabel('Humidity (%)')
        self.ax2.grid(True, alpha=0.3)
        self.ax2.set_title('Humidity')
        
        self.ax3.set_ylabel('Pressure (hPa)')
        self.ax3.set_xlabel('Time')
        self.ax3.grid(True, alpha=0.3)
        self.ax3.set_title('Pressure')
        
        # Format x-axis
        for ax in [self.ax1, self.ax2, self.ax3]:
            ax.xaxis.set_major_formatter(mdates.DateFormatter('%H:%M:%S'))
            ax.xaxis.set_major_locator(mdates.SecondLocator(interval=30))
    
    def update_plot(self, frame):
        """Update the plot with new data"""
        # Clear previous plots
        self.ax1.clear()
        self.ax2.clear()
        self.ax3.clear()
        
        # Setup axes again
        self.setup_axes()
        
        # Plot data for each address
        for idx, (address, data) in enumerate(sensor_data.items()):
            if len(data['timestamps']) > 0:
                color = self.colors[idx % len(self.colors)]
                label_suffix = " (Built-in)" if address == 0 else f" (Board {address})"
                
                # Temperature plot
                self.ax1.plot(data['timestamps'], data['temperature'], 
                            color=color, marker='o', markersize=2, linewidth=1.5,
                            label=f"Address {address}{label_suffix}")
                
                # Humidity plot
                self.ax2.plot(data['timestamps'], data['humidity'], 
                            color=color, marker='s', markersize=2, linewidth=1.5,
                            label=f"Address {address}{label_suffix}")
                
                # Pressure plot
                self.ax3.plot(data['timestamps'], data['pressure'], 
                            color=color, marker='^', markersize=2, linewidth=1.5,
                            label=f"Address {address}{label_suffix}")
        
        # Add legends
        if sensor_data:
            self.ax1.legend(loc='upper left', bbox_to_anchor=(1, 1), fontsize=8)
            self.ax2.legend(loc='upper left', bbox_to_anchor=(1, 1), fontsize=8)
            self.ax3.legend(loc='upper left', bbox_to_anchor=(1, 1), fontsize=8)
        
        # Rotate x-axis labels
        for ax in [self.ax1, self.ax2, self.ax3]:
            plt.setp(ax.xaxis.get_majorticklabels(), rotation=45)
        
        # Adjust layout
        self.fig.tight_layout()
        self.fig.subplots_adjust(right=0.85)
        
        return []
    
    def stop(self):
        """Stop the animation"""
        if hasattr(self, 'animation'):
            self.animation.event_source.stop()

class RealtimeTable:
    def __init__(self, parent_window):
        self.parent_window = parent_window
        
        # Create treeview for table display
        columns = ("Address", "Type", "Temperature (°C)", "Humidity (%)", "Pressure (hPa)", "Last Updated")
        self.tree = ttk.Treeview(parent_window, columns=columns, show="headings", height=15)
        
        # Configure columns
        self.tree.heading("Address", text="Address")
        self.tree.heading("Type", text="Sensor Type")
        self.tree.heading("Temperature (°C)", text="Temperature (°C)")
        self.tree.heading("Humidity (%)", text="Humidity (%)")
        self.tree.heading("Pressure (hPa)", text="Pressure (hPa)")
        self.tree.heading("Last Updated", text="Last Updated")
        
        self.tree.column("Address", width=80, anchor=tk.CENTER)
        self.tree.column("Type", width=120, anchor=tk.CENTER)
        self.tree.column("Temperature (°C)", width=120, anchor=tk.CENTER)
        self.tree.column("Humidity (%)", width=120, anchor=tk.CENTER)
        self.tree.column("Pressure (hPa)", width=120, anchor=tk.CENTER)
        self.tree.column("Last Updated", width=150, anchor=tk.CENTER)
        
        self.tree.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        # Start periodic updates
        self.update_table()
        
    def update_table(self):
        """Update the table with current sensor data"""
        # Clear existing items
        for item in self.tree.get_children():
            self.tree.delete(item)
        
        # Re-populate with current data
        for address, data in sensor_data.items():
            if len(data['timestamps']) > 0:
                sensor_type = "Built-in" if address == 0 else f"Board {address}"
                last_temp = data['temperature'][-1] if data['temperature'] else "N/A"
                last_humidity = data['humidity'][-1] if data['humidity'] else "N/A"
                last_pressure = data['pressure'][-1] if data['pressure'] else "N/A"
                last_time = data['timestamps'][-1].strftime('%H:%M:%S') if data['timestamps'] else "N/A"
                
                self.tree.insert("", tk.END, values=(
                    address, sensor_type, 
                    f"{last_temp:.1f}" if last_temp != "N/A" else "N/A",
                    f"{last_humidity:.1f}" if last_humidity != "N/A" else "N/A",
                    f"{last_pressure:.1f}" if last_pressure != "N/A" else "N/A",
                    last_time
                ))
        
        # Schedule next update
        self.parent_window.after(update_interval, self.update_table)

def show_realtime_sensor_chart():
    """Display real-time sensor data chart"""
    window_id = f"chart_{len(realtime_windows)}"
    
    # Create a new window for the sensor data
    sensor_window = tk.Toplevel()
    sensor_window.title("Real-time Sensor Data Chart")
    sensor_window.geometry("1200x800")
    
    # Create the realtime chart
    chart = RealtimeChart(sensor_window)
    
    # Register this window
    realtime_windows[window_id] = {
        'active': True,
        'root': sensor_window,
        'chart': chart,
        'type': 'chart'
    }
    
    # Add control buttons
    button_frame = tk.Frame(sensor_window)
    button_frame.pack(pady=10)
    
    def clear_data():
        sensor_data.clear()
        print("Sensor data cleared")
    
    def on_close():
        realtime_windows[window_id]['active'] = False
        chart.stop()
        del realtime_windows[window_id]
        sensor_window.destroy()
    
    clear_button = tk.Button(button_frame, text="Clear Data", command=clear_data)
    clear_button.pack(side=tk.LEFT, padx=5)
    
    close_button = tk.Button(button_frame, text="Close", command=on_close)
    close_button.pack(side=tk.LEFT, padx=5)
    
    sensor_window.protocol("WM_DELETE_WINDOW", on_close)

def show_realtime_sensor_table():
    """Display real-time sensor data table"""
    window_id = f"table_{len(realtime_windows)}"
    
    # Create a new window for the sensor table
    table_window = tk.Toplevel()
    table_window.title("Real-time Sensor Data Table")
    table_window.geometry("800x400")
    
    # Create the realtime table
    table = RealtimeTable(table_window)
    
    # Register this window
    realtime_windows[window_id] = {
        'active': True,
        'root': table_window,
        'table': table,
        'type': 'table'
    }
    
    # Add control buttons
    button_frame = tk.Frame(table_window)
    button_frame.pack(pady=10)
    
    def clear_data():
        sensor_data.clear()
        print("Sensor data cleared")
    
    def on_close():
        realtime_windows[window_id]['active'] = False
        del realtime_windows[window_id]
        table_window.destroy()
    
    clear_button = tk.Button(button_frame, text="Clear Data", command=clear_data)
    clear_button.pack(side=tk.LEFT, padx=5)
    
    close_button = tk.Button(button_frame, text="Close", command=on_close)
    close_button.pack(side=tk.LEFT, padx=5)
    
    table_window.protocol("WM_DELETE_WINDOW", on_close)

def show_sensor_data():
    """Display static sensor data in a chart window (original function for compatibility)"""
    if not sensor_data:
        print("No sensor data available to display")
        return
    
    # Create a new window for the sensor data
    sensor_window = tk.Toplevel()
    sensor_window.title("Sensor Data Visualization (Static)")
    sensor_window.geometry("1200x800")
    
    # Create a figure with subplots
    fig = Figure(figsize=(12, 8), dpi=100)
    fig.suptitle("Sensor Data from Multiple Addresses", fontsize=16)
    
    # Create subplots for each sensor type
    ax1 = fig.add_subplot(311)
    ax2 = fig.add_subplot(312)
    ax3 = fig.add_subplot(313)
    
    # Colors for different addresses
    colors = ['blue', 'red', 'green', 'orange', 'purple', 'brown', 'pink', 'gray']
    
    # Plot data for each address
    for idx, (address, data) in enumerate(sensor_data.items()):
        if len(data['timestamps']) > 0:
            color = colors[idx % len(colors)]
            label_suffix = " (Built-in)" if address == 0 else f" (Board {address})"
            
            # Temperature plot
            ax1.plot(data['timestamps'], data['temperature'], 
                    color=color, marker='o', markersize=3, linewidth=1.5,
                    label=f"Address {address}{label_suffix}")
            
            # Humidity plot
            ax2.plot(data['timestamps'], data['humidity'], 
                    color=color, marker='s', markersize=3, linewidth=1.5,
                    label=f"Address {address}{label_suffix}")
            
            # Pressure plot
            ax3.plot(data['timestamps'], data['pressure'], 
                    color=color, marker='^', markersize=3, linewidth=1.5,
                    label=f"Address {address}{label_suffix}")
    
    # Configure temperature subplot
    ax1.set_ylabel('Temperature (°C)')
    ax1.grid(True, alpha=0.3)
    ax1.legend(loc='upper left', bbox_to_anchor=(1, 1))
    
    # Configure humidity subplot
    ax2.set_ylabel('Humidity (%)')
    ax2.grid(True, alpha=0.3)
    ax2.legend(loc='upper left', bbox_to_anchor=(1, 1))
    
    # Configure pressure subplot
    ax3.set_ylabel('Pressure (hPa)')
    ax3.set_xlabel('Time')
    ax3.grid(True, alpha=0.3)
    ax3.legend(loc='upper left', bbox_to_anchor=(1, 1))
    
    # Format x-axis to show time nicely
    for ax in [ax1, ax2, ax3]:
        ax.xaxis.set_major_formatter(mdates.DateFormatter('%H:%M:%S'))
        ax.xaxis.set_major_locator(mdates.SecondLocator(interval=30))
        plt.setp(ax.xaxis.get_majorticklabels(), rotation=45)
    
    # Adjust layout to prevent legend cutoff
    fig.tight_layout()
    fig.subplots_adjust(right=0.85)
    
    # Create canvas and add to window
    canvas = FigureCanvasTkAgg(fig, sensor_window)
    canvas.draw()
    canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)
    
    # Add control buttons
    button_frame = tk.Frame(sensor_window)
    button_frame.pack(pady=10)
    
    def refresh_chart():
        canvas.draw()
    
    def clear_data():
        sensor_data.clear()
        refresh_chart()
    
    refresh_button = tk.Button(button_frame, text="Refresh Chart", command=refresh_chart)
    refresh_button.pack(side=tk.LEFT, padx=5)
    
    clear_button = tk.Button(button_frame, text="Clear Data", command=clear_data)
    clear_button.pack(side=tk.LEFT, padx=5)
    
    close_button = tk.Button(button_frame, text="Close", command=sensor_window.destroy)
    close_button.pack(side=tk.LEFT, padx=5)

def show_sensor_table():
    """Display static sensor data in a table format (original function for compatibility)"""
    if not sensor_data:
        print("No sensor data available to display")
        return
    
    # Create a new window for the sensor table
    table_window = tk.Toplevel()
    table_window.title("Current Sensor Readings (Static)")
    table_window.geometry("800x400")
    
    # Create treeview for table display
    columns = ("Address", "Type", "Temperature (°C)", "Humidity (%)", "Pressure (hPa)", "Last Updated")
    tree = ttk.Treeview(table_window, columns=columns, show="headings", height=15)
    
    # Configure columns
    tree.heading("Address", text="Address")
    tree.heading("Type", text="Sensor Type")
    tree.heading("Temperature (°C)", text="Temperature (°C)")
    tree.heading("Humidity (%)", text="Humidity (%)")
    tree.heading("Pressure (hPa)", text="Pressure (hPa)")
    tree.heading("Last Updated", text="Last Updated")
    
    tree.column("Address", width=80, anchor=tk.CENTER)
    tree.column("Type", width=120, anchor=tk.CENTER)
    tree.column("Temperature (°C)", width=120, anchor=tk.CENTER)
    tree.column("Humidity (%)", width=120, anchor=tk.CENTER)
    tree.column("Pressure (hPa)", width=120, anchor=tk.CENTER)
    tree.column("Last Updated", width=150, anchor=tk.CENTER)
    
    # Add data to table
    for address, data in sensor_data.items():
        if len(data['timestamps']) > 0:
            sensor_type = "Built-in" if address == 0 else f"Board {address}"
            last_temp = data['temperature'][-1] if data['temperature'] else "N/A"
            last_humidity = data['humidity'][-1] if data['humidity'] else "N/A"
            last_pressure = data['pressure'][-1] if data['pressure'] else "N/A"
            last_time = data['timestamps'][-1].strftime('%H:%M:%S') if data['timestamps'] else "N/A"
            
            tree.insert("", tk.END, values=(
                address, sensor_type, 
                f"{last_temp:.1f}" if last_temp != "N/A" else "N/A",
                f"{last_humidity:.1f}" if last_humidity != "N/A" else "N/A",
                f"{last_pressure:.1f}" if last_pressure != "N/A" else "N/A",
                last_time
            ))
    
    tree.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
    
    # Add refresh and close buttons
    button_frame = tk.Frame(table_window)
    button_frame.pack(pady=10)
    
    def refresh_table():
        # Clear existing items
        for item in tree.get_children():
            tree.delete(item)
        
        # Re-populate with current data
        for address, data in sensor_data.items():
            if len(data['timestamps']) > 0:
                sensor_type = "Built-in" if address == 0 else f"Board {address}"
                last_temp = data['temperature'][-1] if data['temperature'] else "N/A"
                last_humidity = data['humidity'][-1] if data['humidity'] else "N/A"
                last_pressure = data['pressure'][-1] if data['pressure'] else "N/A"
                last_time = data['timestamps'][-1].strftime('%H:%M:%S') if data['timestamps'] else "N/A"
                
                tree.insert("", tk.END, values=(
                    address, sensor_type, 
                    f"{last_temp:.1f}" if last_temp != "N/A" else "N/A",
                    f"{last_humidity:.1f}" if last_humidity != "N/A" else "N/A",
                    f"{last_pressure:.1f}" if last_pressure != "N/A" else "N/A",
                    last_time
                ))
    
    refresh_button = tk.Button(button_frame, text="Refresh", command=refresh_table)
    refresh_button.pack(side=tk.LEFT, padx=5)
    
    close_button = tk.Button(button_frame, text="Close", command=table_window.destroy)
    close_button.pack(side=tk.LEFT, padx=5)

def get_heap_info(data):
    # open a new window
    heap_window = tk.Toplevel()
    heap_window.title("Heap Info")
    heap_window.geometry("600x400")
    label = tk.Label(heap_window, text="Heap Information")
    label.pack(pady=10)
    
    # create a text widget to display the heap info
    text = tk.Text(heap_window, wrap=tk.WORD, font=("Arial", 12))
    text.pack(expand=True, fill=tk.BOTH, padx=10, pady=10)
    
    # insert the heap info into the text widget
    if "free_heap" in data:
        # {'free_heap': 193764, 'largest_free_block': 110592, 'status': 200, 'timestamp': 57720}
        free_heap = data.get("free_heap", "N/A")
        largest_free_block = data.get("largest_free_block", "N/A")
        
        # Format the heap information
        heap_info = (
            f"Free Heap: {free_heap} bytes\n"
            f"Largest Free Block: {largest_free_block} bytes\n"
        )
        
        text.insert(tk.END, heap_info)
        
    else:
        text.insert(tk.END, "No heap information available.")
    
    close_button = tk.Button(heap_window, text="Close", command=heap_window.destroy)
    close_button.pack(pady=10)
    
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
    
    # Subscribe to channel 2 for sensor data
    seaport.subscribe(2, lambda data: sensor_data_received(data), debug=True)

    # create a simple GUI to allow for a user to send commands
    root = tk.Tk()
    root.title("Serial Port Tester with Real-time Monitoring")
    root.geometry("700x1400")
    label = tk.Label(root, text="Press Ctrl+C to exit")
    label.pack(pady=20)
    
    # Main control buttons frame
    frame = tk.Frame(root)
    frame.pack(pady=10)

    button_ping = tk.Button(frame, text="Send Ping", command=lambda: seaport.publish(254, {"cmd": "ping"}))
    button_ping.pack(side=tk.LEFT, padx=5)

    button_mem = tk.Button(frame, text="Get Task Memory Usage", command=lambda: seaport.publish(254, {"cmd": "get_water_level"}))
    button_mem.pack(side=tk.LEFT, padx=5)

    button_heap = tk.Button(frame, text="Get Heap Info", command=lambda: seaport.publish(254, {"cmd": "get_heap_info"}))
    button_heap.pack(side=tk.LEFT, padx=5)

    # Real-time sensor data control buttons frame
    realtime_frame = tk.Frame(root)
    realtime_frame.pack(pady=10)
    
    realtime_label = tk.Label(realtime_frame, text="Real-time Sensor Data Controls:", font=("Arial", 12, "bold"))
    realtime_label.pack()
    
    realtime_button_frame = tk.Frame(realtime_frame)
    realtime_button_frame.pack(pady=5)
    
    realtime_chart_button = tk.Button(realtime_button_frame, text="Show Real-time Chart", command=show_realtime_sensor_chart, 
                                     bg="lightgreen", font=("Arial", 10, "bold"))
    realtime_chart_button.pack(side=tk.LEFT, padx=5)
    
    realtime_table_button = tk.Button(realtime_button_frame, text="Show Real-time Table", command=show_realtime_sensor_table,
                                     bg="lightblue", font=("Arial", 10, "bold"))
    realtime_table_button.pack(side=tk.LEFT, padx=5)

    # Static sensor data control buttons frame
    static_frame = tk.Frame(root)
    static_frame.pack(pady=10)
    
    static_label = tk.Label(static_frame, text="Static Sensor Data Controls:", font=("Arial", 12))
    static_label.pack()
    
    static_button_frame = tk.Frame(static_frame)
    static_button_frame.pack(pady=5)
    
    chart_button = tk.Button(static_button_frame, text="Show Static Chart", command=show_sensor_data)
    chart_button.pack(side=tk.LEFT, padx=5)
    
    table_button = tk.Button(static_button_frame, text="Show Static Table", command=show_sensor_table)
    table_button.pack(side=tk.LEFT, padx=5)
    
    def clear_sensor_data():
        sensor_data.clear()
        print("Sensor data cleared")
    
    clear_sensor_button = tk.Button(static_button_frame, text="Clear Sensor Data", command=clear_sensor_data)
    clear_sensor_button.pack(side=tk.LEFT, padx=5)

    # Motor control sliders
    motor_frame = tk.Frame(root)
    motor_frame.pack(pady=10)
    
    motor_label = tk.Label(motor_frame, text="Motor Controls:", font=("Arial", 12, "bold"))
    motor_label.pack()

    # add sliders that send signals to the serial port when moved
    sliders = []
    for i in range(8):
        slider = tk.Scale(motor_frame, from_=-1, to=1, orient=tk.HORIZONTAL, label=f"Motor Speed {i+1}", 
                         resolution=0.01, length=600, width=30)
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
        # Stop all realtime windows
        for window_id in list(realtime_windows.keys()):
            if realtime_windows[window_id]['active']:
                if 'chart' in realtime_windows[window_id]:
                    realtime_windows[window_id]['chart'].stop()
                realtime_windows[window_id]['active'] = False
        realtime_windows.clear()
        
        seaport.stop()
        root.destroy()
        exit(0)

    seaport.start()

    root.protocol("WM_DELETE_WINDOW", on_close)
    root.mainloop()

if __name__ == "__main__":
    main()