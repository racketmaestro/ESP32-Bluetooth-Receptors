#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <NimBLEDevice.h>
#include <NimBLEAdvertisedDevice.h>
#include "NimBLEEddystoneURL.h"
#include "NimBLEEddystoneTLM.h"
#include "NimBLEBeacon.h"
#include <RealTimeClock.h>
#include <map>
#include <SD.h>

std::map<std::string, NimBLEAdvertisedDevice*> oldDevices;
File dataFile; 
uint32_t chipId = ESP.getEfuseMac();

// Function to convert a byte into a 2-character hexadecimal string
String byteToHexString(uint8_t byte, bool uppercase = true) {
  const char* hexChars = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
  String hexString = "";
  hexString += hexChars[(byte >> 4) & 0x0F];
  hexString += hexChars[byte & 0x0F];
  return hexString;
}

// Function to convert a byte array into a hexadecimal string
String bytesToHexString(const uint8_t* bytes, int len, bool uppercase = true) {
  String hexString = "";
  for(int i = 0; i < len; i++) {
    hexString += byteToHexString(bytes[i], uppercase);
  }
  return hexString;
}



void setup() {
    Serial.begin(115200);
    setupRealTimeClock();

    if (!SD.begin(5)) { // replace 5 with your actual CS pin if it's different
      Serial.println("SD card initialization failed!");
      return;
    }
    // Open file
    dataFile = SD.open("/data.csv", FILE_WRITE);
    if (!dataFile) {
      Serial.println("Error opening data.csv");
    return;
    }
    // Write header
    dataFile.println("receptor_id,timestamp,mac_address,manufacturer_data,RSSI,payload,status");
    
    Serial.println("Starting BLE scan");
    NimBLEDevice::init("");
}

void loop() {
    std::map<std::string, NimBLEAdvertisedDevice*> newDevices;

    NimBLEScanResults scanResults = NimBLEDevice::getScan()->start(5, false);

    for (int i = 0; i < scanResults.getCount(); i++) {
        NimBLEAdvertisedDevice* device = new NimBLEAdvertisedDevice(scanResults.getDevice(i));
        newDevices[device->getAddress().toString()] = device;
    }

    for (auto const& x: oldDevices) {
        if (newDevices.find(x.first) == newDevices.end()) {
            DateTime lostTime = rtc.now();
            Serial.printf("Lost Device: %s\n", x.second->toString().c_str());
            Serial.printf("Time: %02d: %02d: %02d\n", lostTime.hour(), lostTime.minute(), lostTime.second());
            Serial.println("\n");

            std::string manufacturerData = x.second->getManufacturerData();
            String manufacturerDataHex = bytesToHexString((const uint8_t*)manufacturerData.data(), manufacturerData.size());


            if (dataFile) {
            // Write to the file
              int bytesWritten = dataFile.printf("%08X, %02d:%02d:%02d, %s, %s, , , Disconnected\n",
                                                  chipId, 
                                                  lostTime.hour(), 
                                                  lostTime.minute(), 
                                                  lostTime.second(),
                                                  x.second->getAddress().toString().c_str(),
                                                  manufacturerDataHex.c_str());

              if (bytesWritten <= 0) {
                Serial.println("Failed to write to file");
              }
              dataFile.flush(); // make sure the data gets written
            } else {
              Serial.println("File not open for writing");
            }
        }
    }

    for (auto const& x: newDevices) {
        if (oldDevices.find(x.first) == oldDevices.end()) {
            DateTime foundTime = rtc.now();
            Serial.printf("New Device found: %s\n", x.second->toString().c_str());
            // Print raw packet data
            uint8_t* payload = x.second->getPayload();
            int payloadLength = x.second->getPayloadLength();

            // Print the time
            Serial.printf("Time: %02d: %02d: %02d\n", foundTime.hour(), foundTime.minute(), foundTime.second());

            // Print the RSSI
            int rssi = x.second->getRSSI();
            Serial.println("RSSI: " + String(rssi) + "dbm");

            // Convert manufacturer data to hex string
            std::string manufacturerData = x.second->getManufacturerData();
            String manufacturerDataHex = bytesToHexString((const uint8_t*)manufacturerData.data(), manufacturerData.size());

            String payloadString;
            for(int i = 0; i < payloadLength; i++) {
                char str[3];
                sprintf(str, "%02X", payload[i]);
                payloadString += str;
                Serial.print(str);
                Serial.print(" ");
            }
            Serial.println("\n");

            if (dataFile) {
            // Write to the file
              int bytesWritten = dataFile.printf("%08X, %02d:%02d:%02d, %s, %s, %d, %s, Connected\n", 
                                                chipId, 
                                                foundTime.hour(), 
                                                foundTime.minute(), 
                                                foundTime.second(), 
                                                x.second->getAddress().toString().c_str(), 
                                                manufacturerDataHex.c_str(),
                                                rssi, 
                                                payloadString.c_str());
              if (bytesWritten <= 0) {
                Serial.println("Failed to write to file");
              }
              dataFile.flush(); // make sure the data gets written
            } else {
              Serial.println("File not open for writing");
            }
        }
    }

    // Release memory of old devices before assigning new ones
    for (auto & x : oldDevices) {
        delete x.second;
    }

    oldDevices = newDevices;
    delay(10000);
}

