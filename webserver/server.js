const express = require('express');
const http = require('http');
const { Server } = require("socket.io");
const { SerialPort } = require('serialport');
const { ReadlineParser } = require('@serialport/parser-readline');

// --- Configuration ---
// !!! IMPORTANT: Replace this with your actual COM port
const SERIAL_PORT = '/dev/tty.usbmodem1403';
const BAUD_RATE = 115200;
const SERVER_PORT = 5000;
// ---------------------

const app = express();
const server = http.createServer(app);

// Setup Socket.IO Server with CORS
const io = new Server(server, {
  cors: {
    origin: "*", // Allow all origins for local testing
    methods: ["GET", "POST"]
  }
});

let serialPort = null;
let parser = null;

// Function to initialize serial connection
function setupSerialPort() {
    console.log(`Attempting to connect to serial port: ${SERIAL_PORT}`);
    
    // Check if the port is already open
    if (serialPort && serialPort.isOpen) {
        serialPort.close();
    }

    try {
        serialPort = new SerialPort({ path: SERIAL_PORT, baudRate: BAUD_RATE });
        
        // Use ReadlineParser to read data line by line (delimited by \n)
        parser = serialPort.pipe(new ReadlineParser({ delimiter: '\n' }));
        
        serialPort.on('open', () => {
            console.log(`Successfully opened serial port ${SERIAL_PORT} at ${BAUD_RATE} baud.`);
        });

        serialPort.on('error', (err) => {
            console.error('Serial Port Error:', err.message);
            // Attempt to re-establish connection after a delay
            setTimeout(setupSerialPort, 5000);
        });

        serialPort.on('close', () => {
            console.log('Serial Port Closed. Attempting to reconnect...');
            serialPort = null;
            parser = null;
            setTimeout(setupSerialPort, 5000);
        });

        // --- Data Parsing Logic ---
        parser.on('data', (line) => {
            const rawLine = line.trim();
            
            if (rawLine) {
                console.log(`Received raw data: ${rawLine}`);
                
                const startTag = "[DATASTART]";
                const endTag = "[DATAEND]";

                if (rawLine.includes(startTag) && rawLine.includes(endTag)) {
                    try {
                        // Extract the data string between the tags
                        const startIndex = rawLine.indexOf(startTag) + startTag.length;
                        const endIndex = rawLine.indexOf(endTag);
                        const dataString = rawLine.substring(startIndex, endIndex).trim();
                        
                        // Parse integer values
                        const values = dataString.split(',').map(v => parseInt(v.trim(), 10));
                        
                        if (values.length === 3 && !values.some(isNaN)) {
                            const data = {
                                "current_state": values[0],
                                "counter": values[1],
                                "safety_halt_released": values[2],
                                "timestamp": Math.floor(Date.now() / 1000)
                            };
                            console.log(`Data parsed and EMITTED:`, data);
                            
                            // Emit the structured data via WebSocket
                            io.emit('stm32_update', data); 
                        } else {
                            console.warn(`Warning: Data string has invalid format or length (${values.length} values, expected 3). Raw: ${dataString}`);
                        }

                    } catch (e) {
                        console.error(`Error parsing data line: ${e}. Raw data: ${rawLine}`);
                    }
                } else {
                    // Emit raw log messages for the console
                    io.emit('log_message', { msg: rawLine });
                }
            }
        });

    } catch (e) {
        console.error('Error creating SerialPort instance:', e.message);
        setTimeout(setupSerialPort, 5000);
    }
}

// Start serial port connection attempt
setupSerialPort();


// Serve a basic message on the root
app.get('/', (req, res) => {
  res.send('STM32 Node.js WebSocket Server Running');
});

// Start the HTTP server
server.listen(SERVER_PORT, '0.0.0.0', () => {
    console.log(`Starting Node.js WebSocket server on http://127.0.0.1:${SERVER_PORT}`);
});