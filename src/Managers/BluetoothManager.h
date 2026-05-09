#ifndef BLUETOOTH_MANAGER_H
#define BLUETOOTH_MANAGER_H

#include <NimBLEDevice.h>
#include "WebRequest/WebRequestData.h"

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

#define MTU_SIZE 500

class BluetoothManager {
	public:
		static BLECharacteristic *commandCharacteristic;
		static BLECharacteristic *notificationUpdateCharacteristic;
		static BLEService *pService;
		static BLEServer *pServer;
		static bool connected;
		static bool waitingForResponse;
		static String lastResponse;
	private:
		static bool initialized;
		static char tmp_buffer[];
	public:
		static void init();
		static void bondDevice();
		static void unbondDevice();
		static void connectDevice();
		static void parseCommand(String value);
		static void requestNotifications();
		static void sendCommand(String str);

		static WebRequestData proxyGetWebRequest(String url, String authorization = "");
		static WebRequestData proxyPostWebRequest(String url, String payload, String authorization = "", String content_type = "");
		
		static void powerOff();
	private:
		static void startBLEAdvertising();
		static void waitForResponse();
		static WebRequestData awaitWebRequestData();
};

extern RTC_DATA_ATTR bool btDeviceRegistered;
extern RTC_DATA_ATTR BLEAddress btLastDevice;

#endif