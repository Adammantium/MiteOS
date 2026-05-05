#ifndef WATCHFACEPAGE_H
#define WATCHFACEPAGE_H
#include "Page.h"
#include "Watchfaces/Watchface.h"

#define WATCHFACE_COUNT 13

class WatchfacePage : public Page {
	public:
		String pageName() override { return TXT_WATCHFACE; };
		WatchfacePage() {};
		void drawPage() override;
		bool onButtonPressed(uint8_t buttonIndex) override;
		Watchface* getWatchface(uint8_t index);
};

extern RTC_DATA_ATTR uint8_t watchFaceId;
extern RTC_DATA_ATTR uint8_t actionTopLeft;
extern RTC_DATA_ATTR uint8_t actionTopRight;
#endif