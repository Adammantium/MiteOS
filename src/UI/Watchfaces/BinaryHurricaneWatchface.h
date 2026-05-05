#ifndef WATCHY_BINARY_HURRICANE_H
#define WATCHY_BINARY_HURRICANE_H

#include "Watchface.h"


class BinaryHurricaneWatchface : public Watchface {

	public:
		String watchfaceName() { return "Hurricane"; }

		void draw();

	private:
		void drawTickMarks();
		void drawDigitalTime();
		void drawAnalogHands();
		
		void drawThickLine(int x0, int y0, int x1, int y1,  // NEW
                       int thickness, uint16_t color);

};

#endif