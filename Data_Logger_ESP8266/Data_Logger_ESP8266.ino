#include <Adafruit_ADS1X15.h>
#include <Wire.h>
#include <ThingSpeak.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>

#include <ArduinoJson.h>

// or Software Serial on Uno, Nano...
#include <SoftwareSerial.h>
SoftwareSerial EspSerial(0, 1);  // RX, TX

// Wi-Fi credentials
// const char *ssid = "CCSPL";          // Wi-Fi name
// const char *password = "CCSPL@123";  // Wi-Fi password

// Wi-Fi credentials
const char* ssid = "vivo Y35";     // Wi-Fi name
const char* password = "12344321";    // Wi-Fi password

// ThingSpeak Settings
//unsigned long channelIDDaily = 2881821;    // Replace with your ThingSpeak Daily Channel ID
//const char * writeAPIKeyDaily = "SK07FLNJHK0JG02G";    // Daily Write API Key
//unsigned long channelIDMonthly = 2881821;    // Replace with your ThingSpeak Monthly Channel ID
//const char * writeAPIKeyMonthly = "SK07FLNJHK0JG02G";    // Monthly Write API Key

unsigned long channelIDDaily = 3011698;               // Replace with your ThingSpeak Daily Channel ID
const char *writeAPIKeyDaily = "2E6R1A14KA43Z1UO";    // Daily Write API Key
unsigned long channelIDMonthly = 3011698;             // Replace with your ThingSpeak Monthly Channel ID
const char *writeAPIKeyMonthly = "2E6R1A14KA43Z1UO";  // Monthly Write API Key

// Static IP configuration
IPAddress local_IP(192, 168, 7, 100);  // Static IP address (change as needed)
IPAddress gateway(192, 168, 7, 1);     // Router gateway address (usually your router's IP)
IPAddress subnet(255, 255, 255, 0);    // Subnet mask
IPAddress dns(8, 8, 8, 8);             // DNS server (optional, Google DNS used here)

// Define pins and variables


const int switchPin = D1;       // Pin for switch status (example)
int productionCount = 0;        // Production count (number of beads)
unsigned long powerOnTime = 0;  // Power on time
unsigned long arcOnTime = 0;    // Arc on time
unsigned long idleTime = 0;     // Idle time

// Constants for scaling ADC value to voltage
const float ADC_MAX = 1023.0;  // Maximum ADC value
const float V_REF = 3.3;       // Reference voltage (1V for ESP8266)

// External ADC Configuration (ADC128D828)
const int ADC_ADDR = 0x28;  // I2C address for ADC128D828 (default: 0x28)

//*******
String myString = "";
char rdata;
int Current = 0, Voltage = 0, EnergyCunsumption = 0, sensor3 = 0;
//*******




int currentVar;              // Variable current
float voltageVar;            // Variable voltage
float currentActual = 1.0;   // Actual current
float voltageActual = 2.5;   // Actual voltage
float phaseEnergy[3];        // 3 phase energy consumption
float gasConsumption = 8.0;  // Gas consumption
bool switchStatus = false;   // Switch status (on/off)
String ipAddress = "";       // Variable to store IP address
String sw_val = "";

// Create an AsyncWebServer object on port 80
AsyncWebServer server(80);

WiFiClient client;

// Wi-Fi connection
void setup() {
  Serial.begin(9600);

  pinMode(D3, INPUT);
  pinMode(D2, OUTPUT);
  pinMode(D4, INPUT);


  // Set static IP configuration
  if (!WiFi.config(local_IP, gateway, subnet, dns)) {
    Serial.println("STA Failed to configure static IP");
  }

  // Connect to Wi-Fi network
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  // Get the IP address after successful connection
  ipAddress = WiFi.localIP().toString();

  // Print the static IP address assigned
  Serial.println("Connected to WiFi");
  Serial.print("Static IP Address: ");
  Serial.println(WiFi.localIP());  // This will be your static IP

  ThingSpeak.begin(client);  // Initialize ThingSpeak

  // Pin setup
  pinMode(switchPin, OUTPUT);    // Set switchPin as OUTPUT for controlling the switch
  digitalWrite(switchPin, LOW);  // Initially, set the machine to OFF

  // Start the web server
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = "<html><head><title>Welding Machine Datalogger</title>";
    html += "<script src='https://cdn.jsdelivr.net/npm/chart.js'></script>";

  
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; margin: 20px; background-color: black; color: white; display: flex; flex-direction: column; align-items: center; }";
    html += "h1 { text-align: center; margin-bottom: 20px; }";

    html += ".param-container { display: flex; flex-wrap: wrap; justify-content: space-between; gap: 10px; width: 100%; max-width: 1200px; box-sizing: border-box; }";
    html += ".param-box { background-color: #333; padding: 10px; border-radius: 10px; width: 30%; text-align: center; box-sizing: border-box; }";
    html += ".param-box h3 { margin: 5px 0; font-size: 16px; }";
    html += ".param-box span { font-size: 18px; font-weight: bold; color: #00FF00; }";
    html += ".chart-container { display: flex; justify-content: space-between; margin-bottom: 20px; flex-wrap: wrap; width: 100%; max-width: 1200px; }";
    html += ".chart-container canvas { max-width: 48%; max-height: 250px; width: 100%; height: 100%; margin-bottom: 20px; background-color: #333; }";
    //html += "chart-Colour { background-color: #333; }";
    html += "button { background-color: #4CAF50; color: white; padding: 10px 20px; margin: 10px; border: none; border-radius: 5px; cursor: pointer; }";
    html += "button:hover { background-color: #45a049; }";
    html += "@media (max-width: 768px) { .param-box { width: 45%; } .chart-container canvas { max-width: 100%; } }";
    html += "@media (max-width: 480px) { .param-box { width: 100%; margin-bottom: 10px; } .chart-container canvas { max-width: 100%; } button { width: 100%; } }";
    html += "</style>";
    html += "</head><body>";
    html += "<h1>Welding Machine Datalogger</h1>";

    // Displaying the parameters in boxes side by side
    html += "<div class='param-container'>";
    html += "<div class='param-box'><h3>Production Count</h3><span id='production_count'>" + String(productionCount) + "</span></div>";
    html += "<div class='param-box'><h3>Power On Time</h3><span id='power_on_time'>" + String(powerOnTime) + "</span> seconds</div>";
    html += "<div class='param-box'><h3>Arc On Time</h3><span id='arc_on_time'>" + String(arcOnTime) + "</span> seconds</div>";
    html += "<div class='param-box'><h3>Idle Time</h3><span id='idle_time'>" + String(idleTime) + "</span> seconds</div>";
    html += "<div class='param-box'><h3>Weld Voltage</h3><span id='weld_voltage'>" + String(voltageVar) + "</span> V</div>";
    html += "<div class='param-box'><h3>Weld Current</h3><span id='weld_current'>" + String(currentVar) + "</span> A</div>";
    html += "<div class='param-box'><h3>Energy Consumption</h3><span id='energy_consumption'>" + String(phaseEnergy[0]) + "</span> Wh</div>";
    html += "<div class='param-box'><h3>Gas Consumption</h3><span id='gas_consumption'>" + String(gasConsumption) + "</span> units</div>";
    html += "<div class='param-box'><h3>Switch Status</h3><span id='switch_status'>" + String(switchStatus ? "ON" : "OFF") + "</span></div>";
    html += "<div class='param-box'><h3>Switch Status</h3><span id='switch_status'>" + String(sw_val) + "</span></div>";
    html += "<div class='param-box'><h3>IP Address</h3><span id='ip_address'>" + ipAddress + "</span></div>";
    //html += "<p>Voltage: " + String(Voltage) + "</p>";

    html += "</div>";

    // On and Off buttons
    html += "<button onclick='toggleMachine(true)'>ON</button>";
    html += "<button onclick='toggleMachine(false)'>OFF</button>";

    // Chart.js for graphical display
    html += "<div class='chart-container'>";
    html += "<canvas  id='weldCurrentChart'></canvas>";
    html += "<canvas id='weldVoltageChart'></canvas>";
    html += "</div>";
    html += "<div class='chart-container'>";
    html += "<canvas id='gasConsumptionChart'></canvas>";
    html += "<canvas id='energyConsumptionChart'></canvas>";
    html += "</div>";

    html += "<script>";

    // Toggle machine function (ON/OFF)
    html += "function toggleMachine(On) {";
    html += "  var xhr = new XMLHttpRequest();";
    html += "  var action = On ? 'on' : 'off';";
    html += "  xhr.open('GET', '/toggle?state=' + action, true);";
    html += "  xhr.send();";
    html += "  xhr.onload = function() {";
    html += "    if (xhr.status == 200) {";
    html += "      document.getElementById('switch_status').innerText = xhr.responseText;";
    html += "    }";
    html += "  };";
    html += "}";

    // Initialize the charts with dummy data (can be replaced with real data)
    html += "var ctx1 = document.getElementById('weldCurrentChart').getContext('2d');";
    html += "var ctx2 = document.getElementById('weldVoltageChart').getContext('2d');";
    html += "var ctx3 = document.getElementById('gasConsumptionChart').getContext('2d');";
    html += "var ctx4 = document.getElementById('energyConsumptionChart').getContext('2d');";

    // Initialize arrays to store data
    html += "var timeLabels = [];";
    html += "var weldCurrentData = [];";
    html += "var weldVoltageData = [];";
    html += "var gasConsumptionData = [];";
    html += "var energyConsumptionData = [];";
    html += "var weldCurrentChart = new Chart(ctx1, {";
    html += "type: 'line',";
    html += "data: { labels: timeLabels, datasets: [{ label: 'Weld Current (A)', data: weldCurrentData, borderColor: 'green', fill: false }] },";

    html += "options: { scales: { x: { type: 'linear', position: 'bottom', title: { display: true, text: 'Time (s)',  } }, y: { beginAtZero: true, title: { display: true, text: 'ADC Value' } } } }";
    html += "});";

    html += "var weldVoltageChart = new Chart(ctx2, {";
    html += "type: 'line',";
    html += "data: { labels: timeLabels, datasets: [{ label: 'Weld Voltage (V)', data: weldVoltageData, borderColor: 'blue', fill: false }] },";
    html += "options: { scales: { x: { type: 'linear', position: 'bottom', title: { display: true, text: 'Time (s)' } }, y: { beginAtZero: true, title: { display: true, text: 'ADC Value' } } } }";
    html += "});";

    html += "var gasConsumptionChart = new Chart(ctx3, {";
    html += "type: 'line',";
    html += "data: { labels: timeLabels, datasets: [{ label: 'Gas Consumption', data: gasConsumptionData, borderColor: 'red', fill: false }] },";
    html += "options: { scales: { x: { type: 'linear', position: 'bottom', title: { display: true, text: 'Time (s)' } }, y: { beginAtZero: true, title: { display: true, text: 'ADC Value' } } } }";
    html += "});";

    html += "var energyConsumptionChart = new Chart(ctx4, {";
    html += "type: 'line',";
    html += "data: { labels: timeLabels, datasets: [{ label: 'Energy Consumption (Wh)', data: energyConsumptionData, borderColor: 'yellow', fill: false }] },";
    html += "options: { scales: { x: { type: 'linear', position: 'bottom', title: { display: true, text: 'Time (s)' } }, y: { beginAtZero: true, title: { display: true, text: 'ADC Value' } } } }";
    html += "});";

    // Update charts with new data
    html += "function updateCharts() {";
    html += "  fetch('/data')";
    html += "    .then(response => response.json())";
    html += "    .then(data => {";
    html += "      var currentTime = Date.now() / 1000;";
    html += "      timeLabels.push(currentTime);";
    html += "      weldCurrentData.push(data.current);";
    html += "      weldVoltageData.push(data.voltage);";
    html += "      gasConsumptionData.push(data.gas);";
    html += "      energyConsumptionData.push(data.energy);";
    html += "      if (timeLabels.length > 50) {";
    html += "        timeLabels.shift();";
    html += "        weldCurrentData.shift();";
    html += "        weldVoltageData.shift();";
    html += "        gasConsumptionData.shift();";
    html += "        energyConsumptionData.shift();";
    html += "      }";
    html += "      weldCurrentChart.update();";
    html += "      weldVoltageChart.update();";
    html += "      gasConsumptionChart.update();";
    html += "      energyConsumptionChart.update();";
    html += "    })";
    html += "    .catch(error => console.error('Error fetching data:', error));";
    html += "}";

    html += "setInterval(updateCharts, 1000);";
    html += "</script>";
    html += "</body></html>";

    request->send(200, "text/html", html);
  });

  // Serve the data as JSON
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Replace with real sensor data

    StaticJsonDocument<200> doc;

    doc["current"] = currentVar;
    doc["voltage"] = voltageVar;
    doc["gas"] = gasConsumption;
    doc["energy"] = phaseEnergy[0];  // For simplicity, just sending the first phase energy
    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
  });

  // Handle toggle machine (ON/OFF)
  server.on("/toggle", HTTP_GET, [](AsyncWebServerRequest *request) {
    String state = request->getParam("state")->value();
    if (state == "on") {
      digitalWrite(switchPin, HIGH);
      request->send(200, "text/plain", "ON");
    } else if (state == "off") {
      digitalWrite(switchPin, LOW);
      request->send(200, "text/plain", "OFF");
    } else {
      request->send(400, "text/plain", "Invalid State");
    }
  });


  // Start server
  server.begin();
}

void loop() {


  //---------------------------------------------------------------------------
  // this lines are Switch Status Showing
  if (digitalRead(D4) == LOW) sw_val = String("Switch Status : ") + String("ON");
  if (digitalRead(D4) == HIGH) sw_val = String("Switch Status : ") + String("OFF");
  
  //---------------------------------------------------------------------------

  //***********
  readSerialFromArduino();
  //*****************

  //**********
  // FOR LED CODE
  int btn1 = digitalRead(D3);
  digitalWrite(D3, LOW);
  if (btn1 == 1) {
    digitalWrite(D2, HIGH);
    Serial.println(btn1);
    delay(100);
  } else {
    digitalWrite(D2, LOW);
    Serial.println(btn1);
    delay(100);
  }

  //***********

  currentVar = Current;         // Variable current (ADC channel 0)
  voltageVar = Voltage;         // Variable voltage (ADC channel 1)
  currentActual = readADC(2);   // Actual current (ADC channel 2)
  voltageActual = readADC(3);   // Actual voltage (ADC channel 3)
  phaseEnergy[0] = readADC(4);  // Phase 1 energy consumption (ADC channel 4)
  phaseEnergy[1] = readADC(5);  // Phase 2 energy consumption (ADC channel 5)
  phaseEnergy[2] = readADC(6);  // Phase 3 energy consumption (ADC channel 6)
  gasConsumption = readADC(7);  // Gas consumption (ADC channel 7)

  // Send data to the Daily ThingSpeak channel
  ThingSpeak.setField(1, currentVar);
  ThingSpeak.setField(2, voltageVar);
  ThingSpeak.setField(3, phaseEnergy[0]);
  ThingSpeak.setField(4, gasConsumption);
  ThingSpeak.setField(5, (long)powerOnTime);  // Casting to long
  ThingSpeak.setField(6, (long)arcOnTime);    // Casting to long
  ThingSpeak.setField(7, (long)idleTime);     // Casting to long
  ThingSpeak.setField(8, productionCount);

  // Update the Daily channel
  int httpCodeDaily = ThingSpeak.writeFields(channelIDDaily, writeAPIKeyDaily);
  if (httpCodeDaily == 200) {
    Serial.println("Daily data sent to ThingSpeak successfully!");
  } else {
    Serial.print("Failed to send Daily data to ThingSpeak!");

    Serial.print(currentVar);
  }

  // Send data to the Monthly ThingSpeak channel
  int httpCodeMonthly = ThingSpeak.writeFields(channelIDMonthly, writeAPIKeyMonthly);
  if (httpCodeMonthly == 200) {
    Serial.println("Monthly data sent to ThingSpeak successfully!");
  } else {
    Serial.println("Failed to send Monthly data to ThingSpeak!");
  }

  // Check if 24 hours have passed since the last data send
  static unsigned long lastSendTime = 0;
  unsigned long currentMillis = millis();

  if (currentMillis - lastSendTime >= 86400000 || (currentMillis < lastSendTime && (ULONG_MAX - lastSendTime + currentMillis >= 86400000))) {
    // Send data to the Daily and Monthly ThingSpeak channels again (only once per day)

    // Send data to the Daily ThingSpeak channel
    ThingSpeak.setField(1, currentVar);
    ThingSpeak.setField(2, voltageVar);
    ThingSpeak.setField(3, phaseEnergy[0]);
    ThingSpeak.setField(4, gasConsumption);
    ThingSpeak.setField(5, (long)powerOnTime);  // Casting to long
    ThingSpeak.setField(6, (long)arcOnTime);    // Casting to long
    ThingSpeak.setField(7, (long)idleTime);     // Casting to long
    ThingSpeak.setField(8, productionCount);

    // Update the Daily channel
    int httpCodeDaily = ThingSpeak.writeFields(channelIDDaily, writeAPIKeyDaily);
    if (httpCodeDaily == 200) {
      Serial.println("Daily data sent to ThingSpeak successfully!");
    } else {
      Serial.print("Failed to send Daily data to ThingSpeak!");
    }

    // Send data to the Monthly ThingSpeak channel
    int httpCodeMonthly = ThingSpeak.writeFields(channelIDMonthly, writeAPIKeyMonthly);
    if (httpCodeMonthly == 200) {
      Serial.println("Monthly data sent to ThingSpeak successfully!");
    } else {
      Serial.println("Failed to send Monthly data to ThingSpeak!");
    }

    lastSendTime = currentMillis;  // Reset last send time to the current time
  }


  // Simulating time-based changes
  powerOnTime++;
  arcOnTime++;
  idleTime++;

  // Increment production count (could be based on actual production logic)
  productionCount++;

  delay(1000);
}

float readADC(int channel) {
  // Read the ADC value for the given channel (0 to 7)
  Wire.beginTransmission(ADC_ADDR);
  Wire.write(0x00 | (channel << 3));  // Select the channel (0-7)
  Wire.endTransmission();
  Wire.requestFrom(ADC_ADDR, 2);  // Request 2 bytes (16-bit data)

  int adcValue = Wire.read() << 8 | Wire.read();  // Combine the 2 bytes into 1 value
  float voltage = (adcValue / ADC_MAX) * V_REF;   // Convert ADC value to voltage

  return voltage;  // Return the voltage
}

//*********
// Read from Arduino
void readSerialFromArduino() {
  while (Serial.available() > 0) {
    rdata = Serial.read();
    myString += rdata;
    if (rdata == '\n') {
      Current = getValue(myString, ',', 0).toInt();
      Voltage = getValue(myString, ',', 1).toInt();
      sensor3 = getValue(myString, ',', 2).toInt();
      myString = "";
      Serial.println(String(Current));
    }
  }
}

// Helper to parse CSV
String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}


//*********

