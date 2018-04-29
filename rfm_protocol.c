#include "rtl_rfm.h"
#include "rfm_protocol.h"

#include "fsk.h" // for filters

int bytesexpected = 0;
int bitphase = -1;

#define CRC_INIT 0x1D0F
#define CRC_POLY 0x1021
#define CRC_POST 0xFFFF
uint16_t crc = CRC_INIT;

uint16_t thecrc = 0;

void docrc(uint8_t thebyte) {
    crc = crc ^ (thebyte << 8);

    for (int i = 0; i < 8; i++) {
        crc <<= 1;
        if (crc & 0x8000) crc ^= CRC_POLY;
    }
}

uint8_t packet_buffer[256];
uint8_t packet_bi = 0;

void print_sanitize(bool force, uint8_t buf[], uint8_t bufi) {
    for (int i = 0; i < bufi; i++) {
        uint8_t chr = buf[i];

        if (force)
        {
            if (chr >= 32) {
                putchar(chr);
            } else {
                printf("[%02X]", chr);
            }
        }
        else
        {
            if (chr >= 32) {
                printv("%c", chr);
            } else {
                printv("[%02X]", chr);
            }
        }
    }
}

void process_byte(uint8_t thebyte) {    
    if (bytesexpected < 0) { //expecting length byte!
        bytesexpected = thebyte + 2; // +2 for crc
        docrc(thebyte);
        return;
    }

    if (bytesexpected > 0) {
        if (bytesexpected > 2) {
            packet_buffer[packet_bi++] = thebyte;
            docrc(thebyte);
        } else {
            thecrc <<= 8;
            thecrc |= thebyte;
        }

        bytesexpected--;
    }

    if (bytesexpected == 0) {
        crc ^= CRC_POST;
        if (crc == thecrc) {
            printv("CRC OK, \t<");
            print_sanitize(true, packet_buffer, packet_bi);
            printv(">");
            printf("\n");
        } else {
            printv("CRC FAIL! [%02X] [%02X] \t<", crc, thecrc);
            print_sanitize(false, packet_buffer, packet_bi);
            printv(">\n");
        }
        
        bitphase = -1; // search for new preamble

        hipass_filter.hold = false; // reset offset hold.
    }
}

uint8_t thisbyte = 0;
uint32_t amble = 0;

extern int samplerate;
extern int freq;

void rfm_decode(uint8_t thebit) {
    if (thebit > 1) return;

    if (bitphase < 0) {
        amble = (amble << 1) | (thebit & 0b1);

        if ((amble & 0x0000FFFF) == 0x00002D4C) { // detect 2 sync bytes "2D4C" = 0010'1101'0100'1100
            hipass_filter.hold = true;

            printv(">> GOT SYNC WORD, ");
            float foffset = (float) hipass_filter.counthold * (float) samplerate / (float) INT16_MAX / (float) hipass_filter.size / 2.0;
            float eppm = 1000000.0 * foffset / freq;
            printv("(OFFSET %.0fHz = %.1f PPM)", foffset, eppm);

            packet_bi = 0;      // reset the packet buffer
            bitphase = 0;       // lock to current bit phase
            crc = CRC_INIT;     // reset CRC engine
            bytesexpected = -1; // tell the byte processor to expect a length byte
        }
    } else {
        thisbyte = (thisbyte << 1) | (thebit & 0b1);
        bitphase++;

        if (bitphase > 7) {
            bitphase = 0;
            process_byte(thisbyte);          
        }
    }
}

void rfm_reset() {
    bytesexpected = 0;      // Length byte not expected yet.
    bitphase = -1;          // search for new preamble
    hipass_filter.hold = false;    // reset offset hold.
    thisbyte = 0;
    amble = 0;
}