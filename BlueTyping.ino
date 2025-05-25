/* MIT License

 Copyright (c) 2025 [Robi Sitompul]

 Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights  
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell    
copies of the Software, and to permit persons to whom the Software is        
furnished to do so, subject to the following conditions:                     

 The above copyright notice and this permission notice shall be included in all 
copies or substantial portions of the Software.                              

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,    
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING     
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER         
DEALINGS IN THE SOFTWARE. */  

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "USB.h"
#include "USBHIDKeyboard.h"
#include "USBHIDMouse.h"
#include "USBHIDConsumerControl.h"

#define SERVICE_UUID        "12345678-1234-5678-1234-56789abcdef0"
#define CHARACTERISTIC_UUID "abcdef12-3456-7890-abcd-ef1234567890"

USBHIDMouse Mouse;
USBHIDKeyboard Keyboard;
USBHIDConsumerControl Consumer;
BLECharacteristic* pCharacteristic;

// === Variabel smoothing dan timing ===
int lastX = 0, lastY = 0;
unsigned long lastMouseMoveTime = 0;
const int mouseUpdateInterval = 10; // minimal delay antar gerakan (ms)
const int sensitivity = 2; // skala gerakan

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
    else if (rx == "[ESC]") {
      Keyboard.write(KEY_ESC);     
    }
    else if (rx == "[VOL_UP]") {
  Consumer.press(HID_USAGE_CONSUMER_VOLUME_INCREMENT);
  delay(100);
  Consumer.release();
}
else if (rx == "[VOL_DOWN]") {
  Consumer.press(HID_USAGE_CONSUMER_VOLUME_DECREMENT);
  delay(100);
  Consumer.release();
}
else if (rx == "[VOL_MUTE]") {
  Consumer.press(HID_USAGE_CONSUMER_MUTE);
  delay(100);
  Consumer.release();
}
else if (rx.startsWith("[MOUSE_MOVE:")) {
  int x = 0, y = 0;
  sscanf(rx.c_str(), "[MOUSE_MOVE:%d,%d]", &x, &y);
  x *= sensitivity;
  y *= sensitivity;

  int smoothX = (lastX * 3 + x) / 4;
  int smoothY = (lastY * 3 + y) / 4;

  // Batasin agar tidak overflow
  smoothX = constrain(smoothX, -127, 127);
  smoothY = constrain(smoothY, -127, 127);

  unsigned long now = millis();
  if ((smoothX != 0 || smoothY != 0) && now - lastMouseMoveTime >= mouseUpdateInterval) {
    Mouse.move(smoothX, smoothY, 0);
    lastMouseMoveTime = now;
    lastX = smoothX;
    lastY = smoothY;
  }
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
      Mouse.move(0, scrollY, 0);
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
  Consumer.begin();
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
