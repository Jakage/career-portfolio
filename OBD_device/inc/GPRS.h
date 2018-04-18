/*
 * GPRS.h
 *
 *  Created on: 24.1.2018
 *      Author: Jake
 */

#ifndef GPRS_H_
#define GPRS_H_

#include "chip.h"

class GPRS {
public:
	GPRS();
	virtual ~GPRS();
	int available();
	void begin(int speed = 9600);
	int read();
	int write(const char* buf, int len);
	int print(int val, int format);
	void flush();
private:
	static const int UART_RB_SIZE = 128;
	/* Transmit and receive ring buffers */
	RINGBUFF_T txring;
	RINGBUFF_T rxring;
	uint8_t rxbuff[UART_RB_SIZE];
	uint8_t txbuff[UART_RB_SIZE];
};

#endif /* GPRS_H_ */
