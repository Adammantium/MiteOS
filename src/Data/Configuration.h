//Network Settings
#define PROXY_WEB_REQUESTS_THROUGH_PHONE true

//Weather Settings
#define OPENWEATHERMAP_URL "http://api.openweathermap.org/data/2.5/weather?id={cityID}&lang={lang}&units={units}&appid={apiKey}" //open weather api using city ID
#define OPENMETEO_URL "https://api.open-meteo.com/v1/forecast?latitude={lat}&longitude={long}&timezone=auto&daily=weather_code,sunrise,sunset,temperature_2m_max,temperature_2m_min&hourly=temperature_2m,weather_code&past_days=0&forecast_days=7&temperature_unit={units}"
#define WEATHER_UPDATE_INTERVAL 10 //must be greater than 5, smaller than 255, measured in minutes
#define WEATHER_DOWNLOAD_INTERVAL 90 //must be greater than 5, smaller than 255, measured in minutes
#define WEATHER_DATA_MAX_AGE 7 // in days

//Notification Settings
#define NOTIFICATION_UPDATE_INTERVAL 15 //must be greater than 1, smaller than 255, measured in minutes

//NTP Settings
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_MIN 0

// Darkmode Settings // TODO Put as in System Settings
#define INVERSE_DARKMODE false // Defines whether the timespan will make it darkmode or lightmode
#define DARKMODE_START_H 23
#define DARKMODE_START_M 0
#define DARKMODE_END_H 6
#define DARKMODE_END_M 30

#define SUNDAY_IS_ZERO // if this is defined, then wday sunday = 0 and not wday sunday = 1 / This will convert the time coming from the RTC, no need to change the code

// Possible Values LANG_EN, LANG_DE, LANG_FR
#define LANG_EN


#define STEPS_PER_KM 1400
#define KM_TO_MILES 0.621371


#define ALARM_COUNT 6


// This will dramatically reduce battery life and responsiveness of the watch
// Should only be used for making screenshot and never for daily driving
#define ENABLE_SCREENSHOTS false
#define DEBUG true




#ifndef CONFIGURATION_H
#define CONFIGURATION_H
#include <Preferences.h>
#include "../lang/lang.h"

#include "../Managers/PhoneConnectionManager.h"

#if DEBUG == true
#define printDebug(a) Serial.println(a)
#else
#define printDebug(a)
#endif

class Configuration : Preferences {
    public:
        static Preferences preferences;
    public:
        static void init();
        
        static void loadAll();
        static void saveAll();
        
        static void saveSettings();
        static void loadSettings();
        
        static void saveAlarms();
        static void loadAlarms();
        
        static void saveBluetooth();
        static void loadBluetooth();
        
        static void saveHassConfig();
        static void loadHassConfig();

        static void saveOwmConfig();
        static void loadOwmConfig();
        
        static void saveNotification(uint8_t index, Notification n);
        static Notification loadNotification(uint8_t index);
        
        static void saveSteps();
        static std::array<uint32_t, 7> loadSteps(bool useCurrentData = true);
        static void saveTotalSteps();
        static uint32_t loadTotalSteps();
        
        // Settings functions - TODO: Replace with App Settings
        static String getWeatherURL() { return OPENWEATHERMAP_URL; };
        static String getWeatherLang() { return TEMP_LANG; };
        static uint8_t getWeatherUpdateInterval() { return WEATHER_UPDATE_INTERVAL; };
        
        static String getNtpServer() { return NTP_SERVER; };
        static short getTimeZone();
		static void setTimeZone(short timezone);

        static bool getInverseDarkMode() { return INVERSE_DARKMODE; };
        static uint8_t getDarkmodeStartH() { return DARKMODE_START_H; };
        static uint8_t getDarkmodeStartM() { return DARKMODE_START_M; };
        static uint8_t getDarkmodeEndH() { return DARKMODE_END_H; };
        static uint8_t getDarkmodeEndM() { return DARKMODE_END_M; };


        static int getSize();
        static int usedSpace();
        static int freeSpace();
    private:
        static bool initialized;
};
#endif