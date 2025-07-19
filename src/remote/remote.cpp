#include "remote.hpp"
#include "config.h"
#include "uuids.h"
#include <NimBLEDevice.h>


static NimBLEServer* server;


/**  None of these are required as they will be handled by the library with defaults. **
 **                       Remove as you see fit for your needs                        */
class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
        Serial.printf("Client address: %s\n", connInfo.getAddress().toString().c_str());

        /**
         *  We can use the connection handle here to ask for different connection parameters.
         *  Args: connection handle, min connection interval, max connection interval
         *  latency, supervision timeout.
         *  Units; Min/Max Intervals: 1.25 millisecond increments.
         *  Latency: number of intervals allowed to skip.
         *  Timeout: 10 millisecond increments.
         */
        pServer->updateConnParams(connInfo.getConnHandle(), 24, 48, 0, 180);
    }

    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
        Serial.printf("Client disconnected - start advertising\n");
        NimBLEDevice::startAdvertising();
    }

} server_callbacks;



namespace RemoteEvents {
    enum Value {
        None = 0,
        Input = 0x01,
    };
};


static InputEventCallback s_input_callback = nullptr;


/** Handler class for characteristic actions */
class CharacteristicCallbacks : public NimBLECharacteristicCallbacks {
    // void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
    //     Serial.printf("%s : onRead(), value: %s\n",
    //                   pCharacteristic->getUUID().toString().c_str(),
    //                   pCharacteristic->getValue().c_str());
    // }

    void onWrite(NimBLECharacteristic* blechar, NimBLEConnInfo& conn_info) override {
        // Serial.printf("%s : onWrite(), value: %s, on core: %d\n",
        //             pCharacteristic->getUUID().toString().c_str(),
        //             pCharacteristic->getValue().c_str(),
        //             xPortGetCoreID());
        // Serial.println("hello");

        auto value = blechar->getValue();
        const uint8_t *data = value.data();
        size_t len = value.length();

        // input command
        if(len == 4 && data[0] == RemoteEvents::Input) {
            InputEvent event;
            event.id = input_id_from_uint8(data[1]);
            event.value = (int8_t)data[2];
            event.shifed = data[3] > 0;

            Serial.printf("id: %d, val: %d, shift: %d", event.id, event.value, event.shifed);
            if(s_input_callback) s_input_callback(event);
        }
    }

    /**
     *  The value returned in code is the NimBLE host return code.
     */
    // void onStatus(NimBLECharacteristic* pCharacteristic, int code) override {
    //     Serial.printf("Notification/Indication return code: %d, %s\n", code, NimBLEUtils::returnCodeToString(code));
    // }

    // /** Peer subscribed to notifications/indications */
    // void onSubscribe(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo, uint16_t subValue) override {
    //     std::string str  = "Client ID: ";
    //     str             += connInfo.getConnHandle();
    //     str             += " Address: ";
    //     str             += connInfo.getAddress().toString();
    //     if (subValue == 0) {
    //         str += " Unsubscribed to ";
    //     } else if (subValue == 1) {
    //         str += " Subscribed to notifications for ";
    //     } else if (subValue == 2) {
    //         str += " Subscribed to indications for ";
    //     } else if (subValue == 3) {
    //         str += " Subscribed to notifications and indications for ";
    //     }
    //     str += std::string(pCharacteristic->getUUID());

    //     Serial.printf("%s\n", str.c_str());
    // }
} command_blechar_cb;


void remote_init() {
    NimBLEDevice::init(BLE_NAME);

    server = NimBLEDevice::createServer();
    server->setCallbacks(&server_callbacks);

    NimBLEService* service = server->createService(SERVER_UUID);

    NimBLECharacteristic* command_blechar =
        service->createCharacteristic(COMMAND_BLECHAR_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);

    service->start();

    command_blechar->setValue("");
    command_blechar->setCallbacks(&command_blechar_cb);

    NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
    advertising->setName(BLE_NAME);
    advertising->addServiceUUID(SERVER_UUID);
    advertising->enableScanResponse(true);

    advertising->start();
}


void remote_set_input_cb(InputEventCallback cb) {
    s_input_callback = cb;
}
