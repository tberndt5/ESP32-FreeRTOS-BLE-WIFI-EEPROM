/************************************************************************************
 * ESP32 RTOS Skeleton with WiFi & BLE Provisioning
 *
 * Description:
 * This project provides a robust template for an ESP32 application using FreeRTOS.
 * It allows a user to set the WiFi network (SSID) and password via a Bluetooth
 * Low Energy (BLE) connection. The credentials are then saved to the ESP32's
 * internal flash memory using the EEPROM library for persistent storage. The device
 * will automatically attempt to connect to the configured WiFi network on startup.
 *
 * Core Features:
 * - FreeRTOS tasks for handling WiFi and BLE concurrently.
 * - BLE server for provisioning WiFi credentials.
 * - EEPROM for persistent storage of credentials.
 * - Automatic WiFi connection management.
 *
 * Author: Tyler Berndt
 * Version: 1
 ************************************************************************************/

// Core Libraries
#include <WiFi.h>
#include <EEPROM.h>

// BLE Libraries
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// --- Pinning Tasks to a Core ---
// Best practice for ESP32 with dual-core architecture is to pin tasks.
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// --- Configuration Constants ---
// EEPROM
#define EEPROM_SIZE 128 // Allocate 128 bytes for EEPROM. 64 for SSID, 64 for password.
#define SSID_ADDR 0     // EEPROM address for SSID
#define PASS_ADDR 64    // EEPROM address for Password

// BLE
#define BLE_SERVER_NAME "ESP32 Provisioning"
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define SSID_CHAR_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8" // Characteristic for Network SSID
#define PASS_CHAR_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a9" // Characteristic for Network Password

// WiFi
#define WIFI_TIMEOUT_MS 10000 // 10-second timeout for WiFi connection

// --- Global Variables ---
// These hold the current WiFi credentials. They are loaded from EEPROM on boot.
String wifi_ssid;
String wifi_password;
bool device_connected = false;

// Forward declarations for tasks
void keepWiFiAlive(void *parameters);
void blinkLED(void *parameters);

// --- Helper Functions ---

/**
 * @brief Reads a string from EEPROM until a null terminator is found.
 * @param address The starting address in EEPROM to read from.
 * @return The string read from EEPROM.
 */
String readStringFromEEPROM(int address) {
    char data[EEPROM_SIZE];
    int len = 0;
    unsigned char k;
    k = EEPROM.read(address);
    while (k != 0 && len < EEPROM_SIZE) {
        k = EEPROM.read(address + len);
        data[len] = k;
        len++;
    }
    data[len] = '\0';
    return String(data);
}

/**
 * @brief Writes a string to a specific EEPROM address.
 * @param address The starting address in EEPROM to write to.
 * @param str The string to write.
 */
void writeStringToEEPROM(int address, const String &str) {
    for (int i = 0; i < str.length(); i++) {
        EEPROM.write(address + i, str[i]);
    }
    EEPROM.write(address + str.length(), '\0'); // Null-terminate the string
    EEPROM.commit();
}

/**
 * @brief Loads WiFi credentials from EEPROM into global variables.
 */
void loadCredentials() {
    EEPROM.begin(EEPROM_SIZE);
    wifi_ssid = readStringFromEEPROM(SSID_ADDR);
    wifi_password = readStringFromEEPROM(PASS_ADDR);
    Serial.println("[INFO] Loaded credentials from EEPROM.");
    Serial.print("  SSID: ");
    Serial.println(wifi_ssid.length() > 0 ? wifi_ssid : "Not Set");
    Serial.print("  Password: ");
    Serial.println(wifi_password.length() > 0 ? "********" : "Not Set");
}

// --- BLE Callback Classes ---

/**
 * @brief Handles BLE server connection and disconnection events.
 */
class ServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer *pServer) {
        device_connected = true;
        Serial.println("[BLE] Client Connected");
    }

    void onDisconnect(BLEServer *pServer) {
        device_connected = false;
        Serial.println("[BLE] Client Disconnected");
        // Restart advertising to allow new connections
        pServer->getAdvertising()->start();
    }
};

/**
 * @brief Handles write events for the SSID BLE characteristic.
 */
class SSIDCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        if (value.length() > 0) {
            wifi_ssid = ""; // Clear previous value
            for (int i = 0; i < value.length(); i++) {
                wifi_ssid += value[i];
            }
            Serial.print("[BLE] New SSID received: ");
            Serial.println(wifi_ssid);
            writeStringToEEPROM(SSID_ADDR, wifi_ssid);
        }
    }
};

/**
 * @brief Handles write events for the Password BLE characteristic.
 */
class PasswordCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        if (value.length() > 0) {
            wifi_password = ""; // Clear previous value
            for (int i = 0; i < value.length(); i++) {
                wifi_password += value[i];
            }
            Serial.println("[BLE] New Password received.");
            writeStringToEEPROM(PASS_ADDR, wifi_password);
            Serial.println("[INFO] Credentials updated. Restarting device to apply changes.");
            delay(1000);
            ESP.restart(); // Restart to connect with new credentials
        }
    }
};

void setup() {
    Serial.begin(115200);
    Serial.println("\n[INFO] Device starting up...");
    pinMode(LED_BUILTIN, OUTPUT);

    // Load stored WiFi credentials from flash memory
    loadCredentials();

    // --- Initialize BLE Server ---
    BLEDevice::init(BLE_SERVER_NAME);
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());
    BLEService *pService = pServer->createService(SERVICE_UUID);

    // Create SSID Characteristic
    BLECharacteristic *pSsidCharacteristic = pService->createCharacteristic(
        SSID_CHAR_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
    pSsidCharacteristic->setCallbacks(new SSIDCallbacks());
    pSsidCharacteristic->setValue(wifi_ssid.c_str()); // Set initial value

    // Create Password Characteristic
    BLECharacteristic *pPassCharacteristic = pService->createCharacteristic(
        PASS_CHAR_UUID,
        BLECharacteristic::PROPERTY_WRITE); // Write-only for security
    pPassCharacteristic->setCallbacks(new PasswordCallbacks());

    pService->start();

    // --- Start BLE Advertising ---
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    BLEDevice::startAdvertising();
    Serial.println("[BLE] Advertising started. Ready for provisioning.");

    // --- Create and Start RTOS Tasks ---
    xTaskCreatePinnedToCore(
        keepWiFiAlive,     // Function to be called
        "WiFi Manager",    // Name of task
        4096,              // Stack size (bytes)
        NULL,              // Parameter to pass
        1,                 // Task priority
        NULL,              // Task handle
        app_cpu            // Core to run on
    );

    xTaskCreatePinnedToCore(
        blinkLED,          // Function to be called
        "LED Blinker",     // Name of task
        1024,              // Stack size
        NULL,              // Parameter
        1,                 // Priority
        NULL,              // Handle
        app_cpu            // Core
    );
}

// --- FreeRTOS Tasks ---

/**
 * @brief Task to manage the WiFi connection.
 * It periodically checks the connection status and attempts to reconnect if necessary.
 */
void keepWiFiAlive(void *parameters) {
    for (;;) {
        // Only try to connect if SSID is configured
        if (wifi_ssid.length() > 0) {
            if (WiFi.status() != WL_CONNECTED) {
                Serial.print("[WIFI] Attempting to connect to SSID: ");
                Serial.println(wifi_ssid);
                WiFi.mode(WIFI_STA);
                WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());

                unsigned long startAttemptTime = millis();
                while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < WIFI_TIMEOUT_MS) {
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                }

                if (WiFi.status() != WL_CONNECTED) {
                    Serial.println("[WIFI] Connection Failed!");
                    vTaskDelay(20000 / portTICK_PERIOD_MS); // Wait before retrying
                } else {
                    Serial.print("[WIFI] Connected! IP Address: ");
                    Serial.println(WiFi.localIP());
                }
            }
        } else {
            Serial.println("[WIFI] SSID not configured. Waiting for BLE provisioning.");
        }
        // Check status every 30 seconds
        vTaskDelay(30000 / portTICK_PERIOD_MS);
    }
}

/**
 * @brief Task to blink the onboard LED. The blink pattern indicates the device status.
 * - Slow blink: Waiting for configuration or disconnected.
 * - Fast blink: BLE client connected.
 * - Solid ON: WiFi connected.
 */
void blinkLED(void *parameters){
    for(;;){
        if (WiFi.status() == WL_CONNECTED) {
            digitalWrite(LED_BUILTIN, HIGH);
            vTaskDelay(1000 / portTICK_PERIOD_MS); // Check every second
        } else if (device_connected) { // BLE connected
            digitalWrite(LED_BUILTIN, HIGH);
            vTaskDelay(150 / portTICK_PERIOD_MS);
            digitalWrite(LED_BUILTIN, LOW);
            vTaskDelay(150 / portTICK_PERIOD_MS);
        } else { // Disconnected
            digitalWrite(LED_BUILTIN, HIGH);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            digitalWrite(LED_BUILTIN, LOW);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
}

void loop() {
    // The main loop is not used in this FreeRTOS-based application.
    // All functionality is handled by tasks.
    vTaskDelete(NULL); // Deletes the loop() task to save resources
}
