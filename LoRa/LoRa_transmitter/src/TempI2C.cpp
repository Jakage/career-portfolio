/*
 * TempI2C.cpp
 *
 *  Created on: 9.4.2018
 *      Author: Jake
 */

#include "inc/TempI2C.h"

TempI2C::TempI2C(int dev) {
	device = LPC_I2C0;
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 22, IOCON_DIGMODE_EN | I2C_MODE);
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 23, IOCON_DIGMODE_EN | I2C_MODE);
	Chip_SWM_EnableFixedPin(SWM_FIXED_I2C0_SCL);
	Chip_SWM_EnableFixedPin(SWM_FIXED_I2C0_SDA);

	/* Enable I2C clock and reset I2C peripheral - the boot ROM does not
	   do this */
	Chip_I2C_Init(device);
	/* Setup clock rate for I2C */
	Chip_I2C_SetClockDiv(device, I2C_CLK_DIVIDER);
	/* Setup I2CM transfer rate */
	Chip_I2CM_SetBusSpeed(device, 100000);
	/* Enable Master Mode */
	Chip_I2CM_Enable(device);

	if(dev == 0) {
		TC74 = 0x48;
		READBIT = 0xC8;
	} else if(dev == 2) {
		TC74 = 0x4A;
		READBIT = 0xCA;
	}
}

TempI2C::~TempI2C(){
	// TODO Auto-generated destructor stub
}


bool TempI2C::I2CInOut(uint8_t devAddr, uint8_t *txBuffPtr, uint16_t txSize, uint8_t *rxBuffPtr, uint16_t rxSize) {
	// Make sure that master is idle
	while(!Chip_I2CM_IsMasterPending(device));

	/* Setup I2C transfer record */
	i2cmXferRec.slaveAddr = devAddr;
	i2cmXferRec.status = 0;
	i2cmXferRec.txSz = txSize;
	i2cmXferRec.rxSz = rxSize;
	i2cmXferRec.txBuff = txBuffPtr;
	i2cmXferRec.rxBuff = rxBuffPtr;

	Chip_I2CM_XferBlocking(device, &i2cmXferRec);

	// Test for valid operation
	if (i2cmXferRec.status == I2CM_STATUS_OK) {
		return true;
	}
	else {
		return false;
	}
}

uint8_t TempI2C::readTemp() {
	uint8_t data;
	// Write read command to TC74
	I2CInOut(TC74, &READTEMP, 1, 0, 0);
	// Write read bit to TC74 and read temperature
	I2CInOut(TC74, 0, 0, &data, 1);
	return data;
}


