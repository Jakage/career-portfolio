/*
===============================================================================
 Name        : main.c
 Author      : Jarkko Aho - Metropolia
 Version     : 0.5
 Copyright   : Reference material for Servoped Oy, code owned by Jarkko Aho
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
#include <cctype>				// toupper() function for AT commands

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "MPU9250.h"			// pins assigned to SCL 0.22 - SDA 0.23
#include "NMEA.h"				// pins assigned to rx 0.9 - tx 0.29
#include "NMEAparser.h"			// getter functions for parsed NMEA data
#include "GPRS.h"				// pins assigned to rx 1.10 - tx 1.9

#define DEVICE_NUMBER	0		// MPU9250 I2C device number
#define SPEED			100000	// MPU9250 I2CM transfer rate

// Global semaphore for printf() functions
SemaphoreHandle_t xBin;

// LPC board setup
static void prvSetupHardware(void) {
	SystemCoreClockUpdate();
	Board_Init();
	Board_LED_Set(0, false);
	Chip_RIT_Init(LPC_RITIMER);
}

extern "C" {
void vConfigureTimerForRunTimeStats(void) {
	Chip_SCT_Init(LPC_SCTSMALL1);
	LPC_SCTSMALL1->CONFIG = SCT_CONFIG_32BIT_COUNTER;
	LPC_SCTSMALL1->CTRL_U = SCT_CTRL_PRE_L(255) | SCT_CTRL_CLRCTR_L;
}
}

// Accelerometer task
static void vMPUTask(void *pvParameters) {
	// Create MPU9250 object
	MPU9250 myMPU(DEVICE_NUMBER, SPEED);

	// Init self test and print values for debugging
	myMPU.selfTest(myMPU.selfTestArray);
	xSemaphoreTake(xBin, 10);
	printf("Self Test:\r\n");
	printf("x-axis self test: acceleration trim within : ");
	printf("%f", myMPU.selfTestArray[0]); printf("%% of factory value\r\n");
	printf("y-axis self test: acceleration trim within : ");
	printf("%f", myMPU.selfTestArray[1]); printf("%% of factory value\r\n");
	printf("z-axis self test: acceleration trim within : ");
	printf("%f", myMPU.selfTestArray[2]); printf("%% of factory value\r\n");
	printf("x-axis self test: gyration trim within : ");
	printf("%f", myMPU.selfTestArray[3]); printf("%% of factory value\r\n");
	printf("y-axis self test: gyration trim within : ");
	printf("%f", myMPU.selfTestArray[4]); printf("%% of factory value\r\n");
	printf("z-axis self test: gyration trim within : ");
	printf("%f", myMPU.selfTestArray[5]); printf("%% of factory value\r\n");
	xSemaphoreGive(xBin);

	// Calibrate device and print bias values
	myMPU.calibrate(myMPU.gyroBias, myMPU.accelBias);
	xSemaphoreTake(xBin, 10);
	printf("MPU9250 Bias:\r\n");
	printf("x: %f mg, y: %f mg, z: %f mg\r\n", (1000*myMPU.accelBias[0]), (1000*myMPU.accelBias[1]), (1000*myMPU.accelBias[2]));
	printf("x: %f o/s, y: %f o/s, z: %f o/s\r\n", myMPU.gyroBias[0], myMPU.gyroBias[1], myMPU.gyroBias[2]);
	printf("\r\n");
	xSemaphoreGive(xBin);

	// Init device based on calibration values
	myMPU.initMPU9250();

	// Init magnetometer for debugging
	myMPU.initAK8963(myMPU.factoryMagCalibration);
	xSemaphoreTake(xBin, 10);
	printf("X-Axis sensitivity adjustment value: %f\r\n", myMPU.factoryMagCalibration[0]);
	printf("Y-Axis sensitivity adjustment value: %f\r\n", myMPU.factoryMagCalibration[1]);
	printf("Z-Axis sensitivity adjustment value: %f\r\n", myMPU.factoryMagCalibration[2]);
	printf("\r\n");
	xSemaphoreGive(xBin);

	// Task main loop
	// Prints the accelerator values
	// These values can be send to GPRS
	while(1) {
		myMPU.readAccelData(myMPU.accelCount);
		myMPU.getAres();

		myMPU.ax = (float)myMPU.accelCount[0]*myMPU.aRes;
		myMPU.ay = (float)myMPU.accelCount[1]*myMPU.aRes;
		myMPU.az = (float)myMPU.accelCount[2]*myMPU.aRes;

		xSemaphoreTake(xBin, 10);
		printf("X: %f mg, Y: %f mg, Z: %f mg\r\n", (1000*myMPU.ax), (1000*myMPU.ay), (1000*myMPU.az));
		xSemaphoreGive(xBin);

		vTaskDelay(configTICK_RATE_HZ / 2);
	}
}

// GPS NMEA task
static void vGPSTask(void *pvParameters) {
	NMEA gps;
	NMEAparser parser;
	bool end;
	while(1) {
		end = false;
		while (end == false) {
			int r = gps.read();
			if (r != -1) {
				parser.fusedata(r);
				if(parser.isdataready()) {
					xSemaphoreTake(xBin, 10);
					printf("Latitude: %f, Longitude: %f\r\n", parser.getLatitude(), parser.getLongitude());
					xSemaphoreGive(xBin);
					end = true;
				} else {
					xSemaphoreTake(xBin, 10);
					printf("NO SIGNAL\r\n");
					xSemaphoreGive(xBin);
					vTaskDelay(configTICK_RATE_HZ / 2);
				}
			} else {
				xSemaphoreTake(xBin, 10);
				printf("-1\r\n");
				xSemaphoreGive(xBin);
				vTaskDelay(configTICK_RATE_HZ / 2);
			}
		}
		vTaskDelay(configTICK_RATE_HZ / 2);
	}
}

/*
// GPRS/GSM 3G task (not in use)
static void vGSMTask(void *pvParameters) {
	GPRS gsm;
	char rx[64] = {0};
	char tx[25] = {0};
	int rc = 0;
	int tc = 0;
	while(1) {
		int t = Board_UARTGetChar();
		if (t != EOF) {
			xSemaphoreTake(xBin, 10);
			Board_UARTPutChar(t);
			xSemaphoreGive(xBin);
			tx[tc++] = toupper(t);
			if (t == '\r') {
				gsm.write(tx, tc);
				for(int i = 0; i < tc; i++) { // probably not needed
					tx[i] = 0;
				}
				tc = 0;
				xSemaphoreTake(xBin, 10);
				printf("\r\n");
				xSemaphoreGive(xBin);
			}
		} else {
			vTaskDelay(1);
		}
		if(gsm.available()) {
			int r = gsm.read();
			if(r != -1) {
				if(rc < 63) {
					rx[rc++] = r;
					rx[rc] = 0;
				}
				if (r == '\n' || rc == 63) {
					xSemaphoreTake(xBin, 10);
					printf(rx);
					xSemaphoreGive(xBin);
					for(int i = 0; i < rc; i++) {
						rx[i] = 0;
					}
					rc = 0;
				}
			}
		}
	}
}

// OBD task TODO: whole task
static void vOBDTask(void *pvParameters) {
	while(1) {

		vTaskDelay(configTICK_RATE_HZ);
	}
}
 */

int main(void) {
	prvSetupHardware();
	xBin = xSemaphoreCreateBinary();

	xTaskCreate(vMPUTask, "vMPUTask",
			configMINIMAL_STACK_SIZE + 256, NULL, (tskIDLE_PRIORITY + 1UL), (TaskHandle_t *) NULL);

	xTaskCreate(vGPSTask, "vGPSTask",
			configMINIMAL_STACK_SIZE + 320, NULL, (tskIDLE_PRIORITY + 1UL), (TaskHandle_t *) NULL);
	/*
	xTaskCreate(vGSMTask, "vGSMTask",
			configMINIMAL_STACK_SIZE + 320, NULL, (tskIDLE_PRIORITY + 1UL), (TaskHandle_t *) NULL);

	xTaskCreate(vOBDTask, "vOBDTask",
		configMINIMAL_STACK_SIZE + 126, NULL, (tskIDLE_PRIORITY + 1UL), (TaskHandle_t *) NULL);
	 */
	vTaskStartScheduler();

	return 1;

}
