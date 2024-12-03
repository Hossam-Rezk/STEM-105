#include <ESP8266WiFi.h>      // Include ESP8266 Wi-Fi library
#include <DHT.h>              // Include DHT library for DHT11 sensor
#include <ArduinoJson.h>      // Include ArduinoJson library for creating JSON data

// Define pin connections  
#define SOUND_SENSOR_PIN A1    // Sound detector connected to analog pin A1
#define DHTPIN 2               // DHT11 temperature sensor connected to digital pin 2
#define AIR_QUALITY_PIN A0     // Air quality sensor (MQ-135) connected to analog pin A0
#define BUZZER_PIN 5           // Piezo buzzer connected to digital pin 5
#define LED_PIN 3              // LED connected to digital pin 3

// Initialize DHT sensor  
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);  

// Wi-Fi credentials  
const char* ssid = "your_SSID";          // Replace with your Wi-Fi SSID
const char* password = "your_PASSWORD";  // Replace with your Wi-Fi password

// Server details  
const char* host = "192.168.1.100";      // IP address of your local server
const int port = 5000;                   // Port of your local server

// Threshold values  
const int SOUND_THRESHOLD_LOW = 20;      // Sound level lower threshold in decibels  
const int SOUND_THRESHOLD_HIGH = 85;     // Sound level upper threshold in decibels  
const float TEMP_THRESHOLD = 35.0;       // Temperature threshold in Celsius  
const int AIR_QUALITY_THRESHOLD = 600;   // Air quality threshold in ppm  

unsigned long previousMillis = 0;  // Stores the last time reading was taken
const long interval = 5000;        // 5-second interval  

WiFiClient client;

void setup() {  
  Serial.begin(9600);
  dht.begin();  
  pinMode(BUZZER_PIN, OUTPUT);  
  pinMode(LED_PIN, OUTPUT);  
  digitalWrite(BUZZER_PIN, LOW); // Ensure buzzer is off at startup  
  digitalWrite(LED_PIN, LOW);    // Ensure LED is off at startup  

  // Connect to Wi-Fi  
  Serial.println();
  Serial.print("Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());  // Print the IP address
}

void loop() {  
  unsigned long currentMillis = millis();  

  // Only take a reading every 5 seconds  
  if (currentMillis - previousMillis >= interval) {  
    previousMillis = currentMillis;  

    // Read data from temperature sensor  
    float temperature = dht.readTemperature();  

    // Check if temperature reading failed  
    if (isnan(temperature)) {  
      Serial.println("Failed to read from DHT sensor!");  
      return;  // Exit loop if there's an error  
    }  

    // Read data from sound sensor (analog read)  
    int soundLevelRaw = analogRead(SOUND_SENSOR_PIN);  

    // Map the sound level reading to a decibel range of 20 to 85  
    int soundLevel = map(soundLevelRaw, 20, 85, SOUND_THRESHOLD_LOW, SOUND_THRESHOLD_HIGH);  

    // Read data from air quality sensor  
    int airQuality = analogRead(AIR_QUALITY_PIN);  

    // Convert analog reading to ppm for MQ-135 (approximation)  
    int airQualityPPM = map(airQuality, 0, 1023, 400, 1000);  

    // Create a JSON object to store the readings  
    StaticJsonDocument<200> doc;  
    doc["temperature"] = temperature;  
    doc["soundLevel"] = soundLevel;  
    doc["airQuality"] = airQualityPPM;  

    // Send the JSON object to the Serial Monitor (this will be sent to the connected website via USB)  
    serializeJson(doc, Serial);  
    Serial.println();  

    // Send data to local Wi-Fi server  
    if (client.connect(host, port)) {
      String jsonStr;
      serializeJson(doc, jsonStr);  // Convert JSON to string

      client.print("POST /data HTTP/1.1\r\n");  // Specify the endpoint on your server
      client.print("Host: " + String(host) + "\r\n");  // Include host
      client.print("Content-Type: application/json\r\n");  // Content type for JSON
      client.print("Content-Length: " + String(jsonStr.length()) + "\r\n\r\n");  // Content length
      client.print(jsonStr);  // Send the JSON string

      Serial.println("Data sent to server");
    } else {
      Serial.println("Connection to server failed");
    }

    // Check if sound level exceeds threshold to light up the LED  
    if (soundLevel >= SOUND_THRESHOLD_HIGH) {  
      digitalWrite(LED_PIN, HIGH); // Activate LED  
      Serial.println("LED: ON");   // Print LED status to Serial Monitor 
    } else {  
      digitalWrite(LED_PIN, LOW);  // Deactivate LED  
      Serial.println("LED: OFF");  // Print LED status to Serial Monitor 
    }  

    // Check if temperature or air quality thresholds are met to play music  
    if (temperature >= TEMP_THRESHOLD || airQualityPPM >= AIR_QUALITY_THRESHOLD) {  
      playMusic(); // Play music on the piezo buzzer  
      Serial.println("Buzzer: ON - Playing music"); // Print buzzer status to Serial Monitor 
    } else {  
      noTone(BUZZER_PIN); // Ensure buzzer is off if no threshold is exceeded  
      Serial.println("Buzzer: OFF - Not playing music"); // Print buzzer status to Serial Monitor 
    }  
  }  
}  

// Function to play music on the piezo buzzer  
void playMusic() {  
  int melody[] = {  
    262, 294, 330, 349, 392, 440, 494, 523 // C4 to C5 notes  
  };  
  int noteDurations[] = {  
    500, 500, 500, 500, 500, 500, 500, 500  
  };  

  for (int thisNote = 0; thisNote < 8; thisNote++) {  
    int noteDuration = noteDurations[thisNote];  
    tone(BUZZER_PIN, melody[thisNote], noteDuration); // Play the note  
    int pauseBetweenNotes = noteDuration * 1.30;  
    delay(pauseBetweenNotes);  
    noTone(BUZZER_PIN); // Stop the tone  
  }  
}  
