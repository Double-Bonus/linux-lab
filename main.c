/*
 * main.c
 *
 *  Created on: May 5, 2023
 *      Author: debian
 */

#include <iostream>
using namespace std;


extern "C" void runRadio(uint8_t *ROLE);

#define TX 1
#define RX 0
#include "support.h"
#include "nrf24.h"

int main() {
	cout << "!!!Hello NRF!!!" << endl; // prints !!!Hello World!!!

	 //spi_initas();
	runRadio(RX);

	return 0;
}
