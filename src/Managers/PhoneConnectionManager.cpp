#include "PhoneConnectionManager.h"

#include "BluetoothManager.h"
#include "HassManager.h"
#include "../Data/Configuration.h"
#include <ArduinoJson.h>
#include "../Data/Base64.hpp"
#include "../Managers/FileManager.h"
#include "WeatherManager.h"

RTC_DATA_ATTR long lastNotificationCheck = 0;

bool PhoneConnectionManager::SyncNotifications(bool force) {
	if(force) lastNotificationCheck = 0;
	
	if((NOW - lastNotificationCheck) / 60 <= NOTIFICATION_UPDATE_INTERVAL) {
		return false;
	}
	
	lastNotificationCheck = NOW;
	
	BluetoothManager::requestNotifications();
	
	if(!BluetoothManager::connected) return false;

	if(BluetoothManager::lastResponse.length() > 0) {
		JsonDocument json;
		deserializeJson(json, BluetoothManager::lastResponse);
		
		if(json.containsKey("count")) {
			uint8_t count = uint8_t(json["count"]);
			if(count > NOTIFICATION_CNT) count = NOTIFICATION_CNT;
			
			FileManager::emptyDir(PATH_NOTIFICATIONS);

			for(uint8_t i = 0; i < count; i++) {
				FileManager::writeFile(PATH_NOTIFICATIONS + String(i), json["nBundleList"][i]);
			}

			return true;
		}
	}
	return false;
}

Notification PhoneConnectionManager::GetNotification(uint8_t index) {
	JsonDocument json;
	deserializeJson(json, FileManager::readFile(PATH_NOTIFICATIONS + String(index)));

	Notification n;

	if(json.containsKey("appName")) {
		String temp = (const char*) json["appName"]; temp.toCharArray(n.app_name, NOTIFICATION_APP_NAME_LENGTH, 0);
		temp = (const char*) json["title"]; temp.toCharArray(n.title, NOTIFICATION_TITLE_LENGTH, 0);
		temp = (const char*) json["text"]; temp.toCharArray(n.message, NOTIFICATION_MESSAGE_LENGTH, 0);
	}

	return n;
}

uint8_t PhoneConnectionManager::GetNotificationCount() {
	return FileManager::dirFileCount(PATH_NOTIFICATIONS);
}

PlaybackInfo PhoneConnectionManager::RequestPlaybackInfo(bool cached) {
	JsonDocument json;

	if(cached && FileManager::exists(PATH_PLAYBACK"dat")) {
		printDebug("Reading cached file...");
		String str = FileManager::readFile(PATH_PLAYBACK"dat");
		if(str.length() > 0) {
			deserializeJson(json, str);
		}
	}else{
		BluetoothManager::sendCommand("GET_PLAYBACK_INFO=");

		if(BluetoothManager::lastResponse.length() > 0) {
			printDebug("Received Data...");
			deserializeJson(json, BluetoothManager::lastResponse);
		}else{
			return PlaybackInfo();
		}
		BluetoothManager::powerOff();
	}
	PlaybackInfo pbi;	
	if(json != nullptr) {
		if(json.containsKey("title")) {
			printDebug("Parsing JSON");
			if(json.containsKey("title")) {
				
				String str = json["title"];
				str.toCharArray(pbi.title, PLAYBACK_TEXT_LENGTH, 0);
			}
			
			if(json.containsKey("album")) {
				String str = json["album"];
				str.toCharArray(pbi.album, PLAYBACK_TEXT_LENGTH, 0);
			}
			
			if(json.containsKey("artist")) {
				String str = json["artist"];
				str.toCharArray(pbi.artist, PLAYBACK_TEXT_LENGTH, 0);
			}
			
			
			if(json.containsKey("position")) {
				pbi.position = long(json["position"]);
			}
			
			if(json.containsKey("duration")) {
				pbi.duration = long(json["duration"]);
			}
			
			if(json.containsKey("timestamp") && json.containsKey("playing")) { // Check if the time should have already passed, then just get the current data
				if(json["playing"]) {
					pbi.playing = true;
					long time_passed = NOW - ((unsigned long) json["timestamp"] + gmtTimeOffset);
					if(time_passed < 0) time_passed = 0;
					if(pbi.position + time_passed >= pbi.duration) {
						if(cached) {
							return RequestPlaybackInfo(false);
						}
					}
					pbi.position += time_passed;
				}
			}
			
			if(json.containsKey("art")) {
				String art = (const char*) json["art"];
				
				// Ineffiecient but works i guess
				int size = decode_base64_length((const unsigned char*) art.c_str());
				printDebug(size);
				if(size <= MAX_PLAYBACK_IMAGE_SIZE * MAX_PLAYBACK_IMAGE_SIZE / 8 + 1) {
					int binary_length = decode_base64((const unsigned char*) art.c_str(), pbi.image);
				}
				pbi.size = sqrt(size * 8);
				printDebug(pbi.size);
			}

			if(!cached) {
				FileManager::writeFile(PATH_PLAYBACK"dat", BluetoothManager::lastResponse);
			}
		}else{
			printDebug("JSON is not valid playback");
			FileManager::deleteFile(PATH_PLAYBACK"dat");
		}
	}
	
	return pbi;
}

bool PhoneConnectionManager::GetGPSPosition() {
	BluetoothManager::sendCommand("GET_POS=");
	
	if(BluetoothManager::lastResponse.length() > 0) {
		JsonDocument json;
		deserializeJson(json, BluetoothManager::lastResponse);
		
		if(json.containsKey("lat") && json.containsKey("lon") && json.containsKey("city")) {
			WeatherManager::setLat(json["lat"]);
			WeatherManager::setLon(json["lon"]);
			WeatherManager::setCity(json["city"]);
			Configuration::saveOwmConfig();
		}
		return true;
	}
	return false;
}

bool PhoneConnectionManager::SyncConfiguration() {
	printDebug("Requesting Configuration...");

	BluetoothManager::sendCommand("GET_CONFIGURATION=");

	Configuration::init();

	bool success = false;
	if(BluetoothManager::lastResponse.length() > 0) {
		JsonDocument json;
		deserializeJson(json, BluetoothManager::lastResponse);
		
		if(json.containsKey("hassUrl")) {
			if(json.containsKey("hassTkn")) {
				HassManager::setURL(json["hassUrl"]);
				HassManager::setToken(json["hassTkn"]);
				
				if(json.containsKey("entities")) {
					String entities = "";
					for(uint8_t i = 0; i < json["entities"].size(); i++) {
						if(i > 0) entities += ",";
						entities += (const char*) json["entities"][i]["name"];
						entities += ",";
						entities += (const char*) json["entities"][i]["key"];
					}
					printDebug(entities);
					HassManager::setEntities(entities);
				}
				success = true;
				Configuration::saveHassConfig();
			}
		}
		
		if(json.containsKey("totp")) {
			String tokens = "";
			for(uint8_t i = 0; i < json["totp"].size(); i++) {
				if(i > 0) tokens += ",";
				tokens += (const char*) json["totp"][i]["name"];
				tokens += ",";
				tokens += (const char*) json["totp"][i]["token"];
			}
			printDebug(tokens);
			success = true;
			Configuration::preferences.putString("totp", tokens);
		}

		if(json.containsKey("owmUnit")) {
			WeatherManager::setUnit(json["owmUnit"]);
			Configuration::saveOwmConfig();
		}
		if(json.containsKey("tz")) {
			Configuration::setTimeZone(json["tz"]);
		}
		if(json.containsKey("time")) {
			tmElements_t tm;
			time_t time = json["time"];
			time /= 1000; // Convert from millisecond to seconds
			time += Configuration::getTimeZone() * 60;
			mRTC.doBreakTime(time, tm);
			mRTC.set(tm);

			#if DEBUG == TRUE
			Serial.print("After RTC.set - Year: ");
			Serial.print(tm.Year);
			Serial.print(" Month: ");
			Serial.print(tm.Month);
			Serial.print(" Day: ");
			Serial.print(tm.Day);
			Serial.print(" Hour: ");
			Serial.print(tm.Hour);
			Serial.print(" Min: ");
			Serial.println(tm.Minute);
			#endif
		}
	}
	return success;
}
bool PhoneConnectionManager::SyncCalendar() {
	printDebug("Requesting Calendar...");

	BluetoothManager::sendCommand("GET_CALENDAR=");
	
	if(BluetoothManager::lastResponse.length() > 0) {
		FileManager::init();

		// Cleanup
		FileManager::emptyDir(PATH_CALENDAR);
		
		/*File root = LittleFS.open(PATH_CALENDAR);
		File file = root.openNextFile();
    	while(file) {
			String name = file.name();
			long startTime = name.substring(0, name.indexOf("_")).toInt();

			if(startTime < NOW) {
				String path = file.path();
				file.close();
				printDebug(path);
				LittleFS.remove(path);
			}
			file = root.openNextFile();
		}*/

		// Write all the data
		JsonDocument json;
		deserializeJson(json, BluetoothManager::lastResponse);
		if(json.containsKey("events")) {
			printDebug(json["events"].size());
			if(json["events"].size() > 0) {
				for(uint16_t i = 0; i < json["events"].size(); i++) {
					String id = json["events"][i]["startTime"].as<String>() + "_" + String(json["events"][i]["id"].as<int>());
					
					FileManager::writeFile(String(PATH_CALENDAR) + id, json["events"][i]);
				}
			}
		}
		return true;
	}
	return false;
}