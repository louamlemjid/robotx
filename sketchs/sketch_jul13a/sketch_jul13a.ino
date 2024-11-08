#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <ArduinoJson.h>

int waterLevelValueSentPin = 4;  // GPIO4 (D2)
int connectedToWifiPin = 5;  //GPIO5 (D1)

ESP8266WebServer server(80);

// Global variables for Wi-Fi and device/user info
String ssid;
String password;
String userName;
String deviceName = "wl10000";  // Default device name

// Tries counter for connecting to the configured network
byte tries = 10;

void setup() {
  Serial.begin(115200);

  pinMode(waterLevelValueSentPin, OUTPUT);
  pinMode(connectedToWifiPin, OUTPUT);
  digitalWrite(connectedToWifiPin, LOW);
  digitalWrite(waterLevelValueSentPin, LOW);
  // Initialize EEPROM with size 512 bytes
  EEPROM.begin(512);

  // Load saved credentials from EEPROM (if available)
  loadCredentialsFromEEPROM();

  // Start the web server for configuring SSID, password, and userName
  setupAccessPoint();

  // Handle root and config requests
  server.on("/", handleRoot);
  server.on("/config", handleWiFiConfig);  // Configuration page at /config
  server.begin();

  Serial.println("HTTP server started");

  // Try to connect to Wi-Fi if credentials are loaded
  if (ssid.length() > 0 && password.length() > 0) {
    connectToWiFi();
  }
}

void loop() {
  int waterLevelValuePin = analogRead(A0);
  Serial.println(waterLevelValuePin);
  server.handleClient();

  // If the ESP8266 is connected to Wi-Fi, make an HTTP PATCH request
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    HTTPClient http;

    client.setInsecure();  // Optionally skip SSL verification

    // Construct the URL using global variables
    String url = "https://smart-home-v418.onrender.com/" + userName + "/" + deviceName;

    http.begin(client, url); // Use global variables to form URL
    http.addHeader("Content-Type", "application/json");

    // Create the JSON payload
    DynamicJsonDocument doc(1024);
    doc["waterLevel"] = waterLevelValuePin;

    String jsonPayload;
    serializeJson(doc, jsonPayload);

    // Send PATCH request
    int httpCode = http.PATCH(jsonPayload); // Send PATCH request

    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println("Response payload:");
      Serial.println(payload);
      digitalWrite(waterLevelValueSentPin, HIGH);
      delay(500);
      digitalWrite(waterLevelValueSentPin, LOW);
    } else {
      Serial.printf("Error on HTTP request: %d\n", httpCode);
    }

    http.end();  // Close the connection
  }

  // Delay before sending the next request
  delay(500);
}

void setupAccessPoint() {
  // Configure ESP as Access Point
  WiFi.softAP("ESP8266_Config");

  Serial.println("Access Point Started");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());
}

void handleRoot() {
  String html = "<html><body>";
  html += "<h1>Welcome to ESP8266</h1>";
  html += "<p>Go to <a href='/config'>Configuration Page</a></p>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}

void handleWiFiConfig() {
  if (server.method() == HTTP_POST) {
    // Check if all the necessary arguments are present in POST
    if (server.hasArg("ssid") && server.hasArg("password") && server.hasArg("userName")) {
      ssid = server.arg("ssid");
      password = server.arg("password");
      userName = server.arg("userName");

      // Output the updated values to the serial monitor
      Serial.println("New Wi-Fi and User Config Received:");
      Serial.println("SSID: " + ssid);
      Serial.println("Password: " + password);
      Serial.println("User Name: " + userName);

      // Save the new credentials to EEPROM
      saveCredentialsToEEPROM();

      // Respond with a JSON object containing the device name
      DynamicJsonDocument jsonResponse(1024);
      jsonResponse["deviceName"] = deviceName;

      String response;
      serializeJson(jsonResponse, response);

      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(200, "application/json", response);

      // Try connecting to the new Wi-Fi
      connectToWiFi();
    } else {
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(400, "text/plain", "Missing one or more arguments: ssid, password, userName.");
    }
  } else {
    String html = "<html><body>";
    html += "<h1>WiFi and Username Setup</h1>";
    html += "<form action=\"/config\" method=\"POST\">";
    html += "SSID: <input type=\"text\" name=\"ssid\"><br>";
    html += "Password: <input type=\"text\" name=\"password\"><br>";
    html += "User Name: <input type=\"text\" name=\"userName\"><br>";
    html += "<input type=\"submit\" value=\"Submit\">";
    html += "</form></body></html>";

    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/html", html);
  }
}

void connectToWiFi() {
  WiFi.begin(ssid.c_str(), password.c_str());
  tries = 10;

  while (--tries && WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    digitalWrite(connectedToWifiPin, HIGH);
      delay(500);
      digitalWrite(connectedToWifiPin, LOW);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to Wi-Fi");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    digitalWrite(connectedToWifiPin, HIGH);
  } else {
    Serial.println("Failed to connect to Wi-Fi");
    digitalWrite(connectedToWifiPin, LOW);

  }
}

void saveCredentialsToEEPROM() {
  EEPROM.begin(512);  // Initialize EEPROM with 512 bytes size

  // Save SSID
  for (int i = 0; i < ssid.length(); i++) {
    EEPROM.write(i, ssid[i]);
  }
  EEPROM.write(ssid.length(), '\0'); // Null-terminate the string

  // Save Password
  int passStart = 100; // Start saving password at position 100
  for (int i = 0; i < password.length(); i++) {
    EEPROM.write(passStart + i, password[i]);
  }
  EEPROM.write(passStart + password.length(), '\0'); // Null-terminate the string

  // Save UserName
  int userNameStart = 200; // Start saving userName at position 200
  for (int i = 0; i < userName.length(); i++) {
    EEPROM.write(userNameStart + i, userName[i]);
  }
  EEPROM.write(userNameStart + userName.length(), '\0'); // Null-terminate the string

  EEPROM.commit();  // Save changes to EEPROM
}

void loadCredentialsFromEEPROM() {
  EEPROM.begin(512);

  // Load SSID
  char ssidChars[100];
  for (int i = 0; i < 100; i++) {
    ssidChars[i] = EEPROM.read(i);
    if (ssidChars[i] == '\0') break;
  }
  ssid = String(ssidChars);

  // Load Password
  char passwordChars[100];
  for (int i = 0; i < 100; i++) {
    passwordChars[i] = EEPROM.read(100 + i);
    if (passwordChars[i] == '\0') break;
  }
  password = String(passwordChars);

  // Load UserName
  char userNameChars[100];
  for (int i = 0; i < 100; i++) {
    userNameChars[i] = EEPROM.read(200 + i);
    if (userNameChars[i] == '\0') break;
  }
  userName = String(userNameChars);
}
