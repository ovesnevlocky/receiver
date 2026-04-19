#pragma once

#include "stdafx.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
//#include <unistd.h>

#define RUNTTIME                            0
#define TABLE                               1
#define HARDWARE                            2

#define CRC_32_RESULT_WIDTH                 32u
#define CRC_32_POLYNOMIAL                   0x04C11DB7u
#define CRC_32_INIT_VALUE                   0xFFFFFFFFu
#define CRC_32_XOR_VALUE                    0xFFFFFFFFu
#define CRC_32_MODE                         RUNTTIME


static uint32_t CRC_ReverseBitOrder32(uint32_t value);

static uint8_t CRC_ReverseBitOrder8(uint8_t value);

uint32_t CRC_CalculateCRC32(const uint8_t* Buffer, uint16_t Length);