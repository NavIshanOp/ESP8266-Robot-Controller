# ESP8266-Robot-Controller

This project showcases an advanced robot controller built using an ESP8266 microcontroller. The controller allows for precise motor and pump control, movement recording, playback, and interactive web-based control through WebSockets.

---

## Features

- **Motor Control**: Forward, backward, left, right, and stop movements.
- **Speed Control**: Three-speed levels (low, medium, high).
- **Pump Control**: Toggle a water pump on and off via a relay.
- **Movement Recording**: Start and stop recording robot movements for later playback.
- **Playback Control**: Play back recorded movements via dynamically added buttons.
- **Return to Start**: Automatically undo movements to return the robot to its starting position.
- **Interactive Web Controller**: A user-friendly HTML interface for controlling the robot.
- **Persistent Pump State**: Saves pump state to EEPROM to retain it after a reboot.

---

## Components

### Hardware
- **ESP8266 Microcontroller**: Provides WiFi capabilities and controls the robot.
- **L298N Motor Driver**: Controls the DC motors for movement.
- **Relay Module**: Controls the water pump.
- **DC Motors**: For movement of the robot.
- **Power Supply**: Suitable for driving the ESP8266 and connected components.

### Software
- **Arduino IDE**: Used for programming the ESP8266.
- **HTML/CSS/JavaScript**: Web-based control interface hosted on the ESP8266.
- **EEPROM Library**: Stores pump state persistently.
- **WebSockets**: Handles communication between the web interface and ESP8266.

---

## Setup Instructions

1. **Install Arduino IDE**:
   - Download and install the Arduino IDE from [arduino.cc](https://www.arduino.cc/).

2. **Install ESP8266 Board**:
   - Add the ESP8266 board to the Arduino IDE through the Boards Manager.

3. **Connect Hardware**:
   - Wire the ESP8266 to the L298N motor driver, relay module, and DC motors as per the pin configuration in the code.

4. **Upload the Code**:
   - Copy the provided code into the Arduino IDE.
   - Set the SSID and password for the ESP8266 access point.
   - Select the correct board and port, then upload the code to the ESP8266.

5. **Access the Web Interface**:
   - Connect to the ESP8266's WiFi network.
   - Open a browser and navigate to the ESP8266's IP address to access the robot controller interface.

---

## Pin Configuration

| Component           | GPIO Pin | ESP8266 Pin |
|---------------------|----------|-------------|
| Motor Enable (Right)| ENA      | GPIO3 (RX)  |
| Motor IN1 (Right)   | IN_1     | GPIO16 (D0) |
| Motor IN2 (Right)   | IN_2     | GPIO5 (D1)  |
| Motor IN3 (Left)    | IN_3     | GPIO4 (D2)  |
| Motor IN4 (Left)    | IN_4     | GPIO0 (D3)  |
| Motor Enable (Left) | ENB      | GPIO1 (TX)  |
| Relay Control       | RELAY_PIN| GPIO13 (D7) |

---

## Usage

1. **Control Movement**:
   - Use the web interface buttons to move the robot forward, backward, left, right, or stop.

2. **Control Speed**:
   - Select speed levels (low, medium, high) using the speed buttons.

3. **Toggle Pump**:
   - Use the "Toggle Pump" button to turn the pump on or off.

4. **Record Movements**:
   - Click "Start Recording" to begin recording movements.
   - Click "Stop Recording" to stop and save the recording.

5. **Play Recordings**:
   - Use the dynamically added buttons to play back recorded movements.

6. **Return to Start**:
   - Click "Return to Start" to undo movements and return to the starting position.

---

## Additional Notes

- Ensure the ESP8266 and robot components are powered adequately.
- Adjust the `delay()` values in the code for fine-tuning movement durations.
- Use the Serial Monitor for debugging and monitoring robot activities.

---

### Author
**Ishan Jaiswal**
