#include "rtl_rfm.h"
#include "rfm_protocol.h"

#define CRC_INIT 0x1D0F
#define CRC_POLY 0x1021
#define CRC_POST 0xFFFF
#define AMBLEMASK 0x0000FFFF
#define AMBLE 0x00002DCA

uint16_t crc = CRC_INIT;
uint16_t thecrc = 0;
CB_VOID open_cb;
CB_VOID close_cb;

uint8_t packet_buffer[256];
uint8_t packet_bi = 0;
int bytesexpected = 0;
int bitphase = -1;
uint8_t thisbyte = 0;
uint32_t amble = 0;

void rfm_init(CB_VOID o, CB_VOID c)
{
    open_cb = o;
    close_cb = c;
}

void docrc(uint8_t thebyte)
{
    crc = crc ^ (thebyte << 8);

    for (int i = 0; i < 8; i++)
    {
        crc = crc & 0x8000 ? (crc << 1) ^ CRC_POLY : crc << 1;
    }
}

char *process_byte(uint8_t thebyte, CB_VOID close_cb)
{
    char *result = NULL;

    if (bytesexpected < 0) //expecting length byte!
    { 
        bytesexpected = thebyte + 2; // +2 for crc
        docrc(thebyte);
        return NULL;
    }

    if (bytesexpected > 0)
    {
        if (bytesexpected > 2)
        {
            packet_buffer[packet_bi++] = thebyte;
            docrc(thebyte);
        }
        else
        {
            thecrc <<= 8;
            thecrc |= thebyte;
        }

        bytesexpected--;
    }

    if (bytesexpected == 0)
    {
        result = malloc(sizeof(char) * (packet_bi + 1));
        memcpy(result, packet_buffer, packet_bi);
        result[packet_bi] = 0;

        crc ^= CRC_POST;
        if (crc == thecrc)
        {
            printv("CRC OK\n");
        } else {
            printv("CRC FAIL! [%02X] [%02X]\n", crc, thecrc);
        }
        
        bitphase = -1; // search for new preamble

        close_cb(); // reset offset hold.
    }

    return result;
}

char *rfm_decode(uint8_t thebit)
{
    if (thebit > 1) return NULL;

    if (bitphase < 0)
    {
        amble = (amble << 1) | (thebit & 0b1);

        if ((amble & AMBLEMASK) == AMBLE)// detect 2 sync bytes "2D4C" = 0010'1101'1100'1010
        {
            open_cb();

            packet_bi = 0;      // reset the packet buffer
            bitphase = 0;       // lock to current bit phase
            crc = CRC_INIT;     // reset CRC engine
            bytesexpected = -1; // tell the byte processor to expect a length byte
        }
    }
    else
    {
        thisbyte = (thisbyte << 1) | (thebit & 0b1);
        bitphase++;

        if (bitphase > 7) 
        {
            bitphase = 0;
            return process_byte(thisbyte, close_cb);          
        }
    }

    return NULL;
}

void rfm_reset()
{
    bytesexpected = 0;      // Length byte not expected yet.
    bitphase = -1;          // search for new preamble
    thisbyte = 0;
    amble = 0;
    packet_bi = 0;
}