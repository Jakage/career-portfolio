/*
===============================================================================
 Name        : main.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
 */

#if defined (__USE_LPCOPEN)
#if defined(NO_BOARD_LIB)
#include "chip.h"
#else
#include "board.h"
#endif
#endif

#include <cr_section_macros.h>
#include <stdint.h>
#include "inttypes.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "LoRaSPI.h"
#include "RaspiUart.h"

// Global semaphore and queue
SemaphoreHandle_t xBin;
QueueHandle_t xQueue;

// Generic Initialization
static void prvSetupHardware(void) {
	SystemCoreClockUpdate();
	Board_Init();
	Board_LED_Set(0, false);
}

extern "C" {
void vConfigureTimerForRunTimeStats(void) {
	Chip_SCT_Init(LPC_SCTSMALL1);
	LPC_SCTSMALL1->CONFIG = SCT_CONFIG_32BIT_COUNTER;
	LPC_SCTSMALL1->CTRL_U = SCT_CTRL_PRE_L(255) | SCT_CTRL_CLRCTR_L;
}
}

static void vReceiverTask(void *pvParameters) {
	LoRaSPI LoRa(LoRaSPI::receiver);
	int ch = 1;

	// LoRa Rx pin
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 24, IOCON_MODE_INACT | IOCON_DIGMODE_EN);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 24);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 24, true);

	// Rx variables
	bool error = false;
	uint8_t buffer[64];

	// Set LoRa Rx mode
	LoRa.writeReg(REG_INVERTIQ, ((LoRa.readReg(REG_INVERTIQ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK) |
			RFLR_INVERTIQ_RX_OFF | RFLR_INVERTIQ_TX_OFF));
	LoRa.writeReg(REG_INVERTIQ2, RFLR_INVERTIQ2_OFF);

	// Set interrupt flags
	LoRa.writeReg(REG_IRQFLAGSMASK,
			//RFLR_IRQFLAGS_RXTIMEOUT |
			//RFLR_IRQFLAGS_RXDONE |
			//RFLR_IRQFLAGS_PAYLOADCRCERROR |
			RFLR_IRQFLAGS_VALIDHEADER | RFLR_IRQFLAGS_TXDONE | RFLR_IRQFLAGS_CADDONE |
			RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL | RFLR_IRQFLAGS_CADDETECTED );

	LoRa.writeReg(REG_OPMODE, OPMODE_RXCONT);

	while(1) {
		int rxTimeout = 0;
		Chip_GPIO_SetPinState(LPC_GPIO, 0, 24, true);

		// Wait 3 seconds for RxDone interrupt flag. Break if not found.
		while(LoRa.readReg(REG_IRQFLAGS) != 0x40) {
			// Check CRC interrupt flag
			if(LoRa.readReg(REG_IRQFLAGS) == 0x60) {
				LoRa.writeReg(REG_IRQFLAGS, 0x20);
				error = true;
			}
			if(rxTimeout == 300) {
				error = true;
				break;
			} else {
				rxTimeout++;
			}
			vTaskDelay(configTICK_RATE_HZ / 100);
			//printf("REG_IRQFLAGS = %" PRIxFAST8 "\r\n", LoRa.readReg(REG_IRQFLAGS)); // Debug test
		}

		Chip_GPIO_SetPinState(LPC_GPIO, 0, 24, false);

		// Clear RxDone interrupt by writing 1 to it and make sure it gets cleared
		while(LoRa.readReg(REG_IRQFLAGS) != 0x00) {
			LoRa.writeReg(REG_IRQFLAGS, 0x40);
		}

		// If CRC flag happened don't read FIFO
		if(error) {
			//printf("CRC ERROR\n\r"); // For debug purposes
			error = false;
		} else {
			uint8_t fifoRxByteAddr = LoRa.readReg(REG_FIFORXBYTEADDR);
			uint8_t rxNbBytes = LoRa.readReg(REG_RXNBBYTES);
			//LoRa.writeReg(REG_FIFORXBASEADDRS, 0);
			LoRa.writeReg(REG_FIFOADDRPTR, (fifoRxByteAddr - rxNbBytes));

			// Read FIFO
			LoRa.readFifo(buffer, rxNbBytes);

			// Debug for-loop for checking FIFO
			for(int i = 0; i < 64; i++) {
				if((char)buffer[i] == '%') {
					i = 64;
					printf("\r\n");
				} else {
					printf("Buffer: %" PRIdFAST8 "\r\n", buffer[i]);
				}
			}

			// Send buffer data to queue for raspi uart
			xQueueSendToBack(xQueue, &buffer, 10);
		}

		// Change channel TODO: Scroll thru all channels (now only 3)
		/*LoRa.writeReg(REG_OPMODE, OPMODE_STNDBY);
		if(ch == 10) { ch = 14; }
		else if(ch == 14) { ch = 17; }
		else { ch = 10; }
		LoRa.setChannel(ch);
		LoRa.writeReg(REG_OPMODE, OPMODE_RXCONT);*/

		// Toggle LED to show activity.
		Board_LED_Toggle(2);

		// Task delay
		vTaskDelay(configTICK_RATE_HZ / 100);
	}
}

static void vRaspiTask(void *pvParameters) {
	RaspiUart uart;
	uint8_t queueBuffer[64];
	//char raspibuff[] = " 000 C Sensor # \r\n";
	char raspibuff[] = "Sensor# /Temp 000C/Help= /Bed= \r\n";
	//unsigned int transnum;
	unsigned int temp;
	//unsigned int help;
	//unsigned int bed;
	while(1) {
		xQueueReceive(xQueue, &queueBuffer, portMAX_DELAY);
		//transnum = queueBuffer[0];
		raspibuff[7] = (unsigned int)queueBuffer[0];
		temp = queueBuffer[1];
		raspibuff[24] = queueBuffer[2];
		raspibuff[30] = queueBuffer[3];

		// Check if temp is below zero
		if(temp > 127) {
			raspibuff[13] = '-';
			temp = temp - 127;
		} else {
			raspibuff[13] = '+';
		}
		// Calculate temperature
		raspibuff[14] = temp/100 + 48;
		raspibuff[15] = (temp/10)%10 + 48;
		raspibuff[16] = temp%10 + 48;
		// eliminate 0s at beginning
		if (raspibuff[14] == '0') {
			raspibuff[14] = ' ';
			if (raspibuff[15] == '0') raspibuff[15] = ' ';
		}
		uart.write(raspibuff, 33);
	}
}

int main(void) {
	prvSetupHardware();
	xBin = xSemaphoreCreateBinary();
	xQueue = xQueueCreate(5, 64);

	xTaskCreate(vReceiverTask, "vReceiverTask",
			configMINIMAL_STACK_SIZE + 512, NULL, (tskIDLE_PRIORITY + 1UL), (TaskHandle_t *) NULL);

	xTaskCreate(vRaspiTask, "vRaspiTask",
			configMINIMAL_STACK_SIZE + 512, NULL, (tskIDLE_PRIORITY + 1UL), (TaskHandle_t *) NULL);

	vTaskStartScheduler();

	return 0;
}
