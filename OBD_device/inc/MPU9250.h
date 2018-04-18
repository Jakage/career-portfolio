/*
 * MPU9250.h
 *
 *  Created on: 23.1.2018
 *      Author: Jake
 */

#ifndef MPU9250_H_
#define MPU9250_H_

#include "board.h"
#include <cmath>

class MPU9250 {
public:
	MPU9250(int deviceNumber, int speed);
	virtual ~MPU9250();
	bool transaction(uint8_t devAddr, uint8_t *txBuffPtr, uint16_t txSize, uint8_t *rxBuffPtr, uint16_t rxSize);
	void getAres();
	void getGres();
	void getMres();
	void readAccelData(int16_t *destination);
	void writeByte(uint8_t devAddr, uint8_t regAddr, uint8_t data);
	uint8_t readByte(uint8_t deviceAddress, uint8_t registerAddress);
	void readBytes(uint8_t deviceAddress, uint8_t registerAddress, uint8_t count, uint8_t *dest);
	void calibrate(float * gyroBias, float * accelBias);
	void selfTest(float *destination);
	void initMPU9250();
	void initAK8963(float *destination);

	int16_t accelCount[3];
	float selfTestArray[6];
	float gyroBias[3] = {0,0,0}, accelBias[3] = {0,0,0};
	float factoryMagCalibration[3] = {0, 0, 0};
	float ax, ay, az;
	float aRes = 2.0f / 32768.0f;
	float gRes = 250.0f / 32768.0f;
	float mRes = 10.0f * 4912.0f / 32760.0f;
private:
	LPC_I2C_T *device;
	static const unsigned int I2C_CLK_DIVIDER = 40;
	static const unsigned int I2C_MODE = 0;

	I2CM_XFER_T  i2cmXferRec;

protected:
	enum Ascale {
		AFS_2G = 0,
		AFS_4G,
		AFS_8G,
		AFS_16G
	};

	enum Gscale {
		GFS_250DPS = 0,
		GFS_500DPS,
		GFS_1000DPS,
		GFS_2000DPS
	};

	enum Mscale {
		MFS_14BITS = 0, // 0.6 mG per LSB
		MFS_16BITS      // 0.15 mG per LSB
	};

	enum M_MODE {
		M_8HZ = 0x02,  // 8 Hz update
		M_100HZ = 0x06 // 100 Hz continuous magnetometer
	};
	uint8_t Ascale = AFS_2G;
	uint8_t Gscale = GFS_250DPS;
	uint8_t Mscale = MFS_16BITS;
	uint8_t Mmode = M_8HZ;

	uint8_t AK8963_ADDRESS = 0x0C;
	uint8_t WHO_AM_I_AK8963 = 0x49; // (AKA WIA) should return 0x48
	uint8_t INFO = 0x01;
	uint8_t AK8963_ST1 = 0x02;  // data ready status bit 0
	uint8_t AK8963_XOUT_L = 0x03;  // data
	uint8_t AK8963_XOUT_H = 0x04;
	uint8_t AK8963_YOUT_L = 0x05;
	uint8_t AK8963_YOUT_H = 0x06;
	uint8_t AK8963_ZOUT_L = 0x07;
	uint8_t AK8963_ZOUT_H = 0x08;
	uint8_t AK8963_ST2 = 0x09;  // Data overflow bit 3 and data read error status bit 2
	uint8_t AK8963_CNTL = 0x0A;  // Power down (0000), single-measurement (0001), self-test (1000) and Fuse ROM (1111) modes on bits 3:0
	uint8_t AK8963_ASTC = 0x0C;  // Self test control
	uint8_t AK8963_I2CDIS = 0x0F;  // I2C disable
	uint8_t AK8963_ASAX = 0x10;  // Fuse ROM x-axis sensitivity adjustment value
	uint8_t AK8963_ASAY = 0x11;  // Fuse ROM y-axis sensitivity adjustment value
	uint8_t AK8963_ASAZ = 0x12;  // Fuse ROM z-axis sensitivity adjustment value

	uint8_t SELF_TEST_X_GYRO = 0x00;
	uint8_t SELF_TEST_Y_GYRO = 0x01;
	uint8_t SELF_TEST_Z_GYRO = 0x02;

	/*uint8_t X_FINE_GAIN = 0x03; // [7:0] fine gain
	uint8_t Y_FINE_GAIN = 0x04;
	uint8_t Z_FINE_GAIN = 0x05;
	uint8_t XA_OFFSET_H = 0x06; // User-defined trim values for accelerometer
	uint8_t XA_OFFSET_L_TC = 0x07;
	uint8_t YA_OFFSET_H = 0x08;
	uint8_t YA_OFFSET_L_TC = 0x09;
	uint8_t ZA_OFFSET_H = 0x0A;
	uint8_t ZA_OFFSET_L_TC = 0x0B; */

	uint8_t SELF_TEST_X_ACCEL = 0x0D;
	uint8_t SELF_TEST_Y_ACCEL = 0x0E;
	uint8_t SELF_TEST_Z_ACCEL = 0x0F;

	uint8_t SELF_TEST_A = 0x10;

	uint8_t XG_OFFSET_H = 0x13;  // User-defined trim values for gyroscope
	uint8_t XG_OFFSET_L = 0x14;
	uint8_t YG_OFFSET_H = 0x15;
	uint8_t YG_OFFSET_L = 0x16;
	uint8_t ZG_OFFSET_H = 0x17;
	uint8_t ZG_OFFSET_L = 0x18;
	uint8_t SMPLRT_DIV = 0x19;
	uint8_t CONFIG = 0x1A;
	uint8_t GYRO_CONFIG = 0x1B;
	uint8_t ACCEL_CONFIG = 0x1C;
	uint8_t ACCEL_CONFIG2 = 0x1D;
	uint8_t LP_ACCEL_ODR = 0x1E;
	uint8_t WOM_THR = 0x1F;

	// Duration counter threshold for motion interrupt generation, 1 kHz rate,
	// LSB = 1 ms
	uint8_t MOT_DUR = 0x20;
	// Zero-motion detection threshold bits [7:0]
	uint8_t ZMOT_THR = 0x21;
	// Duration counter threshold for zero motion interrupt generation, 16 Hz rate,
	// LSB = 64 ms
	uint8_t ZRMOT_DUR = 0x22;
	uint8_t FIFO_EN = 0x23;
	uint8_t I2C_MST_CTRL = 0x24;
	uint8_t I2C_SLV0_ADDR = 0x25;
	uint8_t I2C_SLV0_REG = 0x26;
	uint8_t I2C_SLV0_CTRL = 0x27;
	uint8_t I2C_SLV1_ADDR = 0x28;
	uint8_t I2C_SLV1_REG = 0x29;
	uint8_t I2C_SLV1_CTRL = 0x2A;
	uint8_t I2C_SLV2_ADDR = 0x2B;
	uint8_t I2C_SLV2_REG = 0x2C;
	uint8_t I2C_SLV2_CTRL = 0x2D;
	uint8_t I2C_SLV3_ADDR = 0x2E;
	uint8_t I2C_SLV3_REG = 0x2F;
	uint8_t I2C_SLV3_CTRL = 0x30;
	uint8_t I2C_SLV4_ADDR = 0x31;
	uint8_t I2C_SLV4_REG = 0x32;
	uint8_t I2C_SLV4_DO = 0x33;
	uint8_t I2C_SLV4_CTRL = 0x34;
	uint8_t I2C_SLV4_DI = 0x35;
	uint8_t I2C_MST_STATUS = 0x36;
	uint8_t INT_PIN_CFG = 0x37;
	uint8_t INT_ENABLE = 0x38;
	uint8_t DMP_INT_STATUS = 0x39;  // Check DMP interrupt
	uint8_t INT_STATUS = 0x3A;
	uint8_t ACCEL_XOUT_H = 0x3B;
	uint8_t ACCEL_XOUT_L = 0x3C;
	uint8_t ACCEL_YOUT_H = 0x3D;
	uint8_t ACCEL_YOUT_L = 0x3E;
	uint8_t ACCEL_ZOUT_H = 0x3F;
	uint8_t ACCEL_ZOUT_L = 0x40;
	uint8_t TEMP_OUT_H = 0x41;
	uint8_t TEMP_OUT_L = 0x42;
	uint8_t GYRO_XOUT_H = 0x43;
	uint8_t GYRO_XOUT_L = 0x44;
	uint8_t GYRO_YOUT_H = 0x45;
	uint8_t GYRO_YOUT_L = 0x46;
	uint8_t GYRO_ZOUT_H = 0x47;
	uint8_t GYRO_ZOUT_L = 0x48;
	uint8_t EXT_SENS_DATA_00 = 0x49;
	uint8_t EXT_SENS_DATA_01 = 0x4A;
	uint8_t EXT_SENS_DATA_02 = 0x4B;
	uint8_t EXT_SENS_DATA_03 = 0x4C;
	uint8_t EXT_SENS_DATA_04 = 0x4D;
	uint8_t EXT_SENS_DATA_05 = 0x4E;
	uint8_t EXT_SENS_DATA_06 = 0x4F;
	uint8_t EXT_SENS_DATA_07 = 0x50;
	uint8_t EXT_SENS_DATA_08 = 0x51;
	uint8_t EXT_SENS_DATA_09 = 0x52;
	uint8_t EXT_SENS_DATA_10 = 0x53;
	uint8_t EXT_SENS_DATA_11 = 0x54;
	uint8_t EXT_SENS_DATA_12 = 0x55;
	uint8_t EXT_SENS_DATA_13 = 0x56;
	uint8_t EXT_SENS_DATA_14 = 0x57;
	uint8_t EXT_SENS_DATA_15 = 0x58;
	uint8_t EXT_SENS_DATA_16 = 0x59;
	uint8_t EXT_SENS_DATA_17 = 0x5A;
	uint8_t EXT_SENS_DATA_18 = 0x5B;
	uint8_t EXT_SENS_DATA_19 = 0x5C;
	uint8_t EXT_SENS_DATA_20 = 0x5D;
	uint8_t EXT_SENS_DATA_21 = 0x5E;
	uint8_t EXT_SENS_DATA_22 = 0x5F;
	uint8_t EXT_SENS_DATA_23 = 0x60;
	uint8_t MOT_DETECT_STATUS = 0x61;
	uint8_t I2C_SLV0_DO = 0x63;
	uint8_t I2C_SLV1_DO = 0x64;
	uint8_t I2C_SLV2_DO = 0x65;
	uint8_t I2C_SLV3_DO = 0x66;
	uint8_t I2C_MST_DELAY_CTRL = 0x67;
	uint8_t SIGNAL_PATH_RESET = 0x68;
	uint8_t MOT_DETECT_CTRL = 0x69;
	uint8_t USER_CTRL = 0x6A; // Bit 7 enable DMP, bit 3 reset DMP
	uint8_t PWR_MGMT_1 = 0x6B; // Device defaults to the SLEEP mode
	uint8_t PWR_MGMT_2 = 0x6C;
	// Using the MPU-9250 breakout board, ADDRESS is 0x68
	// Seven-bit device address is 110100 for 0x68 and 110101 for 0x69
	uint8_t MPU9250_ADDRESS = 0x68;
	uint8_t DMP_BANK = 0x6D;  // Activates a specific bank in the DMP
	uint8_t DMP_RW_PNT = 0x6E;  // Set read/write pointer to a specific start address in specified DMP bank
	uint8_t DMP_REG = 0x6F;  // Register in DMP from which to read or to which to write
	uint8_t DMP_REG_1 = 0x70;
	uint8_t DMP_REG_2 = 0x71;
	uint8_t FIFO_COUNTH = 0x72;
	uint8_t FIFO_COUNTL = 0x73;
	uint8_t FIFO_R_W = 0x74;
	uint8_t WHO_AM_I_MPU9250 = 0x75; // Should return 0x71
	uint8_t XA_OFFSET_H = 0x77;
	uint8_t XA_OFFSET_L = 0x78;
	uint8_t YA_OFFSET_H = 0x7A;
	uint8_t YA_OFFSET_L = 0x7B;
	uint8_t ZA_OFFSET_H = 0x7D;
	uint8_t ZA_OFFSET_L = 0x7E;
	uint8_t READ_FLAG = 0x80;

	bool magInit();
	void kickHardware();
	void select();
	void deselect();
};


#endif /* MPU9250_H_ */
