#!/bin/bash
rm *.o
g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"main.d" -MT"main.o" -o "main.o" "main.c" -lpaho-mqtt3c
gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"nrf24.d" -MT"nrf24.o" -o nrf24.o nrf24.c
gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"support.d" -MT"support.o" -o support.o support.c
g++ -o "lab3_rf" main.o nrf24.o support.o -lpaho-mqtt3c
