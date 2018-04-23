/*
 * TempI2C.h
 *
 *  Created on: 9.4.2018
 *      Author: Jake
 */

#ifndef INC_TEMPI2C_H_
#define INC_TEMPI2C_H_

#include "board.h"

class TempI2C {
public:
	TempI2C(int dev);
	virtual ~TempI2C();
	bool I2CInOut(uint8_t devAddr, uint8_t *txBuffPtr, uint16_t txSize, uint8_t *rxBuffPtr, uint16_t rxSize);
	uint8_t readTemp();
private:
	LPC_I2C_T *device;
	I2CM_XFER_T  i2cmXferRec;
	static const unsigned int I2C_CLK_DIVIDER = 40;
	static const unsigned int I2C_MODE = 0;

	uint8_t TC74; // 0x48 for TC74A0 and 0x4A for TC74A2
	uint8_t READBIT; // 0xC8 for TC74A0 and 0xCA for TC74A2
	uint8_t READTEMP = 0x00; // Read register command
};

#endif /* INC_TEMPI2C_H_ */
