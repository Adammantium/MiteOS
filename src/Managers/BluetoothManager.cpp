#include "BluetoothManager.h"
#include "../MiteOS.h"
#include "../Images/menu_icons.h"

BLECharacteristic *BluetoothManager::commandCharacteristic;
BLECharacteristic *BluetoothManager::notificationUpdateCharacteristic;
BLEService *BluetoothManager::pService;
BLEServer *BluetoothManager::pServer;
bool BluetoothManager::connected = false;
bool BluetoothManager::initialized = false;
bool BluetoothManager::waitingForResponse = false;

String BluetoothManager::lastResponse;
char BluetoothManager::tmp_buffer[500];

RTC_DATA_ATTR bool btDeviceRegistered(false);
RTC_DATA_ATTR BLEAddress btLastDevice(std::string("0"), 0);

class cb : public BLEServerCallbacks {
	void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
		BluetoothManager::connected = true;	
		printDebug("BLE Device Connected");
	}
	void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
		BluetoothManager::connected = false;
		printDebug("BLE Device Disconnected");
	}
};

class ccb : public BLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override {
		String rxValue = String(pCharacteristic->getValue().c_str());
		
		if (rxValue.length() > 0) {
			//Serial.println(rxValue.length());
			/*
			for (int i = 0; i < rxValue.length(); i++) {
				Serial.print(rxValue[i]);
			}
			Serial.println(" ");
			*/
			
			BluetoothManager::parseCommand(rxValue);
		}
	}
};


static int my_gap_event_handler(ble_gap_event *event, void *param) {
    switch (event->type) {
        case BLE_GAP_EVENT_ENC_CHANGE: {
			printDebug("Encryption state changed");
            // Check if encryption/pairing was successful
            if (event->enc_change.status == 0) {
				printDebug("Pairing was successful");
                // Get connection info to find the peer address
                struct ble_gap_conn_desc desc;
                if (ble_gap_conn_find(event->enc_change.conn_handle, &desc) == 0) {
                    NimBLEAddress address = NimBLEAddress(desc.peer_id_addr);
                    
                    // Add to whitelist
                    NimBLEDevice::whiteListAdd(address);
                    
                    btLastDevice = address;
                    btDeviceRegistered = true;
                    
                    #ifdef DEBUG
                    Serial.print("Bonded with ");
                    Serial.println(address.toString().c_str());
                    #endif
                }
            }else{
				printDebug("Pairing failed");
			}
            break;
        }
    }
	return 0;
}

/* BLE interfacing functions
*/
void BluetoothManager::init() {
	if(initialized) return;
	
	WifiConnectionManager::powerOff();
	
	printDebug("Initializing BT Device");
	BLEDevice::init("Mite");
	BLEDevice::setMTU(MTU_SIZE);
	BLEDevice::setSecurityAuth(false, false, true);

	pServer = BLEDevice::createServer();
	
	// add server callback so we can detect when we're connected.
	pServer->setCallbacks(new cb());
	
	initialized = true;
}

void BluetoothManager::unbondDevice() {
	printDebug("Unbonding current device...");
	
	btDeviceRegistered = false;
	btLastDevice = BLEAddress(std::string("0"), 0);
	
	Configuration::saveBluetooth();
}

void BluetoothManager::bondDevice() {
	init();
	
	btDeviceRegistered = false;
	
	printDebug("Waiting for Device to bond...");
	
	BLEDevice::setSecurityAuth(true, false, true);
	pServer->getAdvertising()->setScanFilter(false, false);

	BLEDevice::setCustomGapHandler(my_gap_event_handler);

	startBLEAdvertising();
	
	uint16_t wait = 0;
	while(!btDeviceRegistered && wait < 300) { // Wait for bonding or half a minute, whatever comes first
		delay(100);
		wait++;
		if(pServer->getConnectedCount() > 0) {
			btDeviceRegistered = true;
			btLastDevice = pServer->getPeerInfoByHandle(0).getIdAddress();
			break; // Device connected instead of bonding, probably already bonded?
		}
	}
	
	connected = pServer->getConnectedCount() > 0;
	
	Configuration::saveBluetooth();
}

void BluetoothManager::connectDevice() {
	if(!btDeviceRegistered) return;
	
	if(connected) return;

	PageManager::showConnectionIcon(icon_bluetooth);
	
	init();
	
	BLEDevice::whiteListAdd(btLastDevice);
	pServer->getAdvertising()->setScanFilter(false, false);
	
	pService = pServer->createService(SERVICE_UUID);
	// define the characteristics and how they can be used
	notificationUpdateCharacteristic = pService->createCharacteristic(
		CHARACTERISTIC_UUID_TX,
		NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ
	);
	
	commandCharacteristic = pService->createCharacteristic(
		CHARACTERISTIC_UUID_RX,
		NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::BROADCAST | NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::WRITE_NR
	);
	commandCharacteristic->setCallbacks(new ccb());
	
	//notificationUpdateCharacteristic->setCallbacks(new notification_update_callback());
	//notificationUpdateCharacteristic->setValue("");
	
	pService->start();
	
	startBLEAdvertising();
	
	uint8_t wait = 0;
	while(!connected && wait < 50) { // Wait for connection or 5 seconds, whatever comes first
		wait++;
		delay(100);
	}
	
	if(connected) delay(1000);
}

void BluetoothManager::startBLEAdvertising() {
	printDebug("startBLEAdvertising");
	BLEAdvertising* advertising = pServer->getAdvertising();
	advertising->setAppearance(192);
	advertising->addServiceUUID(SERVICE_UUID);
	//advertising->setScanResponse(true);
	//advertising->setMinPreferred(0x06); // functions that help with iPhone connections issue
	//advertising->setMinPreferred(0x12);
	advertising->start();
}

void BluetoothManager::parseCommand(String value) {

	if (value.startsWith("ECHO=")) {
		value.replace("ECHO=", "");
		printDebug(value);

	} else if (value.startsWith("NOTIFICATION_LIST")) {
		value.replace("NOTIFICATION_LIST=", "");
		//createNotificationList(value);
		printDebug(value);

	} else if (value.startsWith("NEW_NOTIFICATION=")) {
		value.replace("NEW_NOTIFICATION=", "");
		//alertNewNotification(value);
		printDebug(value);
		//NEW_NOTIFICATION={"appName":"Messages","category":"msg","id":0,"pName":"com.google.android.apps.messaging","text":"MHNYMA AΠO KINHTO","title":"Μωρό μου"}
		//NEW_NOTIFICATION={"appName":"Gmail","category":"email","id":0,"pName":"com.google.android.gm","subText":"themelisx@gmail.com","text":"Hello","title":"Παναγιώτης Θ"}

	} else if (value.startsWith("GET_LIST")) {
		//getNotificationList();
		printDebug(value);

	} else if (value.startsWith("ICON=")) {
		value.replace("ICON=", "");
		/*
		int inputStringLength = value.length();  //Get length of input
		char *inputString = (char *)malloc(inputStringLength + 1);
		value.getBytes((unsigned char *)inputString, inputStringLength);

		int decodedLength = BASE64::decodeLength(inputString);
		uint8_t *base64Result = (uint8_t *)malloc(inputStringLength);

		BASE64::decode(inputString, base64Result);

		drawArrayJpeg(base64Result, sizeof(base64Result), 0, 0);  //last two are coordinates to draw image

		free(base64Result);
		free(inputString);
		*/
	} else {
		//printDebug("Unknown command");
		//printDebug(value);
		
		//Serial.println(value.length());
	}
	
	BluetoothManager::lastResponse = BluetoothManager::lastResponse + value;
	// Make sure the data has ended or is smaller than our MTU size (with some padding)
	if(value.endsWith("}") || value.length() < MTU_SIZE - 8) {
		BluetoothManager::waitingForResponse = false;
	}
}

void BluetoothManager::requestNotifications() {
	printDebug("Requesting notification list...");
	int notificationType = 0;  //All notifications
	sprintf(tmp_buffer, "GET_NOTIF_LIST=%d", notificationType);
	sendCommand(tmp_buffer);
}

void BluetoothManager::sendCommand(String str) {
	// First try to connect, if its not connected after that, just exit
	if(!connected) connectDevice();
	if(!connected) return;
	
	BluetoothManager::lastResponse = "";
	commandCharacteristic->setValue("");
	delay(10);
	printDebug(str);
	notificationUpdateCharacteristic->setValue(str.c_str());
	notificationUpdateCharacteristic->notify();
	
	waitForResponse();
}

void BluetoothManager::waitForResponse() {
	waitingForResponse = true;
	BluetoothManager::lastResponse = "";
	
	uint8_t wait = 0;
	while(waitingForResponse && wait < 50 && connected) { // Wait for response or 5 seconds, whatever comes first
		if(wait % 10 == 0) notificationUpdateCharacteristic->notify();

		wait++;
		//Serial.println(commandCharacteristic->getLength());

		delay(100);
	}
	//Serial.println(wait);
	//Serial.println(commandCharacteristic->getLength());
	printDebug(BluetoothManager::lastResponse.length());
	printDebug(BluetoothManager::lastResponse);
	
	waitingForResponse = false;
}

WebRequestData BluetoothManager::awaitWebRequestData() {
	WebRequestData data = WebRequestData();
	if(BluetoothManager::lastResponse.length() == 0 || BluetoothManager::lastResponse == "ERR") {
		data.httpResponseCode = -1;
	}else{
		data.httpResponseCode = 200;
		data.responseData = BluetoothManager::lastResponse;
	}
	return data;
}

WebRequestData BluetoothManager::proxyGetWebRequest(String url, String authorization) {
	String full = url;
	if(!authorization.isEmpty()) full += ";" + authorization;

	sprintf(tmp_buffer, "WR_GET=%s", full.c_str());
	sendCommand(tmp_buffer);

	return BluetoothManager::awaitWebRequestData();
}

WebRequestData BluetoothManager::proxyPostWebRequest(String url, String payload, String authorization, String content_type) {
	String full = url + ";" + payload;
	if(!authorization.isEmpty()) full += ";" + authorization + ";" + content_type;
	
	sprintf(tmp_buffer, "WR_POST=%s", full.c_str());
	sendCommand(tmp_buffer);

	return BluetoothManager::awaitWebRequestData();
}

void BluetoothManager::powerOff() {
	btStop();
	initialized = false;
}