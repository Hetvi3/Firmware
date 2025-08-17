#ifndef __CONFIG_H
#define __CONFIG_H

// I2C Pin Config 
#define SYS_I2C_PORT 0

// TFT Display Configuration
#define TFT_WIDTH  240
#define TFT_HEIGHT 320

#define TFT_CS     39
#define TFT_DC     21
#define TFT_RST    47
#define TFT_SCLK   7
#define TFT_MOSI   6
#define TFT_BL     38

// Touch Screen Configuration
#define TOUCH_SDA  4
#define TOUCH_SCL  5
#define TOUCH_RST  42
#define TOUCH_INT  41

// esp-now configuration
#define MAX_SLAVES 9
#define ESPNOW_RETRY_COUNT 3
#define ESPNOW_TIMEOUT_MS 5000
#define SLAVE_DATA_TIMEOUT 30000
#define MQTT_PUBLISH_INTERVAL 10000

// Optional features
#define MONKEY_TEST_ENABLE 0
#define MIC_BUF_SIZE 256  // Keep only if you're adding audio in future

#endif  // __CONFIG_H
