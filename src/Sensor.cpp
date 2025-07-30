#ifdef CONFIG_ENABLE_SENSOR_SEN54

#include "Sensor.h"
#include <Wire.h>

extern TwoWire Wire;  //  Reuse globally initialized Wire

volatile int AQI = 0;         // Define the global variable
volatile float temperature = 0.0; // Define global temperature variable
volatile float humidity = 0.0;    // Define global humidity variable

Sensor_t sensor_data;

SensirionI2CSen5x sen5x;

TaskHandle_t sensorTaskHandle = nullptr;

String deviceName = "";

#ifdef CONFIG_ENABLE_LVGL

unsigned long bootTime = 0;
 #define EYE_COLOR_ACTIVATION_DELAY 45000  

lv_chart_series_t *ui_PM1chart_series_1 = {0};
static lv_coord_t ui_PM1chart_series_1_array[CHART_DATA_LENGTH] = {0};

lv_chart_series_t *ui_PM25chart_series_1 = {0};
static lv_coord_t ui_PM25chart_series_1_array[CHART_DATA_LENGTH] = {0};

lv_chart_series_t *ui_PM4chart_series_1 = {0};
static lv_coord_t ui_PM4chart_series_1_array[CHART_DATA_LENGTH] = {0};

lv_chart_series_t *ui_PM10chart_series_1 = {0};
static lv_coord_t ui_PM10chart_series_1_array[CHART_DATA_LENGTH] = {0};

lv_chart_series_t *ui_TVOCchart_series_1 = {0};
static lv_coord_t ui_TVOCchart_series_1_array[CHART_DATA_LENGTH] = {0};

#endif

// PM1.0 Breakpoints
AQIBreakpoint pm1Bps[] = {{0.0, 8.0, 0, 50},      {8.1, 25.4, 51, 100},
                          {25.5, 35.4, 101, 150}, {35.5, 50.4, 151, 200},
                          {50.5, 75.4, 201, 300}, {75.5, 500.4, 301, 500}};

// PM2.5 Breakpoints
AQIBreakpoint pm4Bps[] = {{0.0, 35.0, 0, 50},       {35.1, 75.4, 51, 100},
                          {75.5, 125.4, 101, 150},  {125.5, 175.4, 151, 200},
                          {175.5, 250.4, 201, 300}, {250.5, 500.4, 301, 500}};

// PM2.5 Breakpoints
AQIBreakpoint pm25Bps[] = {{0.0, 12.0, 0, 50},       {12.1, 35.4, 51, 100},
                           {35.5, 55.4, 101, 150},   {55.5, 150.4, 151, 200},
                           {150.5, 250.4, 201, 300}, {250.5, 500.4, 301, 500}};

// PM10 Breakpoints
AQIBreakpoint pm10Bps[] = {{0, 54, 0, 50},       {55, 154, 51, 100},
                           {155, 254, 101, 150}, {255, 354, 151, 200},
                           {355, 424, 201, 300}, {425, 604, 301, 500}};

// TVOC Breakpoints (example values)
AQIBreakpoint tvocBps[] = {{0.0, 300, 0, 50},      {300, 500, 51, 100},
                           {500, 1000, 101, 150},  {1000, 3000, 151, 200},
                           {4000, 5000, 201, 300}, {5000, 10000, 301, 500}};
           
// Function to calculate sub-index
int calculateSubIndex(float Cp, AQIBreakpoint bp) {
    float Ip =
        ((bp.Ip_Hi - bp.Ip_Lo) / (bp.Cp_Hi - bp.Cp_Lo)) * (Cp - bp.Cp_Lo) +
        bp.Ip_Lo;
    return (int)round(Ip);
}

// Function to get breakpoint for a given concentration
AQIBreakpoint getBreakpoint(float Cp, AQIBreakpoint bps[], int numBps) {
    for (int i = 0; i < numBps; i++) {
        if (Cp >= bps[i].Cp_Lo && Cp <= bps[i].Cp_Hi) {
            return bps[i];
        }
    }
    // Return the highest breakpoint if Cp exceeds the range
    return bps[numBps - 1];
}

// Function to get AQI category
String getAQICategory(int aqi) {
    if (aqi >= 0 && aqi <= 50)
        return "Good";
    else if (aqi <= 100)
        return "Satisfactory";
    else if (aqi <= 150)
        return "Moderate";
    else if (aqi <= 200)
        return "Unhealthy";
    else if (aqi <= 300)
        return "Very Unhealthy";
    else
        return "Hazardous";
}

uint32_t getAQIColor(int aqi) {
    if (aqi >= 0 && aqi <= 50)
        return 0x00E400; // Good
    else if (aqi <= 100)
        return 0x9CFF9C; // Satisfactory
    else if (aqi <= 150)
        return 0xFFFF00; // Moderate
    else if (aqi <= 200)
        return 0xFF7E00; // Unhealthy
    else if (aqi <= 300)
        return 0xFF0000; // Very Unhealthy
    else
        return 0x8F3F97; // Hazardous
}

#ifdef CONFIG_ENABLE_LVGL

void setupCharts()
{
    ui_PM1chart_series_1 = lv_chart_add_series(
        ui_Chart1, lv_color_hex(0x41b4d1), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_ext_y_array(ui_Chart1, ui_PM1chart_series_1,
                             ui_PM1chart_series_1_array);
    lv_chart_set_range(ui_Chart1, LV_CHART_AXIS_PRIMARY_Y, 0, 50);

    ui_PM25chart_series_1 = lv_chart_add_series(
        ui_Chart3, lv_color_hex(0x41b4d1), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_ext_y_array(ui_Chart3, ui_PM25chart_series_1,
                             ui_PM25chart_series_1_array);
    lv_chart_set_range(ui_Chart3, LV_CHART_AXIS_PRIMARY_Y, 0, 50);

    ui_PM10chart_series_1 = lv_chart_add_series(
        ui_Chart4, lv_color_hex(0x41b4d1), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_ext_y_array(ui_Chart4, ui_PM10chart_series_1,
                             ui_PM10chart_series_1_array);
    lv_chart_set_range(ui_Chart4, LV_CHART_AXIS_PRIMARY_Y, 0, 50);

    ui_PM4chart_series_1 = lv_chart_add_series(
        ui_Chart2, lv_color_hex(0x41b4d1), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_ext_y_array(ui_Chart2, ui_PM4chart_series_1,
                             ui_PM4chart_series_1_array);
    lv_chart_set_range(ui_Chart2, LV_CHART_AXIS_PRIMARY_Y, 0, 50);

    ui_TVOCchart_series_1 = lv_chart_add_series(
        ui_Chart5, lv_color_hex(0x41b4d1), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_ext_y_array(ui_Chart5, ui_TVOCchart_series_1,
                             ui_TVOCchart_series_1_array);
    lv_chart_set_range(ui_Chart5, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
}

#endif

bool isSen5xPresent()
{
    Wire.beginTransmission(0x69); // Replace 0x69 with your sensor's I2C address
    return (Wire.endTransmission() == 0);
}

void sensorData(void *params) {
    // Wire.begin(4, 5); initialized in the main
    sen5x.begin(Wire);

    uint16_t error;
    char errorMessage[256];
    error = sen5x.deviceReset();
    if (error) {
        // Serial.print("Error trying to execute deviceReset(): ");
        errorToString(error, errorMessage, 256);
        // Serial.println(errorMessage);
    }

    float tempOffset = 0.0;
    error = sen5x.setTemperatureOffsetSimple(tempOffset);
    if (error) {
        // Serial.print("Error trying to execute setTemperatureOffsetSimple(): ");
        errorToString(error, errorMessage, 256);
        // Serial.println(errorMessage);
    } else {
        // Serial.print("Temperature Offset set to ");
        // Serial.println(" deg. Celsius (SEN54/SEN55 only");
    }

    // Start Measurement
    error = sen5x.startMeasurement();
    if (error) {
        // Serial.print("Error trying to execute startMeasurement(): ");
        errorToString(error, errorMessage, 256);
        // Serial.println(errorMessage);
    }

    #ifdef CONFIG_ENABLE_LVGL
    setupCharts();
    #endif

    mqtt_setup();
    String mac = WiFi.macAddress();
    mac.replace(":", "");
    deviceName = "AIROWL_" + mac.substring(6);

    while (1) {
        uint16_t error;
        char errorMessage[256];

        // Read Measurement
        float t_pm1;
        float t_pm25;
        float t_pm4;
        float t_pm10;
        float t_hum;
        float t_temp;
        float vocIndex;
        float noxIndex;

        error = sen5x.readMeasuredValues(t_pm1, t_pm25, t_pm4, t_pm10, t_hum,
                                         t_temp, vocIndex, noxIndex);

        if (error || isnan(t_pm1) || isnan(t_pm25) || isnan(t_pm4) ||
            isnan(t_pm10) || isnan(vocIndex)) {
            // M5.Log.print("Error trying to execute readMeasuredValues(): ");
            errorToString(error, errorMessage, 256);
            // M5.Log.println(errorMessage);
        } else {
            sensor_data.pm1 += t_pm1;
            sensor_data.pm25 += t_pm25;
            sensor_data.pm10 += t_pm10;
            sensor_data.pm4 += t_pm4;
            sensor_data.count++;

            #ifdef CONFIG_ENABLE_LVGL

            char pm1buffer[6] = {0};
            dtostrf(t_pm1, 6, 1, pm1buffer);
            lv_label_set_text(ui_pm1value, pm1buffer);

            char pm25buffer[6] = {0};
            dtostrf(t_pm25, 6, 1, pm25buffer);
            lv_label_set_text(ui_pm25value, pm25buffer);

            char pm4buffer[6] = {0};
            dtostrf(t_pm4, 6, 1, pm4buffer);
            lv_label_set_text(ui_pm4value, pm4buffer);

            char pm10buffer[6] = {0};
            dtostrf(t_pm10, 6, 1, pm10buffer);
            lv_label_set_text(ui_pm10value, pm10buffer);

            char tvocbuffer[6] = {0};
            if (isnan(vocIndex)) {
                // M5.Log.println("n/a");
            } else {
                sensor_data.tvoc += vocIndex;
                dtostrf(vocIndex, 6, 1, tvocbuffer);
                lv_label_set_text(ui_tvocvalue, tvocbuffer);
            }

            char humbuffer[4] = {0};
            if (isnan(t_hum)) {
                // Humidity n/a handling (unchanged)
            } else {
                dtostrf(t_hum, 4, 1, humbuffer);
                lv_label_set_text(ui_humd, humbuffer);
                humidity = t_hum; // Update global humidity variable
            }

            char tempbuffer[4] = {0};
            if (isnan(t_temp)) {
                // Temperature n/a handling (unchanged)
            } else {
                dtostrf(t_temp, 4, 1, tempbuffer);
                lv_label_set_text(ui_temp, tempbuffer);
                temperature = t_temp; // Update global temperature variable
            }

            #endif

            if (sensor_data.count == DATA_FREQ) {
                float avgPM1 = (sensor_data.pm1 / sensor_data.count);
                float avgPM25 = (sensor_data.pm25 / sensor_data.count);
                float avgPM10 = (sensor_data.pm10 / sensor_data.count);
                float avgPM4 = (sensor_data.pm4 / sensor_data.count);
                float avgTVOC = (sensor_data.tvoc / sensor_data.count);

                AQIBreakpoint pm25Bp = getBreakpoint(
                    avgPM25, pm25Bps, sizeof(pm25Bps) / sizeof(pm25Bps[0]));
                AQIBreakpoint pm10Bp = getBreakpoint(
                    avgPM10, pm10Bps, sizeof(pm10Bps) / sizeof(pm10Bps[0]));
                AQIBreakpoint tvocBp = getBreakpoint(
                    avgTVOC, tvocBps, sizeof(tvocBps) / sizeof(tvocBps[0]));
                AQIBreakpoint pm1Bp = getBreakpoint(
                    avgPM1, pm1Bps, sizeof(tvocBps) / sizeof(tvocBps[0]));
                AQIBreakpoint pm4Bp = getBreakpoint(
                    avgPM4, pm4Bps, sizeof(tvocBps) / sizeof(tvocBps[0]));

                // Calculate sub-indices
                int pm25Index = calculateSubIndex(avgPM25, pm25Bp);
                int pm10Index = calculateSubIndex(avgPM10, pm10Bp);
                int tvocIndex = calculateSubIndex(avgTVOC, tvocBp);
                int pm1Index = calculateSubIndex(avgPM1, pm1Bp);
                int pm4Index = calculateSubIndex(avgPM4, pm4Bp);

                #ifdef CONFIG_ENABLE_LVGL
                
                uint32_t pm25_color;
                uint32_t pm10_color;
                uint32_t tvoc_color;
                uint32_t pm1_color;
                uint32_t pm4_color;

                if (millis() - bootTime < EYE_COLOR_ACTIVATION_DELAY){
                    pm25_color = 0xFFFFFF;
                    pm10_color = 0xFFFFFF;
                    tvoc_color = 0xFFFFFF;
                    pm1_color = 0xFFFFFF;
                    pm4_color = 0xFFFFFF;
                }
                else{
                    pm25_color = getAQIColor(pm25Index);
                    pm10_color = getAQIColor(pm10Index);
                    tvoc_color = getAQIColor(tvocIndex);
                    pm1_color = getAQIColor(pm1Index);
                    pm4_color = getAQIColor(pm4Index);
                }

                lv_obj_set_style_text_color(ui_pm25value,
                                            lv_color_hex(pm25_color),
                                            LV_PART_MAIN | LV_STATE_DEFAULT);

                lv_obj_set_style_text_color(ui_pm10value,
                                            lv_color_hex(pm10_color),
                                            LV_PART_MAIN | LV_STATE_DEFAULT);

                lv_obj_set_style_text_color(ui_tvocvalue,
                                            lv_color_hex(tvoc_color),
                                            LV_PART_MAIN | LV_STATE_DEFAULT);

                lv_obj_set_style_text_color(ui_pm1value,
                                            lv_color_hex(pm1_color),
                                            LV_PART_MAIN | LV_STATE_DEFAULT);

                lv_obj_set_style_text_color(ui_pm4value,
                                            lv_color_hex(pm4_color),
                                            LV_PART_MAIN | LV_STATE_DEFAULT);
                #endif
                
                // Combine sub-indices (choose one method)
                // Method 1: Maximum Sub-Index
                int aqi = max(max(pm25Index, pm10Index), tvocIndex);
                AQI = aqi;
                // Method 2: Weighted Average
                // float aqi = (pm25Index * 0.5) + (pm10Index * 0.3) +
                // (tvocIndex * 0.2); aqi = round(aqi);

                #ifdef CONFIG_ENABLE_LVGL
                // Get AQI category
                String airQualityCategory = getAQICategory(aqi);
                uint32_t eye_color;

                // Check if 60 seconds have passed since power-on
                if (millis() - bootTime < EYE_COLOR_ACTIVATION_DELAY) {
                    eye_color = 0xFFFFFF;  // White during warmup
                } else {
                    eye_color = getAQIColor(aqi);
                }

                lv_obj_set_style_bg_color(ui_lefteye, lv_color_hex(eye_color), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_bg_color(ui_righteye, lv_color_hex(eye_color), LV_PART_MAIN | LV_STATE_DEFAULT);

                // Set PM1 Chart Screen
                char pm1avg[6] = {0};
                char pm1max[6] = {0};
                dtostrf(avgPM1, 6, 1, pm1avg);
                // lv_label_set_text(ui_pm1avg, pm1avg);
                // shift the other values to the left
                memcpy(ui_PM1chart_series_1_array,
                       ui_PM1chart_series_1_array + 1,
                       (CHART_DATA_LENGTH - 1) * sizeof(lv_coord_t));
                if (sensor_data.pm1_max < avgPM1) {
                    sensor_data.pm1_max = int(avgPM1);
                    if (sensor_data.pm1_max > 50)
                        lv_chart_set_range(ui_Chart1, LV_CHART_AXIS_PRIMARY_Y,
                                           0, sensor_data.pm1_max + 20);
                }
                dtostrf(sensor_data.pm1_max, 6, 1, pm1max);
                lv_label_set_text(ui_pm1maxvalue, pm1max);
                // Insert new value
                ui_PM1chart_series_1_array[CHART_DATA_LENGTH - 1] =
                    (uint16_t)(avgPM1);
                lv_chart_set_ext_y_array(ui_Chart1, ui_PM1chart_series_1,
                                         ui_PM1chart_series_1_array);

                // Set PM2.5 Chart Screens
                char pm25avg[6] = {0};
                char pm25max[6] = {0};
                dtostrf(avgPM25, 6, 1, pm25avg);
                // lv_label_set_text(ui_pm25avg, pm25avg);
                // shift the other values to the left
                memcpy(ui_PM25chart_series_1_array,
                       ui_PM25chart_series_1_array + 1,
                       (CHART_DATA_LENGTH - 1) * sizeof(lv_coord_t));
                if (sensor_data.pm25_max < (avgPM25)) {
                    sensor_data.pm25_max = int(avgPM25);
                    if (sensor_data.pm25_max > 50)
                        lv_chart_set_range(ui_Chart3,
                                           LV_CHART_AXIS_PRIMARY_Y, 0,
                                           sensor_data.pm25_max + 20);
                }
                dtostrf(sensor_data.pm25_max, 6, 1, pm25max);
                lv_label_set_text(ui_pm25maxvalue, pm25max);
                // Insert new value
                ui_PM25chart_series_1_array[CHART_DATA_LENGTH - 1] =
                    (uint16_t)(avgPM25);
                lv_chart_set_ext_y_array(ui_Chart3, ui_PM25chart_series_1,
                                         ui_PM25chart_series_1_array);

                // Set PM10 Chart Screens
                char pm10avg[6] = {0};
                char pm10max[6] = {0};
                dtostrf(avgPM10, 6, 1, pm10avg);
                // lv_label_set_text(ui_pm10avg, pm10avg);
                // shift the other values to the left
                memcpy(ui_PM10chart_series_1_array,
                       ui_PM10chart_series_1_array + 1,
                       (CHART_DATA_LENGTH - 1) * sizeof(lv_coord_t));
                if (sensor_data.pm10_max < (avgPM10)) {
                    sensor_data.pm10_max = int(avgPM10);
                    if (sensor_data.pm10_max > 50)
                        lv_chart_set_range(ui_Chart4,
                                           LV_CHART_AXIS_PRIMARY_Y, 0,
                                           sensor_data.pm10_max + 20);
                }
                dtostrf(sensor_data.pm10_max, 6, 1, pm10max);
                lv_label_set_text(ui_pm10maxvalue, pm10max);
                // Insert new value
                ui_PM10chart_series_1_array[CHART_DATA_LENGTH - 1] =
                    (uint16_t)(avgPM10);
                lv_chart_set_ext_y_array(ui_Chart4, ui_PM10chart_series_1,
                                         ui_PM10chart_series_1_array);

                // Set PM4 Chart Screens
                char pm4avg[6] = {0};
                char pm4max[6] = {0};
                dtostrf(avgPM4, 6, 1, pm4avg);
                // lv_label_set_text(ui_pm4avg, pm4avg);
                // shift the other values to the left
                memcpy(ui_PM4chart_series_1_array,
                       ui_PM4chart_series_1_array + 1,
                       (CHART_DATA_LENGTH - 1) * sizeof(lv_coord_t));
                if (sensor_data.pm4_max < (avgPM4)) {
                    sensor_data.pm4_max = int(avgPM4);
                    if (sensor_data.pm4_max > 50)
                        lv_chart_set_range(ui_Chart2, LV_CHART_AXIS_PRIMARY_Y,
                                           0, sensor_data.pm4_max + 20);
                }
                dtostrf(sensor_data.pm4_max, 6, 1, pm4max);
                lv_label_set_text(ui_pm4maxvalue1, pm4max);
                // Insert new value
                ui_PM4chart_series_1_array[CHART_DATA_LENGTH - 1] =
                    (uint16_t)(avgPM4);
                lv_chart_set_ext_y_array(ui_Chart2, ui_PM4chart_series_1,
                                         ui_PM4chart_series_1_array);

                // Set TVOC Chart Screens
                char tvocavg[6] = {0};
                char tvocmax[6] = {0};
                dtostrf(avgTVOC, 6, 1, tvocavg);
                // lv_label_set_text(ui_tvocavg, tvocavg);
                // shift the other values to the left
                memcpy(ui_TVOCchart_series_1_array,
                       ui_TVOCchart_series_1_array + 1,
                       (CHART_DATA_LENGTH - 1) * sizeof(lv_coord_t));
                if (sensor_data.tvoc_max < (avgTVOC)) {
                    sensor_data.tvoc_max = int(avgTVOC);
                    if (sensor_data.tvoc_max > 100)
                        lv_chart_set_range(ui_Chart5,
                                           LV_CHART_AXIS_PRIMARY_Y, 0,
                                           sensor_data.tvoc_max + 20);
                }
                dtostrf(sensor_data.tvoc_max, 6, 1, tvocmax);
                lv_label_set_text(ui_tvocmaxvalue, tvocmax);
                // Insert new value
                ui_TVOCchart_series_1_array[CHART_DATA_LENGTH - 1] =
                    (uint16_t)(avgTVOC);
                lv_chart_set_ext_y_array(ui_Chart5, ui_TVOCchart_series_1,
                                         ui_TVOCchart_series_1_array);

                #endif
            
                if (WiFi.status() == WL_CONNECTED) {

                    #ifdef CONFIG_ENABLE_LVGL
                    lv_img_set_src(ui_nose, &ui_img_airowl_2_png);
                    #endif

                    if (!mqttClient.connected()) {
                        String mac = WiFi.macAddress();
                        mac.replace(":", "");
                        String ClientId = "AIROWL_" + mac.substring(6);
                        mqtt_reconnect(ClientId.c_str());
                    }
                    // Construct the JSON string
                    String jsonString = "{";
                    jsonString += "\"deviceId\":\"";
                    jsonString += deviceName;
                    jsonString += "\",";
                    jsonString += "\"p3\":";
                    jsonString += String(avgPM1, 2);
                    jsonString += ",";
                    jsonString += "\"p1\":";
                    jsonString += String(avgPM25, 2);
                    jsonString += ",";
                    jsonString += "\"p2\":";
                    jsonString += String(avgPM10, 2);
                    jsonString += ",";
                    jsonString += "\"p5\":";
                    jsonString += String(avgPM4, 2);
                    jsonString += ",";
                    jsonString += "\"v2\":";
                    jsonString += String(avgTVOC, 2);
                    jsonString += "}";
                    mqttClient.publish("airowl", jsonString.c_str());
                    mqttClient.loop();
                } else {

                    #ifdef CONFIG_ENABLE_LVGL
                    lv_img_set_src(ui_nose, &ui_img_airowl_1_png);
                    #endif
                }
                sensor_data.pm1 = 0;
                sensor_data.pm10 = 0;
                sensor_data.pm25 = 0;
                sensor_data.pm4 = 0;
                sensor_data.tvoc = 0;
                sensor_data.count = 0;
            }
        }
        esp_task_wdt_reset(); // Reset watchdog for this task
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1 second
    }
}

void initSensor()
{
    xTaskCreatePinnedToCore(sensorData, "SensorTask", 8192, nullptr, 2, &sensorTaskHandle, 1);
    if (sensorTaskHandle) {
        esp_task_wdt_add(sensorTaskHandle);
    } else {
        Serial.println("[Sensor] Failed to start Sensor Task!");
    }
}

void restartSensorTask() {
    // if (sensorTaskHandle) {
    //     esp_task_wdt_delete(sensorTaskHandle);
    //     vTaskDelete(sensorTaskHandle);
    //     sensorTaskHandle = nullptr;
    // }

    BaseType_t result = xTaskCreatePinnedToCore(sensorData, "SensorTask", 8192, nullptr, 2, &sensorTaskHandle, 1);
    if (result == pdPASS && sensorTaskHandle) {
        esp_task_wdt_add(sensorTaskHandle);
        Serial.println("[Sensor] Sensor task restarted");
    } else {
        Serial.println("[Sensor] Failed to restart Sensor task!");
        sensorTaskHandle = nullptr;
    }
}

#endif