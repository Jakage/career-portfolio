/*
 * Arm.cpp
 *
 *  Created on: Oct 11, 2017
 *      Author: robin
 */

#include "Arm.h"

Arm::Arm(IOInput motor, IOInput direction, IOInput limit1, IOInput limit2) {
	this->motor = new DigitalIoPin(motor.port, motor.pin, DigitalIoPin::output);
	this->direction = new DigitalIoPin(direction.port, direction.pin,
			DigitalIoPin::output, direction.invert);
	this->switch1 = new DigitalIoPin(limit1.port, limit1.pin,
			DigitalIoPin::pullup, true);
	this->switch2 = new DigitalIoPin(limit2.port, limit2.pin,
			DigitalIoPin::pullup, true);

	this->currentStep = 0;
	this->steps = 0;
}

Arm::~Arm() {
}
