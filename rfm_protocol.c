#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "rtl_rfm.h"
#include "rfm_protocol.h"
#include "fsk.h" // for bool hold

int bytesexpected = 0;
int bitphase = -1;

#define CRC_INIT 0x1D0F
#define CRC_POLY 0x1021
#define CRC_POST 0xFFFF
uint16_t crc = 0x1D0F;

uint16_t thecrc = 0;

void docrc(uint8_t thebyte) {
	crc = crc ^ (thebyte << 8);

	for (int i = 0; i < 8; i++) {
		if (crc & 0x8000) {
			crc <<= 1;
			crc ^= CRC_POLY;
		} else {
			crc <<= 1;
		}
	}
}

uint8_t packet_buffer[256];
uint8_t packet_bi = 0;

void print_sanitize(uint8_t buf[], uint8_t bufi) {
	for (int i = 0; i < bufi; i++) {
		uint8_t chr = buf[i];

		if (chr >= 32) {
			putchar(chr);
		} else {
			printf("[%02X]", chr);
		}
	}
}

void process_byte(uint8_t thebyte) {	
	if (bytesexpected < 0) { //expecting length byte!
		bytesexpected = thebyte + 2; // +2 for crc
		//printf("BE: %d", bytesexpected);
		docrc(thebyte);
	} else if (bytesexpected > 0) {
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
			if (!quiet) printf("CRC OK, <");
			print_sanitize(packet_buffer, packet_bi);
			if (!quiet) printf(">");

			printf("\n");
		} else {
			if (!quiet) printf("CRC FAIL! [%02X] [%02X]\n", crc, thecrc);
		}
		
		//printf("'%.*s'", packet_bi, packet_buffer);
		bitphase = -1; // search for new preamble

		hold = false; // reset offset hold.
	}
}


void rfm_decode(uint8_t thebit) {
	if (debugplot) putchar('C'); 

	static uint8_t thisbyte = 0;
	static uint32_t amble = 0;

	if (bitphase >= 0) {
		thisbyte = (thisbyte << 1) | (thebit & 0b1);

		bitphase++;

		if (bitphase > 7) {
			bitphase = 0;
			process_byte(thisbyte);			
		}
	}

	if (bitphase < 0) {
		amble = (amble << 1) | (thebit & 0b1);

		// "2D4C" = 0010'1101'0100'1100

		if ((amble & 0x0000FFFF) == 0x00002D4C) { // detect 2 sync bytes
			offsethold = latestoffset;
			hold = true;

			if (!quiet) printf(">> GOT SYNC WORD, ");
			float theoffset = latestoffset * (samplerate / 32768.0) / 1000;
			if (!quiet) printf(" (OFFSET %.2fkHz) ", theoffset);

			packet_bi = 0;
			bitphase = 0;
			crc = CRC_INIT;

			//bitphase = -1; // temp
			bytesexpected = -1; // tell the above section to expect length byte
		}
	}
}