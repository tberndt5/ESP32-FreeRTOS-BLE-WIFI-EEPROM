/***********************************************
 * ESP32 RTOS Skeleton with WiFi,BLE, and ESP32
 * Description: Set WiFi Network/Password using
 * BLE and store in flash with EEPROM.
 * 
 * Author: Tyler Berndt
 */
 
// Libraries
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <EEPROM.h>
#include <WiFi.h>

#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// Globals
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define NETWORK_UUID        "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define PASSWORD_UUID       "beb5483e-36e1-4688-b7f5-ea07361b26a9"
#define EEPROM_SIZE         300
#define SETTINGS_LENGTH     50
#define BLESERVERNAME       "YOUR APP"
bool deviceConnected = false;
char WIFI_NETWORK[50] = "Enter your network";
char WIFI_PASSWORD[50] = "Enter your password";
int WIFI_TIMEOUT_MS = 10000;

// Bluetooth Service callbacks
/********************************************
 * class name: MyServerCallbacks()
 * inherit: BLEServerCallbacks
 * functions: onConnect(), onDisconnect()
 * description: Defines what the BLE server
 * does when connected or disconnected to.
 ********************************************/
class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer){
    deviceConnected = true;
  }
  void onDisconnect(BLEServer* pServer){
    deviceConnected = false;
    pServer->getAdvertising()->start();
  }
};
// Bluetooth Network Characteristic callbacks
/********************************************
 * class name: MyNetworkCallbacks()
 * inherit: BLECharacteristicCallbacks
 * functions: onWrite()
 * description: Defines what happens when the
 * network characteristic gets a message.
 ********************************************/
class MyNetworkCallbacks: public BLECharacteristicCallbacks {
  /********************************************
  * name: onWrite()
  * parameters: *networkCharacteristic
  * description: When a BLE client sends a
  * message to the network UUID, the value is
  * stored in EEPROM.
  ********************************************/
  void onWrite(BLECharacteristic *networkCharacteristic){
    char network[SETTINGS_LENGTH] = {0};
    std::string rxValue = networkCharacteristic->getValue();
    for(int i=0;i<rxValue.length();i++){
      network[i] = rxValue[i];
      Serial.println(rxValue[i]);
    }
    for(int i=0;i<SETTINGS_LENGTH;i++){
      WIFI_NETWORK[i] = network[i];
      EEPROM.write(i,network[i]);
      EEPROM.commit();
    }
    Serial.print("[BLE] Changed WiFi Netowrk to: ");
    Serial.print(WIFI_NETWORK);
    Serial.println("");
  }
};
// Bluetooth Password Characteristic callbacks
/********************************************
 * class name: MyPasswordCallbacks()
 * inherit: BLECharacteristicCallbacks
 * functions: onWrite()
 * description: Defines what happens when the
 * password characteristic gets a message.
 ********************************************/
class MyPasswordCallbacks: public BLECharacteristicCallbacks {
  /********************************************
  * name: onWrite()
  * parameters: *passwordCharacteristic
  * description: When a BLE client sends a
  * message to the password UUID, the value is
  * stored in EEPROM.
  ********************************************/
  void onWrite(BLECharacteristic *passwordCharacteristic){
    char password[SETTINGS_LENGTH] = {0};
    std::string rxValue = passwordCharacteristic->getValue();
    for(int i=0;i < rxValue.length();i++){
      password[i] = rxValue[i];
    }
    for(int i=0;i<SETTINGS_LENGTH;i++){
      WIFI_PASSWORD[i] = password[i];
      EEPROM.write(SETTINGS_LENGTH+i,password[i]);
      EEPROM.commit();
    }
    Serial.print("[BLE] Changed WiFi Password to: ");
    Serial.print(WIFI_PASSWORD);
    Serial.println("");
  }
};
// RTOS Tasks
/********************************************
 * name: bleStatus()
 * parameters: none
 * description: Outputs the status on the
 * BLE server.
 ********************************************/
void bleStatus(void *parameter){
  while(1){
    if(deviceConnected == true){
      Serial.println("[BLE] Connected");
    }
    else{
      Serial.println("[BLE] Disconnected");
    }
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}
/********************************************
 * name: keepWiFiAlive()
 * parameters: none
 * description: Checks to see if there is
 * a WiFi connection. If there is not then
 * it tries to make one.
 ********************************************/
void keepWiFiAlive(void *parameters){
  for(;;){
    if(WiFi.status() == WL_CONNECTED){
      Serial.println("[WIFI] Wifi still connected");
      vTaskDelay(10000 / portTICK_PERIOD_MS);
      continue;
    }
    Serial.println("[WIFI] Wifi Connecting");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_NETWORK,WIFI_PASSWORD);
    unsigned long startAttemptTime = millis();
    // Keep looping while we're not connected and haven't reached the timeout
    while(WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < WIFI_TIMEOUT_MS){}
    // When we could not make a Wifi connection
    if(WiFi.status() != WL_CONNECTED){
      Serial.println("[WIFI] Failed");
      vTaskDelay(20000 / portTICK_PERIOD_MS);
      continue;
    }
    Serial.println("[WIFI] Connected: " + WiFi.localIP());
  }
}
void myTask(void *parameters){
  for(;;){
    Serial.println("SUBSCRIBE FOR MORE TUTORIALS!");
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}
// Functions
/********************************************
 * name: getWiFiSettings()
 * parameters: none
 * description: Gets WiFi network settigns
 * from EEPROM and stores them in a global
 * variable.
 ********************************************/
void getWiFiSettings(){
  // Iterate through EEPROM to get WiFi settings
  for(int i=0;i<SETTINGS_LENGTH*2;i++){
    int myByte = EEPROM.read(i);
    if(i < SETTINGS_LENGTH){
      if(myByte == 0){
        // To terminal character array ~string
        WIFI_NETWORK[i] = EEPROM.read(i);
      }
      else{
        WIFI_NETWORK[i] = (char)EEPROM.read(i);
      }
    }
    else{
      if(myByte == 0){
        // To terminal character array ~string
        WIFI_PASSWORD[i-(SETTINGS_LENGTH)] = EEPROM.read(i);
      }
      else{
        WIFI_PASSWORD[i-(SETTINGS_LENGTH)] = (char)EEPROM.read(i);
      }
    }
  }
  Serial.print("[WIFI] Network: ");
  Serial.print(WIFI_NETWORK);
  Serial.println("");
  Serial.print("[WIFI] Password: ");
  Serial.print(WIFI_PASSWORD);
  Serial.println("");
}

void setup() {
  // Put your setup code here, to run once:
  Serial.begin(115200);

  // Initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);

  // Get WiFi settings from EEPROM
  getWiFiSettings();
  
  // Create the BLE Device
  BLEDevice::init(BLESERVERNAME);

  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *networkCharacteristic = pService->createCharacteristic(
                                         NETWORK_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  BLECharacteristic *passwordCharacteristic = pService->createCharacteristic(
                                         PASSWORD_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  networkCharacteristic->setCallbacks(new MyNetworkCallbacks());
  passwordCharacteristic->setCallbacks(new MyPasswordCallbacks());
  networkCharacteristic->setValue(WIFI_NETWORK);
  passwordCharacteristic->setValue(WIFI_PASSWORD);
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  // Task to for BLE
  xTaskCreatePinnedToCore(
    bleStatus,    // Function to be called
    "Bluetooth status", // Name of task
    1024,         // Stack size. bytes
    NULL,         // Parameter to pass to function
    3,            // Task priority
    NULL,         // Task handle
    app_cpu);     // Run
  // Task for WiFi
  xTaskCreatePinnedToCore(
    keepWiFiAlive,    // Function to be called
    "Keep WiFi Alive", // Name of task
    4024,         // Stack size. bytes
    NULL,         // Parameter to pass to function
    2,            // Task priority
    NULL,         // Task handle
    app_cpu);     // Run
  // Personal task
  xTaskCreatePinnedToCore(
    myTask,       // Function to be called
    "My Task",    // Name of task
    1024,         // Stack size. bytes
    NULL,         // Parameter to pass to function
    1,            // Task priority
    NULL,         // Task handle
    app_cpu);     // Run
}

void loop() {
  // The code never gets here!
}
