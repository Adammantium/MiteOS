#include "WeatherManager.h"

#include "../MiteOS.h"
#include "WifiConnectionManager.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "../Managers/FileManager.h"
#include "../Images/weather_icons.h"
#include "../WebRequest/WebRequest.h"

RTC_DATA_ATTR long lastWeatherCheck = 0;
RTC_DATA_ATTR long lastWeatherDownload = 0;

RTC_DATA_ATTR WeatherData currentWeatherData;

String WeatherManager::city;
String WeatherManager::unit;
String WeatherManager::token;
String WeatherManager::lat;
String WeatherManager::lon;

bool WeatherManager::isDataCurrent() {
	return currentWeatherData.timestamp > 0 && day(NOW - currentWeatherData.timestamp) <= WEATHER_DATA_MAX_AGE;
}



WeatherData WeatherManager::getWeatherData(bool cached) {
	if(!cached) {
		lastWeatherCheck = 0;
		lastWeatherDownload = 0;
	}

	// Check if weather data is current enough to be cached or just force an update
	if( WeatherManager::isDataCurrent()
	&& (NOW - lastWeatherCheck) / 60 <= WEATHER_UPDATE_INTERVAL
	)
		return currentWeatherData;

	Configuration::loadOwmConfig();

	loadOpenMeteoData(WeatherManager::getUnit(), WeatherManager::getLat(), WeatherManager::getLon());
	
	char buffer[11];
	snprintf(buffer, 11, "%04d-%02d-%02d", tmYearToCalendar(MiteOS::currentTime.Year), MiteOS::currentTime.Month, MiteOS::currentTime.Day);
	String day = String(buffer);

	snprintf(buffer, 6, "%02d:%02d", MiteOS::currentTime.Hour, 0);
	String time = String(buffer);


	WeatherDataDay dayData = getWeatherDataDay(day);
	WeatherData timeData = getWeatherDataTime(day, time);

	currentWeatherData.temperature = timeData.temperature;
	currentWeatherData.weatherConditionCode = timeData.weatherConditionCode;
	currentWeatherData.isMetric = timeData.isMetric;
	getWeatherDescription(currentWeatherData.weatherConditionCode).toCharArray(currentWeatherData.weatherDescription, 30);
	getCity().toCharArray(currentWeatherData.cityName, 30);

	currentWeatherData.sunrise = dayData.sunrise;
	currentWeatherData.sunset = dayData.sunset;
	currentWeatherData.timestamp = Configuration::preferences.getULong64("wts", NOW);;
	lastWeatherCheck = NOW;

	return currentWeatherData;
	/*return _getWeatherData( WeatherManager::getCity()
						  , WeatherManager::getUnit()
						  , Configuration::getWeatherLang()
						  , Configuration::getWeatherURL()
						  , WeatherManager::getToken()
						  );
	*/
}

WeatherData WeatherManager::_getWeatherData(String cityID, String units, String lang, String url, String apiKey) {
	
	currentWeatherData.isMetric = units == String("metric");

	if((NOW - lastWeatherCheck) / 60 > WEATHER_UPDATE_INTERVAL && apiKey.length() > 0 && cityID.length() > 0 && units.length() > 0) {
		lastWeatherCheck = NOW;

		String weatherQueryURL = url;
		weatherQueryURL.replace("{cityID}", cityID);
		weatherQueryURL.replace("{units}", units);
		weatherQueryURL.replace("{lang}", lang);
		weatherQueryURL.replace("{apiKey}", apiKey);
		
		WebRequestData data = WebRequest::GET(url);
		if (data.httpResponseCode == 200) {
			String payload             = data.getString();

			JsonDocument json;
			deserializeJson(json, payload);
			currentWeatherData.temperature = int(json["main"]["temp"]);
			currentWeatherData.weatherConditionCode = int(json["weather"][0]["id"]);
			String desc = json["weather"][0]["description"];
			desc.toCharArray(currentWeatherData.weatherDescription, 30);
			getCity().toCharArray(currentWeatherData.cityName, 30);
			//currentWeatherData.external = true;
			
			//gmtTimeOffset = int(json["timezone"]);

			breakTime((time_t)(int)json["sys"]["sunrise"] + gmtTimeOffset, currentWeatherData.sunrise);
			breakTime((time_t)(int)json["sys"]["sunset"] + gmtTimeOffset, currentWeatherData.sunset);
			
			// sync NTP during weather API call and use timezone of lat & lon
			WifiConnectionManager::syncNTP(gmtTimeOffset);

			currentWeatherData.timestamp = NOW;
		}
		
		// turn off radios
		WifiConnectionManager::powerOff();
		btStop();
	}

	uint8_t chip_temperature = accSensor.readTemperature(); // celsius
	if (!currentWeatherData.isMetric) {
		chip_temperature = chip_temperature * 9. / 5. + 32.; // fahrenheit
	}
	currentWeatherData.chip_temperature = chip_temperature;

	return currentWeatherData;
}

double WeatherManager::_Julian(int32_t year, int32_t month, const double &day) {
	int32_t b, c, e;
	b = 0;
	if (month < 3) {
		year--;
		month += 12;
	}
	
	if (year > 1582 || (year == 1582 && month > 10) ||
		(year == 1582 && month == 10 && day > 15)) {
		int32_t a;
		a = year / 100;
		b = 2 - a + a / 4;
	}
	c = 365.25 * year;
	e = 30.6001 * (month + 1);

	return b + c + e + day + 1720994.5;
}

float WeatherManager::getMoonPhase() {
	return getMoonPhase(MiteOS::currentTime.Year, MiteOS::currentTime.Month, MiteOS::currentTime.Day, MiteOS::currentTime.Hour, MiteOS::currentTime.Minute);
}


// Phase from 0 - 0.5 = Increasing Moon
// Phase from 0.5 - 1 = Decreasing Moon
float WeatherManager::getMoonPhase(uint8_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute) {
	double j = _Julian(((int32_t) year) + 1970, month, (double) day + (hour / 24.0) + (minute / 1440.0));
	
	// Calculate illumination (synodic) phase.
	// From number of days since new moon on Julian date MOON_SYNODIC_OFFSET
	// (1815UTC January 6, 2000), determine remainder of incomplete cycle.
	float phase = (j - MOON_SYNODIC_OFFSET) / MOON_SYNODIC_PERIOD;
	phase -= floor(phase);
	
	return phase;
}

void WeatherManager::loadOpenMeteoData(String units, String lat, String lon) {
	String unit = units == String("metric") ? "celsius" : "fahrenheit";
	currentWeatherData.isMetric = units == String("metric");

	#if PROXY_WEB_REQUESTS_THROUGH_PHONE
	if(!PhoneConnectionManager::GetGPSPosition())
		return;
	#endif
	
	if(lat.length() > 0 && lon.length() > 0) {
		lastWeatherDownload = NOW;


		String weatherQueryURL = OPENMETEO_URL;
		weatherQueryURL.replace("{lat}", lat);
		weatherQueryURL.replace("{long}", lon);
		weatherQueryURL.replace("{units}", unit);
		
		WebRequestData data = WebRequest::GET(weatherQueryURL);
		if(data.isSuccess()) {
			String payload = data.getString();

			//JsonDocument json;
			//deserializeJson(json, payload);

			FileManager::writeFile(PATH_WEATHER"data", payload);
			
			Configuration::preferences.putULong64("wts", NOW);
		}
		
		WifiConnectionManager::powerOff();
		btStop();
	}
}
bool WeatherManager::parseISOToTm(const char* str, tmElements_t &tm) {
  int year, month, day, hour, minute;
  
  // sscanf gibt die Anzahl der erfolgreich gematchten Werte zurück
  // 't' im Format-String fängt das Trennzeichen ab
  if (sscanf(str, "%d-%d-%dT%d:%d", &year, &month, &day, &hour, &minute) == 5) {
    tm.Year = CalendarYrToTm(year); // Konvertiert z.B. 2026 in den Offset seit 1970
    tm.Month = month;
    tm.Day = day;
    tm.Hour = hour;
    tm.Minute = minute;
    tm.Second = 0; // Nicht im String enthalten, daher auf 0 setzen
    return true;
  }
  return false;
}

WeatherDataDay WeatherManager::getWeatherDataDay(String day) {
	WeatherDataDay weatherData;

	if(FileManager::exists(PATH_WEATHER"data")) {
		String file = FileManager::readFile(PATH_WEATHER"data");
		
		JsonDocument json;
		deserializeJson(json, file);

		if(json.containsKey("daily")) {
			if(json["daily"].containsKey("time")) {
				JsonArray arr = json["daily"]["time"].as<JsonArray>();
				int index = -1;
				for(int i = 0;i < arr.size(); i++) {
					if(day.equalsIgnoreCase(arr[i].as<String>())) {
						index = i;
						break;
					}
				}
				if(index >= 0) {
					WeatherManager::parseISOToTm(json["daily"]["sunrise"][index], weatherData.sunrise);
					WeatherManager::parseISOToTm(json["daily"]["sunset"][index], weatherData.sunset);
					weatherData.weatherConditionCode = json["daily"]["weather_code"][index];
					weatherData.temperatureMin = json["daily"]["temperature_2m_min"][index];
					weatherData.temperatureMax = json["daily"]["temperature_2m_max"][index];
				}else{
					printDebug(day + " not found in weather data");
					weatherData.weatherConditionCode = -1;
				}
			}
		}
	}
	return weatherData;
}
WeatherData WeatherManager::getWeatherDataTime(String day, String time) {
	WeatherData weatherData;
	if(FileManager::exists(PATH_WEATHER"data")) {
		String file = FileManager::readFile(PATH_WEATHER"data");
		
		JsonDocument json;
		deserializeJson(json, file);

		String key = day + "T" + time;

		if(json.containsKey("hourly")) {
			if(json["hourly"].containsKey("time")) {
				JsonArray arr = json["hourly"]["time"].as<JsonArray>();
				int index = -1;
				for(int i = 0;i < arr.size(); i++) {
					if(key.equalsIgnoreCase(arr[i].as<String>())) {
						index = i;
						break;
					}
				}
				if(index >= 0) {
					weatherData.weatherConditionCode = json["hourly"]["weather_code"][index];
					weatherData.temperature = json["hourly"]["temperature_2m"][index];
					weatherData.isMetric = String("°C").equalsIgnoreCase(json["hourly_units"]["temperature_2m"]);
				}else{
					printDebug(key + " not found in weather data");
				}
			}
		}
	}
	return weatherData;
}

String WeatherManager::getWeatherDescription(int code) {
	switch (code) {
		case 0: return TXT_WEATHER_CLEAR_SKY;
		case 1: return TXT_WEATHER_MAINLY_CLEAR;
		case 2: return TXT_WEATHER_PARTLY_CLOUDY;
		case 3: return TXT_WEATHER_OVERCAST;

		case 45: return TXT_WEATHER_FOG;
		case 48: return TXT_WEATHER_DEPOSITING_RIME_FOG;

		case 51: return TXT_WEATHER_LIGHT_DRIZZLE;
		case 53: return TXT_WEATHER_MODERATE_DRIZZLE;
		case 55: return TXT_WEATHER_DENSE_DRIZZLE;
		
		case 56: return TXT_WEATHER_LIGHT_FREEZING_DRIZZLE;
		case 57: return TXT_WEATHER_DENSE_FREEZING_DRIZZLE;

		case 61: return TXT_WEATHER_LIGHT_RAIN;
		case 63: return TXT_WEATHER_MODERATE_RAIN;
		case 65: return TXT_WEATHER_HEAVY_RAIN;

		case 66: return TXT_WEATHER_LIGHT_FREEZING_RAIN;
		case 67: return TXT_WEATHER_HEAVY_FREEZING_RAIN;

		case 71: return TXT_WEATHER_SLIGHT_SNOWFALL;
		case 73: return TXT_WEATHER_MODERATE_SNOWFALL;
		case 75: return TXT_WEATHER_HEAVY_SNOWFALL;

		case 77: return TXT_WEATHER_SNOW_GRAINS;

		case 95: return TXT_WEATHER_SLIGHT_THUNDERSTORM;
		case 96: return TXT_WEATHER_HEAVY_THUNDERSTORM;
		case 99: return TXT_WEATHER_HEAVY_THUNDERSTORM;
		
		default:
			return "";
	}
}

const unsigned char* WeatherManager::getWeatherIcon(int code) {
	if(code >= 90)
		return thunderstorm;
	else if(code >= 80)
		return rain;
	else if(code >= 70)
		return snow;
	else if(code >= 60)
		return rain;
	else if(code >= 50)
		return drizzle;
	else if(code >= 40)
		return atmosphere;
	else if(code >= 3)
		return cloudy;
	else if(code >= 1)
		return cloudsun;
	
	return sunny;
}

const unsigned char* WeatherManager::getSmallWeatherIcon(int code) {
	if(code >= 90)
		return weather_small_thunderstorm;
	else if(code >= 80)
		return weather_small_rain_mix;
	else if(code >= 70)
		return weather_small_snow;
	else if(code >= 60)
		return weather_small_rain_mix;
	else if(code >= 3)
		return weather_small_cloud;
	else if(code >= 1)
		return weather_small_cloudy;

	return weather_small_sunny;
}