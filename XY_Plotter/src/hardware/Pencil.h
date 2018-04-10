/*
 * Pencil.h
 *
 *  Created on: Oct 4, 2017
 *      Author: robin
 */

#ifndef HARDWARE_PENCIL_H_
#define HARDWARE_PENCIL_H_

#include "FreeRTOS.h"
#include "chip.h"
#include "../data/IOInput.h"

class Pencil {
public:
	Pencil(IOInput input);
	virtual ~Pencil();

	void move(int angle);


private:
	// TODO: In the config?
	const int UP_ANGLE = 160;
	const int DOWN_ANGLE = 90;

	const int MAX_PWM_VALUE = 20000;
	const int CLOCKWISE_PULSE = 1100;
	const int CENTER_PULSE = 1600;
	const int COUNTER_CLOCKWISE_PULSE = 2000 - 20;
};

#endif /* HARDWARE_PENCIL_H_ */
