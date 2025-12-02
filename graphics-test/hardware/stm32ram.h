#ifndef STM32RAM_H
#define STM32RAM_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define SYSRAM_SIZE     (4 * 1024 * 1024)

extern uint8_t SYSRAM[SYSRAM_SIZE]; // the virtual memory

#endif // STM32RAM_H
