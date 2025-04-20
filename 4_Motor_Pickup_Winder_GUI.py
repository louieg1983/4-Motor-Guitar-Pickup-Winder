import tkinter as tk
from tkinter import ttk, messagebox
import serial
import serial.tools.list_ports
import threading
import time

class WinderControlApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Winding Machine Control")
        self.root.geometry("750x550")
        self.root.configure(padx=10, pady=10)
        
        # Serial connection variables
        self.serial_conn = None
        self.connected = False
        self.monitoring_thread = None
        self.thread_running = False
        
        # Winding machine variables
        self.current_wind_count = 0
        self.desired_wind_count = 1000
        self.winder_speed = 150000
        self.traverse_delay = 500
        self.left_limit = 0
        self.right_limit = 6400
        
        # UI layout
        self.create_interface()
        
        # Update status periodically
        self.root.after(100, self.update_status)
    
    def create_interface(self):
        # Set up grid for two-column layout
        self.root.grid_columnconfigure(0, weight=1)
        self.root.grid_columnconfigure(1, weight=1)
        
        # Create main frames
        left_frame = ttk.Frame(self.root)
        left_frame.grid(row=0, column=0, sticky="nsew", padx=5, pady=5)
        
        right_frame = ttk.Frame(self.root)
        right_frame.grid(row=0, column=1, sticky="nsew", padx=5, pady=5)
        
        # Create connection section
        self.create_connection_section(left_frame)
        
        # Create winder control section
        self.create_winder_control_section(left_frame)
        
        # Create traverse control section
        self.create_traverse_control_section(left_frame)
        
        # Create motor buttons section
        self.create_motor_buttons_section(left_frame)
        
        # Create status display section
        self.create_status_section(right_frame)
        
        # Create console section
        self.create_console_section(right_frame)
    
    def create_connection_section(self, parent):
        connection_frame = ttk.LabelFrame(parent, text="Connection")
        connection_frame.pack(fill="x", padx=5, pady=5)
        
        # Port selection row
        port_frame = ttk.Frame(connection_frame)
        port_frame.pack(fill="x", padx=5, pady=5)
        
        ttk.Label(port_frame, text="Port:").pack(side="left")
        
        # Get available ports
        self.port_var = tk.StringVar()
        self.port_combo = ttk.Combobox(port_frame, textvariable=self.port_var)
        self.port_combo.pack(side="left", padx=5, expand=True, fill="x")
        self.refresh_ports()
        
        ttk.Button(port_frame, text="Refresh", command=self.refresh_ports).pack(side="left", padx=2)
        ttk.Button(port_frame, text="Connect", command=self.connect_serial).pack(side="left", padx=2)
        ttk.Button(port_frame, text="Disconnect", command=self.disconnect_serial).pack(side="left", padx=2)
        
        # Status indicator
        status_frame = ttk.Frame(connection_frame)
        status_frame.pack(fill="x", padx=5, pady=5)
        
        self.connection_status = ttk.Label(status_frame, text="Status: Disconnected", foreground="red")
        self.connection_status.pack(side="left")
    
    def create_winder_control_section(self, parent):
        winder_frame = ttk.LabelFrame(parent, text="Winder Control")
        winder_frame.pack(fill="x", padx=5, pady=5)
        
        # Wind count row
        wind_count_frame = ttk.Frame(winder_frame)
        wind_count_frame.pack(fill="x", padx=5, pady=5)
        
        ttk.Label(wind_count_frame, text="Desired Wind Count:").pack(side="left")
        self.wind_count_var = tk.StringVar(value=str(self.desired_wind_count))
        wind_count_entry = ttk.Entry(wind_count_frame, textvariable=self.wind_count_var, width=8)
        wind_count_entry.pack(side="left", padx=5)
        ttk.Button(wind_count_frame, text="Set", command=self.set_wind_count).pack(side="left")
        
        # Winder speed row
        speed_frame = ttk.Frame(winder_frame)
        speed_frame.pack(fill="x", padx=5, pady=5)
        
        ttk.Label(speed_frame, text="Winder Speed (Hz):").pack(side="left")
        self.winder_speed_var = tk.StringVar(value=str(self.winder_speed))
        
        self.winder_speed_slider = ttk.Scale(speed_frame, from_=1000, to=300000, orient="horizontal", 
                                           command=self.update_winder_speed_display)
        self.winder_speed_slider.set(self.winder_speed)
        self.winder_speed_slider.pack(side="left", padx=5, expand=True, fill="x")
        
        winder_speed_entry = ttk.Entry(speed_frame, textvariable=self.winder_speed_var, width=8)
        winder_speed_entry.pack(side="left", padx=5)
        ttk.Button(speed_frame, text="Set", command=self.set_winder_speed).pack(side="left")
        
        # Create a row of preset speed buttons
        preset_frame = ttk.Frame(winder_frame)
        preset_frame.pack(fill="x", padx=5, pady=5)
        
        ttk.Label(preset_frame, text="Speed Presets:").pack(side="left")
        ttk.Button(preset_frame, text="Slow (50k)", command=lambda: self.set_preset_speed(50000)).pack(side="left", padx=2)
        ttk.Button(preset_frame, text="Medium (100k)", command=lambda: self.set_preset_speed(100000)).pack(side="left", padx=2)
        ttk.Button(preset_frame, text="Fast (150k)", command=lambda: self.set_preset_speed(150000)).pack(side="left", padx=2)
        ttk.Button(preset_frame, text="Ultra (200k)", command=lambda: self.set_preset_speed(200000)).pack(side="left", padx=2)
    
    def create_traverse_control_section(self, parent):
        traverse_frame = ttk.LabelFrame(parent, text="Traverse Control")
        traverse_frame.pack(fill="x", padx=5, pady=5)
        
        # Traverse speed row
        speed_frame = ttk.Frame(traverse_frame)
        speed_frame.pack(fill="x", padx=5, pady=5)
        
        ttk.Label(speed_frame, text="Traverse Delay (µs):").pack(side="left")
        self.traverse_delay_var = tk.StringVar(value=str(self.traverse_delay))
        
        self.traverse_delay_slider = ttk.Scale(speed_frame, from_=100, to=5000, orient="horizontal", 
                                             command=self.update_traverse_delay_display)
        self.traverse_delay_slider.set(self.traverse_delay)
        self.traverse_delay_slider.pack(side="left", padx=5, expand=True, fill="x")
        
        traverse_delay_entry = ttk.Entry(speed_frame, textvariable=self.traverse_delay_var, width=8)
        traverse_delay_entry.pack(side="left", padx=5)
        ttk.Button(speed_frame, text="Set", command=self.set_traverse_delay).pack(side="left")
        
        # Left limit row
        left_limit_frame = ttk.Frame(traverse_frame)
        left_limit_frame.pack(fill="x", padx=5, pady=5)
        
        ttk.Label(left_limit_frame, text="Left Limit (steps):").pack(side="left")
        self.left_limit_var = tk.StringVar(value=str(self.left_limit))
        
        self.left_limit_slider = ttk.Scale(left_limit_frame, from_=0, to=5000, orient="horizontal", 
                                         command=self.update_left_limit_display)
        self.left_limit_slider.set(self.left_limit)
        self.left_limit_slider.pack(side="left", padx=5, expand=True, fill="x")
        
        left_limit_entry = ttk.Entry(left_limit_frame, textvariable=self.left_limit_var, width=8)
        left_limit_entry.pack(side="left", padx=5)
        ttk.Button(left_limit_frame, text="Set", command=self.set_left_limit).pack(side="left")
        
        # Right limit row
        right_limit_frame = ttk.Frame(traverse_frame)
        right_limit_frame.pack(fill="x", padx=5, pady=5)
        
        ttk.Label(right_limit_frame, text="Right Limit (steps):").pack(side="left")
        self.right_limit_var = tk.StringVar(value=str(self.right_limit))
        
        self.right_limit_slider = ttk.Scale(right_limit_frame, from_=500, to=12800, orient="horizontal", 
                                          command=self.update_right_limit_display)
        self.right_limit_slider.set(self.right_limit)
        self.right_limit_slider.pack(side="left", padx=5, expand=True, fill="x")
        
        right_limit_entry = ttk.Entry(right_limit_frame, textvariable=self.right_limit_var, width=8)
        right_limit_entry.pack(side="left", padx=5)
        ttk.Button(right_limit_frame, text="Set", command=self.set_right_limit).pack(side="left")
        
        # Home position button
        home_frame = ttk.Frame(traverse_frame)
        home_frame.pack(fill="x", padx=5, pady=5)
        
        ttk.Button(home_frame, text="Reset Traverse Home", command=self.reset_traverse_home).pack(side="left")
    
    def create_motor_buttons_section(self, parent):
        button_frame = ttk.LabelFrame(parent, text="Motor Controls")
        button_frame.pack(fill="x", padx=5, pady=5)
        
        # Main control buttons
        btn_frame = ttk.Frame(button_frame)
        btn_frame.pack(fill="x", padx=5, pady=10)
        
        start_btn = ttk.Button(btn_frame, text="START", command=self.start_motors, width=15)
        start_btn.pack(side="left", padx=5, expand=True, fill="x")
        start_btn.configure(style='Green.TButton')
        
        stop_btn = ttk.Button(btn_frame, text="STOP", command=self.stop_motors, width=15)
        stop_btn.pack(side="left", padx=5, expand=True, fill="x")
        stop_btn.configure(style='Red.TButton')
        
        # Additional controls
        extra_frame = ttk.Frame(button_frame)
        extra_frame.pack(fill="x", padx=5, pady=5)
        
        ttk.Button(extra_frame, text="RESET COUNTER", command=self.reset_counter).pack(side="left", padx=5, expand=True, fill="x")
        ttk.Button(extra_frame, text="DISABLE ALL", command=self.disable_all_motors).pack(side="left", padx=5, expand=True, fill="x")
    
    def create_status_section(self, parent):
        status_frame = ttk.LabelFrame(parent, text="Status")
        status_frame.pack(fill="both", padx=5, pady=5, expand=True)
        
        # Current wind count display
        count_frame = ttk.Frame(status_frame)
        count_frame.pack(fill="x", padx=5, pady=10)
        
        ttk.Label(count_frame, text="Current Wind Count:", font=("Arial", 12)).pack(side="left")
        self.current_count_label = ttk.Label(count_frame, text="0", font=("Arial", 24))
        self.current_count_label.pack(side="left", padx=10)
        
        # Target count display
        target_frame = ttk.Frame(status_frame)
        target_frame.pack(fill="x", padx=5, pady=5)
        
        ttk.Label(target_frame, text="Target:", font=("Arial", 10)).pack(side="left")
        self.target_count_label = ttk.Label(target_frame, text="1000", font=("Arial", 10))
        self.target_count_label.pack(side="left", padx=5)
        
        # Progress bar
        progress_frame = ttk.Frame(status_frame)
        progress_frame.pack(fill="x", padx=5, pady=10)
        
        ttk.Label(progress_frame, text="Progress:").pack(side="left")
        self.progress_var = tk.DoubleVar(value=0)
        self.progress_bar = ttk.Progressbar(progress_frame, variable=self.progress_var, length=200, mode="determinate")
        self.progress_bar.pack(side="left", padx=5, expand=True, fill="x")
        self.progress_percent = ttk.Label(progress_frame, text="0%")
        self.progress_percent.pack(side="left", padx=5)
        
        # Motor status
        motor_frame = ttk.Frame(status_frame)
        motor_frame.pack(fill="x", padx=5, pady=10)
        
        ttk.Label(motor_frame, text="Motors:").pack(side="left")
        self.motor_status_label = ttk.Label(motor_frame, text="STOPPED", foreground="red")
        self.motor_status_label.pack(side="left", padx=5)
    
    def create_console_section(self, parent):
        console_frame = ttk.LabelFrame(parent, text="Console")
        console_frame.pack(fill="both", padx=5, pady=5, expand=True)
        
        # Console output with scrollbar
        self.console = tk.Text(console_frame, height=15, width=50)
        self.console.pack(side="left", fill="both", expand=True, padx=5, pady=5)
        
        scrollbar = ttk.Scrollbar(console_frame, command=self.console.yview)
        scrollbar.pack(side="right", fill="y", pady=5)
        self.console.config(yscrollcommand=scrollbar.set)
        
        # Make the console read-only
        self.console.config(state=tk.DISABLED)
    
    # Utility functions
    def refresh_ports(self):
        ports = serial.tools.list_ports.comports()
        available_ports = [port.device for port in ports]
        self.port_combo['values'] = available_ports
        if available_ports:
            self.port_combo.current(0)
    
    def connect_serial(self):
        if self.connected:
            messagebox.showinfo("Info", "Already connected")
            return
        
        port = self.port_var.get()
        if not port:
            messagebox.showerror("Error", "No port selected")
            return
        
        try:
            self.serial_conn = serial.Serial(port, 115200, timeout=1)
            self.connected = True
            self.connection_status.config(text="Status: Connected", foreground="green")
            
            # Start the monitoring thread
            self.thread_running = True
            self.monitoring_thread = threading.Thread(target=self.monitor_serial)
            self.monitoring_thread.daemon = True
            self.monitoring_thread.start()
            
            self.add_to_console(f"Connected to {port}")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to connect: {str(e)}")
    
    def disconnect_serial(self):
        if not self.connected:
            return
        
        # Stop the monitoring thread
        self.thread_running = False
        if self.monitoring_thread:
            self.monitoring_thread.join(timeout=1)
        
        if self.serial_conn:
            self.serial_conn.close()
            self.serial_conn = None
        
        self.connected = False
        self.connection_status.config(text="Status: Disconnected", foreground="red")
        self.add_to_console("Disconnected")
    
    def add_to_console(self, message):
        self.console.config(state=tk.NORMAL)
        self.console.insert(tk.END, message + "\n")
        self.console.see(tk.END)
        self.console.config(state=tk.DISABLED)
    
    def monitor_serial(self):
        while self.thread_running and self.serial_conn:
            try:
                if self.serial_conn.in_waiting:
                    line = self.serial_conn.readline().decode('utf-8').strip()
                    if line:
                        # Parse wind count from responses
                        if "Wind Count:" in line:
                            try:
                                count_value = line.split("Wind Count:")[1].strip()
                                self.current_wind_count = int(count_value)
                            except (ValueError, IndexError):
                                pass
                        
                        self.add_to_console(line)
            except Exception as e:
                self.add_to_console(f"Error reading: {str(e)}")
                break
            time.sleep(0.01)
    
    def send_command(self, command):
        if not self.connected or not self.serial_conn:
            messagebox.showerror("Error", "Not connected to device")
            return False
        
        try:
            self.serial_conn.write((command + "\n").encode())
            self.add_to_console(f"Sent: {command}")
            return True
        except Exception as e:
            self.add_to_console(f"Error sending command: {str(e)}")
            return False
    
    # UI update functions
    def update_winder_speed_display(self, value):
        value = int(float(value))
        self.winder_speed_var.set(str(value))
    
    def update_traverse_delay_display(self, value):
        value = int(float(value))
        self.traverse_delay_var.set(str(value))
    
    def update_left_limit_display(self, value):
        value = int(float(value))
        self.left_limit_var.set(str(value))
    
    def update_right_limit_display(self, value):
        value = int(float(value))
        self.right_limit_var.set(str(value))
    
    # Command functions
    def set_wind_count(self):
        try:
            count = int(self.wind_count_var.get())
            if count <= 0:
                messagebox.showerror("Error", "Wind count must be greater than 0")
                return
            
            self.desired_wind_count = count
            self.target_count_label.config(text=str(count))
            self.send_command(f"N{count}")
        except ValueError:
            messagebox.showerror("Error", "Invalid wind count value")
    
    def set_winder_speed(self):
        try:
            speed = int(self.winder_speed_var.get())
            if speed <= 0:
                messagebox.showerror("Error", "Speed must be greater than 0")
                return
            
            self.winder_speed = speed
            self.winder_speed_slider.set(speed)
            self.send_command(f"w_speed:{speed}")
        except ValueError:
            messagebox.showerror("Error", "Invalid speed value")
    
    def set_preset_speed(self, speed):
        self.winder_speed = speed
        self.winder_speed_var.set(str(speed))
        self.winder_speed_slider.set(speed)
        self.send_command(f"w_speed:{speed}")
    
    def set_traverse_delay(self):
        try:
            delay = int(self.traverse_delay_var.get())
            if delay <= 0:
                messagebox.showerror("Error", "Delay must be greater than 0")
                return
            
            self.traverse_delay = delay
            self.traverse_delay_slider.set(delay)
            self.send_command(f"t_speed:{delay}")
        except ValueError:
            messagebox.showerror("Error", "Invalid delay value")
    
    def set_left_limit(self):
        try:
            limit = int(self.left_limit_var.get())
            if limit < 0:
                messagebox.showerror("Error", "Left limit cannot be negative")
                return
            
            self.left_limit = limit
            self.left_limit_slider.set(limit)
            self.send_command(f"t_leftlimit:{limit}")
        except ValueError:
            messagebox.showerror("Error", "Invalid limit value")
    
    def set_right_limit(self):
        try:
            limit = int(self.right_limit_var.get())
            if limit <= 0 or limit > 12800:
                messagebox.showerror("Error", "Right limit must be between 1 and 12800")
                return
            
            self.right_limit = limit
            self.right_limit_slider.set(limit)
            self.send_command(f"t_rightlimit:{limit}")
        except ValueError:
            messagebox.showerror("Error", "Invalid limit value")
    
    def start_motors(self):
        self.send_command("S")
        self.motor_status_label.config(text="RUNNING", foreground="green")
    
    def stop_motors(self):
        self.send_command("T")
        self.motor_status_label.config(text="STOPPED", foreground="red")
    
    def reset_counter(self):
        self.send_command("R")
        self.current_wind_count = 0
        self.current_count_label.config(text="0")
        self.progress_var.set(0)
        self.progress_percent.config(text="0%")
    
    def disable_all_motors(self):
        self.send_command("disable_all_motors")
        self.motor_status_label.config(text="DISABLED", foreground="red")
    
    def reset_traverse_home(self):
        self.send_command("t_home")
    
    def update_status(self):
        # Update wind count display
        self.current_count_label.config(text=str(self.current_wind_count))
        
        # Update progress bar
        if self.desired_wind_count > 0:
            progress = (self.current_wind_count / self.desired_wind_count) * 100
            self.progress_var.set(min(progress, 100))
            self.progress_percent.config(text=f"{int(min(progress, 100))}%")
            
            # Check if winding is complete
            if self.current_wind_count >= self.desired_wind_count:
                self.motor_status_label.config(text="COMPLETE", foreground="blue")
        
        # Schedule the next update
        self.root.after(100, self.update_status)

# Create colored button styles
def setup_styles():
    style = ttk.Style()
    style.configure('Green.TButton', background='green')
    style.configure('Red.TButton', background='red')

if __name__ == "__main__":
    root = tk.Tk()
    setup_styles()
    app = WinderControlApp(root)
    root.mainloop()
