/*
 * NMEA.h
 *
 *  Created on: 30.1.2018
 *      Author: Jake
 */

#ifndef NMEA_H_
#define NMEA_H_

#include "chip.h"

class NMEA {
public:
	NMEA();
	virtual ~NMEA();
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

#endif /* NMEA_H_ */
