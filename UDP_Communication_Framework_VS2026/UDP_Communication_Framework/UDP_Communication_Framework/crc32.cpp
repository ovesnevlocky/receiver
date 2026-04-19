#include "stdafx.h"
#include "crc32.h"

static uint32_t CRC_ReverseBitOrder32(uint32_t value)
{
    uint32_t reversed = 0u;

    for (uint8_t i = 31u; value; )
    {
        reversed |= (value & 1u) << i;
        value >>= 1u;
        --i;
    }

    return reversed;
}

static uint8_t CRC_ReverseBitOrder8(uint8_t value)
{
    value = (value & 0xF0) >> 4u | (value & 0x0F) << 4u;
    value = (value & 0xCC) >> 2u | (value & 0x33) << 2u;
    value = (value & 0xAA) >> 1u | (value & 0x55) << 1u;

    return value;
}


uint32_t CRC_CalculateCRC32(const uint8_t* Buffer, uint16_t Length)
{
    uint32_t retVal = 0u;
    uint16_t byteIndex = 0u;

    if (Buffer != NULL)
    {
        retVal = CRC_32_INIT_VALUE;

        /* Do calculation procedure for each byte */
        for (byteIndex = 0u; byteIndex < Length; byteIndex++)
        {
            /* XOR new byte with temp result */
            retVal ^= (CRC_ReverseBitOrder8(Buffer[byteIndex]) << (CRC_32_RESULT_WIDTH - 8u));

            uint8_t bitIndex = 0u;
            /* Do calculation for current data */
            for (bitIndex = 0u; bitIndex < 8u; bitIndex++)
            {
                if (retVal & (1u << (CRC_32_RESULT_WIDTH - 1u)))
                {
                    retVal = (retVal << 1u) ^ CRC_32_POLYNOMIAL;
                }
                else
                {
                    retVal = (retVal << 1u);
                }
            }
        }
    }
    else
        exit(1);
    /* XOR result with specified value */
    retVal ^= CRC_32_XOR_VALUE;

    /* Reflect result */
    retVal = CRC_ReverseBitOrder32(retVal);

    return retVal;
}