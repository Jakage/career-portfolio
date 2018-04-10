/*
 * Laser.cpp
 *
 *  Created on: Oct 5, 2017
 *      Author: robin
 */

#include "Laser.h"
#include "board.h"

Laser::Laser(IOInput input) {
	Chip_SCT_Init(LPC_SCT1);
	Chip_SWM_MovablePortPinAssign(SWM_SCT1_OUT1_O, input.port, input.pin);

	LPC_SCT1->CONFIG |= (1 << 17); // Two 16-bit timers, auto limit
	LPC_SCT1->CTRL_L |= (72 - 1) << 5;// Set prescaler, SCTimer/PWM Clock = 1 Mhz

	LPC_SCT1->MATCHREL[0].L = this->FULL_POWER - 1;// Match 0 @ 1000/1MHz = (1 kHz PWM freq)
	LPC_SCT1->MATCHREL[1].L = 0;// Match 1 user for duty cycle (in 10 steps)

	LPC_SCT1->EVENT[0].STATE = 0xFFFFFFFF;// Event 0 happens in all states
	LPC_SCT1->EVENT[0].CTRL = (1 << 12);// Match 0 condition only

	LPC_SCT1->EVENT[1].STATE = 0xFFFFFFFF;// Event 1 happens in all state
	LPC_SCT1->EVENT[1].CTRL = (1 << 0) | (1 << 12);// Match 1 condition only

	LPC_SCT1->OUT[1].SET = (1 << 0);// Event 0 will set SCTx_OUT0
	LPC_SCT1->OUT[1].CLR = (1 << 1);// Event 1 will clear SCTx_OUT0

	LPC_SCT1->CTRL_L &= ~(1 << 2);// Unhalt by clearing bit 2 of CTRL req
}

void Laser::setPower(int power) {
	double percentage = (double) power / (double) this->MDRAW_POWER_MAX;
	int laserPower = (int) this->FULL_POWER * percentage;
	LPC_SCT1->MATCHREL[1].L = laserPower;
}

Laser::~Laser() {

}
