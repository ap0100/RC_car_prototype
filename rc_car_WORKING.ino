#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// --- Motor Pins ---
const int IN1 = 26; // Motor A
const int IN2 = 33; // Motor A
const int IN3 = 13; // Motor B
const int IN4 = 4;  // Motor B

// --- LED Pins ---
const int L1 = 18;
const int L2 = 23;
bool lightsOn = false;

// --- BLE UUIDs ---
#define SERVICE_UUID           "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID_RX "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLECharacteristic *pRxCharacteristic;
bool deviceConnected = false;

// --- Motor Helper Functions ---
void stopMotors() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
}
void goForward() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}
void goReverse() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
}
void turnLeft() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, LOW);
}
void turnRight() {
  digitalWrite(IN1, LOW);  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
}

// --- LED Helper ---
void toggleLights() {
  lightsOn = !lightsOn;
  digitalWrite(L1, lightsOn ? HIGH : LOW);
  digitalWrite(L2, lightsOn ? HIGH : LOW);
  Serial.print("Lights: ");
  Serial.println(lightsOn ? "ON" : "OFF");
}

// --- BLE Callbacks ---
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("Client connected");
    };
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("Client disconnected");
      stopMotors(); // safety: stop motors if connection drops
      pServer->startAdvertising();
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      String value = pCharacteristic->getValue();
      if (value.length() > 0) {
        char cmd = value[0];
        Serial.print("Received: "); Serial.println(cmd);

        switch(cmd) {
          case 'F': goForward(); break;
          case 'B': goReverse(); break;
          case 'L': turnLeft(); break;
          case 'R': turnRight(); break;
          case 'S': stopMotors(); break;
          case 'T': toggleLights(); break;
          default: break;
        }
      }
    }
};

void setup() {
  Serial.begin(115200);

  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(L1, OUTPUT);  pinMode(L2, OUTPUT);
  stopMotors();
  digitalWrite(L1, LOW);
  digitalWrite(L2, LOW);

  BLEDevice::init("ESP32_RC_Car");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);
  pRxCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_RX,
                        BLECharacteristic::PROPERTY_WRITE
                      );
  pRxCharacteristic->setCallbacks(new MyCallbacks());

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  BLEDevice::startAdvertising();

  Serial.println("BLE RC Car ready. Waiting for connections...");
}

void loop() {
  delay(100);
}