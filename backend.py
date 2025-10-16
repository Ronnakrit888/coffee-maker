import serial
import time
import threading
from flask import Flask
from flask_socketio import SocketIO, emit
from flask_cors import CORS

# --- Configuration ---
# !!! IMPORTANT: Replace 'COM3' with your actual COM port
SERIAL_PORT = 'COM3'
BAUD_RATE = 115200  # Ensure this matches your STM32's baud rate
# ---------------------

app = Flask(__name__)
# Enable CORS for the frontend on port 3000 (or whichever port you use)
CORS(app, resources={r"/*": {"origins": "*"}}) 

# Initialize SocketIO
socketio = SocketIO(app, cors_allowed_origins="*", async_mode='eventlet') 

# Flag to control the serial reading thread
running = True

def read_serial_data():
    """Continuously reads data from the serial port and emits it via WebSocket."""
    print(f"Starting serial monitor thread on {SERIAL_PORT}...")
    ser = None
    
    # Loop to continuously try and connect
    while running:
        if ser is None:
            try:
                # Initialize serial connection
                ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
                print(f"Successfully opened serial port {SERIAL_PORT} at {BAUD_RATE} baud.")
            except serial.SerialException as e:
                print(f"Error opening serial port {SERIAL_PORT}: {e}")
                print("Retrying in 5 seconds...")
                time.sleep(5)
                continue

        # Main loop to read and parse data
        try:
            line = ser.readline().decode('utf-8').strip()
            
            if line:
                print(f"Received raw data: {line}")
                
                start_tag = "[DATASTART]"
                end_tag = "[DATAEND]"

                if start_tag in line and end_tag in line:
                    try:
                        # Extract and parse the data string
                        data_string = line.split(start_tag)[1].split(end_tag)[0].strip()
                        values = [int(v.strip()) for v in data_string.split(',')]
                        
                        if len(values) == 3:
                            data = {
                                "current_state": values[0],
                                "counter": values[1],
                                "safety_halt_released": values[2],
                                "timestamp": time.time()
                            }
                            print(f"Data parsed and EMITTED: {data}")
                            
                            # EMIT the data through the WebSocket
                            # 'stm32_update' is the event name the client listens to
                            socketio.emit('stm32_update', data) 
                        else:
                            print(f"Warning: Data string has {len(values)} values, expected 3.")

                    except Exception as e:
                        print(f"Error parsing data line: {e}. Raw data: {line}")

                # Emit raw display messages as well for a debug/log console
                else:
                    socketio.emit('log_message', {'msg': line})

        except serial.SerialException as e:
            print(f"Serial port error: {e}. Reconnecting...")
            if ser:
                ser.close()
            ser = None
            time.sleep(2)
            
        except Exception as e:
            print(f"An unexpected error occurred: {e}")
            time.sleep(1) 
            
    if ser:
        ser.close()
    print("Serial reading thread terminated.")


# Start the serial reading thread when the app launches
serial_thread = threading.Thread(target=read_serial_data, daemon=True)
serial_thread.start()

# Basic route just to confirm the Flask app is running (optional)
@app.route('/')
def index():
    return "STM32 WebSocket Server Running"

if __name__ == '__main__':
    # Flask-SocketIO runs the application using 'eventlet' in asynchronous mode
    print("Starting Flask-SocketIO server on http://127.0.0.1:5000")
    # 'allow_unsafe_werkzeug=True' is needed for eventlet
    socketio.run(app, host='0.0.0.0', port=5000, allow_unsafe_werkzeug=True)