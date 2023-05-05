/**
 * @copyright Copyright (c) 2023 NanoAvionics, LLC. All Rights Reserved.
 *//*
 * support.c
 *
 *  Created on: Jun 12, 2020
 *      Author: asien
 */
#include "support.h"
#include "main.h"
#include "nrf24.h"
#include "ds180lib.h"

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;


uint32_t data_to_send[10];
uint32_t data_received[10];




uint8_t nRF24_payload[32];

// Pipe number
nRF24_RXResult pipe;

uint32_t i,j,k;

// Length of received payload
uint8_t payload_length=20;



void UART_SendStr(char *string) {
	HAL_UART_Transmit(&huart2, (uint8_t *) string, (uint16_t) strlen(string), 200);
}

void UART_SendChar(char b) {
	HAL_UART_Transmit(&huart2, (uint8_t *) &b, 1, 200);
}


void Toggle_LED() {
	HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
}




void UART_SendBufHex(char *buf, uint16_t bufsize) {
	uint16_t i;
	char ch;
	for (i = 0; i < bufsize; i++) {
		ch = *buf++;
		UART_SendChar(HEX_CHARS[(ch >> 4)   % 0x10]);
		UART_SendChar(HEX_CHARS[(ch & 0x0f) % 0x10]);
	}
}
void UART_SendHex8(uint16_t num) {
	UART_SendChar(HEX_CHARS[(num >> 4)   % 0x10]);
	UART_SendChar(HEX_CHARS[(num & 0x0f) % 0x10]);
}

void UART_SendInt(int32_t num) {
	char str[10]; // 10 chars max for INT32_MAX
	int i = 0;
	if (num < 0) {
		UART_SendChar('-');
		num *= -1;
	}
	do str[i++] = (char) (num % 10 + '0'); while ((num /= 10) > 0);
	for (i--; i >= 0; i--) UART_SendChar(str[i]);
}






void runRadio(uint8_t *ROLE)
{

	UART_SendStr("\r\nMeasure DS18B20 temperature and send using nRF24L01 \r\n");

	nRF24_CE_H();
	nRF24_CSN_L();
	Delay_ms(100);
	nRF24_CSN_H();

		// RX/TX disabled

		// Configure the nRF24L01+
		UART_SendStr("Connecting to nRF24L01+ : ");

		if (!nRF24_Check()) {
			UART_SendStr("FAIL\r\n");
//	 		while(1);
		}
		else {UART_SendStr("OK\r\n");}

		if (!nRF24_Check()) {
					UART_SendStr("FAIL\r\n");
					//while(1);
				}
				else {UART_SendStr("OK\r\n");}

		if (!nRF24_Check()) {
							UART_SendStr("FAIL\r\n");
						//	while(1);
						}
						else {UART_SendStr("OK\r\n");}

		// Initialize the nRF24L01 to its default state
		nRF24_Init();


		if (ROLE==1) // TX
		{

			TX_single();

		}

		else
		{
			RX_single();

		}

}

void RX_single(void)

{
	// This is simple receiver with one RX pipe:
		//   - pipe#1 address: '0xE7 0x1C 0xE3'
		//   - payload: 5 bytes
		//   - RF channel: 115 (2515MHz)
		//   - data rate: 250kbps (minimum possible, to increase reception reliability)
		//   - CRC scheme: 2 byte

	    // The transmitter sends a 5-byte packets to the address '0xE7 0x1C 0xE3' without Auto-ACK (ShockBurst disabled)

	    // Disable ShockBurst for all RX pipes
	    nRF24_DisableAA(0xFF);

	    // Set RF channel
	    nRF24_SetRFChannel(115);

	    // Set data rate
	    nRF24_SetDataRate(nRF24_DR_250kbps);

	    // Set CRC scheme
	    nRF24_SetCRCScheme(nRF24_CRC_2byte);

	    // Set address width, its common for all pipes (RX and TX)
	    nRF24_SetAddrWidth(3);

	    // Configure RX PIPE#0
   	    static const uint8_t nRF24_ADDR0[] = { 0xE7, 0x1C, 0xE6 };
  	    nRF24_SetAddr(nRF24_PIPE0, nRF24_ADDR0); // program address for RX pipe #0
   	    nRF24_SetRXPipe(nRF24_PIPE0, nRF24_AA_OFF, payload_length); // Auto-ACK: disabled, payload length: 5 bytes



	    // Set operational mode (PRX == receiver)
	    nRF24_SetOperationalMode(nRF24_MODE_RX);

	    // Wake the transceiver
	    nRF24_SetPowerMode(nRF24_PWR_UP);

	    // Put the transceiver to the RX mode
	    nRF24_CE_H();


	    UART_SendStr("START");
	    nRF24_ClearIRQFlags();

	    int pipe_count=0;
	    uint8_t pipe_nrf=0;
	    // The main loop
	    while (1)
	    {
	    	//
	    	// Constantly poll the status of the RX FIFO and get a payload if FIFO is not empty
	    	//
	    	// This is far from best solution, but it's ok for testing purposes
	    	// More smart way is to use the IRQ pin :)


	    	if (nRF24_GetStatus_RXFIFO() != nRF24_STATUS_RXFIFO_EMPTY)
	    	{
	    	// Get a payload from the transceiver


	    		pipe_nrf = nRF24_ReadPayload(nRF24_payload, &payload_length);

	    		UART_SendStr("\r\n");
	    		UART_SendStr("RCV PIPE#");
	    		UART_SendInt(pipe_nrf);
	    		UART_SendStr("\r\n");
	    		// Clear all pending IRQ flags

	    		// Print a payload contents to UART

	    		UART_SendStr(" PAYLOAD:>");
	    		UART_SendStr(nRF24_payload);
//	    		uint16_t temp_data = (nRF24_payload[1]<<8) + nRF24_payload[0];
//	    		UART_SendInt(temp_data);
	    		UART_SendStr("<\r\n");

	    		nRF24_ClearIRQFlags();


	    	}
	    }

}

void TX_single(void)
{

	// This is simple transmitter (to one logic address):
		//   - TX address: '0xE7 0x1C 0xE3'
		//   - payload: 5 bytes
		//   - RF channel: 115 (2515MHz)
		//   - data rate: 250kbps (minimum possible, to increase reception reliability)
		//   - CRC scheme: 2 byte

	    // The transmitter sends a 5-byte packets to the address '0xE7 0x1C 0xE3' without Auto-ACK (ShockBurst disabled)

	    // Disable ShockBurst for all RX pipes
	    nRF24_DisableAA(0xFF);

	    // Set RF channel
	    nRF24_SetRFChannel(95);

	    // Set data rate
	    nRF24_SetDataRate(nRF24_DR_250kbps);

	    // Set CRC scheme
	    nRF24_SetCRCScheme(nRF24_CRC_2byte);

	    // Set address width, its common for all pipes (RX and TX)
	    nRF24_SetAddrWidth(3);

	    // Configure TX PIPES
	    static const uint8_t nRF24_ADDR_1[] = {0xE5, 0x1C, 0xE5 };
	    nRF24_SetAddr(nRF24_PIPETX, nRF24_ADDR_1); // program TX address

	    // Configure TX PIPE2-4
	    static const uint8_t nRF24_ADDR_2[] = {0xE5, 0x1C, 0xE6 };
	    static const uint8_t nRF24_ADDR_3[] = {0xE5, 0x1C, 0xE7 };
	    static const uint8_t nRF24_ADDR_4[] = {0xE5, 0x1C, 0xE8 };


	    char message[20]="1 2 3 4 5";
	    strcpy(nRF24_payload, message);


	    // Set TX power (maximum)
	    nRF24_SetTXPower(nRF24_TXPWR_0dBm);

	    // Set operational mode (PTX == transmitter)
	    nRF24_SetOperationalMode(nRF24_MODE_TX);

	    // Clear any pending IRQ flags
	    nRF24_ClearIRQFlags();

	    // Wake the transceiver
	    nRF24_SetPowerMode(nRF24_PWR_UP);

  	  float Temperature = 0;
  	  int currentAddrs = 0;

	    while (1) {

	    		// Disable ShockBurst for all RX pipes
				nRF24_DisableAA(0xFF);

				// Set RF channel
				nRF24_SetRFChannel(95);

				// Set data rate
				nRF24_SetDataRate(nRF24_DR_250kbps);

				// Set CRC scheme
				nRF24_SetCRCScheme(nRF24_CRC_2byte);

				// Set address width, its common for all pipes (RX and TX)
				nRF24_SetAddrWidth(3);

				// switch TX address
				if(currentAddrs == 0){
					nRF24_SetAddr(nRF24_PIPETX, nRF24_ADDR_1); // program TX address
				} else if(currentAddrs == 1){
					nRF24_SetAddr(nRF24_PIPETX, nRF24_ADDR_2); // program TX address
				} else if(currentAddrs == 2){
					nRF24_SetAddr(nRF24_PIPETX, nRF24_ADDR_3); // program TX address
				} else {
					nRF24_SetAddr(nRF24_PIPETX, nRF24_ADDR_4); // program TX address
				}

	    	    nRF24_SetTXPower(nRF24_TXPWR_0dBm);

	    	    // Set operational mode (PTX == transmitter)
	    	    nRF24_SetOperationalMode(nRF24_MODE_TX);

	    	    // Clear any pending IRQ flags
	    	    nRF24_ClearIRQFlags();

	    	    // Wake the transceiver
	    	    nRF24_SetPowerMode(nRF24_PWR_UP);
	    	    UART_SendStr("Sending to addres: ");


	    	    char buffer[10];
	    	    sprintf(buffer, "add: %d\r\n", currentAddrs);
	    	    UART_SendStr(buffer);

				// increment if not greater than 3 else reset to 0
				if(currentAddrs < 3){
					currentAddrs++;
				} else {
					currentAddrs = 0;
				}


	    	 Temperature = Read_temp();
	    	sprintf(nRF24_payload, "%2.2f", Temperature);




	    	tx_res = nRF24_TransmitPacket(nRF24_payload, payload_length);
	    	UART_SendStr("PAYLOAD:>");
	    	UART_SendStr(nRF24_payload);
	    	UART_SendStr("\r\n");

	    	switch (tx_res) {
				case nRF24_TX_SUCCESS:
					UART_SendStr("OK");
					Toggle_LED();
					break;
				case nRF24_TX_TIMEOUT:
					UART_SendStr("TIMEOUT");
					break;
				case nRF24_TX_MAXRT:
					UART_SendStr("MAX RETRANSMIT");
					break;
				default:
					UART_SendStr("ERROR");
					break;
			}
	    	UART_SendStr("\r\n");

	    	Delay_ms(1000);
	    }
}


nRF24_TXResult nRF24_TransmitPacket(uint8_t *pBuf, uint8_t length) {
	volatile uint32_t wait = nRF24_WAIT_TIMEOUT;
	uint8_t status;

	// Deassert the CE pin (in case if it still high)
	nRF24_CE_L();

	// Transfer a data from the specified buffer to the TX FIFO
	nRF24_WritePayload(pBuf, length);

	// Start a transmission by asserting CE pin (must be held at least 10us)
	nRF24_CE_H();

	// Poll the transceiver status register until one of the following flags will be set:
	//   TX_DS  - means the packet has been transmitted
	//   MAX_RT - means the maximum number of TX retransmits happened
	// note: this solution is far from perfect, better to use IRQ instead of polling the status
	do {
		status = nRF24_GetStatus();
		if (status & (nRF24_FLAG_TX_DS | nRF24_FLAG_MAX_RT)) {
			break;
		}
	} while (wait--);

	// Deassert the CE pin (Standby-II --> Standby-I)
	nRF24_CE_L();

	if (!wait) {
		// Timeout
		return nRF24_TX_TIMEOUT;
	}

	// Check the flags in STATUS register
	UART_SendStr("[");
	UART_SendHex8(status);
	UART_SendStr("] ");

	// Clear pending IRQ flags
    nRF24_ClearIRQFlags();

	if (status & nRF24_FLAG_MAX_RT) {
		// Auto retransmit counter exceeds the programmed maximum limit (FIFO is not removed)
		return nRF24_TX_MAXRT;
	}

	if (status & nRF24_FLAG_TX_DS) {
		// Successful transmission
		return nRF24_TX_SUCCESS;
	}

	// Some banana happens, a payload remains in the TX FIFO, flush it
	nRF24_FlushTX();

	return nRF24_TX_ERROR;
}
