/*
 * LoRaSPI.cpp
 *
 *  Created on: 26.3.2018
 *      Author: Jake
 */

#include "LoRaSPI.h"
//#include "chip.h"

LoRaSPI::LoRaSPI(transreceiver mode) {
	this->mode = mode;
	Init_SPI_PinMux();
	setupSpiMaster();
	configureModule();
}

LoRaSPI::~LoRaSPI() {
	// TODO Auto-generated destructor stub
}

/* Initializes pin muxing for SPI interface - note that SystemInit() may
   already setup your pin muxing at system startup */
void LoRaSPI::Init_SPI_PinMux() {
#if (defined(BOARD_NXP_LPCXPRESSO_1549))

	/* Enable the clock to the Switch Matrix */
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_SWM);
	/*
	 * Initialize SPI0 pins connect
	 * SCK0: PINASSIGN3[15:8]: Select P0.0
	 * MOSI0: PINASSIGN3[23:16]: Select P0.16
	 * MISO0: PINASSIGN3[31:24] : Select P0.10
	 * SSEL0: PINASSIGN4[7:0]: Select P0.9
	 */
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 0, (IOCON_MODE_INACT | IOCON_DIGMODE_EN));
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 16, (IOCON_MODE_INACT | IOCON_DIGMODE_EN));
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 10, (IOCON_MODE_INACT | IOCON_DIGMODE_EN));
	//Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 9, (IOCON_MODE_INACT | IOCON_DIGMODE_EN));

	Chip_SWM_MovablePinAssign(SWM_SPI0_SCK_IO, 0);	/* P0.0 */
	Chip_SWM_MovablePinAssign(SWM_SPI0_MOSI_IO, 16);/* P0.16 */
	Chip_SWM_MovablePinAssign(SWM_SPI0_MISO_IO, 10);/* P0.10 */
	//Chip_SWM_MovablePinAssign(SWM_SPI0_SSELSN_0_IO, 9);	/* P0.9*/

	// Set slave select as GPIO for manual toggle
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 9);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 9, true);

	/* Disable the clock to the Switch Matrix to save power */
	//Chip_Clock_DisablePeriphClock(SYSCTL_CLOCK_SWM);
#else
	/* Configure your own SPI pin muxing here if needed */
#warning "No SPI pin muxing defined"
#endif
}

/* Setup SPI handle and parameters */
void LoRaSPI::setupSpiMaster() {
	SPI_CFG_T spiCfg;
	SPI_DELAY_CONFIG_T spiDelayCfg;
	/* Initialize SPI Block */
	Chip_SPI_Init(LPC_SPI0);
	/* Set SPI Config register */
	spiCfg.ClkDiv = 0xFFFF;	/* Set Clock divider to maximum */
	spiCfg.Mode = SPI_MODE_MASTER;	/* Enable Master Mode */
	spiCfg.ClockMode = SPI_CLOCK_MODE0;	/* Enable Mode 0 */
	spiCfg.DataOrder = SPI_DATA_MSB_FIRST;	/* Transmit MSB first */
	/* Slave select polarity is active low */
	spiCfg.SSELPol = (SPI_CFG_SPOL0_LO | SPI_CFG_SPOL1_LO | SPI_CFG_SPOL2_LO | SPI_CFG_SPOL3_LO);
	Chip_SPI_SetConfig(LPC_SPI0, &spiCfg);
	/* Set Delay register */
	spiDelayCfg.PreDelay = 2;
	spiDelayCfg.PostDelay = 2;
	spiDelayCfg.FrameDelay = 2;
	spiDelayCfg.TransferDelay = 2;
	Chip_SPI_DelayConfig(LPC_SPI0, &spiDelayCfg);
	/* Enable Loopback mode for this example */
	//Chip_SPI_EnableLoopBack(LPC_SPI0);
	/* Enable SPI0 */
	Chip_SPI_Enable(LPC_SPI0);
}

void LoRaSPI::setChannel(int ch) {
	uint32_t freq;
	switch(ch) {
	case 10:
		freq = 865200000;
		break;
	case 11:
		freq = 865500000;
		break;
	case 12:
		freq = 865800000;
		break;
	case 13:
		freq = 866100000;
		break;
	case 14:
		freq = 866400000;
		break;
	case 15:
		freq = 866700000;
		break;
	case 16:
		freq = 867000000;
		break;
	case 17:
		freq = 868000000;
		break;
	default:
		freq = 868000000;
		break;
	}
	freq = (uint32_t)((double)freq / FREQ_STEP);
	writeReg(REG_FRFMSB, (uint8_t)((freq >> 16) & 0xFF));
	writeReg(REG_FRFMID, (uint8_t)((freq >> 8) & 0xFF));
	writeReg(REG_FRFLSB, (uint8_t)(freq & 0xFF));
}

void LoRaSPI::configureModule() {
	// Set channel
	setChannel(ch);

	// Set device to sleep mode
	writeReg(REG_OPMODE, OPMODE_SLEEP);

	// Set module modulation mode as LoRa modulation
	writeReg(REG_OPMODE, LORA);

	// Set device back to standby mode
	writeReg(REG_OPMODE, OPMODE_STNDBY);

	if(mode == receiver) {
		writeReg(REG_MODEMCONFIG1,
				(readReg(REG_MODEMCONFIG1) & RFLR_MODEMCONFIG1_BW_MASK & RFLR_MODEMCONFIG1_CODINGRATE_MASK &
						RFLR_MODEMCONFIG1_IMPLICITHEADER_MASK & RFLR_MODEMCONFIG1_RXPAYLOADCRC_MASK &
						RFLR_MODEMCONFIG1_LOWDATARATEOPTIMIZE_MASK) | (bandwidth << 6) | (coderate << 3) |
						(fixLen << 2) | (crcOn << 1) | 0x00);

		writeReg(REG_MODEMCONFIG2,
				(readReg(REG_MODEMCONFIG2) & RFLR_MODEMCONFIG2_SF_MASK & RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK) |
				(datarate << 4) | ((symbTimeout >> 8) & ~RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK));

		writeReg(REG_SYMBTIMEOUTLSB, (uint8_t)(symbTimeout & 0xFF));
		writeReg(REG_PREAMBLEMSB, (uint8_t)((preambleLen >> 8) & 0xFF));
		writeReg(REG_PREAMBLELSB, (uint8_t)(preambleLen & 0xFF));

		writeReg(REG_DETECTOPTIMIZE,
				(readReg(REG_DETECTOPTIMIZE) & RFLR_DETECTIONOPTIMIZE_MASK) | RFLR_DETECTIONOPTIMIZE_SF7_TO_SF12);

		writeReg(REG_DETECTIONTHRESHOLD, RFLR_DETECTIONTHRESH_SF7_TO_SF12);
	}
	else if (mode == transmitter) {
		// Set power configuration
		uint8_t paConfig = readReg(REG_PACONFIG);
		uint8_t paDac = readReg(REG_PADAC);
		paConfig = (paConfig & RF_PACONFIG_PASELECT_MASK) | RF_PACONFIG_PASELECT_PABOOST;
		if((paConfig & RF_PACONFIG_PASELECT_PABOOST) == RF_PACONFIG_PASELECT_PABOOST) {
			if(power > 17) {
				paDac = (paDac & RF_PADAC_20DBM_MASK) | RF_PADAC_20DBM_ON;
			}
			else {
				paDac = (paDac & RF_PADAC_20DBM_MASK) | RF_PADAC_20DBM_OFF;
			}
			if((paDac & RF_PADAC_20DBM_ON) == RF_PADAC_20DBM_ON) {
				if(power < 5) {
					power = 5;
				}
				if(power > 20) {
					power = 20;
				}
				paConfig = (paConfig & RFLR_PACONFIG_OUTPUTPOWER_MASK) | (uint8_t)((uint16_t)(power - 5) & 0x0F);
			}
			else {
				if(power < 2) {
					power = 2;
				}
				if(power > 17) {
					power = 17;
				}
				paConfig = (paConfig & RFLR_PACONFIG_OUTPUTPOWER_MASK) | (uint8_t)((uint16_t)(power - 2) & 0x0F);
			}
		}
		else {
			if(power < -1) {
				power = -1;
			}
			if(power > 14) {
				power = 14;
			}
			paConfig = (paConfig & RFLR_PACONFIG_OUTPUTPOWER_MASK) | (uint8_t)((uint16_t)(power + 1) & 0x0F);
		}
		writeReg(REG_PACONFIG, paConfig);
		writeReg(REG_PADAC, paDac);

		// LoRa settings config
		writeReg(REG_MODEMCONFIG1,
				(readReg(REG_MODEMCONFIG1) & RFLR_MODEMCONFIG1_BW_MASK & RFLR_MODEMCONFIG1_CODINGRATE_MASK &
						RFLR_MODEMCONFIG1_IMPLICITHEADER_MASK & RFLR_MODEMCONFIG1_RXPAYLOADCRC_MASK &
						RFLR_MODEMCONFIG1_LOWDATARATEOPTIMIZE_MASK) | (bandwidth << 6) | (coderate << 3) |
						(fixLen << 2) | (crcOn << 1) | 0x00);

		writeReg(REG_MODEMCONFIG2, (readReg(REG_MODEMCONFIG2) & RFLR_MODEMCONFIG2_SF_MASK) | (datarate << 4));

		writeReg(REG_PREAMBLEMSB, (preambleLen >> 8) & 0x00FF);
		writeReg(REG_PREAMBLELSB, preambleLen & 0xFF);

		writeReg(REG_DETECTOPTIMIZE,
				(readReg(REG_DETECTOPTIMIZE) & RFLR_DETECTIONOPTIMIZE_MASK) | RFLR_DETECTIONOPTIMIZE_SF7_TO_SF12);

		writeReg(REG_DETECTIONTHRESHOLD, RFLR_DETECTIONTHRESH_SF7_TO_SF12);
	}
}

// Turn on LED to indicate an error
void LoRaSPI::errorSPI() {
	Board_LED_Set(0, true);
	while (1) {}
}

// Master SPI transmit in polling mode
uint16_t LoRaSPI::SpiInOut(uint16_t data) {
	uint16_t rxData;

	XferSetup.pTx = &data;	/* Transmit Buffer */
	XferSetup.pRx = &rxData; /* Receive Buffer */
	XferSetup.DataSize = 8;	/* Data size in bits */
	XferSetup.Length = 1;	/* Total frame length */
	/* Deassert all slave select pins for manual toggle */
	XferSetup.ssel = SPI_TXCTL_DEASSERT_SSEL0 | SPI_TXCTL_DEASSERT_SSEL1 | SPI_TXCTL_DEASSERT_SSEL2 |
			SPI_TXCTL_DEASSERT_SSEL3;
	XferSetup.TxCnt = 0; /* Transmit Counter */
	XferSetup.RxCnt = 0;
	/* Transfer message as SPI master via polling */
	if (Chip_SPI_RWFrames_Blocking(LPC_SPI0, &XferSetup) > 0);
	else {
		/* Signal SPI error */
		errorSPI();
	}
	return rxData;
}

void LoRaSPI::readBuffer(uint8_t addr, uint8_t *buffer, uint8_t size) {
	//NSS = 0;
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 9, false);
	SpiInOut(addr & 0x7F);
	for(uint8_t i = 0; i < size; i++) {
		buffer[i] = SpiInOut(0);
	}
	//NSS = 1;
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 9, true);
}

void LoRaSPI::writeBuffer(uint8_t addr, uint8_t *buffer, uint8_t size) {
	//NSS = 0;
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 9, false);
	SpiInOut(addr | 0x80);
	for(uint8_t i = 0; i < size; i++) {
		SpiInOut(buffer[i]);
	}
	//NSS = 1;
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 9, true);
}

void LoRaSPI::readFifo(uint8_t *buffer, uint8_t size) {
	readBuffer(0, buffer, size);
}

void LoRaSPI::writeFifo(uint8_t *buffer, uint8_t size) {
	writeBuffer(0, buffer, size);
}

uint8_t LoRaSPI::readReg(uint8_t addr) {
	uint8_t data;
	readBuffer(addr, &data, 1);
	return data;
}

void LoRaSPI::writeReg(uint8_t addr, uint8_t data) {
	writeBuffer(addr, &data, 1);
}


