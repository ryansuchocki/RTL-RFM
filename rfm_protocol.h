#ifndef _RFM_H_GUARD
    #define _RFM_H_GUARD

	void rfm_init(CB_VOID o, CB_VOID c);
    char *rfm_decode(uint8_t thebit);
    void rfm_reset();

#endif