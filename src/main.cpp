#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>

#include <NimBLEDevice.h>
#include <NimBLEAdvertisedDevice.h>
#include "NimBLEEddystoneURL.h"
#include "NimBLEEddystoneTLM.h"
#include "NimBLEBeacon.h"

#include <RealTimeClock.h>

/** Define a class to handle the callbacks when events occur on the scanning process */
class MyAdvertisedDeviceCallbacks: public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
        DateTime now = rtc.now();
        Serial.printf("Advertised Device found: %s \n", advertisedDevice->toString().c_str());
        // Print raw packet data
        uint8_t* payload = advertisedDevice->getPayload();
        int payloadLength = advertisedDevice->getPayloadLength();

        // Print the time
        Serial.printf("Time: %02d: %02d: %02d:", now.hour(), now.minute(), now.second());
        Serial.println("\n");

        // Print the RSSI
        int rssi = advertisedDevice->getRSSI();
        Serial.println("RSSI: " + String(rssi) + "dbm");
        
        // Serial.printf("Payload length: %d\n", payloadLength);
        for(int i = 0; i < payloadLength; i++) {
            Serial.printf("%02X ", payload[i]);
        }
        Serial.println("\n");



        // Try to find any UUID in the vicinity
        if (advertisedDevice->haveServiceUUID()) {
          NimBLEUUID uuid = advertisedDevice->getServiceUUID().toString();
          Serial.printf("Service UUID: %s" , uuid);
        }

    }
};

void setup() {
    Serial.begin(115200);
    setupRealTimeClock();
    Serial.println("Starting BLE scan");

    // Initialize the BLE device
    NimBLEDevice::init("");

    // Set the callbacks that will handle the scanning results
    NimBLEDevice::getScan()->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());

    // Start scanning for devices and print out each one found
    NimBLEDevice::getScan()->start(0);  // this is a non-blocking function
}

void loop() {
    // put your main code here, to run repeatedly:
    delay(2000);
}
