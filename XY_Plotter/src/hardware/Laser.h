/*
 * Laser.h
 *
 *  Created on: Oct 5, 2017
 *      Author: robin
 */

#ifndef HARDWARE_LASER_H_
#define HARDWARE_LASER_H_

#include "FreeRTOS.h"
#include "chip.h"
#include "../data/IOInput.h"

class Laser {
public:
	Laser(IOInput input);
	virtual ~Laser();

	void toggle();

	void setPower(int power);

private:
	const int FULL_POWER = 1000;
	const int MDRAW_POWER_MAX = 255;
};

#endif /* HARDWARE_LASER_H_ */
