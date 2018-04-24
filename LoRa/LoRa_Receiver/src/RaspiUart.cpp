/*
 * RaspiUart.cpp
 *
 *  Created on: 29.3.2018
 *      Author: Jake
 */

#include "RaspiUart.h"

#define LPC_USART       LPC_USART1
#define LPC_IRQNUM      UART1_IRQn
#define LPC_UARTHNDLR   UART1_IRQHandler

/* Enable this define to use integer clocking instead of the fractional baud
   rate generator */
#define USE_INTEGER_CLOCK

static RINGBUFF_T *rxring1;
static RINGBUFF_T *txring1;

extern "C" {
	/**
	 * @brief	UART interrupt handler using ring buffers
	 * @return	Nothing
	 */
	void LPC_UARTHNDLR(void) {
		/* Want to handle any errors? Do it here. */

		/* Use default ring buffer handler. Override this with your own
		   code if you need more capability. */
		Chip_UART_IRQRBHandler(LPC_USART, rxring1, txring1);
	}
}

RaspiUart::RaspiUart() {
	Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 10, (IOCON_MODE_INACT | IOCON_DIGMODE_EN));
	Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 9, (IOCON_MODE_INACT | IOCON_DIGMODE_EN));

	/* UART signal muxing via SWM */
	Chip_SWM_MovablePortPinAssign(SWM_UART1_RXD_I, 1, 10);
	Chip_SWM_MovablePortPinAssign(SWM_UART1_TXD_O, 1, 9);

	/* Before setting up the UART, the global UART clock for USARTS 1-4
	   must first be setup. This requires setting the UART divider and
	   the UART base clock rate to 16x the maximum UART rate for all
	   UARTs. */
	#if defined(USE_INTEGER_CLOCK)
		/* Use main clock rate as base for UART baud rate divider */
		Chip_Clock_SetUARTBaseClockRate(Chip_Clock_GetMainClockRate(), false);

	#else
		/* Use 128x expected UART baud rate for fractional baud mode. */
		Chip_Clock_SetUARTBaseClockRate((115200 * 128), true);
	#endif

	/* Setup UART */
	Chip_UART_Init(LPC_USART);
	Chip_UART_ConfigData(LPC_USART, UART_CFG_DATALEN_8 | UART_CFG_PARITY_NONE | UART_CFG_STOPLEN_2);
	Chip_UART_SetBaud(LPC_USART, 9600);

	/* Optional for low clock rates only: Chip_UART_SetBaudWithRTC32K(LPC_USART, 300); */
	Chip_UART_Enable(LPC_USART);
	Chip_UART_TXEnable(LPC_USART);

	/* Before using the ring buffers, initialize them using the ring
	   buffer init function */
	RingBuffer_Init(&rxring, rxbuff, 1, UART_RB_SIZE);
	RingBuffer_Init(&txring, txbuff, 1, UART_RB_SIZE);

	/* set the fixed pointers for ISR
	 * a fixed ISR binds this object to a single UART, which is not proper OO programming
	 */
	rxring1 = &rxring;
	txring1 = &txring;

	/* Enable receive data and line status interrupt */
	Chip_UART_IntEnable(LPC_USART, UART_INTEN_RXRDY);
	Chip_UART_IntDisable(LPC_USART, UART_INTEN_TXRDY);	/* May not be needed */

	/* Enable UART interrupt */
	NVIC_EnableIRQ(LPC_IRQNUM);
}

RaspiUart::~RaspiUart() {
	/* DeInitialize UART peripheral */
	NVIC_DisableIRQ(LPC_IRQNUM);
	Chip_UART_DeInit(LPC_USART);
}

int RaspiUart::available() {
	return RingBuffer_GetCount(&rxring);
}

void RaspiUart::begin(int speed) {
	Chip_UART_SetBaud(LPC_USART, speed);
}

int RaspiUart::read() {
	uint8_t byte;
	int value;
	value = Chip_UART_ReadRB(LPC_USART, &rxring, &byte, 1);
	if(value > 0) return (byte);
	return -1;
}

int RaspiUart::write(const char* buf, int len) {
	Chip_UART_SendRB(LPC_USART, &txring, buf, len);
	return len;
}

int RaspiUart::print(int val, int format) {
	uint8_t byte = val;
	Chip_UART_SendRB(LPC_USART, &txring, &byte, 1);
	return (0);
}

void RaspiUart::flush() {
	while(RingBuffer_GetCount(&txring)>0) __WFI();
}


