/*
 * Pencil.cpp
 *
 *  Created on: Oct 4, 2017
 *      Author: robin
 */

#include "Pencil.h"

Pencil::Pencil(IOInput input) {
	Chip_SCT_Init(LPC_SCT0);
	Chip_SWM_MovablePortPinAssign(SWM_SCT0_OUT0_O, input.port, input.pin);

	LPC_SCT0->CONFIG |= (1 << 17); // Two 16-bit timers, auto limit
	LPC_SCT0->CTRL_L |= (72 - 1) << 5;// Set prescaler, SCTimer/PWM Clock = 1 Mhz

	LPC_SCT0->MATCHREL[0].L = MAX_PWM_VALUE - 1;// Match 0 @ 1000/1MHz = (1 kHz PWM freq)
	LPC_SCT0->MATCHREL[1].L = CLOCKWISE_PULSE;// Match 1 user for duty cycle (in 10 steps)

	LPC_SCT0->EVENT[0].STATE = 0xFFFFFFFF;// Event 0 happens in all states
	LPC_SCT0->EVENT[0].CTRL = (1 << 12);// Match 0 condition only

	LPC_SCT0->EVENT[1].STATE = 0xFFFFFFFF;// Event 1 happens in all state
	LPC_SCT0->EVENT[1].CTRL = (1 << 0) | (1 << 12);// Match 1 condition only

	LPC_SCT0->OUT[0].SET = (1 << 0);// Event 0 will set SCTx_OUT0
	LPC_SCT0->OUT[0].CLR = (1 << 1);// Event 1 will clear SCTx_OUT0

	LPC_SCT0->CTRL_L &= ~(1 << 2);// Unhalt by clearing bit 2 of CTRL req
}

Pencil::~Pencil() {
}

void Pencil::move(int angle) {
	if(angle == UP_ANGLE) {
		LPC_SCT0->MATCHREL[1].L = CLOCKWISE_PULSE;
	} else if(angle == DOWN_ANGLE) {
		LPC_SCT0->MATCHREL[1].L = CENTER_PULSE;
	} else {
		// TODO: Move it to the angle?
	}
}
