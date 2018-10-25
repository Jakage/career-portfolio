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
#include "DigitalIoPin.h"

// Global parameters
#define TRANSNUM	'3'	// Transmitter's number 1-8
#define TEMPNUM		2	// 0 for TC74A0 and 2 for TC74A2
#define FREQCHANNEL	17	// Transmitter's channel 10-17

// Global functions
SemaphoreHandle_t xBin;
QueueHandle_t xQueue;
struct packet {
	uint8_t pTemp = 20;
	uint8_t pHelp = 'N';
	uint8_t pBed = 'N';
};
packet t;

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

// FreeRTOS Task functions
static void vTransmitterTask(void *pvParameters) {
	LoRaSPI LoRa(LoRaSPI::transmitter);
	LoRa.setChannel(FREQCHANNEL); // Set transmitter's channel. Channels 10-17

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

	for(int i = 4; i < buffer_size; i++) {
		buffer[i] = '%';
	}

	while (1) {
		//xQueueReceive(xQueue, &buffer[0], portMAX_DELAY);
		buffer[0] = TRANSNUM;
		buffer[1] = t.pTemp;
		buffer[2] = t.pHelp;
		buffer[3] = t.pBed;
		//buffer[1] = '2';
		//buffer[2] = '\n';

		/*xSemaphoreTake(xBin, 10);
		printf("Buffer0: %" PRIdFAST8 "\r\n", buffer[0]);
		xSemaphoreGive(xBin);*/

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

static void vTempSensorTask(void *pvParameters) {
	TempI2C thermal(TEMPNUM); // 0 for TC74A0 and 2 for TC74A2
	while(1) {
		t.pTemp = thermal.readTemp();
		vTaskDelay(configTICK_RATE_HZ);
	}
	/*uint8_t temp;
	while(1) {
		temp = thermal.readTemp();
		//printf("TEMP SENSOR: %" PRIdFAST8 "\r\n", temp);
		//xSemaphoreTake(xBin, 10);
		xQueueSendToBack(xQueue, &temp, 10);
		//xSemaphoreGive(xBin);
		// Task delay
		vTaskDelay(configTICK_RATE_HZ);
	}*/
}

static void vHelpBtnTask(void *pvParameters) {
	DigitalIoPin helpBtn (0, 8, DigitalIoPin::pullup, true);
	while(1) {
		if (helpBtn.read()) {
			t.pHelp = 'Y';
			printf("SEND HELP\r\n");
			vTaskDelay(configTICK_RATE_HZ * 10);
			t.pHelp = 'N';
		}
		vTaskDelay(configTICK_RATE_HZ / 10);
	}
	/*uint8_t help = 'H';
	while(1) {
		if (helpBtn.read()) {
			//xSemaphoreTake(xBin, 10);
			for (int i = 0; i < 5; i++) {
				xQueueSendToFront(xQueue, &help, portMAX_DELAY);
			}
			printf("SEND HELP\r\n");
			//while(helpBtn.read()) {}
			//xSemaphoreGive(xBin);
		}
		vTaskDelay(configTICK_RATE_HZ);
	}*/
}

static void vBedSensorTask(void *pvParameters) {
	DigitalIoPin bedSensor (1, 6, DigitalIoPin::pullup, true);
	while(1) {
		if (bedSensor.read()) {
			t.pBed = 'Y';
			printf("BED IN USE\r\n");
		} else {
			t.pBed = 'N';
		}
		vTaskDelay(configTICK_RATE_HZ);
	}
}

// Main loop
int main(void) {
	prvSetupHardware();
	xBin = xSemaphoreCreateBinary();
	xQueue = xQueueCreate(5, 1);

	xTaskCreate(vTransmitterTask, "vTransmitterTask",
			configMINIMAL_STACK_SIZE + 256, NULL, (tskIDLE_PRIORITY + 1UL), (TaskHandle_t *) NULL);

	xTaskCreate(vTempSensorTask, "vTempSensorTask",
			configMINIMAL_STACK_SIZE + 256, NULL, (tskIDLE_PRIORITY + 1UL), (TaskHandle_t *) NULL);

	xTaskCreate(vHelpBtnTask, "vHelpBtnTask",
			configMINIMAL_STACK_SIZE + 128, NULL, (tskIDLE_PRIORITY + 1UL), (TaskHandle_t *) NULL);

	xTaskCreate(vBedSensorTask, "vBedSensorTask",
			configMINIMAL_STACK_SIZE + 128, NULL, (tskIDLE_PRIORITY + 1UL), (TaskHandle_t *) NULL);

	vTaskStartScheduler();

	return 0;
}


