const express = require('express');
const http = require('http');
const { Server } = require("socket.io");
const { SerialPort } = require('serialport');
const { ReadlineParser } = require('@serialport/parser-readline');
require('dotenv').config();

// --- Configuration ---
const SERIAL_PORT = process.env.SERIAL_PORT || '/dev/tty.usbmodem1403';
const BAUD_RATE = parseInt(process.env.BAUD_RATE) || 115200;
const SERVER_PORT = parseInt(process.env.SERVER_PORT) || 5000;
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
    console.log(`Baud Rate: ${BAUD_RATE}`);
    
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

                const startSummaryTag = "[SUMMARYSTART]";
                const endSummaryTag = "[SUMMARYEND]";
                
                let dataString = '';

                // 1. Parse Primary State Data
                if (rawLine.includes(startTag) && rawLine.includes(endTag)) {
                    try {
                        const startIndex = rawLine.indexOf(startTag) + startTag.length;
                        const endIndex = rawLine.indexOf(endTag);
                        dataString = rawLine.substring(startIndex, endIndex).trim();
                        
                        const values = dataString.split(',').map(v => parseInt(v.trim(), 10));
                        
                        if (values.length === 3 && !values.some(isNaN)) {
                            const data = {
                                "current_state": values[0],
                                "counter": values[1],
                                "safety_halt_released": values[2],
                                "timestamp": Math.floor(Date.now() / 1000)
                            };
                            console.log(`[STATE] Data parsed and EMITTED:`, data);
                            io.emit('stm32_update', data); 
                        } else {
                            console.warn(`[STATE] Warning: Data string has invalid format or length. Raw: ${dataString}`);
                        }

                    } catch (e) {
                        console.error(`[STATE] Error parsing data line: ${e}. Raw data: ${rawLine}`);
                    }
                }
                
                // 2. Parse Summary Data
                else if (rawLine.includes(startSummaryTag) && rawLine.includes(endSummaryTag)) {
                    try {
                        const startIndex = rawLine.indexOf(startSummaryTag) + startSummaryTag.length;
                        const endIndex = rawLine.indexOf(endSummaryTag);
                        dataString = rawLine.substring(startIndex, endIndex).trim();

                        // The summary has 6 indices: Menu, Temp, Bean, Tamping, Roast, Shots (pre-incremented)
                        const values = dataString.split(',').map(v => parseInt(v.trim(), 10));
                        
                        if (values.length === 7 && !values.some(isNaN)) {
                            const summaryData = {
                                "menu_idx": values[0],
                                "temp_idx": values[1],
                                "bean_idx": values[2],
                                "tamping_idx": values[3],
                                "roast_idx": values[4],
                                "is_safety_idx" : values[5],
                                "shots": values[6], // This is the final shot count (1-indexed)
                                "timestamp": Math.floor(Date.now() / 1000)
                            };
                            console.log(`[SUMMARY] Data parsed and EMITTED:`, summaryData);
                            // Emit using a NEW event name
                            io.emit('stm32_summary', summaryData); 
                        } else {
                             console.warn(`[SUMMARY] Warning: Data string has invalid format or length. Raw: ${dataString}`);
                        }

                    } catch (e) {
                        console.error(`[SUMMARY] Error parsing data line: ${e}. Raw data: ${rawLine}`);
                    }
                }
                // 3. Emit raw log messages
                else {
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