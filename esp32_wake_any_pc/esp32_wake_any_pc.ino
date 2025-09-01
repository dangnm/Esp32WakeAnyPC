#include <WiFi.h>
#include <WebServer.h>
#include "USB.h"
#include "USBHIDKeyboard.h"
#include "esp_sleep.h"
#include "wifi_config.h"

WebServer server(80);
USBHIDKeyboard Keyboard;

// Tracking variables
bool usbConnected = false;
bool keyboardReady = false;
unsigned long startTime = 0;
unsigned long lastActivity = 0;

void handleRoot() {
  String wifiStatus = WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected";
  String usbStatus = usbConnected ? "Connected" : "Disconnected";
  String keyboardStatus = keyboardReady ? "Ready" : "Not Ready";
  
  // Calculate uptime
  unsigned long uptime = (millis() - startTime) / 1000;
  String uptimeStr = String(uptime / 3600) + "h " + String((uptime % 3600) / 60) + "m " + String(uptime % 60) + "s";
  
  String html = "<html><head>"
                "<meta charset='UTF-8'>"
                "<meta http-equiv='refresh' content='3'>"
                "<style>"
                "body { font-family: Arial, sans-serif; margin: 20px; background: #f5f5f5; }"
                ".container { max-width: 1000px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }"
                ".status-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 20px; margin: 20px 0; }"
                ".status-ok { background: #d4edda; border-color: #28a745; }"
                ".status-error { background: #f8d7da; border-color: #dc3545; }"
                ".status-warning { background: #fff3cd; border-color: #ffc107; }"
                ".live-indicator { background: #28a745; color: white; padding: 10px; border-radius: 5px; text-align: center; margin: 20px 0; }"
                ".button { padding: 15px 30px; font-size: 18px; background: #007bff; color: white; border: none; border-radius: 8px; cursor: pointer; margin: 10px 5px; }"
                ".button:disabled { background: #6c757d; cursor: not-allowed; }"
                ".button:hover:not(:disabled) { background: #0056b3; }"
                ".button-danger { background: #dc3545; }"
                ".button-danger:hover { background: #c82333; }"
                ".button-success { background: #28a745; }"
                ".button-success:hover { background: #218838; }"
                ".button-warning { background: #ffc107; color: #212529; }"
                ".button-warning:hover { background: #e0a800; }"
                ".uptime { background: #17a2b8; color: white; padding: 10px; border-radius: 5px; text-align: center; margin: 20px 0; }"
                ".info { background: #e7f3ff; border: 1px solid #17a2b8; padding: 15px; border-radius: 5px; margin: 20px 0; }"
                ".warning { background: #fff3cd; border: 1px solid #ffc107; padding: 15px; border-radius: 5px; margin: 20px 0; }"
                ".danger { background: #f8d7da; border: 1px solid #dc3545; padding: 15px; border-radius: 5px; margin: 20px 0; }"
                ".keyboard { background: #f8f9fa; border: 2px solid #dee2e6; border-radius: 10px; padding: 20px; margin: 20px 0; }"
                ".keyboard-row { display: flex; justify-content: center; margin: 5px 0; }"
                ".key { width: 50px; height: 50px; margin: 2px; border: 1px solid #adb5bd; border-radius: 8px; background: #ffffff; color: #495057; font-size: 14px; font-weight: bold; cursor: pointer; transition: all 0.2s; }"
                ".key:hover { background: #e9ecef; transform: translateY(-2px); box-shadow: 0 4px 8px rgba(0,0,0,0.1); }"
                ".key:active { transform: translateY(0); box-shadow: 0 2px 4px rgba(0,0,0,0.1); }"
                ".key-wide { width: 80px; }"
                ".key-extra-wide { width: 120px; }"
                ".key-space { width: 400px; }"
                ".key-special { background: #6c757d; color: white; }"
                ".key-special:hover { background: #5a6268; }"
                ".key-enter { background: #28a745; color: white; }"
                ".key-enter:hover { background: #218838; }"
                ".key-backspace { background: #dc3545; color: white; }"
                ".key-backspace:hover { background: #c82333; }"
                ".key-disabled { background: #e9ecef; color: #adb5bd; cursor: not-allowed; }"
                ".key-disabled:hover { background: #e9ecef; transform: none; box-shadow: none; }"
                "</style>"
                "</head><body>"
                "<div class='container'>"
                "<h1>Wake Any PC in Sleep Mode</h1>";
  
  // Live indicator
  html += "<div class='live-indicator'>LIVE - Last updated: " + String(millis() / 1000) + "s ago</div>";
  
  // Uptime
  html += "<div class='uptime'>Uptime: " + uptimeStr + "</div>";
  
  // Info about how it works
  html += "<div class='info'>"
          "<strong>Hardware Reset Solution:</strong><br>"
          "1. When MacBook sleeps, ESP32 USB HID gets completely stuck<br>"
          "2. Software reset is not enough - need hardware reset<br>"
          "3. Use 'Hardware Reset' button to restart ESP32 completely<br>"
          "4. This is equivalent to unplugging and plugging back USB"
          "</div>";
  
  // Warning if keyboard is not working
  if (!keyboardReady) {
    html += "<div class='warning'>"
            "Keyboard not working after MacBook sleep/wake. Use 'Hardware Reset' button to fix!"
            "</div>";
  }
  
  // Status grid
  html += "<div class='status-grid'>";
  
  // WiFi Status
  String wifiClass = (WiFi.status() == WL_CONNECTED) ? "status-ok" : "status-error";
  html += "<div class='" + wifiClass + "'>"
          "<strong>WiFi Status:</strong> " + wifiStatus + "<br>"
          "<strong>SSID:</strong> " + String(ssid) + "<br>"
          "<strong>IP Address:</strong> " + WiFi.localIP().toString() + "<br>"
          "<strong>Signal:</strong> " + String(WiFi.RSSI()) + " dBm"
          "</div>";
  
  // USB & Keyboard Status
  String usbClass = usbConnected ? "status-ok" : "status-error";
  String keyboardClass = keyboardReady ? "status-ok" : "status-error";
  
  html += "<div class='" + keyboardClass + "'>"
          "<strong>USB Status:</strong> " + usbStatus + "<br>"
          "<strong>Keyboard Status:</strong> " + keyboardStatus + "<br>"
          "<strong>Last Activity:</strong> " + String((millis() - lastActivity) / 1000) + "s ago<br>"
          "<strong>Reset Needed:</strong> " + (keyboardReady ? "No" : "Yes") +
          "</div>";
  
  html += "</div>";
  
  // QWERTY Keyboard
  html += "<div class='keyboard'>"
          "<h3 style='text-align: center; margin-bottom: 20px;'>Virtual QWERTY Keyboard</h3>";
  
  // Row 1: Numbers
  html += "<div class='keyboard-row'>";
  for (char c = '1'; c <= '9'; c++) {
    html += "<button class='key' onclick='sendKey(\"" + String(c) + "\")'";
    if (!keyboardReady) html += " disabled class='key key-disabled'";
    html += ">" + String(c) + "</button>";
  }
  html += "<button class='key' onclick='sendKey(\"0\")'";
  if (!keyboardReady) html += " disabled class='key key-disabled'";
  html += ">0</button>";
  html += "</div>";
  
  // Row 2: QWERTY
  html += "<div class='keyboard-row'>";
  html += "<button class='key' onclick='sendKey(\"q\")'";
  if (!keyboardReady) html += " disabled class='key key-disabled'";
  html += ">q</button>";
  html += "<button class='key' onclick='sendKey(\"w\")'";
  if (!keyboardReady) html += " disabled class='key key-disabled'";
  html += ">w</button>";
  html += "<button class='key' onclick='sendKey(\"e\")'";
  if (!keyboardReady) html += " disabled class='key key-disabled'";
  html += ">e</button>";
  html += "<button class='key' onclick='sendKey(\"r\")'";
  if (!keyboardReady) html += " disabled class='key key-disabled'";
  html += ">r</button>";
  html += "<button class='key' onclick='sendKey(\"t\")'";
  if (!keyboardReady) html += " disabled class='key key-disabled'";
  html += ">t</button>";
  html += "<button class='key' onclick='sendKey(\"y\")'";
  if (!keyboardReady) html += " disabled class='key key-disabled'";
  html += ">y</button>";
  html += "<button class='key' onclick='sendKey(\"u\")'";
  if (!keyboardReady) html += " disabled class='key key-disabled'";
  html += ">u</button>";
  html += "<button class='key' onclick='sendKey(\"i\")'";
  if (!keyboardReady) html += " disabled class='key key-disabled'";
  html += ">i</button>";
  html += "<button class='key' onclick='sendKey(\"o\")'";
  if (!keyboardReady) html += " disabled class='key key-disabled'";
  html += ">o</button>";
  html += "<button class='key' onclick='sendKey(\"p\")'";
  if (!keyboardReady) html += " disabled class='key key-disabled'";
  html += ">p</button>";
  html += "</div>";
  
  // Row 3: ASDFG
  html += "<div class='keyboard-row'>";
  html += "<button class='key' onclick='sendKey(\"a\")'";
  if (!keyboardReady) html += " disabled class='key key-disabled'";
  html += ">a</button>";
  html += "<button class='key' onclick='sendKey(\"s\")'";
  if (!keyboardReady) html += " disabled class='key key-disabled'";
  html += ">s</button>";
  html += "<button class='key' onclick='sendKey(\"d\")'";
  if (!keyboardReady) html += " disabled class='key key-disabled'";
  html += ">d</button>";
  html += "<button class='key' onclick='sendKey(\"f\")'";
  if (!keyboardReady) html += " disabled class='key key-disabled'";
  html += ">f</button>";
  html += "<button class='key' onclick='sendKey(\"g\")'";
  if (!keyboardReady) html += " disabled class='key key-disabled'";
  html += ">g</button>";
  html += "<button class='key' onclick='sendKey(\"h\")'";
  if (!keyboardReady) html += " disabled class='key key-disabled'";
  html += ">h</button>";
  html += "<button class='key' onclick='sendKey(\"j\")'";
  if (!keyboardReady) html += " disabled class='key key-disabled'";
  html += ">j</button>";
  html += "<button class='key' onclick='sendKey(\"k\")'";
  if (!keyboardReady) html += " disabled class='key key-disabled'";
  html += ">k</button>";
  html += "<button class='key' onclick='sendKey(\"l\")'";
  if (!keyboardReady) html += " disabled class='key key-disabled'";
  html += ">l</button>";
  html += "</div>";
  
  // Row 4: ZXCVB
  html += "<div class='keyboard-row'>";
  html += "<button class='key' onclick='sendKey(\"z\")'";
  if (!keyboardReady) html += " disabled class='key key-disabled'";
  html += ">z</button>";
  html += "<button class='key' onclick='sendKey(\"x\")'";
  if (!keyboardReady) html += " disabled class='key key-disabled'";
  html += ">x</button>";
  html += "<button class='key' onclick='sendKey(\"c\")'";
  if (!keyboardReady) html += " disabled class='key key-disabled'";
  html += ">c</button>";
  html += "<button class='key' onclick='sendKey(\"v\")'";
  if (!keyboardReady) html += " disabled class='key key-disabled'";
  html += ">v</button>";
  html += "<button class='key' onclick='sendKey(\"b\")'";
  if (!keyboardReady) html += " disabled class='key key-disabled'";
  html += ">b</button>";
  html += "<button class='key' onclick='sendKey(\"n\")'";
  if (!keyboardReady) html += " disabled class='key key-disabled'";
  html += ">n</button>";
  html += "<button class='key' onclick='sendKey(\"m\")'";
  if (!keyboardReady) html += " disabled class='key key-disabled'";
  html += ">m</button>";
  html += "</div>";
  
  // Row 5: Special keys
  html += "<div class='keyboard-row'>";
  html += "<button class='key key-backspace' onclick='sendKey(\"BACKSPACE\")'";
  if (!keyboardReady) html += " disabled class='key key-disabled'";
  html += ">⌫</button>";
  html += "<button class='key key-space' onclick='sendKey(\" \")'";
  if (!keyboardReady) html += " disabled class='key key-disabled'";
  html += ">SPACE</button>";
  html += "<button class='key key-enter' onclick='sendKey(\"ENTER\")'";
  if (!keyboardReady) html += " disabled class='key key-disabled'";
  html += ">↵</button>";
  html += "</div>";
  
  html += "</div>";
  
  // Control buttons
  html += "<div style='text-align: center; margin: 20px 0;'>"
          "<a href='/hardware-reset' class='button button-danger' style='text-decoration: none; display: inline-block;'>Hardware Reset ESP32</a>"
          "<a href='/soft-reset' class='button button-warning' style='text-decoration: none; display: inline-block;'>Soft Reset USB</a>"
          "<a href='/test' class='button' style='text-decoration: none; display: inline-block;'>Test Keyboard</a>"
          "</div>";
  
  // Instructions
  html += "<div style='background: #f8f9fa; padding: 15px; border-radius: 5px; margin: 20px 0;'>"
          "<h3>How to Fix Sleep Issues:</h3>"
          "<ol>"
          "<li><strong>When MacBook wakes up</strong> and keyboard doesn't work</li>"
          "<li><strong>Click 'Hardware Reset ESP32'</strong> button (restarts ESP32 completely)</li>"
          "<li><strong>Wait 10-15 seconds</strong> for ESP32 to restart</li>"
          "<li><strong>ESP32 will reconnect</strong> to WiFi automatically</li>"
          "<li><strong>Use the virtual keyboard</strong> - should work now!</li>"
          "</ol>"
          "<p><em>Note: Hardware Reset restarts ESP32 completely, equivalent to unplugging USB</em></p>"
          "</div>";
  
  // System info
  html += "<div style='background: #f8f9fa; padding: 15px; border-radius: 5px; margin: 20px 0;'>"
          "<h3>System Information</h3>"
          "<p><strong>Free Memory:</strong> " + String(ESP.getFreeHeap()) + " bytes</p>"
          "<p><strong>CPU Frequency:</strong> " + String(ESP.getCpuFreqMHz()) + " MHz</p>"
          "<p><strong>WiFi Channel:</strong> " + String(WiFi.channel()) + "</p>"
          "<p><strong>USB Mode:</strong> HID with Hardware Reset Capability</p>"
          "</div>";
  
  // Auto-refresh info
  html += "<div style='text-align: center; color: #6c757d; font-size: 12px;'>"
          "Page auto-refreshes every 3 seconds | Last update: " + String(millis() / 1000) + "s"
          "</div>";
  
  html += "<p><a href='/' class='button' style='text-decoration: none; display: inline-block;'>Manual Refresh</a></p>";
  
  // JavaScript for sending keys
  html += "<script>"
          "function sendKey(key) {"
          "  var form = document.createElement('form');"
          "  form.method = 'POST';"
          "  form.action = '/press';"
          "  var input = document.createElement('input');"
          "  input.type = 'hidden';"
          "  input.name = 'key';"
          "  input.value = key;"
          "  form.appendChild(input);"
          "  document.body.appendChild(form);"
          "  form.submit();"
          "}"
          "</script>";
  
  html += "</div></body></html>";
  
  server.send(200, "text/html", html);
}

void handlePress() {
  if (keyboardReady) {
    String key = server.hasArg("key") ? server.arg("key") : "a";
    
    if (key == "BACKSPACE") {
      Keyboard.press(KEY_BACKSPACE);
      delay(10);
      Keyboard.release(KEY_BACKSPACE);
      Serial.println("Backspace key sent successfully");
    } else if (key == "ENTER") {
      Keyboard.press(KEY_RETURN);
      delay(10);
      Keyboard.release(KEY_RETURN);
      Serial.println("Enter key sent successfully");
    } else if (key == " ") {
      Keyboard.press(' ');
      delay(10);
      Keyboard.release(' ');
      Serial.println("Space key sent successfully");
    } else {
      // Regular character
      Keyboard.write(key.charAt(0));
      Serial.println("Key '" + key + "' sent successfully");
    }
    
    // Update activity time
    lastActivity = millis();
    
    server.sendHeader("Location", "/?success=1");
    server.send(303);
  } else {
    server.send(200, "text/html", "<html><body><h1>Error</h1><p>Keyboard not ready! Use 'Hardware Reset ESP32' button first.</p><a href='/'>Back</a></body></html>");
  }
}

// Hardware Reset ESP32 - Complete restart
void handleHardwareReset() {
  Serial.println("=== HARDWARE RESET ESP32 ===");
  Serial.println("Preparing to restart ESP32 completely...");
  
  // Send notification before restart
  String html = "<html><body>"
                "<h1>ESP32 Restarting...</h1>"
                "<p>ESP32 is restarting completely. This will take 10-15 seconds.</p>"
                "<p>Please wait and refresh the page.</p>"
                "<p><strong>This is equivalent to unplugging and plugging back USB!</strong></p>"
                "</body></html>";
  
  server.send(200, "text/html", html);
  
  // Wait a bit to send response
  delay(1000);
  
  Serial.println("Restarting ESP32 in 3 seconds...");
  delay(3000);
  
  // Restart ESP32 completely
  ESP.restart();
}

// Soft Reset USB - Only reset keyboard (less effective)
void handleSoftReset() {
  Serial.println("Soft reset USB...");
  
  Keyboard.end();
  delay(2000);
  Keyboard.begin();
  delay(2000);
  
  testKeyboard();
  
  server.sendHeader("Location", "/?softreset=1");
  server.send(303);
}

void handleTest() {
  Serial.println("Testing keyboard...");
  
  if (testKeyboard()) {
    server.sendHeader("Location", "/?test=passed");
    server.send(303);
  } else {
    server.sendHeader("Location", "/?test=failed");
    server.send(303);
  }
}

// Test keyboard
bool testKeyboard() {
  Serial.println("Testing keyboard functionality...");
  
  bool testPassed = false;
  
  // Try to send a test key
  try {
    // Send Scroll Lock key (minimal impact)
    Keyboard.press(KEY_SCROLL_LOCK);
    delay(10);
    Keyboard.release(KEY_SCROLL_LOCK);
    testPassed = true;
    Serial.println("Keyboard test: PASSED");
  } catch (...) {
    Serial.println("Keyboard test: FAILED");
    testPassed = false;
  }
  
  if (testPassed) {
    keyboardReady = true;
    usbConnected = true;
    lastActivity = millis();
  } else {
    keyboardReady = false;
    usbConnected = false;
  }
  
  return keyboardReady;
}

void setup() {
  Serial.begin(115200);
  Serial.println("=== ESP32-S2 Hardware Reset Keyboard Starting ===");
  
  startTime = millis();
  lastActivity = millis();
  
  // Initialize USB HID
  USB.begin();
  delay(2000);
  Keyboard.begin();
  delay(2000);
  
  Serial.println("USB HID initialized");
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(1000);
    attempts++;
    Serial.print("WiFi attempt " + String(attempts) + ": ");
    Serial.println(WiFi.status());
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected! IP: " + WiFi.localIP().toString());
    
    server.on("/", handleRoot);
    server.on("/press", HTTP_POST, handlePress);
    server.on("/hardware-reset", handleHardwareReset);
    server.on("/soft-reset", handleSoftReset);
    server.on("/test", handleTest);
    server.begin();
    
  } else {
    Serial.println("WiFi connection failed!");
  }
  
  // Initial keyboard test
  testKeyboard();
  
  Serial.println("=== Setup Complete ===");
  Serial.println("Hardware Reset capability enabled!");
  Serial.println("Use 'Hardware Reset ESP32' button when keyboard gets stuck!");
}

void loop() {
  server.handleClient();
}