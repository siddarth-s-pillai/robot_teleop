


# **ESP32 MQTT Testing Without ROS2 (Inside Docker Container)**

## **1. Prerequisites**

* ESP32 or ESP8266 board.
* Ubuntu host running Docker.
* Docker container for ROS2 or development environment.
* ESP32 connected to the same WiFi network as host machine.

## Install MQTT Library on ESP32

In Arduino IDE: Sketch > Include Library > Manage Libraries

Install PubSubClient library.


---

## **2. Update ESP32 Code**

Here is the updated code with correct MQTT server IP:

```cpp
#include <WiFi.h>         // ESP32, or #include <ESP8266WiFi.h> for NodeMCU
#include <PubSubClient.h>

const char* ssid = "YourWiFiSSID";
const char* password = "YourWiFiPassword";

// Update this to your host machine LAN IP
const char* mqtt_server = "192.168.0.238"; 

#define LED_PIN 2

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastMillis = 0;

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected!");

  client.setServer(mqtt_server, 1883);
  client.setCallback(mqttCallback);

  connectMQTT();
}

void connectMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
      client.subscribe("esp32/led"); // subscribe to LED commands
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 2s");
      delay(2000);
    }
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  Serial.print("Received message: ");
  Serial.println(message);

  if (message == "ON") digitalWrite(LED_PIN, HIGH);
  else if (message == "OFF") digitalWrite(LED_PIN, LOW);
}

void loop() {
  if (!client.connected()) {
    connectMQTT();
  }
  client.loop();

  if (millis() - lastMillis >= 500) {
    lastMillis = millis();
    Serial.println("Signal sent!");
    client.publish("esp32/signal", "1"); // publish signal every 0.5s
  }
}
```

**✅ Changes to note:**

1. `mqtt_server` must be the **host machine LAN IP**, not `192.168.x.x`.
2. Ensure WiFi SSID and password are correct.
3. LED pin defined as 2 — adjust if your board differs.

---

## **3. Docker Container Setup**

1. **Install Mosquitto inside container:**

```bash
sudo apt update
sudo apt install software-properties-common
sudo add-apt-repository universe
sudo apt update
sudo apt install mosquitto mosquitto-clients netcat-openbsd telnet iputils-ping -y
```

2. **Start Mosquitto manually (Docker has no systemd):**

```bash
mosquitto -v
```

> Optional: run in background: `mosquitto -d`

3. **Ensure Mosquitto listens on all interfaces:**

```bash
netstat -tlnp | grep 1883
```

Expected:

```
0.0.0.0:1883
```

If it shows `127.0.0.1:1883`, create `/etc/mosquitto/conf.d/default.conf`:

```
listener 1883 0.0.0.0
allow_anonymous true
```

Then restart Mosquitto:

```bash
mosquitto -c /etc/mosquitto/conf.d/default.conf -v
```

4. **Start the container with host networking** (so ESP32 can reach broker):

```bash
docker run -it --network host <your_image>
```

---

## **4. Check Connectivity Inside Container**

1. Ping host from container:

```bash
ping -c 4 192.168.0.238
```

Expected:

```
4 packets transmitted, 4 received, 0% packet loss
```

2. Test Mosquitto with publish/subscribe:

```bash
# Subscribe to a test topic
mosquitto_sub -h 192.168.0.238 -t "test"

# Publish a message from another terminal
mosquitto_pub -h 192.168.0.238 -t "test" -m "hello"
```

You should see `"hello"` in the subscriber terminal.

---

## **5. Flash ESP32 and Test**

1. Upload the updated code to ESP32.
2. Open Serial Monitor (115200 baud) and observe:

```
Connecting to WiFi...Connected!
Connecting to MQTT...connected
Signal sent!
```

3. Test commands:

```bash
# Turn LED on
mosquitto_pub -h 192.168.0.238 -t "esp32/led" -m "ON"

# Turn LED off
mosquitto_pub -h 192.168.0.238 -t "esp32/led" -m "OFF"
```

Serial monitor should show:

```
Received message: ON
Received message: OFF
```

LED on ESP32 should toggle accordingly.

---

## **6. Troubleshooting Checklist**

| Symptom              | Likely Cause                          | Solution                                                               |
| -------------------- | ------------------------------------- | ---------------------------------------------------------------------- |
| rc = -2 (MQTT)       | ESP32 cannot reach broker             | Ensure `mqtt_server` = host LAN IP and container is using host network |
| Connection refused   | Mosquitto listening only on 127.0.0.1 | Update config: `listener 1883 0.0.0.0`                                 |
| No messages seen     | Topic mismatch                        | Verify topic strings match (`esp32/led` or `esp32/signal`)             |
| Serial monitor empty | WiFi not connected                    | Check WiFi SSID/password                                               |

---

This documentation now covers:

1. **Updated ESP32 code**
2. **Docker container setup**
3. **Networking checks**
4. **Mosquitto configuration**
5. **Testing MQTT publish/subscribe**
6. **Troubleshooting steps**

