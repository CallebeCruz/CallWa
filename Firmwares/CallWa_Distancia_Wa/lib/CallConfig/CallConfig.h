#pragma once

// ======================================================
// ===== ESCOLHA O PAPEL DO DISPOSITIVO AQUI =========
// ======================================================

//#define DEVICE_ROLE_CALL
#define DEVICE_ROLE_WAL


// ======================================================
// ===== WIFI (comum aos dois) =========================
// ======================================================

#define WIFI_SSID       "VIVOFIBRA-WIFI6-0D91"
#define WIFI_PASSWORD   "79pXpBacHBgawyD"


// ======================================================
// ===== MQTT SERVER (comum aos dois) ==================
// ======================================================

#define MQTT_SERVER     "70d4da565d9c476782999dc9303945b0.s1.eu.hivemq.cloud"
#define MQTT_PORT       8883
#define MQTT_PASS       "Serzinhos2025"
#define MQTT_TOPIC_PING "luminaria/ping"


// ======================================================
// ===== IDENTIDADE DINÂMICA ===========================
// ======================================================

#ifdef DEVICE_ROLE_CALL

    #define DEVICE_ID       "ESP32_CALL"
    #define CLIENT_ID       "ESP32_Call"
    #define PEER_ID         "ESP32_WAL"

    #define MQTT_USER       "ESP_CALL"

    #define MQTT_TOPIC_OUT  "luminaria/para_waleria"
    #define MQTT_TOPIC_IN   "luminaria/para_voce"

#endif


#ifdef DEVICE_ROLE_WAL

    #define DEVICE_ID       "ESP_WAL"
    #define CLIENT_ID       "ESP32_Wal"
    #define PEER_ID         "ESP32_Call"

    #define MQTT_USER       "ESP_WALL"

    #define MQTT_TOPIC_OUT  "luminaria/para_voce"
    #define MQTT_TOPIC_IN   "luminaria/para_waleria"

#endif


// ======================================================
// ===== MQTT SETTINGS =================================
// ======================================================

#define MQTT_KEEPALIVE        20
#define MQTT_BUFFER_SIZE      768
#define MQTT_SOCKET_TIMEOUT   1


// ======================================================
// ===== TIMERS ========================================
// ======================================================

#define PING_INTERVAL_MS      1000
#define REPEAT_INTERVAL_MS    30000
#define BLINK_FAST_MS         200
#define BLINK_SLOW_MS         1000
#define TOUCH_DEBOUNCE_MS     1000


// ======================================================
// ===== LED ===========================================
// ======================================================

#define LED_PIN         4
#define LED_TYPE        WS2812B
#define COLOR_ORDER     GRB
#define NUM_LEDS        144


// ======================================================
// ===== EFFECTS =======================================
// ======================================================

#define STEPS_PER_TURN  28
#define EFFECT_REPEAT_INTERVAL_MS (1UL * 30UL * 1000UL)