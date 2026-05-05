#include "WatchfacePage.h"
#include "../MiteOS.h"

#include "Watchfaces/Watchface.h"
#include "Watchfaces/MiteoWatchface.h"
#include "Watchfaces/BinaryHurricaneWatchface.h"
#include "Watchfaces/SEG7Watchface.h"
#include "Watchfaces/BTTFWatchface.h"
#include "Watchfaces/PokemonWatchface.h"
#include "Watchfaces/MachPaintWatchface.h"
#include "Watchfaces/AnalogWatchface.h"
#include "Watchfaces/HobbitTimeWatchface.h"
#include "Watchfaces/CalendarWatchface.h"
#include "Watchfaces/TrainWatchface.h"
#include "Watchfaces/BadForEyeWatchface.h"
#include "Watchfaces/TetrisWatchface.h"
#include "Watchfaces/StarryHorizonWatchface.h"

RTC_DATA_ATTR uint8_t watchFaceId = 0;
RTC_DATA_ATTR uint8_t actionTopLeft = GLOBAL_PAGE_SETTINGS;
RTC_DATA_ATTR uint8_t actionTopRight = GLOBAL_PAGE_NOTIFICATIONS;

PROGMEM MiteoWatchface miteoWatchface;
PROGMEM BinaryHurricaneWatchface binaryHurricaneWatchface;
PROGMEM SEG7Watchface seg7Watchface;
PROGMEM BTTFWatchface bttfWatchface;
PROGMEM PokemonWatchface pokemonWatchface;
PROGMEM MacPaintWatchface macPaintWatchface;
PROGMEM AnalogWatchface analogWatchface;
PROGMEM HobbitTimeWatchface hobbitTimeWatchface;
PROGMEM CalendarWatchface calendarWatchface;
PROGMEM TrainWatchface trainWatchface;
PROGMEM BadForEyeWatchface badForEyeWatchface;
PROGMEM TetrisWatchface tetrisWatchface;
PROGMEM StarryHorizonWatchface starryHorizonWatchface;
PROGMEM Watchface* watchfaces[] = {
	&miteoWatchface,
	&binaryHurricaneWatchface,
	&seg7Watchface,
	&bttfWatchface,
	&pokemonWatchface,
	&macPaintWatchface,
	&analogWatchface,
	&hobbitTimeWatchface,
	&calendarWatchface,
	&trainWatchface,
	&badForEyeWatchface,
	&tetrisWatchface,
	&starryHorizonWatchface,
	// DONT FORGET TO INCREATE WATCHFACE_COUNT IN HEADER FILE
};

void WatchfacePage::drawPage() {
	printDebug("Drawing Watchface");
	
	mDisplay.fillScreen(BACKGROUND_COLOR);
	mDisplay.setTextColor(FOREGROUND_COLOR);
	
	watchfaces[watchFaceId]->draw();
}

bool WatchfacePage::onButtonPressed(uint8_t buttonIndex) {
	if(buttonIndex == BTN_BACK) {
		PageManager::showPage(actionTopLeft);
		return true;
	}
	
	if(buttonIndex == BTN_UP) {
		PageManager::showPage(actionTopRight);
		return true;
	}
	
	if(buttonIndex == BTN_MENU) {
		PageManager::showPage(GLOBAL_PAGE_APPS);
		return true;
	}
	return false;
}

Watchface* WatchfacePage::getWatchface(uint8_t index) {
	if(index > WATCHFACE_COUNT) return watchfaces[0];
	return watchfaces[index];
}