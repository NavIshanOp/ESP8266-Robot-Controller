#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <EEPROM.h>
#include <vector>

// SSID and Password for your ESP Access Point
const char* ssid = "volder";
const char* password = "volder@9810";

// Pins for motor control
#define ENA   3    // Enable/speed motors Right    GPIO3(RX)
#define IN_1  16   // L298N in1 motors Right       GPIO16(D0)
#define IN_2  5    // L298N in2 motors Right       GPIO5(D1)
#define IN_3  4    // L298N in3 motors Left        GPIO4(D2)
#define IN_4  0    // L298N in4 motors Left        GPIO0(D3)
#define ENB   1    // Enable/speed motors Left     GPIO1(TX)

// Pin for relay control
#define RELAY_PIN 13 // GPIO13(D7) for controlling the relay module

int speedCar = 255; // 0 to 255
char Data;
bool pumpState = false; // Variable to store pump state
bool commandReceived = false; // Flag to track command reception
bool isRecording = false; // Flag to track recording state

struct Movement {
    char direction;
    unsigned long timestamp;
};

std::vector<Movement> movements; // Vector to store movements
std::vector<std::vector<Movement>> recordedMovementsList; // Vector to store multiple recordings

static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Ishan's Robot</title>
<style>
  body {
    background-color: #808080;
    font-family: Arial, Helvetica, Sans-Serif;
    color: #000000;
  }
  #robot-controller {
    text-align: center;
    font-family: "Lucida Sans Unicode", "Lucida Grande", sans-serif;
    font-size: 24px;
    padding: 10px;
    background-color: #FFFF33;
    color: #000000;
    border-radius: 8px;
  }
  .button {
    display: block;
    width: 100%;
    padding: 15px;
    margin: 10px 0;
    font-size: 18px;
    border: none;
    border-radius: 50px; /* Changed to create rounded shape */
    cursor: pointer;
    transition: background-color 0.3s ease;
  }
  .button.green {
    background-color: #090;
    color: #FFFFFF;
  }
  .button.red {
    background-color: #F00;
    color: #FFFFFF;
  }
  .button.blue {
    background-color: #03C;
    color: #FFFFFF;
  }
  .footer {
    text-align: center;
    font-family: "Comic Sans MS", cursive;
    font-size: 20px;
    color: #F00;
    margin-top: 20px;
  }
  .button-container {
    display: flex;
    justify-content: center;
    gap: 10px;
  }
</style>
  <script>
    var websock;
    function start() {
      websock = new WebSocket('ws://' + window.location.hostname + ':81/');
      websock.onopen = function(evt) { console.log('websock open'); };
      websock.onclose = function(evt) { console.log('websock close'); };
      websock.onerror = function(evt) { console.log(evt); };
      websock.onmessage = function(evt) {
        console.log(evt);
        var e = document.getElementById('ledstatus');
        if (evt.data === 'ledon') {
          e.style.color = 'red';
        } else if (evt.data === 'ledoff') {
          e.style.color = 'black';
        } else if (evt.data.startsWith('addButton')) {
          var index = evt.data.substring(9);
          addPlayButton(index);
        } else {
          console.log('unknown event');
        }
      };
    }
    function buttonclick(e) {
      websock.send(e.id);
    }
    function togglePump() {
      var pumpBtn = document.getElementById("pumpBtn");
      pumpBtn.innerHTML = pumpBtn.innerHTML === 'Turn Pump On' ? 'Turn Pump Off' : 'Turn Pump On';
      websock.send(pumpBtn.innerHTML === 'Turn Pump On' ? 'relayon' : 'relayoff');
    }
    function returnToStart() {
      websock.send('return');
    }
    function startRecording() {
      websock.send('startRecording');
    }
    function stopRecording() {
      websock.send('stopRecording');
    }
    function playRecording(index) {
      websock.send('playRecording' + index);
    }
    function addPlayButton(index) {
      var buttonContainer = document.getElementById('playButtonsContainer');
      var button = document.createElement('button');
      button.id = 'playRecBtn' + index;
      button.type = 'button';
      button.className = 'button blue';
      button.innerHTML = 'Play Recording ' + (index + 1);
      button.onclick = function() { playRecording(index); };
      buttonContainer.appendChild(button);
    }
  </script>
</head>
<body onload="javascript:start();">
  <div id="robot-controller">
    Robot Controller
  </div>
  <div align="center">
    <button id="F" type="button" onclick="buttonclick(this);" class="button green">Forward</button>
    <button id="L" type="button" onclick="buttonclick(this);" class="button green">Turn Left</button>
    <button id="S" type="button" onclick="buttonclick(this);" class="button red">Stop</button>
    <button id="R" type="button" onclick="buttonclick(this);" class="button green">Turn Right</button>
    <button id="B" type="button" onclick="buttonclick(this);" class="button green">Backward</button>
    <button id="a" type="button" onclick="buttonclick(this);" class="button blue">Speed Low</button>
    <button id="b" type="button" onclick="buttonclick(this);" class="button blue">Speed Medium</button>
    <button id="c" type="button" onclick="buttonclick(this);" class="button blue">Speed High</button>
    <button id="pumpBtn" type="button" onclick="togglePump();" class="button blue">Toggle Pump</button>
    <button id="returnBtn" type="button" onclick="returnToStart();" class="button red">Return to Start</button>
    <div class="button-container">
      <button id="startRecBtn" type="button" onclick="startRecording();" class="button blue">Start Recording</button>
      <button id="stopRecBtn" type="button" onclick="stopRecording();" class="button blue">Stop Recording</button>
    </div>
    <div id="playButtonsContainer"></div>
  </div>
  <p class="footer">Ishan Jaiswal</p>
</body>
</html>
)rawliteral";

WebSocketsServer webSocket = WebSocketsServer(81);
ESP8266WebServer server(80);

void setup() {
  Serial.begin(115200);

  pinMode(ENA, OUTPUT);
  pinMode(IN_1, OUTPUT);
  pinMode(IN_2, OUTPUT);
  pinMode(IN_3, OUTPUT);
  pinMode(IN_4, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT); // Set relay pin as output

  EEPROM.begin(512); // Initialize EEPROM
  pumpState = EEPROM.read(0); // Read pump state from EEPROM
  digitalWrite(RELAY_PIN, pumpState ? HIGH : LOW); // Set relay to stored state

  delay(1000);
  Serial.println(">> Setup");
  WiFi.mode(WIFI_AP);           // Only Access point
  WiFi.softAP(ssid, password);  // Start HOTspot removing password will disable security

  IPAddress myIP = WiFi.softAPIP(); // Get IP address
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(myIP);

  server.on("/", []() {
    server.send(200, "text/html", INDEX_HTML);
  });

  server.begin();
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  if (commandReceived) {
    if (Data == 'a') speedCar = 70;
    else if (Data == 'b') speedCar = 150;
    else if (Data == 'c') speedCar = 255;
    else if (Data == 'F') forward();
    else if (Data == 'B') backward();
    else if (Data == 'R') turnRight();
    else if (Data == 'L') turnLeft();
    else if (Data == 'S') stop();

    analogWrite(ENA, speedCar);
    analogWrite(ENB, speedCar);
    commandReceived = false; // Reset command received flag
  }

  webSocket.loop();
  server.handleClient();
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *Payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {
      IPAddress ip = webSocket.remoteIP(num);
      Serial.printf("[%u] Connection from ", num);
      Serial.println(ip.toString());
      webSocket.sendTXT(num, "Connected");
    }
      break;
    case WStype_TEXT:
      Data = (char)Payload[0];
      commandReceived = true; // Set command received flag

      if (strcmp((char *)Payload, "relayon") == 0) {
        digitalWrite(RELAY_PIN, HIGH); // Turn on relay
        pumpState = true; // Update pump state
      }
      else if (strcmp((char *)Payload, "relayoff") == 0) {
        digitalWrite(RELAY_PIN, LOW); // Turn off relay
        pumpState = false; // Update pump state
      }

      EEPROM.write(0, pumpState); // Store pump state in EEPROM
      EEPROM.commit(); // Save changes to EEPROM

      // Check for recording control commands
      if (strcmp((char *)Payload, "startRecording") == 0) {
        startRecording();
      } else if (strcmp((char *)Payload, "stopRecording") == 0) {
        stopRecording();
      } else if (strncmp((char *)Payload, "playRecording", 13) == 0) {
        int index = atoi((char *)Payload + 13); // Extract index from payload
        playRecording(index);
      } else if (strcmp((char *)Payload, "return") == 0) {
        returnToStart();
      }
      break;
  }
}

void startRecording() {
  isRecording = true;
  movements.clear(); // Clear current movements
  Serial.println("Recording started");
}

void stopRecording() {
    isRecording = false;
    recordedMovementsList.push_back(movements); // Save current movements to recordedMovementsList
    int recordingIndex = recordedMovementsList.size() - 1;
    String message = "addButton" + String(recordingIndex); // Create a String object
    webSocket.broadcastTXT(message); // Inform clients to add a new play button
    Serial.println("Recording stopped");
}

void playRecording(int index) {
  if (index >= 0 && index < recordedMovementsList.size()) {
    Serial.printf("Playing recording %d\n", index + 1);
    for (const auto &movement : recordedMovementsList[index]) {
      switch (movement.direction) {
        case 'F':
          forward();
          break;
        case 'B':
          backward();
          break;
        case 'R':
          turnRight();
          break;
        case 'L':
          turnLeft();
          break;
        case 'S':
          stop();
          break;
      }
      delay(500); // Adjust delay based on movement duration
      stop(); // Stop between movements
    }
  } else {
    Serial.println("Invalid recording index");
  }
}

void recordMovement(char direction) {
  unsigned long currentTime = millis();
  movements.push_back({direction, currentTime});
}

void forward() {
  digitalWrite(IN_1, HIGH);
  digitalWrite(IN_2, LOW);
  digitalWrite(IN_3, LOW);
  digitalWrite(IN_4, HIGH);
  recordMovement('F');
}

void backward() {
  digitalWrite(IN_1, LOW);
  digitalWrite(IN_2, HIGH);
  digitalWrite(IN_3, HIGH);
  digitalWrite(IN_4, LOW);
  recordMovement('B');
}

void turnRight() {
  digitalWrite(IN_1, HIGH);
  digitalWrite(IN_2, LOW);
  digitalWrite(IN_3, HIGH);
  digitalWrite(IN_4, LOW);
  recordMovement('R');
}

void turnLeft() {
  digitalWrite(IN_1, LOW);
  digitalWrite(IN_2, HIGH);
  digitalWrite(IN_3, LOW);
  digitalWrite(IN_4, HIGH);
  recordMovement('L');
}

void stop() {
  digitalWrite(IN_1, LOW);
  digitalWrite(IN_2, LOW);
  digitalWrite(IN_3, LOW);
  digitalWrite(IN_4, LOW);
  recordMovement('S');
}

void returnToStart() {
  for (auto it = movements.rbegin(); it != movements.rend(); ++it) {
    switch (it->direction) {
      case 'F':
        backward();
        delay(500); // Adjust delay for time taken to undo the movement
        stop();
        break;
      case 'B':
        forward();
        delay(500);
        stop();
        break;
      case 'R':
        turnLeft();
        delay(500);
        stop();
        break;
      case 'L':
        turnRight();
        delay(500);
        stop();
        break;
    }
  }
  movements.clear(); // Clear the movements after returning to the start
}