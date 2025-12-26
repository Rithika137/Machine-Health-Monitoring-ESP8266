#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char* ssid = "R";            // your WiFi SSID
const char* password = "rithhh4726"; // your WiFi password

ESP8266WebServer server(80);

const int sensorPin = D2; // Photoelectric sensor
const int ledPin = D6;
const int buzzerPin = D7;

volatile unsigned long pulseCount = 0;
unsigned long lastTime = 0;
float rpm = 0;
float frequency = 0;

// Dummy variables for new parameters (simulate or add sensors as needed)
float machineHealth = 100.0; // Percentage, e.g., based on RPM
String machineStatus = "Normal";
unsigned long downtimeHistory = 0; // Cumulative downtime in seconds (dummy)
float energyConsumption = 0.0; // Dummy in kWh
float stabilityVibration = 0.0; // Dummy in mm/s²

void IRAM_ATTR countPulse() {
  pulseCount++;
}

const char webpage[] PROGMEM = R"htmlpage(
<!DOCTYPE html>
<html>
<head>
<title>Machine Health Monitor</title>
<script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
<style>
body {font-family: Arial, sans-serif; text-align: center; background: #000; color: #fff; margin: 0; padding: 20px;}
h1 {color: #fff; margin-bottom: 30px;}
.container {display: flex; flex-wrap: wrap; justify-content: center; gap: 20px; max-width: 1000px; margin: 0 auto;}
.card {width: 200px; height: 100px; background: #111; padding: 10px; box-shadow: 0 0 15px rgba(255,255,255,0.1); border-radius: 15px; border: 1px solid #333; display: flex; align-items: center; justify-content: center;}
.data {font-size: 16px; margin: 0; text-align: center;}
.status-normal {color: #0f0;}
.status-slow {color: #ff0;}
.status-fast {color: #f00;}
.graph-container {width: 100%; max-width: 800px; margin: 20px auto;}
</style>
</head>
<body>
<h1>Machine Health Monitor</h1>
<div class="container">
  <div class="card">
    <div class="data">RPM: <span id="rpm">0</span></div>
  </div>
  <div class="card">
    <div class="data">Frequency: <span id="freq">0</span> Hz</div>
  </div>
  <div class="card">
    <div class="data">Machine Health: <span id="health">100</span>%</div>
  </div>
  <div class="card">
    <div class="data">Machine Status: <span id="status" class="status-normal">Normal</span></div>
  </div>
  <div class="card">
    <div class="data">Downtime History: <span id="downtime">0</span> s</div>
  </div>
  <div class="card">
    <div class="data">Energy Consumption: <span id="energy">0.0</span> kWh</div>
  </div>
  <div class="card">
    <div class="data">Stability Vibration: <span id="vibration">0.0</span> mm/s²</div>
  </div>
</div>
<div class="graph-container">
  <canvas id="healthChart"></canvas>
</div>

<script>
let rpmData = [];
let freqData = [];
let healthData = [];
let labels = [];
let lastRpm = 0;
let lastFreq = 0;
let lastHealth = 100;

const ctx = document.getElementById('healthChart').getContext('2d');
const chart = new Chart(ctx, {
  type: 'line',
  data: {
    labels: labels,
    datasets: [
      {
        label: 'RPM',
        data: rpmData,
        borderColor: '#00ff00',
        backgroundColor: 'rgba(0,255,0,0.1)',
        fill: false,
      },
      {
        label: 'Frequency (Hz)',
        data: freqData,
        borderColor: '#0000ff',
        backgroundColor: 'rgba(0,0,255,0.1)',
        fill: false,
      },
      {
        label: 'Machine Health (%)',
        data: healthData,
        borderColor: '#ff0000',
        backgroundColor: 'rgba(255,0,0,0.1)',
        fill: false,
      }
    ]
  },
  options: {
    responsive: true,
    scales: {
      x: { ticks: { color: '#fff' }, grid: { color: '#333' } },
      y: { ticks: { color: '#fff' }, grid: { color: '#333' } }
    },
    plugins: {
      legend: { labels: { color: '#fff' } }
    }
  }
});

const updateDashboard = () => {
  fetch('/data').then(res => res.json()).then(data => {
    document.getElementById('rpm').innerText = data.rpm;
    document.getElementById('freq').innerText = data.frequency;
    document.getElementById('health').innerText = data.machineHealth;
    document.getElementById('downtime').innerText = data.downtimeHistory;
    document.getElementById('energy').innerText = data.energyConsumption;
    document.getElementById('vibration').innerText = data.stabilityVibration;
    
    let statusEl = document.getElementById('status');
    if(data.machineStatus === "Slow"){ statusEl.className="status-slow"; }
    else if(data.machineStatus === "Fast"){ statusEl.className="status-fast"; }
    else { statusEl.className="status-normal"; }
    statusEl.innerText = data.machineStatus;
    
    // Update graph data only if values have changed to avoid flat lines
    if (data.rpm !== lastRpm || data.frequency !== lastFreq || data.machineHealth !== lastHealth) {
      rpmData.push(data.rpm);
      freqData.push(data.frequency);
      healthData.push(data.machineHealth);
      labels.push(new Date().toLocaleTimeString());
      
      lastRpm = data.rpm;
      lastFreq = data.frequency;
      lastHealth = data.machineHealth;
      
      if (rpmData.length > 50) { // Keep last 50 points
        rpmData.shift();
        freqData.shift();
        healthData.shift();
        labels.shift();
      }
      
      chart.update();
    }
  });
};

setInterval(updateDashboard, 500); // Fast update every 500ms
</script>
</body>
</html>
)htmlpage";

void setup() {
  Serial.begin(115200);
  pinMode(sensorPin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(sensorPin), countPulse, RISING);

  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/", []() {
    server.send_P(200, "text/html", webpage);
  });

  server.on("/data", []() {
    unsigned long currentTime = millis();
    if (currentTime - lastTime >= 1000) {
      detachInterrupt(digitalPinToInterrupt(sensorPin));
      float pulsesPerSecond = (float)pulseCount / ((currentTime - lastTime) / 1000.0);
      frequency = pulsesPerSecond;
      rpm = frequency * 60.0;
      pulseCount = 0;
      lastTime = currentTime;
      attachInterrupt(digitalPinToInterrupt(sensorPin), countPulse, RISING);
      
      // Simulate machine health based on RPM (e.g., optimal around 90 RPM)
      machineHealth = 100.0 - abs(rpm - 90.0) * 2.0;
      if (machineHealth < 0) machineHealth = 0;
      
      // Simulate downtime (increment if status is not normal)
      if (machineStatus != "Normal") downtimeHistory += 1;
      
      // Simulate energy consumption (dummy increment)
      energyConsumption += 0.01;
      
      // Simulate stability vibration (dummy based on RPM variation)
      stabilityVibration = random(0, 10) + abs(rpm - 90.0) * 0.1;
    }

    if (rpm < 50) {
      machineStatus = "Slow";
      digitalWrite(ledPin, LOW);
      digitalWrite(buzzerPin, HIGH);
    } else if (rpm > 130) {
      machineStatus = "Fast";
      digitalWrite(ledPin, HIGH);
      digitalWrite(buzzerPin, HIGH);
    } else {
      machineStatus = "Normal";
      digitalWrite(ledPin, HIGH);
      digitalWrite(buzzerPin, LOW);
    }

    String json = "{\"rpm\":" + String(rpm, 1) +
                  ",\"frequency\":" + String(frequency, 2) +
                  ",\"machineHealth\":" + String(machineHealth, 1) +
                  ",\"machineStatus\":\"" + machineStatus + "\"" +
                  ",\"downtimeHistory\":" + String(downtimeHistory) +
                  ",\"energyConsumption\":" + String(energyConsumption, 2) +
                  ",\"stabilityVibration\":" + String(stabilityVibration, 1) + "}";
    server.send(200, "application/json", json);
  });

  server.begin();
  Serial.println("Server started!");
}

void loop() {
  server.handleClient();
}
