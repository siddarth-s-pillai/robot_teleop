
# **Documentation: ESP32 Dev Board Web Server LED + Signal**

## **1. Hardware Requirements**

* ESP32 Dev Board
* USB cable for programming
* No additional components required (uses onboard LED)

---

## **2. Arduino IDE Setup (Ubuntu)**

1. Install Arduino IDE:

```bash
sudo apt update
sudo apt install arduino
```

2. Install ESP32 Board Support:

   * Open **Arduino IDE > File > Preferences**
   * Add URL in **Additional Boards Manager URLs**:

```
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```

* Open **Tools > Board > Boards Manager**, search **esp32**, and install **esp32 by Espressif Systems**

3. Connect ESP32 via USB and check port:

```bash
ls /dev/ttyUSB*
```

* Example: `/dev/ttyUSB0`

4. Add user to dialout group (if not done):

```bash
sudo usermod -a -G dialout $USER
```

* Log out and log back in.

---

## **3. Arduino IDE Configuration**

* **Board:** Tools > Board > ESP32 Dev Module
* **Port:** Tools > Port > `/dev/ttyUSB0`
* **Baud:** 115200

---

## **4. Arduino Sketch (ESP32)**

```cpp
#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "YourWiFiSSID";       
const char* password = "YourWiFiPassword";

#define LED_PIN 2    // onboard LED GPIO

WebServer server(80);

const char WEB_PAGE[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head><title>ESP32 LED Blink</title></head>
<body>
  <h1>ESP32 LED Control</h1>
  <button onclick="sendData(1)">LED ON</button>
  <button onclick="sendData(0)">LED OFF</button>
  <p>LED State: <span id="LEDState">NA</span></p>

<script>
function sendData(state) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if(this.readyState==4 && this.status==200){
      document.getElementById("LEDState").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET","setLED?LEDstate="+state,true);
  xhttp.send();
}
</script>
</body>
</html>
)=====";

void handleRoot(){ server.send(200,"text/html",FPSTR(WEB_PAGE)); }

void handleLED(){
  String state="OFF";
  String arg = server.arg("LEDstate");
  if(arg=="1"){ digitalWrite(LED_PIN, HIGH); state="ON"; } // LED active HIGH
  else{ digitalWrite(LED_PIN, LOW); state="OFF"; }
  server.send(200,"text/plain",state);
}

unsigned long lastMillis = 0;

void setup(){
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  WiFi.begin(ssid,password);
  Serial.print("Connecting to WiFi");
  while(WiFi.status()!=WL_CONNECTED){ delay(500); Serial.print("."); }
  Serial.println();
  Serial.print("Connected. IP: "); Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/setLED", handleLED);
  server.begin();
  Serial.println("HTTP server started");
}

void loop(){
  server.handleClient();
  
  if(millis() - lastMillis >= 500){
    lastMillis = millis();
    Serial.println("Signal sent!");  // every 0.5s
  }
}
```

---

## **5. Testing**

1. Upload sketch to ESP32.
2. Open **Serial Monitor**, 115200 baud.
3. Press **RESET** on ESP32.
4. Check output:

```
Connecting to WiFi...
Connected. IP: 192.168.x.x
HTTP server started
Signal sent!
```

5. Open browser on same WiFi, type `http://<ESP32_IP>`
6. Use buttons to toggle onboard LED.

---

# **Documentation: NodeMCU 1.0 (ESP-12E Module) Web Server LED + Signal**

## **1. Hardware Requirements**

* NodeMCU (ESP8266)
* USB cable for programming
* No additional components required (uses onboard LED, GPIO 2 / D4)

---

## **2. Arduino IDE Setup (Ubuntu)**

1. Install Arduino IDE (same as ESP32).

2. Install ESP8266 Board Support:

   * Open **Arduino IDE > File > Preferences**
   * Add URL in **Additional Boards Manager URLs**:

```
http://arduino.esp8266.com/stable/package_esp8266com_index.json
```

* Open **Tools > Board > Boards Manager**, search **ESP8266**, and install **ESP8266 by ESP8266 Community**

3. Connect NodeMCU via USB:

```bash
ls /dev/ttyUSB*
```

* Example: `/dev/ttyUSB0`

4. Add user to dialout group (if not done):

```bash
sudo usermod -a -G dialout $USER
```

* Log out and log back in.

---

## **3. Arduino IDE Configuration**

* **Board:** Tools > Board > NodeMCU 1.0 (ESP-12E Module)
* **Port:** Tools > Port > `/dev/ttyUSB0`
* **Baud:** 115200

---

## **4. Arduino Sketch (ESP8266)**

```cpp
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char* ssid = "YourWiFiSSID";
const char* password = "YourWiFiPassword";

#define LED_PIN 2   // onboard LED GPIO

ESP8266WebServer server(80);

const char WEB_PAGE[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head><title>NodeMCU LED Blink</title></head>
<body>
  <h1>NodeMCU LED Control</h1>
  <button onclick="sendData(1)">LED ON</button>
  <button onclick="sendData(0)">LED OFF</button>
  <p>LED State: <span id="LEDState">NA</span></p>

<script>
function sendData(state) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if(this.readyState==4 && this.status==200){
      document.getElementById("LEDState").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET","setLED?LEDstate="+state,true);
  xhttp.send();
}
</script>
</body>
</html>
)=====";

void handleRoot(){ server.send(200,"text/html",FPSTR(WEB_PAGE)); }

void handleLED(){
  String state="OFF";
  String arg = server.arg("LEDstate");
  if(arg=="1"){ digitalWrite(LED_PIN,LOW); state="ON"; } // LED active LOW
  else{ digitalWrite(LED_PIN,HIGH); state="OFF"; }
  server.send(200,"text/plain",state);
}

unsigned long lastMillis = 0;

void setup(){
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN,HIGH);

  WiFi.begin(ssid,password);
  Serial.print("Connecting to WiFi");
  while(WiFi.status()!=WL_CONNECTED){ delay(500); Serial.print("."); }
  Serial.println();
  Serial.print("Connected. IP: "); Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/setLED", handleLED);
  server.begin();
  Serial.println("HTTP server started");
}

void loop(){
  server.handleClient();
  
  if(millis() - lastMillis >= 500){
    lastMillis = millis();
    Serial.println("Signal sent!");  // every 0.5s
  }
}
```

---

## **5. Testing**

1. Upload sketch to NodeMCU.
2. Open **Serial Monitor**, 115200 baud.
3. Press **RESET** on NodeMCU.
4. Check output:

```
Connecting to WiFi...
Connected. IP: 192.168.x.x
HTTP server started
Signal sent!
```

5. Open browser on same WiFi, type `http://<NodeMCU_IP>`
6. Use buttons to toggle onboard LED.

---

### ✅ **Summary of Differences Between ESP32 and NodeMCU**

| Feature        | ESP32 Dev Board                  | NodeMCU (ESP8266)                     |
| -------------- | -------------------------------- | ------------------------------------- |
| Header files   | `WiFi.h`, `WebServer.h`          | `ESP8266WiFi.h`, `ESP8266WebServer.h` |
| Onboard LED    | Usually GPIO 2, active HIGH      | GPIO 2, active LOW                    |
| WiFi stack     | Dual-core, more RAM              | Single-core, less RAM                 |
| Port selection | `/dev/ttyUSB0` or `/dev/ttyACM0` | `/dev/ttyUSB0`                        |

---
