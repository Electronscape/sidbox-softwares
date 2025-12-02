#include "stm32ram.h"

// 4MEG system ram for testing, Sidbox has 6MEG of user RAM, 2 is for the OS and API, and 1 MEG for the Firmware stack

uint8_t SYSRAM[SYSRAM_SIZE];
