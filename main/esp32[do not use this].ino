#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <Preferences.h>
#include <vector>

// SSID and Password for your ESP Access Point
const char* ssid = "volder";
const char* password = "volder@9810";

// Pins for motor control (update these as per your ESP32 setup)
#define ENA   23  // Enable/speed motors Right
#define IN_1  22  // L298N in1 motors Right
#define IN_2  21  // L298N in2 motors Right
#define IN_3  19  // L298N in3 motors Left
#define IN_4  18  // L298N in4 motors Left
#define ENB   5   // Enable/speed motors Left

// Pin for relay control
#define RELAY_PIN 13 // GPIO13 for controlling the relay module

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

Preferences preferences; // Non-volatile storage

WebSocketsServer webSocket(81);
WebServer server(80);

// HTML content
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

void setup() {
  Serial.begin(115200);

  // Motor pins setup
  pinMode(ENA, OUTPUT);
  pinMode(IN_1, OUTPUT);
  pinMode(IN_2, OUTPUT);
  pinMode(IN_3, OUTPUT);
  pinMode(IN_4, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT); // Set relay pin as output

  // Initialize Preferences for non-volatile storage
  preferences.begin("robot", false);
  pumpState = preferences.getBool("pumpState", false);
  digitalWrite(RELAY_PIN, pumpState ? HIGH : LOW);

  // Set up WiFi
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("IP Address: ");
  Serial.println(myIP);

  // Start web server
  server.on("/", []() {
    server.send(200, "text/html", INDEX_HTML);
  });
  server.begin();

  // Start WebSocket server
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  if (commandReceived) {
    handleCommand();
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
    case WStype_CONNECTED:
      Serial.printf("[%u] Connected\n", num);
      webSocket.sendTXT(num, "Connected");
      break;
    case WStype_TEXT:
      Data = (char)Payload[0];
      commandReceived = true;

      // Handle specific commands
      if (strcmp((char*)Payload, "relayon") == 0) {
        digitalWrite(RELAY_PIN, HIGH);
        pumpState = true;
      } else if (strcmp((char*)Payload, "relayoff") == 0) {
        digitalWrite(RELAY_PIN, LOW);
        pumpState = false;
      }
      preferences.putBool("pumpState", pumpState);

      // Recording commands
      if (strcmp((char*)Payload, "startRecording") == 0) startRecording();
      else if (strcmp((char*)Payload, "stopRecording") == 0) stopRecording();
      else if (strncmp((char*)Payload, "playRecording", 13) == 0) {
        int index = atoi((char*)Payload + 13);
        playRecording(index);
      } else if (strcmp((char*)Payload, "return") == 0) returnToStart();
      break;
  }
}

void handleCommand() {
  if (Data == 'a') speedCar = 70;
  else if (Data == 'b') speedCar = 150;
  else if (Data == 'c') speedCar = 255;
  else if (Data == 'F') forward();
  else if (Data == 'B') backward();
  else if (Data == 'R') turnRight();
  else if (Data == 'L') turnLeft();
  else if (Data == 'S') stop();

  // Use ledcWrite for PWM on ESP32
  ledcWrite(0, speedCar); // Channel 0 for ENA
  ledcWrite(1, speedCar); // Channel 1 for ENB
}

void startRecording() {
  isRecording = true;
  movements.clear();
  Serial.println("Recording started");
}

void stopRecording() {
  isRecording = false;
  recordedMovementsList.push_back(movements);
  String message = "addButton" + String(recordedMovementsList.size() - 1);
  webSocket.broadcastTXT(message);
  Serial.println("Recording stopped");
}

void playRecording(int index) {
  if (index < recordedMovementsList.size()) {
    Serial.printf("Playing recording %d\n", index + 1);
    for (const auto& movement : recordedMovementsList[index]) {
      switch (movement.direction) {
        case 'F': forward(); break;
        case 'B': backward(); break;
        case 'R': turnRight(); break;
        case 'L': turnLeft(); break;
        case 'S': stop(); break;
      }
      delay(500); // Adjust based on movement duration
      stop();
    }
  } else {
    Serial.println("Invalid recording index");
  }
}

void returnToStart() {
  for (auto it = movements.rbegin(); it != movements.rend(); ++it) {
    switch (it->direction) {
      case 'F': backward(); delay(500); stop(); break;
      case 'B': forward(); delay(500); stop(); break;
      case 'R': turnLeft(); delay(500); stop(); break;
      case 'L': turnRight(); delay(500); stop(); break;
    }
  }
  movements.clear();
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

void recordMovement(char direction) {
  movements.push_back({direction, millis()});
}