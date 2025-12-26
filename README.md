# Machine Health Monitoring using ESP8266

This project monitors machine vibration using a photoelectric sensor and displays
real-time data on a web dashboard hosted by ESP8266.

## Features
- Real-time RPM and frequency measurement
- Machine health estimation
- Downtime tracking
- Web-based dashboard with graphs
- LED and buzzer alerts

## Hardware Used
- ESP8266 (NodeMCU)
- Photoelectric sensor
- Motor
- LED
- Buzzer

## Software Used
- Arduino IDE
- ESP8266 Board Package
- Chart.js (for visualization)

## How it Works
ESP8266 counts sensor pulses using interrupts of a piece of reflection tape on disc connected to motor shaft, calculates RPM, and hosts a web page that displays machine health parameters like energy consumption, down time, RPM, frequency, machine status and graph in real time.

## Author
Rithika

