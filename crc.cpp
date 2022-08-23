#include "crc.h"

u16 crc16(const u8* data, u32 length)
{
    u16 crc = 0xFFFF;

    for (u32 i = 0; i < length; i++) {
        crc ^= data[i];

        for (u32 j = 0; j < 8; j++)
            crc = (crc >> 1) ^ ((crc & 1) ? 0xA001 : 0);
    }

    return crc;
}
