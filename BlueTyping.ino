#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "USB.h"
#include "USBHIDKeyboard.h"
#include "USBHIDMouse.h"

#define SERVICE_UUID        "12345678-1234-5678-1234-56789abcdef0"
#define CHARACTERISTIC_UUID "abcdef12-3456-7890-abcd-ef1234567890"

USBHIDMouse Mouse;
USBHIDKeyboard Keyboard;
BLECharacteristic* pCharacteristic;

// === BLE Disconnect Callback ===
class MyServerCallbacks : public BLEServerCallbacks {
  void onDisconnect(BLEServer* pServer) {
    Serial.println("BLE disconnected, restarting advertising...");
    delay(100);
    BLEDevice::startAdvertising();
  }
};

// === BLE Write Callback ===
class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pChar) {
    String rx = String(pChar->getValue().c_str());

    Serial.print("Received BLE: ");
    Serial.println(rx);

    if (rx == "[BS]") {
      Keyboard.write(KEY_BACKSPACE);
    }
    else if (rx == "\n") {
      Keyboard.write(KEY_RETURN);
    }
    else if (rx == "[UP]") {
      Keyboard.write(KEY_UP_ARROW);
    }
    else if (rx == "[DOWN]") {
      Keyboard.write(KEY_DOWN_ARROW);
    }
    else if (rx == "[LEFT]") {
      Keyboard.write(KEY_LEFT_ARROW);
    }
    else if (rx == "[RIGHT]") {
      Keyboard.write(KEY_RIGHT_ARROW);
    }
    else if (rx == "[ESC][WIN]") {
      Keyboard.press(KEY_LEFT_GUI);  // Tombol Windows (GUI)
      Keyboard.press(KEY_ESC);       // Tombol ESC
      delay(100);                    // Delay kecil untuk memastikan terkirim
      Keyboard.releaseAll();         // Lepaskan semua tombol
    }
    else if (rx.startsWith("[MOUSE_MOVE:")) {
      int x = 0, y = 0;
      sscanf(rx.c_str(), "[MOUSE_MOVE:%d,%d]", &x, &y);
      Mouse.move(x, y, 0);  // Tambah scroll = 0 (parameter ketiga wajib)
    }
    else if (rx == "[MOUSE_CLICK]") {
      Mouse.click(MOUSE_LEFT);
    }
    else if (rx == "[MOUSE_RIGHT]") {
      Mouse.click(MOUSE_RIGHT);
    }
    else if (rx.startsWith("[MOUSE_SCROLL:")) {
      int scrollY = 0;
      sscanf(rx.c_str(), "[MOUSE_SCROLL:%d]", &scrollY);
      Mouse.move(0, scrollY);
    }
    else {
      Keyboard.print(rx);
    }
  }
};

void setup() {
  Serial.begin(115200);
  delay(500);
  
  Mouse.begin();
  Keyboard.begin();
  delay(3000);  

  BLEDevice::init("BlueTyping");
  BLEServer* pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks()); 

  BLEService* pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_WRITE
  );
  pCharacteristic->addDescriptor(new BLE2902());
  pCharacteristic->setCallbacks(new MyCallbacks());

  pService->start();

  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();

  Serial.println("BLE + USB HID Ready!");
}

void loop() {
  delay(10);
}
