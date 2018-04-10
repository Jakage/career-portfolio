/*
 * Arm.h
 *
 *  Created on: Oct 11, 2017
 *      Author: robin
 */

#ifndef HARDWARE_ARM_H_
#define HARDWARE_ARM_H_

#include "../hardware/DigitalIoPin.h"
#include "../data/IOInput.h"

class Arm {
public:
	Arm(IOInput motor, IOInput direction, IOInput limit1, IOInput limit2);
	virtual ~Arm();

	int steps = 0, currentStep = 0;
	bool currentDirection = false;
	DigitalIoPin *motor, *direction, *switch1, *switch2, *state;
};

#endif /* HARDWARE_ARM_H_ */
