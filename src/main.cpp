#include <Arduino.h>
#include <esp_task_wdt.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <WiFiManagerTz.h>
#include <Wire.h>
#include "config.h"
#include "time_func.h"

#ifdef CONFIG_ENABLE_LVGL
#include "ui/lv_setup.h"
#include "ui/ui.h"
#else
#include "Flags/led_module.h"
#endif

#ifdef CONFIG_ENABLE_SENSOR_SEN54
#include "Sensor.h"
#endif

#ifdef CONFIG_ENABLE_SENSOR_PMSA003A
#include "Flags/Sensor_Pms.h"
#endif

#ifdef CONFIG_ENABLE_SENSOR_SHT
#include "Sensor_sht.h"
#endif

#ifdef CONFIG_ENABLE_OTA_ANEDYA
#include "Flags/ota_module.h"
#endif

#ifdef CONFIG_ENABLE_ESP_NOW
#include "Flags/espnow_module.h"
#endif

#ifdef CONFIG_ESP_MATTER_ENABLE
#include "ui/matter_wrapper.h"
#endif

#include "Flags/mqtt_module.h"

#define WDT_TIMEOUT_SECONDS 500
#define FIRMWARE_VERSION "version - 3.1"

WiFiManager wm;
extern WiFiManager wm;

// -------------------- Handle NTP sync --------------------
void on_time_available(struct timeval *t) {
  struct tm timeInfo;
  getLocalTime(&timeInfo, 1000);
  Serial.println(&timeInfo, "%A, %B %d %Y %H:%M:%S zone %Z %z ");
}

// -------------------- Setup --------------------
void setup() {

  Serial.begin(115200);
  Serial.println("===== AIROWL BOOT =====");

  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = WDT_TIMEOUT_SECONDS * 1000,
    .idle_core_mask = 0,
    .trigger_panic = true
  };
  esp_task_wdt_init(&wdt_config);
  esp_task_wdt_add(NULL);

  // ----- Dynamic AP Setup via WiFiManager -----
  WiFi.mode(WIFI_STA);
  WiFi.begin();

  String mac = WiFi.macAddress(); mac.replace(":", "");
  String apName = "AIROWL_" + mac.substring(6);
  
  WiFiManagerNS::NTP::onTimeAvailable(&on_time_available);
  WiFiManagerNS::init(&wm, nullptr);
  std::vector<const char *> menu = {"wifi", "info", "custom", "param", "sep", "restart", "exit"};
  wm.setMenu(menu);
  wm.setTitle("AIROWL Configuration");
  wm.setConfigPortalBlocking(false);
  wm.setConfigPortalTimeout(120); 
  wm.setConnectTimeout(60);    
  wm.setDebugOutput(true);
  
  bool connected = wm.autoConnect(apName.c_str(), "12345678");

  unsigned long wifi_start = millis();
  while (!WiFi.isConnected() && millis() - wifi_start < 120000) {
    wm.process();
    esp_task_wdt_reset();
  }

  if (!connected) {
    Serial.println("[WiFiManager] Portal timeout or user exited.");
  } else {
    Serial.printf("[WiFiManager] Connected to WiFi: %s\n", WiFi.SSID().c_str());
  }

  mqtt_setup();

  // ----- LVGL UI -----
  #ifdef CONFIG_ENABLE_LVGL
  Wire.begin(4, 5);
  delay(1000);
  Serial.println("\n[SCAN] Starting I2C Bus Scan...");

  int devices = 0;
  for (uint8_t address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      Serial.print("[FOUND] I2C device at 0x");
      Serial.println(address, HEX);
      devices++;
    }
    delay(5);
  }

  if (devices == 0) {
      Serial.println("[RESULT] No I2C devices found.");
    } else {
      Serial.printf("[RESULT] Total %d I2C device(s) found.\n", devices);
    }
    Serial.println("[LVGL] Starting LVGL initialization...");
    lv_begin();
    Serial.println("[LVGL] LVGL begin completed");
    ui_init();

    Serial.println("[LVGL] LVGL task started");
    lv_label_set_text(ui_devicename, apName.c_str());
    lv_label_set_text(ui_qrcodename, apName.c_str());
    lv_label_set_text(ui_firmwareversion, FIRMWARE_VERSION);

    String qrcodeurl = (WiFi.status() == WL_CONNECTED) ? "https://opendata.oizom.com/device/" + apName : "WIFI:T:WPA;S:" + apName + ";P:12345678;;";
    lv_obj_t* qrcode_obj = lv_qrcode_create(ui_qrcode, 150, lv_color_black(), lv_color_white());
    lv_obj_center(qrcode_obj);
    lv_qrcode_update(qrcode_obj, qrcodeurl.c_str(), qrcodeurl.length());
  #else
  // ----- Fallback LED status if no LVGL -----
    delay(500);
    led_status_init(LED_PIN); 
    restartLEDTask(); 
    Serial.println("[LED] LED initialized");
  #endif
  
  // ----- Sensor Task -----
  // #ifdef CONFIG_ENABLE_SENSOR_SEN54
  //   initSensor();
  //   Serial.println("[Sensor] Sensor task started");
  // #endif

  #ifdef CONFIG_ENABLE_SENSOR_PMSA003A
    initSensor();
    Serial.println("[Sensor-PMS] PM Sensor task started");
  #endif

  // ----- SHT Sensor Task -----
  #ifdef CONFIG_ENABLE_SENSOR_SHT
    initSHTTask();
    Serial.println("[SHT] SHT Sensor task started");
  #endif

  Serial.println("\n[SCAN] Starting I2C Bus Scan...");

  // ----- ESP-NOW Comm -----
  #ifdef CONFIG_ENABLE_ESP_NOW
    initESPNow();
    restartESPNowTask();
    Serial.println("[ESP-NOW] ESP-NOW initialized and task started");
  #endif

  // ----- Anedya OTA -----
  #ifdef CONFIG_ENABLE_OTA_ANEDYA
    initOTA();
    Serial.println("[OTA] OTA initialized");
  #endif

  // ----- Matter -----
  #ifdef CONFIG_ESP_MATTER_ENABLE
    initMatter();
    Serial.println("[Matter] Matter Initialized");
  #endif

  // ----- Clock Sync -----
  time_init();
}

// -------------------- Main Loop --------------------
void loop() {
  #ifdef CONFIG_ENABLE_LVGL
    lv_handler();
  #endif

  #ifndef CONFIG_ENABLE_LVGL
    //led logic if needed
  #endif

  #ifdef CONFIG_ENABLE_OTA_ANEDYA
    ota_loop();
  #endif

  // #ifdef CONFIG_ENABLE_SENSOR_SEN54
  //   // Sensor logic if required
  // #endif

   #ifdef CONFIG_ENABLE_SENSOR_PMSA003A
    //  Serial.println("[PM] PM sensor Initialized");
  #endif

  #ifdef CONFIG_ENABLE_SENSOR_SHT 
    // SHT Sensor logic if needed
  #endif

  #ifdef CONFIG_ENABLE_ESP_NOW
    // espnow_loop runs inside its task
    espnow_loop();
  #endif

  #ifdef CONFIG_ESP_MATTER_ENABLE
    matter_loop();
  #endif

  if (!mqttClient.connected()) {
    mqtt_reconnect("AIROWL"); 
  } else {
    mqttClient.loop(); 
  }

  update_time();
  esp_task_wdt_reset();
  delay(10); 
}
