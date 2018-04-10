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

//#define TEST_PLOTTER

// Taken from: https://stackoverflow.com/a/3437484
#define MAX(a,b) ((a > b) ? a : b)
#define ABS(a) (a < 0 ? -a : a)

#include <cr_section_macros.h>
#include <limits.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "parser/GCode.h"
#include "parser/data/Command.h"
#include "hardware/Arm.h"
#include "hardware/Laser.h"
#include "hardware/Pencil.h"
#include "data/IOInput.h"
#include "data/Bresenham.h"
#include "data/RITMessage.h"
#include "data/Config.h"

static const int ARM_LENGTH = 2; // The amount of arms on the plotter
static const int HORIZONTAL = 0; // The horizontal arm index in the arms array
static const int VERTICAL = 1; // The vertical arm index in the arms array
static const int CALIBRATING_RUNS = 1; // The amount of runs needed to calibrate the plotter
static const int STEP_OFFSET = 10; // The amount of steps that are used to offset the plotter
static const int SPEEDUP_RATE = 20; // The percentage (between 0-100) that will be used to accelerate the motor
static const int DEFAULT_PPS = 5000; // The start speed value of the motor before accelerating
static const int TARGET_PPS = 8000; // The end speed value of the motor after accelerating
static const int STEPS = 2; // The amount of steps each call to RIT will take
static const double ERROR_MULTIPLIER = 1; // The multiplier used when calculating the rounding error

static const double ACCELERATE_UPPER_LIMIT = 1.0; // The maximum multiplier when drawing large lines, it can't go higher than 1.0
static const double ACCELERATE_LOWER_LIMIT = 0.1; // The minimum multiplier when drawing tiny lines, it can't go lower than 0.2

Config cfg;
Pencil *pencil;
Laser *laser;
Bresenham algo;

SemaphoreHandle_t ritDone;
QueueHandle_t sendQueue, ritQueue;

double xError = 0, yError = 0;
bool usingLaser = false;

volatile int _count[ARM_LENGTH] = { 0, 0 };
Arm *arms[ARM_LENGTH];

// Used when calibrating
bool canCalibrate[ARM_LENGTH];
bool motorState[ARM_LENGTH];
bool state[ARM_LENGTH];
int curTry[ARM_LENGTH] = { 0, 0 };
DigitalIoPin *statePin[ARM_LENGTH];
bool isCalibrating = true;

void RIT_Calibrate(int id);
void RIT_Start(int xCount, int yCount, int us);

/**
 * Maps the value from one range to another range.
 * This function comes from Arduino: https://www.arduino.cc/en/Reference/Map
 *
 * @param x The value that will be converted
 * @param in_min First range minimum value
 * @param in_max First range maximum value
 * @param out_min Second range minimum value
 * @param out_max Second range maximum value
 * @return A value that is in the new range
 */
long map(long x, long in_min, long in_max, long out_min, long out_max) {
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

/* Sets up system hardware */
static void prvSetupHardware(void) {
	SystemCoreClockUpdate();
	Board_Init();
	Board_Debug_Init();

	Chip_RIT_Init(LPC_RITIMER);
	NVIC_SetPriority(RITIMER_IRQn,
	configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 5);

	/* Initial LED0 state is off */
	Board_LED_Set(0, false);
	Board_LED_Set(1, false);
}

/* the following is required if runtime statistics are to be collected */
extern "C" {
void vConfigureTimerForRunTimeStats(void) {
	Chip_SCT_Init(LPC_SCTSMALL1);
	LPC_SCTSMALL1->CONFIG = SCT_CONFIG_32BIT_COUNTER;
	LPC_SCTSMALL1->CTRL_U = SCT_CTRL_PRE_L(255) | SCT_CTRL_CLRCTR_L; // set prescaler to 256 (255 + 1), and start timer
}
}

/**
 * Setup the width, height and all the ports. When TEST_PLOTTER is defined,
 * the ports and pins are setup for the simple stepper motor arms (the one used in the exercise)
 */
void setup() {
	cfg.width = 380;
	cfg.height = 330;

	IOInput pencilInput;
	pencilInput.port = 0;
#if defined(TEST_PLOTTER)
	pencilInput.pin = 8;
#else
	pencilInput.pin = 10;
#endif
	pencil = new Pencil(pencilInput);

#if !defined(TEST_PLOTTER)
	IOInput laserInput;
	laserInput.port = 0;
	laserInput.pin = 12;
	laser = new Laser(laserInput);
#endif

	IOInput motor1;
	motor1.port = 0;
#if defined(TEST_PLOTTER)
	motor1.pin = 24;
#else
	motor1.pin = 27;
#endif

	IOInput dir1;
#if defined(TEST_PLOTTER)
	dir1.port = 1;
	dir1.pin = 0;
	dir1.invert = false;
#else
	dir1.port = 0;
	dir1.pin = 28;
	dir1.invert = false;
#endif

	IOInput m1Limit1;
#if defined(TEST_PLOTTER)
	m1Limit1.port = 0;
	m1Limit1.pin = 27;
#else
	m1Limit1.port = 1;
	m1Limit1.pin = 3;
#endif

	IOInput m1Limit2;
#if defined(TEST_PLOTTER)
	m1Limit2.port = 0;
	m1Limit2.pin = 28;
#else
	m1Limit2.port = 0;
	m1Limit2.pin = 0;
#endif

	IOInput motor2;
	motor2.port = 0;
#if defined(TEST_PLOTTER)
	motor2.pin = 10;
#else
	motor2.pin = 24;
#endif

	IOInput dir2;
#if defined(TEST_PLOTTER)
	dir2.port = 0;
	dir2.pin = 9;
	dir2.invert = false;
#else
	dir2.port = 1;
	dir2.pin = 0;
	dir2.invert = true;
#endif

	IOInput m2Limit1;
#if defined(TEST_PLOTTER)
	m2Limit1.port = 0;
	m2Limit1.pin = 29;
#else
	m2Limit1.port = 0;
	m2Limit1.pin = 9;
#endif

	IOInput m2Limit2;
#if defined(TEST_PLOTTER)
	m2Limit2.port = 1;
	m2Limit2.pin = 9;
#else
	m2Limit2.port = 0;
	m2Limit2.pin = 29;
#endif

	arms[HORIZONTAL] = new Arm(motor1, dir1, m1Limit1, m1Limit2);
	arms[VERTICAL] = new Arm(motor2, dir2, m2Limit1, m2Limit2);
}

/**
 * Calibrate the arms with the help of RIT. This allows for a fast calibration.
 * The task will block the program until it's fully calibrated.
 */
void RIT_Calibrate(int id) {
	Arm *arm = arms[id];
	arm->direction->write(arm->currentDirection);

	if (canCalibrate[id]) {
		arm->motor->write(motorState[id]);
		motorState[id] = (bool) !motorState[id];
	}

	if (!state[id]) {
		if (!canCalibrate[id]) {
			canCalibrate[id] = !(arm->switch1->read() || arm->switch2->read());
		} else {
			if (arm->switch1->read()) {
				arm->currentDirection = (bool) !arm->currentDirection;
				statePin[id] = arm->switch2;
				state[id] = true;
			} else if (arm->switch2->read()) {
				arm->currentDirection = (bool) !arm->currentDirection;
				statePin[id] = arm->switch1;
				state[id] = true;
			}
		}
	} else {
		arm->steps++;

		if (statePin[id]->read()) {
			curTry[id]++;

			arm->currentDirection = (bool) !arm->currentDirection;

			if (statePin[id] == arm->switch1) {
				statePin[id] = arm->switch2;
			} else {
				statePin[id] = arm->switch1;
			}

			if (curTry[id] >= CALIBRATING_RUNS) {
				arm->steps = (arm->steps / CALIBRATING_RUNS);
				_count[id] = 0;

				if (_count[0] == 0 && _count[1] == 0) {
					isCalibrating = false;
				}
			}
		}
	}
}

/**
 * Toggles the state of the motor (either true or false)
 *
 * @param id The id of the motor (either HORIZONTAL or VERTICAL)
 */
void toggleMotor(int id) {
	arms[id]->motor->write(motorState[id]);
	motorState[id] = (bool) !motorState[id];
}

/**
 * The handler for RIT. If the motor is calibrating the calibrating function is called.
 * Otherwise the motors will be toggled until the count for the motor reached zero.
 */
extern "C" void RIT_IRQHandler(void) {
	// This used to check if a context switch is required
	portBASE_TYPE xHigherPriorityWoken = pdFALSE;

	// Tell timer that we have processed the interrupt.
	// Timer then removes the IRQ until next match occurs
	Chip_RIT_ClearIntStatus(LPC_RITIMER);

	if (_count[0] > 0 || _count[1] > 0) {
		if (isCalibrating) {
			for (int i = 0; i < ARM_LENGTH; i++) {
				if (_count[i] > 0) {
					_count[i]--;
					RIT_Calibrate(i);
				}
			}
		} else {
			if (_count[HORIZONTAL] > 0) {
				// While the horizontal count is not 0, toggle the horizontal motor
				_count[HORIZONTAL]--;
				toggleMotor(HORIZONTAL);
			}

			if (_count[VERTICAL] > 0) {
				// While the horizontal count is not 0, toggle the vertical motor
				_count[VERTICAL]--;
				toggleMotor(VERTICAL);
			}
		}
	} else {
		// Disable timer
		Chip_RIT_Disable(LPC_RITIMER);
		// Give semaphore and set context switch flag if a higher priority task was woken up
		xSemaphoreGiveFromISR(ritDone, &xHigherPriorityWoken);
	}

	// End the ISR and (possibly) do a context switch
	portEND_SWITCHING_ISR(xHigherPriorityWoken);
}

/**
 * Start RIT. The xCound and yCount will be used to either move the horizontal or vertical axis.
 *
 * @param xCount The amount of toggles the x-axis need
 * @param yCount The amount of toggles the y-axis need
 * @param us The amount of microseconds the task should run for.
 */
void RIT_Start(int xCount, int yCount, int us) {
	int64_t cmp_value;

	cmp_value = (uint64_t) Chip_Clock_GetSystemClockRate() * (uint64_t) us
			/ 1000000;

	// disable timer during configuration
	Chip_RIT_Disable(LPC_RITIMER);

	_count[HORIZONTAL] = xCount;
	_count[VERTICAL] = yCount;

	// enable automatic clear on when compare value==timer value // this makes interrupts trigger periodically
	Chip_RIT_EnableCompClear(LPC_RITIMER);

	// reset the counter
	Chip_RIT_SetCounter(LPC_RITIMER, 0);
	Chip_RIT_SetCompareValue(LPC_RITIMER, cmp_value);

	// start counting
	Chip_RIT_Enable(LPC_RITIMER);

	// Enable the interrupt signal in NVIC (the interrupt controller)
	NVIC_EnableIRQ(RITIMER_IRQn);

	// wait for ISR to tell that we're done
	if (xSemaphoreTake(ritDone, portMAX_DELAY) == pdTRUE) {
		// Disable the interrupt signal in NVIC (the interrupt controller)
		NVIC_DisableIRQ(RITIMER_IRQn);
	} else {
		// unexpected error
	}
}

/**
 * Setup bresenham's algorithm.
 *
 * @param x0 Start horizontal
 * @param y0 Start vertical
 * @param x1 End horizontal
 * @param y1 End vertical
 */
void setupBresenham(int x0, int y0, int x1, int y1) {
	// Calculate the difference between the start and end position.
	algo.dx = x1 - x0;
	algo.dy = y1 - y0;

	// Get the maximum amount of steps that are needed to go the targeted position.
	algo.max = MAX(ABS(algo.dx), ABS(algo.dy));
	// Calculate the deltaX by dividing the horizontal difference by the maximum amount of steps
	algo.deltaX = (double) ABS(algo.dx) / (double) algo.max;
	// Calculate the deltaY by dividing the vertical difference by the maximum amount of steps
	algo.deltaY = (double) ABS(algo.dy) / (double) algo.max;
}


/**
 * Move the arms with bresenham's algorithm to the specified location.
 *
 * @param speed The speed that it should reach after accelerating
 * @param xCount The horizontal position
 * @param yCount The vertical position
 */
void moveArms(int speed, int xCount, int yCount) {
	// Get the maximum amount of steps, this can be either the horizontal or the vertical amount of steps
	int steps = MAX(xCount, yCount) / STEPS;
	// Get the multiplier that will be used on the speeds.
	// This is used so smaller lines won't be accelerating to full speed.
	double multiplier = (double) steps / (arms[HORIZONTAL]->steps / 2);
	if (multiplier > ACCELERATE_UPPER_LIMIT) {
		multiplier = ACCELERATE_UPPER_LIMIT;
	} else if (multiplier < ACCELERATE_LOWER_LIMIT) {
		multiplier = ACCELERATE_LOWER_LIMIT;
	}

	speed *= multiplier;
	int startSpeed = DEFAULT_PPS * multiplier;

	int currentSpeed = startSpeed;
	int diff = (speed - currentSpeed);

	int posX = (arms[HORIZONTAL]->currentStep) / STEPS;
	int posY = (arms[VERTICAL]->currentStep) / STEPS;

	int tarX = posX + (xCount / STEPS);
	int tarY = posY + (yCount / STEPS);

	for (int i = 0; (posX != tarX) || (posY != tarY); i++) {
		// If the laser is not used, the speed should be calculated, otherwise the speed should be constant
		if (!usingLaser) {
			double percentage = (double) i / (steps * 0.2);
			double percentage2 = (double) (steps - i) / (steps * 0.2);
			double p = (double) i / steps;
			int percent = (int) (p * 100);

			if (percent <= SPEEDUP_RATE) {
				currentSpeed = startSpeed + (percentage * diff);
			} else if (percent >= 100 - SPEEDUP_RATE) {
				currentSpeed = startSpeed + (percentage2 * diff);
			}
		}

		// While horizontal position hasn't reached it's target position
		if (posX != tarX) {
			// Add the deltaX, which is the horizontal steps divided by the maximum amount of steps
			algo.cntX += algo.deltaX;

			if (algo.cntX >= 1.0) {
				// Move the horizontal sarm one place further
				posX += 1;
				RIT_Start(STEPS, 0, 100000 / currentSpeed);
				algo.cntX -= 1.0;
			}
		}

		// While vertical position hasn't reached it's target position
		if (posY != tarY) {
			// Add the deltaY, which is the vertical steps divided by the maximum amount of steps
			algo.cntY += algo.deltaY;

			if (algo.cntY >= 1.0) {
				// Move the vertical arm one place further
				posY += 1;
				RIT_Start(0, STEPS, 100000 / currentSpeed);
				algo.cntY -= 1.0;
			}
		}

		// Stop the loop when the horizontal and vertical position are at the targeted position.
		if (posX == tarX && posY == tarY) {
			break;
		}
	}

}

/**
 * The task for handling the RIT queue.
 * It's first job is to block the program until it's done calibrating.
 * After that the task will wait for a message in the RIT queue and move when it received a message.
 *
 * Before moving bresenham's algorithm is setup.
 * After moving the posiiton are set with the amount of steps it took in the horizontal and vertical direction.
 */
void taskMove(void *params) {
	// Make sure the arms are calibrated
	RIT_Start(INT_MAX, INT_MAX, 100);

	RITMessage msg;
	while (true) {
		// Wait for the RIT queue
		xQueueReceive(ritQueue, &msg, portMAX_DELAY);

		// Setup bresenham's alhorithm with the new values.
		setupBresenham(0, 0, msg.xSteps, msg.ySteps);
		// Move the arms to xSteps and ySteps and accelerate to TARGET_PPS speed
		moveArms(TARGET_PPS, msg.xSteps, msg.ySteps);

		// Increment the current arm position with the amount of steps taken.
		// When the the motor direction is switched, the steps should be negative.
//		arms[HORIZONTAL]->currentStep +=
//				(!msg.xSwitch) ? -msg.xSteps : msg.xSteps;
//		arms[VERTICAL]->currentStep +=
//				(!msg.ySwitch) ? -msg.ySteps : msg.ySteps;

		arms[HORIZONTAL]->currentStep = msg.xNewPosition;
		arms[VERTICAL]->currentStep = msg.yNewPosition;

		DEBUGOUT("OK\r\n");
	}
}

/**
 * Read the UART for commands (which are send from mDraw). After receiving a command, it will parse the command and send it to the command queue.
 * Note: The M10 command is returned here, because otherwise mDraw would crash.
 */
void taskReadDebug(void *param) {
	std::string line;
	GCode parser;

	while (true) {
		// Get chars from the usb. When no character is given Board_UARTGetChar will return EOF (255).
		char uartChar = Board_UARTGetChar();

		if (uartChar != 255) {
			// Wait for the new line to be given or an enter (13).
			// An enter is also checked for debugging purposes.
			if (uartChar == '\n' || uartChar == 13) {
				// Check whether M10 if found in the command
				int found = line.find("M10");

				if (found >= 0) {
					// Setup the plotter, by telling the following paramters to mDraw:
					// Width, height, motor X dir, motor Y dir, motor switch, speed, pen up pos, pen down pos
					DEBUGOUT(
							"M10 XY %d %d 0.00 0.00 A0 B0 H0 S100 U160 D90\r\n",
							cfg.width, cfg.height);
					DEBUGOUT("OK\r\n");
				} else {
					// Make the std::string into a char array
					// To reduce the amount of memory needed to parse the GCode a char array is used
					char *writeable = new char[line.size() + 1];
					std::copy(line.begin(), line.end(), writeable);
					writeable[line.size()] = '\0';

					Command cmd = parser.feed(writeable);
					// Delete the char array, because the plotter doesn't need it after parsing.
					delete[] writeable;

					if (cmd.type == CommandType::MOVE) {
						// Send the command to the move queue, which will transform it into commands to move the arms with RIT
						xQueueSend(sendQueue, &cmd, 100);
					} else if (cmd.type == CommandType::LASER) {
						// If the power is bigger than 0, the plotter is using the laser.
						// This will disable acceleration.
						usingLaser = cmd.power > 0;
						laser->setPower(cmd.power);

						Board_UARTPutSTR("\r\nOK\r\n");
					} else if (cmd.type == CommandType::PEN) {
						// Move the pencil to the specified angle.
						// For now the pencil can only respond to the angles given in the M10 command.
						pencil->move(cmd.angle);
						Board_UARTPutSTR("\r\nOK\r\n");
					} else if (cmd.type != CommandType::START) {
						cmd.type = CommandType::MOVE;
						cmd.x = 0;
						cmd.y = 0;
						xQueueSend(sendQueue, &cmd, 100);

						Board_UARTPutSTR("\r\nOK\r\n");
					}
				}

				line = "";
			} else {
				// Append the character to the line.
				line += uartChar;
			}

			// Echo the command back to UART. This allows us to see what's happening in mDraw.
			Board_UARTPutChar(uartChar);
		}
	}
}

/**
 * Get the amount of steps the arm should take.
 *
 * @param id  The id of the motor (either HORIZONTAL or VERTICAL)
 * @param val The end value
 */
long getStepValue(int id, int val) {
	long step = arms[id]->currentStep;
	return step - val;
}

/**
 * Process the command into coordinates that can be used in the plotter.
 *
 * @param cmd The GCode command
 * @return An RIT message which can be used by the move task
 */
RITMessage processCommand(Command cmd) {
	if(cmd.x <= 0) {
		cmd.x = 0;
	}

	if(cmd.y <= 0){
		cmd.y = 1;
	}

	// Calculate the multiplier needed to convert GCode coordinate into the plotters coordinates.
	// This can only work after calibrating.
	double multiplierX = (double) arms[HORIZONTAL]->steps / (double) cfg.width;
	double multiplierY = (double) arms[VERTICAL]->steps / (double) cfg.height;

	// Calculate the absolute position the arm needs to travel to
	long tempXVal = cmd.x * multiplierX;
	long tempYVal = (cfg.height - cmd.y) * multiplierY;

	// Calculate the same absolute position in doubles, which will be used to detect rounding errors
	double xError = (double) (cmd.x * multiplierX) - tempXVal;
	double yError = (double) ((cfg.height - cmd.y) * multiplierY) - tempYVal;

	// Calculate the relative position, this is also the amount of steps needed.
	long newXVal = getStepValue(HORIZONTAL, tempXVal);
	long newYVal = getStepValue(VERTICAL, tempYVal);

	// Calculate how big the error is and add it to the values
	int xVal = (ERROR_MULTIPLIER * xError) + 0.5;
	int yVal = (ERROR_MULTIPLIER * yError) + 0.5;

//	tempXVal += xVal;
//	newXVal += xVal;
//
//	tempYVal += yVal;
//	newYVal += yVal;

	RITMessage msg;
	// Check if the horizontal or vertical motors should be switched,
	// if they do need to be switched, make the values positive and switch the motor direction.
	msg.xSwitch = newXVal < 0;
	msg.ySwitch = newYVal < 0;

	if (msg.xSwitch) {
		newXVal *= -1;
		arms[HORIZONTAL]->direction->write(false);
	} else {
		arms[HORIZONTAL]->direction->write(true);
	}

	if (msg.ySwitch) {
		newYVal *= -1;
		arms[VERTICAL]->direction->write(false);
	} else {
		arms[VERTICAL]->direction->write(true);
	}

	msg.xSteps = newXVal;
	msg.xNewPosition = tempXVal;
	msg.ySteps = newYVal;
	msg.yNewPosition = tempYVal;

	return msg;
}

/**
 * The task that converts an command into an RIT message
 */
void taskReceiveCommand(void *params) {
	Command cmd;

	while (true) {
		// Wait for a GCode command in the sendQueue
		xQueueReceive(sendQueue, &cmd, portMAX_DELAY);

		// Convert the GCode command into an RIT message that can be used to move the arms.
		RITMessage msg = processCommand(cmd);

		// Send the RIT message to the RIT queue
		xQueueSend(ritQueue, &msg, 100);
	}
}

int main(void) {
	prvSetupHardware();

	// Setup motors, pencil, and lasers
	setup();

	// The send queue is used to pass around GCode commands
	sendQueue = xQueueCreate(1, sizeof(Command));
	// The RIT queue is used to move the arms with RIT
	ritQueue = xQueueCreate(1, sizeof(RITMessage));

	// The done semaphore that allows the RIT_Start to stop blocking and return to the next program instructions
	ritDone = xSemaphoreCreateBinary();

	// Spawn the tasks that are needed to run the plotter
	xTaskCreate(taskReadDebug, "vTaskReadDebug", configMINIMAL_STACK_SIZE + 256,
			NULL, (tskIDLE_PRIORITY + 1UL), (TaskHandle_t *) NULL);

	xTaskCreate(taskReceiveCommand, "vTaskReceiveCommand",
			configMINIMAL_STACK_SIZE + 256, NULL, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t *) NULL);

	xTaskCreate(taskMove, "vTaskMove", configMINIMAL_STACK_SIZE + 512, NULL,
			(tskIDLE_PRIORITY + 3UL), (TaskHandle_t *) NULL);

	// Start the program!
	vTaskStartScheduler();

	return 1;
}
