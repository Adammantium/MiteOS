#include "BinaryHurricaneWatchface.h"
#include "../../Fonts/Staubach25pt7b.h"
#include "BinaryHurricane/BinaryHurricaneResources.h"

#define CX     (DISPLAY_WIDTH / 2)
#define CY     (DISPLAY_HEIGHT / 2)

void BinaryHurricaneWatchface::draw() {
    // Paint the entire screen black first — this is what makes the
    // face white-on-black regardless of the project's global macros.
    mDisplay.fillScreen(BACKGROUND_COLOR);

	mDisplay.drawBitmap(15, 15, watch_face_background, 170, 170, FOREGROUND_COLOR);

    drawTickMarks();
    drawDigitalTime();
    drawAnalogHands();
}

// =====================================================================
// TICK MARKS
// =====================================================================
void BinaryHurricaneWatchface::drawTickMarks() {
    const int outerR      = 95;
    const int innerMajorR = 91;
    const int innerMinorR = 97;

    for (int m = 0; m < 60; m++) {
        float ang  = (m * 6.0f - 90.0f) * DEG_TO_RAD;
        float cosA = cos(ang);
        float sinA = sin(ang);

        bool isMajor = (m % 5 == 0);
        int innerR = isMajor ? innerMajorR : innerMinorR;

        int x1 = CX + (int)(cosA * innerR);
        int y1 = CY + (int)(sinA * innerR);
        int x2 = CX + (int)(cosA * outerR);
        int y2 = CY + (int)(sinA * outerR);

        if (isMajor) {
            mDisplay.drawLine(x1, y1, x2, y2, FOREGROUND_COLOR);
            mDisplay.drawLine(x1 - 1, y1, x2 - 1, y2, FOREGROUND_COLOR);
            mDisplay.drawLine(x1 + 1, y1, x2 + 1, y2, FOREGROUND_COLOR);
            mDisplay.drawLine(x1, y1 - 1, x2, y2 - 1, FOREGROUND_COLOR);
			mDisplay.drawLine(x1, y1 + 1, x2, y2 + 1, FOREGROUND_COLOR);
        } else {
            mDisplay.fillRect(x2, y2, 2, 2, FOREGROUND_COLOR);
        }
    }
}

// =====================================================================
// VORTEX — discrete lightning spokes (no fillTriangle blob).
// =====================================================================

void BinaryHurricaneWatchface::drawThickLine(int x0, int y0, int x1, int y1,
                                              int thickness, uint16_t color) {
    float dx = x1 - x0, dy = y1 - y0;
    float len = sqrt(dx*dx + dy*dy);
    if (len < 0.5f) return;
    float nx = -dy / len, ny = dx / len;
    int half = thickness / 2;
    for (int i = -half; i <= half; i++) {
        mDisplay.drawLine(x0 + (int)(nx*i), y0 + (int)(ny*i),
                          x1 + (int)(nx*i), y1 + (int)(ny*i), color);
    }
}

// =====================================================================
// DIGITAL TIME
// =====================================================================
void BinaryHurricaneWatchface::drawDigitalTime() {
    int hh = MiteOS::currentTime.Hour;
    int mm = MiteOS::currentTime.Minute;

    char hourStr[3], minStr[3];
    snprintf(hourStr, sizeof(hourStr), "%02d", hh);
    snprintf(minStr,  sizeof(minStr),  "%02d", mm);

    mDisplay.setFont(&Staubach25pt7b);

    int16_t x1, y1; uint16_t w, h;

    mDisplay.getTextBounds(hourStr, 0, 0, &x1, &y1, &w, &h);
    int hX = CX - 26 - w;
    int hY = CY + h / 2 - 4;
	
	mDisplay.setTextColor(BACKGROUND_COLOR);
	for(int8_t x = -1; x <= 1; x++) {
		for(int8_t y = -1; y <= 1; y++) {
    		mDisplay.setCursor(hX + (x * 4), hY + (y * 4));
   			mDisplay.print(hourStr);
		}
	}
	mDisplay.setTextColor(FOREGROUND_COLOR);
    mDisplay.setCursor(hX, hY);
    mDisplay.print(hourStr);

    mDisplay.getTextBounds(minStr, 0, 0, &x1, &y1, &w, &h);
	
	mDisplay.setTextColor(BACKGROUND_COLOR);
	for(int8_t x = -1; x <= 1; x++) {
		for(int8_t y = -1; y <= 1; y++) {
   			mDisplay.setCursor(CX + 20 + (x * 4), hY + (y * 4));
   			mDisplay.print(minStr);
		}
	}
	mDisplay.setTextColor(FOREGROUND_COLOR);
    mDisplay.setCursor(CX + 20, hY);
    mDisplay.print(minStr);
}

// =====================================================================
// ANALOG HANDS
// =====================================================================
void BinaryHurricaneWatchface::drawAnalogHands() {
    int hh = MiteOS::currentTime.Hour % 12;
    int mm = MiteOS::currentTime.Minute;

    float minAngle  = (mm * 6.0f - 90.0f) * DEG_TO_RAD;
    float hourAngle = ((hh * 30.0f) + (mm * 0.5f) - 90.0f) * DEG_TO_RAD;

    int mx = CX + (int)(cos(minAngle)  * 82);
    int my = CY + (int)(sin(minAngle)  * 82);
    int mex = CX + (int)(cos(minAngle)  * 83);
    int mey = CY + (int)(sin(minAngle)  * 83);
    int hx = CX + (int)(cos(hourAngle) * 55);
    int hy = CY + (int)(sin(hourAngle) * 55);
    int hex = CX + (int)(cos(hourAngle) * 56);
    int hey = CY + (int)(sin(hourAngle) * 56);

    drawThickLine(CX, CY, mex, mey, 9, BACKGROUND_COLOR);
    drawThickLine(CX, CY, hex, hey, 10, BACKGROUND_COLOR);
    drawThickLine(CX, CY, mx, my, 5, FOREGROUND_COLOR);
    drawThickLine(CX, CY, hx, hy, 6, FOREGROUND_COLOR);

    // Pivot: white disc with black hole
    mDisplay.fillCircle(CX, CY, 5, FOREGROUND_COLOR);
    mDisplay.fillCircle(CX, CY, 2, BACKGROUND_COLOR);
}