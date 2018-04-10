/*
 * IOInput.h
 *
 *  Created on: Oct 3, 2017
 *      Author: robin
 */

#ifndef DATA_RITMESSAGE_H_
#define DATA_RITMESSAGE_H_

#include "FreeRTOS.h"
#include "semphr.h"

struct RITMessage {
	double xSteps = 0, ySteps = 0;
	double xNewPosition = 0, yNewPosition = 0;
	bool xSwitch = false, ySwitch = false;
};

#endif /* DATA_IOINPUT_H_ */
