#include "ble.h"
#include "config.h"
#include "storage.h"
#include <NimBLEDevice.h>

static NimBLEServer* pServer = nullptr;
static NimBLECharacteristic* pTxCharacteristic = nullptr;
static NimBLECharacteristic* pRxCharacteristic = nullptr;
static bool bleActive = false;
static bool deviceConnected = false;
static bool oldDeviceConnected = false;

// Static callback instances to avoid memory issues
class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
        Serial.println("BLE: onConnect called");
        deviceConnected = true;
    }

    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
        Serial.printf("BLE: onDisconnect called, reason=%d\n", reason);
        deviceConnected = false;
    }
};

class RxCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
        Serial.println("BLE: onWrite called");
        std::string rxValue = pCharacteristic->getValue();
        if (rxValue.length() > 0) {
            Serial.printf("BLE received: %s\n", rxValue.c_str());

            if (rxValue == "SYNC") {
                Serial.println("BLE sync requested");
                bleSendWorkoutLog();
            } else if (rxValue == "PING") {
                bleSend("PONG\n");
            }
        }
    }
};

// Static instances
static ServerCallbacks serverCallbacks;
static RxCallbacks rxCallbacks;

bool bleInit() {
    Serial.println("BLE: Initializing...");

    NimBLEDevice::init(BLE_DEVICE_NAME);
    Serial.println("BLE: Device initialized");

    pServer = NimBLEDevice::createServer();
    if (!pServer) {
        Serial.println("BLE: Failed to create server");
        return false;
    }
    pServer->setCallbacks(&serverCallbacks);
    Serial.println("BLE: Server created");

    NimBLEService* pService = pServer->createService(BLE_SERVICE_UUID);
    if (!pService) {
        Serial.println("BLE: Failed to create service");
        return false;
    }
    Serial.println("BLE: Service created");

    pTxCharacteristic = pService->createCharacteristic(
        BLE_TX_CHAR_UUID,
        NIMBLE_PROPERTY::NOTIFY
    );
    if (!pTxCharacteristic) {
        Serial.println("BLE: Failed to create TX characteristic");
        return false;
    }
    Serial.println("BLE: TX characteristic created");

    pRxCharacteristic = pService->createCharacteristic(
        BLE_RX_CHAR_UUID,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );
    if (!pRxCharacteristic) {
        Serial.println("BLE: Failed to create RX characteristic");
        return false;
    }
    pRxCharacteristic->setCallbacks(&rxCallbacks);
    Serial.println("BLE: RX characteristic created");

    pService->start();
    Serial.println("BLE: Service started");

    Serial.println("BLE: Initialized successfully");
    return true;
}

void bleStart() {
    if (bleActive) {
        Serial.println("BLE: Already active");
        return;
    }

    Serial.println("BLE: Starting advertising...");
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(BLE_SERVICE_UUID);
    pAdvertising->start();

    bleActive = true;
    Serial.println("BLE: Advertising started");
}

void bleStop() {
    if (!bleActive) return;

    Serial.println("BLE: Stopping...");
    NimBLEDevice::getAdvertising()->stop();
    bleActive = false;
    deviceConnected = false;
    Serial.println("BLE: Stopped");
}

bool bleIsActive() {
    return bleActive;
}

bool bleIsConnected() {
    return deviceConnected;
}

size_t bleSend(const char* data) {
    if (!deviceConnected || !pTxCharacteristic) return 0;

    size_t len = strlen(data);
    const size_t chunkSize = 20;
    size_t sent = 0;

    while (sent < len) {
        size_t remaining = len - sent;
        size_t toSend = (remaining > chunkSize) ? chunkSize : remaining;

        pTxCharacteristic->setValue((uint8_t*)(data + sent), toSend);
        pTxCharacteristic->notify();
        sent += toSend;

        delay(10);
    }

    return sent;
}

size_t bleSend(const String& data) {
    return bleSend(data.c_str());
}

bool bleSendWorkoutLog() {
    if (!deviceConnected) {
        Serial.println("Cannot send log: not connected");
        return false;
    }

    if (!fileExists(LOGFILE)) {
        bleSend("NO_DATA\n");
        Serial.println("No workout log file exists");
        return false;
    }

    Serial.println("Sending workout log over BLE...");
    bleSend("BEGIN_LOG\n");

    bool success = readFileByLine(LOGFILE, [](const String& line) {
        bleSend(line + "\n");
    });

    bleSend("END_LOG\n");
    Serial.println("Workout log sent");

    return success;
}

void bleUpdate() {
    if (!deviceConnected && oldDeviceConnected && bleActive) {
        delay(500);
        NimBLEDevice::getAdvertising()->start();
        Serial.println("BLE: Restarted advertising");
    }

    oldDeviceConnected = deviceConnected;
}
