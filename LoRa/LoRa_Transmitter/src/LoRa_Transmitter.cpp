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
#include "TempI2C.h"

// Global semaphore for printf() functions
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

static void vTransmitterTask(void *pvParameters) {
	LoRaSPI LoRa(LoRaSPI::transmitter);
	LoRa.setChannel(14); // Set transmitter's channel. Channels 10-17

	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 24, IOCON_MODE_INACT | IOCON_DIGMODE_EN);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, 0, 24);
	Chip_GPIO_SetPinState(LPC_GPIO, 0, 24, true);

	int count = 0;
	uint8_t buffer_size = 64;
	uint8_t buffer[buffer_size];

	LoRa.writeReg(REG_INVERTIQ, ((LoRa.readReg(REG_INVERTIQ) & RFLR_INVERTIQ_TX_MASK & RFLR_INVERTIQ_RX_MASK) |
			RFLR_INVERTIQ_RX_OFF | RFLR_INVERTIQ_TX_OFF));
	LoRa.writeReg(REG_INVERTIQ2, RFLR_INVERTIQ2_OFF);

	LoRa.writeReg(REG_IRQFLAGSMASK,
			RFLR_IRQFLAGS_RXTIMEOUT | RFLR_IRQFLAGS_RXDONE |
			RFLR_IRQFLAGS_PAYLOADCRCERROR | RFLR_IRQFLAGS_VALIDHEADER |
			//RFLR_IRQFLAGS_TXDONE |
			RFLR_IRQFLAGS_CADDONE |
			RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL | RFLR_IRQFLAGS_CADDETECTED);

	// Initializes the payload size
	LoRa.writeReg(REG_PAYLOADLENGTH, buffer_size);

	for(int i = 1; i < buffer_size; i++) {
		buffer[i] = '%';
	}

	while (1) {
		xQueueReceive(xQueue, &buffer[0], portMAX_DELAY);
		buffer[1] = '2';
		//buffer[2] = '\n';

		//printf("Buffer0: %" PRIdFAST8 "\r\n", buffer[0]);

		Chip_GPIO_SetPinState(LPC_GPIO, 0, 24, true);

		// Full buffer used for Tx
		LoRa.writeReg(REG_FIFOTXBASEADDRS, 0);
		LoRa.writeReg(REG_FIFOADDRPTR, 0);

		// Write payload buffer
		LoRa.writeFifo(buffer, buffer_size);

		LoRa.writeReg(REG_OPMODE, OPMODE_TX);

		// Wait for TxDone interrupt
		while(LoRa.readReg(REG_IRQFLAGS) != 0x08) {}

		// Clear TxDone interrupt by writing 1 to it and make sure it gets cleared
		while(LoRa.readReg(REG_IRQFLAGS) != 0x00) {
			LoRa.writeReg(REG_IRQFLAGS, 0x08);
		}

		Chip_GPIO_SetPinState(LPC_GPIO, 0, 24, false);

		if (count >= 9) {
			count = 0;
		} else {
			count++;
		}

		// Toggle LED to show activity
		Board_LED_Toggle(1);

		// Task delay
		//vTaskDelay(configTICK_RATE_HZ / 100);
	}
}

static void vSensorTask(void *pvParameters) {
	TempI2C thermal(2); // 0 for TC74A0 and 2 for TC74A2
	uint8_t temp;
	while(1) {
		temp = thermal.readTemp();
		//printf("TEMP SENSOR: %" PRIdFAST8 "\r\n", temp);
		xQueueSendToBack(xQueue, &temp, 10);
		// Task delay
		vTaskDelay(configTICK_RATE_HZ);
	}
}

int main(void) {
	prvSetupHardware();
	xBin = xSemaphoreCreateBinary();
	xQueue = xQueueCreate(5, 1);

	xTaskCreate(vTransmitterTask, "vTransmitterTask",
			configMINIMAL_STACK_SIZE + 256, NULL, (tskIDLE_PRIORITY + 1UL), (TaskHandle_t *) NULL);

	xTaskCreate(vSensorTask, "vSensorTask",
			configMINIMAL_STACK_SIZE + 256, NULL, (tskIDLE_PRIORITY + 1UL), (TaskHandle_t *) NULL);

	vTaskStartScheduler();

	return 0;
}


