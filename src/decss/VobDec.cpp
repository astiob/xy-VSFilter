#include "StdAfx.h"
#include "vobdec.h"

static BYTE reverse[0x100], table[0x100] = {
    0x33, 0x73, 0x3B, 0x26, 0x63, 0x23, 0x6B, 0x76, 0x3E, 0x7E, 0x36, 0x2B, 0x6E, 0x2E, 0x66, 0x7B,
    0xD3, 0x93, 0xDB, 0x06, 0x43, 0x03, 0x4B, 0x96, 0xDE, 0x9E, 0xD6, 0x0B, 0x4E, 0x0E, 0x46, 0x9B,
    0x57, 0x17, 0x5F, 0x82, 0xC7, 0x87, 0xCF, 0x12, 0x5A, 0x1A, 0x52, 0x8F, 0xCA, 0x8A, 0xC2, 0x1F,
    0xD9, 0x99, 0xD1, 0x00, 0x49, 0x09, 0x41, 0x90, 0xD8, 0x98, 0xD0, 0x01, 0x48, 0x08, 0x40, 0x91,
    0x3D, 0x7D, 0x35, 0x24, 0x6D, 0x2D, 0x65, 0x74, 0x3C, 0x7C, 0x34, 0x25, 0x6C, 0x2C, 0x64, 0x75,
    0xDD, 0x9D, 0xD5, 0x04, 0x4D, 0x0D, 0x45, 0x94, 0xDC, 0x9C, 0xD4, 0x05, 0x4C, 0x0C, 0x44, 0x95,
    0x59, 0x19, 0x51, 0x80, 0xC9, 0x89, 0xC1, 0x10, 0x58, 0x18, 0x50, 0x81, 0xC8, 0x88, 0xC0, 0x11,
    0xD7, 0x97, 0xDF, 0x02, 0x47, 0x07, 0x4F, 0x92, 0xDA, 0x9A, 0xD2, 0x0F, 0x4A, 0x0A, 0x42, 0x9F,
    0x53, 0x13, 0x5B, 0x86, 0xC3, 0x83, 0xCB, 0x16, 0x5E, 0x1E, 0x56, 0x8B, 0xCE, 0x8E, 0xC6, 0x1B,
    0xB3, 0xF3, 0xBB, 0xA6, 0xE3, 0xA3, 0xEB, 0xF6, 0xBE, 0xFE, 0xB6, 0xAB, 0xEE, 0xAE, 0xE6, 0xFB,
    0x37, 0x77, 0x3F, 0x22, 0x67, 0x27, 0x6F, 0x72, 0x3A, 0x7A, 0x32, 0x2F, 0x6A, 0x2A, 0x62, 0x7F,
    0xB9, 0xF9, 0xB1, 0xA0, 0xE9, 0xA9, 0xE1, 0xF0, 0xB8, 0xF8, 0xB0, 0xA1, 0xE8, 0xA8, 0xE0, 0xF1,
    0x5D, 0x1D, 0x55, 0x84, 0xCD, 0x8D, 0xC5, 0x14, 0x5C, 0x1C, 0x54, 0x85, 0xCC, 0x8C, 0xC4, 0x15,
    0xBD, 0xFD, 0xB5, 0xA4, 0xED, 0xAD, 0xE5, 0xF4, 0xBC, 0xFC, 0xB4, 0xA5, 0xEC, 0xAC, 0xE4, 0xF5,
    0x39, 0x79, 0x31, 0x20, 0x69, 0x29, 0x61, 0x70, 0x38, 0x78, 0x30, 0x21, 0x68, 0x28, 0x60, 0x71,
    0xB7, 0xF7, 0xBF, 0xA2, 0xE7, 0xA7, 0xEF, 0xF2, 0xBA, 0xFA, 0xB2, 0xAF, 0xEA, 0xAA, 0xE2, 0xFF,
};

CVobDec::CVobDec()
{
    m_fFoundKey = false;

    for (DWORD loop0 = 0; loop0 < 0x100; loop0++) {
        BYTE value = 0;

        for (DWORD loop1 = 0; loop1 < 8; loop1++) {
            value |= ((loop0 >> loop1) & 1) << (7 - loop1);
        }

        reverse[loop0] = value;
    }

}

CVobDec::~CVobDec()
{
}

void CVobDec::ClockLfsr0Forward(int& lfsr0)
{
    int temp = (lfsr0 << 3) | (lfsr0 >> 14);
    lfsr0 = (lfsr0 >> 8) | ((((((temp << 3) ^ temp) << 3) ^ temp ^ lfsr0) & 0xFF) << 9);
}

void CVobDec::ClockLfsr1Forward(int& lfsr1)
{
    lfsr1 = (lfsr1 >> 8) | ((((((((lfsr1 >> 8) ^ lfsr1) >> 1) ^ lfsr1) >> 3) ^ lfsr1) & 0xFF) << 17);
}

void CVobDec::ClockBackward(int& lfsr0, int& lfsr1)
{
    int temp0, temp1;

    lfsr0 = ((lfsr0 << 8) ^ ((((lfsr0 >> 3) ^ lfsr0) >> 6) & 0xFF)) & ((1 << 17) - 1);
    temp0 = ((lfsr1 >> 17) ^ (lfsr1 >> 4)) & 0xFF;
    temp1 = (lfsr1 << 5) | (temp0 >> 3);
    temp1 = ((temp1 >> 1) ^ temp1) & 0xFF;
    lfsr1 = ((lfsr1 << 8) | ((((((temp1 >> 2) ^ temp1) >> 1) ^ temp1) >> 3) ^ temp1 ^ temp0)) & ((1 << 25) - 1);
}

void CVobDec::Salt(const BYTE salt[5], int& lfsr0, int& lfsr1)
{
    lfsr0 ^= (reverse[salt[0]] << 9) | reverse[salt[1]];
    lfsr1 ^= ((reverse[salt[2]] & 0xE0) << 17) | ((reverse[salt[2]] & 0x1F) << 16) | (reverse[salt[3]] << 8) | reverse[salt[4]];
}

int CVobDec::FindLfsr(const BYTE* crypt, int offset, const BYTE* plain)
{
    int loop0, loop1, lfsr0, lfsr1, carry, count;

    for (loop0 = count = 0; loop0 != (1 << 18); loop0++) {
        lfsr0 = loop0 >> 1;
        carry = loop0 & 0x01;

        for (loop1 = lfsr1 = 0; loop1 != 4; loop1++) {
            ClockLfsr0Forward(lfsr0);
            carry = (table[crypt[offset + loop1]] ^ plain[loop1]) - ((lfsr0 >> 9) ^ 0xFF) - carry;
            lfsr1 = (lfsr1 >> 8) | ((carry & 0xFF) << 17);
            carry = (carry >> 8) & 0x01;
        }
        for (; loop1 != 7; loop1++) {
            ClockLfsr0Forward(lfsr0);
            ClockLfsr1Forward(lfsr1);
            carry += ((lfsr0 >> 9) ^ 0xFF) + (lfsr1 >> 17);
            if ((carry & 0xFF) != (table[crypt[offset + loop1]] ^ plain[loop1])) {
                break;
            }
            carry >>= 8;
        }
        if (loop1 == 7) {
            for (loop1 = 0; loop1 != 6; loop1++) {
                ClockBackward(lfsr0, lfsr1);
            }
            carry = ((lfsr0 >> 9) ^ 0xFF) + (lfsr1 >> 17) + (loop0 & 0x01);
            if ((carry & 0xFF) == (table[crypt[offset]] ^ plain[0])) {
                for (loop1 = 0; loop1 != offset + 1; loop1++) {
                    ClockBackward(lfsr0, lfsr1);
                }
                if (lfsr0 & 0x100 && lfsr1 & 0x200000) {
                    m_lfsr0 = lfsr0;
                    m_lfsr1 = lfsr1;
                    count++;
                }
            }
        }
    }

    return count;
}

bool CVobDec::FindKey(BYTE* buff)
{
    BYTE plain[7] = {0x00, 0x00, 0x01, 0xBE, 0x00, 0x00, 0xFF};
    int offset, left, flag = 0, block = 0, count, maxblock = 20000;

    m_fFoundKey = false;

    if (buff[0x14] & 0x30) {
        flag |= 0x01;

        if (*(DWORD*)&buff[0x00] == 0xba010000 && (*(DWORD*)&buff[0x0e] & 0xffffff) == 0x010000) {
            offset = 0x14 + (buff[0x12] << 8) + buff[0x13];
            if (0x80 <= offset && offset <= 0x7F9) {
                flag |= 0x02;
                left = 0x800 - offset - 6;
                plain[4] = (char)(left >> 8);
                plain[5] = (char)left;
                if ((count = FindLfsr(buff + 0x80, offset - 0x80, plain)) == 1) {
                    Salt(buff + 0x54, m_lfsr0, m_lfsr1);
                    m_fFoundKey = true;
                } else if (count) {
                    //                  printf(_T("\rblock %d reported %d possible keys, skipping\n"), block, count);
                }
            }
        }
    }

    return (m_fFoundKey);
}

void CVobDec::Decrypt(BYTE* buff)
{
    if (buff[0x14] & 0x30) {
        buff[0x14] &= ~0x30;

        int lfsr0 = m_lfsr0, lfsr1 = m_lfsr1;

        Salt(buff + 0x54, lfsr0, lfsr1);

        buff += 0x80;

        for (int loop0 = 0, carry = 0; loop0 != 0x800 - 0x80; loop0++, buff++) {
            ClockLfsr0Forward(lfsr0);
            ClockLfsr1Forward(lfsr1);
            carry += ((lfsr0 >> 9) ^ 0xFF) + (lfsr1 >> 17);
            *buff = BYTE(table[*buff] ^ carry);
            carry >>= 8;
        }
    }
}
