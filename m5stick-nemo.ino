// M5StickC Plus 2 - Speaker Hat 2 Audio Player
// Simple audio streaming player for M5StickC Plus 2 with Speaker Hat 2

#include <M5StickCPlus2.h>
#include "Audio.h"
#include <WiFi.h>

// Speaker Hat 2 I2S pins
#define I2S_DOUT_HAT2 25
#define I2S_BCLK_HAT2 26
#define I2S_LRC_HAT2  0

// Audio URL for online streaming
#define AUDIO_URL "https://media-ssl.musicradio.com/HeartLondon"

// WiFi credentials
#define WIFI_SSID "BATCAVE"
#define WIFI_PASSWORD "KimuraCheeseRodeo"

// Display settings
#define BGCOLOR BLACK
#define FGCOLOR GREEN
#define SMALL_TEXT 2

// Button pins
#define M5_BUTTON_HOME 37
#define M5_BUTTON_RST 39

// Audio object
Audio audio;
bool audioInitialized = false;
bool wifiConnected = false;

// Connect to WiFi
void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    return;
  }
  
  Serial.println("Connecting to WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println("");
    Serial.print("WiFi connected! IP: ");
    Serial.println(WiFi.localIP());
  } else {
    wifiConnected = false;
    Serial.println("");
    Serial.println("WiFi connection failed!");
  }
}

// Check button press
bool check_home_press() {
  return (digitalRead(M5_BUTTON_HOME) == LOW);
}

bool check_next_press() {
  return (digitalRead(M5_BUTTON_RST) == LOW);
}

void setup() {
  Serial.begin(115200);
  M5.begin();
  
  // Button setup
  pinMode(M5_BUTTON_HOME, INPUT);
  pinMode(M5_BUTTON_RST, INPUT);
  
  // Display setup
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BGCOLOR);
  M5.Lcd.setTextColor(FGCOLOR, BGCOLOR);
  M5.Lcd.setTextSize(SMALL_TEXT);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.println("Speaker Hat 2");
  M5.Lcd.println("Initializing...");
  
  // Connect to WiFi
  connectWiFi();
  
  // Initialize audio
  if (wifiConnected) {
    M5.Lcd.println("WiFi: OK");
    M5.Lcd.println("Setting up audio...");
    
    // Configure I2S pins
    audio.setPinout(I2S_BCLK_HAT2, I2S_LRC_HAT2, I2S_DOUT_HAT2);
    // Set I2S format - try LSB first (some DACs need this)
    audio.setI2SCommFMT_LSB(true);
    // Set volume (0-21, or use setVolumeSteps for more control)
    audio.setVolumeSteps(64);
    audio.setVolume(63);
    
    audioInitialized = true;
    M5.Lcd.println("Audio: OK");
    
    // Start playing
    audio.connecttohost(AUDIO_URL);
    M5.Lcd.fillScreen(BGCOLOR);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.println("Playing...");
  } else {
    M5.Lcd.println("WiFi: FAILED");
    M5.Lcd.println("Check credentials");
    delay(3000);
  }
}

void loop() {
  // Process audio
  if (audioInitialized) {
    audio.loop();
  }
  
  // Update display every 500ms
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 500) {
    lastUpdate = millis();
    
    M5.Lcd.fillScreen(BGCOLOR);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.setTextColor(FGCOLOR, BGCOLOR);
    
    // WiFi status
    M5.Lcd.print("WiFi: ");
    if (wifiConnected && WiFi.status() == WL_CONNECTED) {
      M5.Lcd.setTextColor(GREEN, BGCOLOR);
      M5.Lcd.println("CONNECTED");
    } else {
      M5.Lcd.setTextColor(RED, BGCOLOR);
      M5.Lcd.println("NOT CONNECTED");
    }
    M5.Lcd.setTextColor(FGCOLOR, BGCOLOR);
    M5.Lcd.println("");
    
    // Audio status
    if (audioInitialized) {
      if (audio.isRunning()) {
        M5.Lcd.setTextColor(GREEN, BGCOLOR);
        M5.Lcd.println("Playing...");
      } else {
        M5.Lcd.setTextColor(YELLOW, BGCOLOR);
        M5.Lcd.println("Stopped");
      }
      M5.Lcd.setTextColor(FGCOLOR, BGCOLOR);
    } else {
      M5.Lcd.setTextColor(RED, BGCOLOR);
      M5.Lcd.println("Audio: ERROR");
      M5.Lcd.setTextColor(FGCOLOR, BGCOLOR);
    }
    
    M5.Lcd.println("");
    M5.Lcd.println("RST: Restart");
    M5.Lcd.println("HOME: Stop/Play");
  }
  
  // Button handling
  if (check_next_press()) {
    // Restart playback
    if (audioInitialized && wifiConnected) {
      if (WiFi.status() != WL_CONNECTED) {
        connectWiFi();
      }
      if (wifiConnected && WiFi.status() == WL_CONNECTED) {
        audio.stopSong();
        delay(100);
        audio.connecttohost(AUDIO_URL);
      }
    }
    delay(250);
  }
  
  if (check_home_press()) {
    // Stop/Start playback
    if (audioInitialized) {
      if (audio.isRunning()) {
        audio.stopSong();
      } else {
        if (wifiConnected && WiFi.status() == WL_CONNECTED) {
          audio.connecttohost(AUDIO_URL);
        }
      }
    }
    delay(250);
  }
  
  delay(10); // Small delay to prevent button bounce
}
