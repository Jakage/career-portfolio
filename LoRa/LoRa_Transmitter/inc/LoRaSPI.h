/*
 * LoRaSPI.h
 *
 *  Created on: 26.3.2018
 *      Author: Jake
 */

#ifndef INC_LORASPI_H_
#define INC_LORASPI_H_

#include "board.h"

class LoRaSPI {
public:
	enum transreceiver {
		transmitter,
		receiver
	};

	LoRaSPI(transreceiver mode); // TODO: config parameters (bandwidth, datarate, coderate etc.)
	~LoRaSPI();
	uint16_t SpiInOut(uint16_t data);
	void readBuffer(uint8_t addr, uint8_t *buffer, uint8_t size);
	void writeBuffer(uint8_t addr, uint8_t *buffer, uint8_t size);
	void readFifo(uint8_t *buffer, uint8_t size);
	void writeFifo(uint8_t *buffer, uint8_t size);
	uint8_t readReg(uint8_t addr);
	void writeReg(uint8_t addr, uint8_t data);
	void setChannel(int ch);

private:
	SPI_DATA_SETUP_T XferSetup;

	transreceiver mode;
	int ch = 10; // Default channel 10 (865.20 MHz)

	// Generic config
	double FREQ_STEP = 61.03515625;
	uint32_t bandwidth = 0;
	uint32_t datarate = 7;
	uint8_t coderate = 1;
	uint16_t preambleLen = 8;
	uint8_t fixLen = 0;
	uint8_t crcOn = 1;

	// Rx specific config
	uint16_t symbTimeout = 5;

	// Tx specific config
	int8_t power = 14;

	void Init_SPI_PinMux();
	void errorSPI();
	void setupSpiMaster();
	void configureModule();

	// LoRa SX1272 Registers
#define REG_FIFO				0x00
#define REG_OPMODE				0x01
#define REG_FRFMSB				0x06
#define REG_FRFMID				0x07
#define REG_FRFLSB				0x08
#define REG_PACONFIG			0x09
#define REG_FIFORXCURADDR		0x10
#define REG_IRQFLAGSMASK		0x11
#define REG_IRQFLAGS			0x12
#define REG_RXNBBYTES			0x13
#define REG_FIFOADDRPTR			0x0D
#define REG_FIFOTXBASEADDRS		0x0E
#define REG_FIFORXBASEADDRS		0x0F
#define REG_MODEMCONFIG1		0x1D
#define REG_MODEMCONFIG2		0x1E
#define REG_SYMBTIMEOUTLSB		0x1F
#define REG_PREAMBLEMSB			0x20
#define REG_PREAMBLELSB			0x21
#define REG_PAYLOADLENGTH		0x22
#define REG_FIFORXBYTEADDR		0x25
#define REG_DETECTOPTIMIZE		0x31
#define REG_NODE_ADRS			0x33
#define REG_INVERTIQ			0x33
#define REG_BROADCAST_ADRS		0x34
#define REG_DETECTIONTHRESHOLD	0x37
#define REG_INVERTIQ2			0x3B
#define REG_PADAC 				0x5A

	// Operating modes
#define OPMODE_SLEEP			0x00
#define OPMODE_STNDBY			0x01
#define OPMODE_TX				0x03
#define OPMODE_RXCONT			0x05
#define OPMODE_MASK				0xF8
#define OPMODE_LORA_MASK		0x7F
#define LORA					0x80

	// Transreceiver configure registers and register masks
#define RFLR_INVERTIQ_RX_OFF                        0x00
#define RFLR_IRQFLAGS_CADDETECTED                   0x01
#define RFLR_INVERTIQ_TX_OFF                        0x01
#define RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL            0x02
#define RFLR_DETECTIONOPTIMIZE_SF7_TO_SF12          0x03
#define RFLR_IRQFLAGS_CADDONE                       0x04
#define RF_PADAC_20DBM_OFF							0x04
#define RF_PADAC_20DBM_ON							0x07
#define RFLR_IRQFLAGS_TXDONE                        0x08
#define RFLR_DETECTIONTHRESH_SF7_TO_SF12            0x0A
#define RFLR_MODEMCONFIG2_SF_MASK                   0x0F
#define RFLR_IRQFLAGS_VALIDHEADER                   0x10
#define RFLR_INVERTIQ2_OFF                          0x1D
#define RFLR_IRQFLAGS_PAYLOADCRCERROR               0x20
#define RFLR_MODEMCONFIG1_BW_MASK                   0x3F
#define RFLR_IRQFLAGS_RXDONE                        0x40
#define RF_PACONFIG_PASELECT_MASK					0x7F
#define RFLR_IRQFLAGS_RXTIMEOUT                     0x80
#define RF_PACONFIG_PASELECT_PABOOST				0x80
#define RFLR_INVERTIQ_RX_MASK                       0xBF
#define RFLR_MODEMCONFIG1_CODINGRATE_MASK           0xC7
#define RFLR_PACONFIG_OUTPUTPOWER_MASK				0xF0
#define RF_PADAC_20DBM_MASK							0xF8
#define RFLR_DETECTIONOPTIMIZE_MASK                 0xF8
#define RFLR_MODEMCONFIG1_IMPLICITHEADER_MASK       0xFB
#define RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK       0xFC
#define RFLR_MODEMCONFIG1_RXPAYLOADCRC_MASK         0xFD
#define RFLR_MODEMCONFIG1_LOWDATARATEOPTIMIZE_MASK  0xFE
#define RFLR_INVERTIQ_TX_MASK                       0xFE
};

#endif /* INC_LORASPI_H_ */


