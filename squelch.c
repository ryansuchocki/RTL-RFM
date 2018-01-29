#include "rtl_rfm.h"
#include "squelch.h"

#include "rfm_protocol.h"

#define SQUELCH_THRESH 10
#define SQUELCH_NUM 16

bool squelch_state = false; // 0 is squelched, 1 is receiving
int squelch_count = 0;

bool squelch(int32_t magnitude_squared, bool debugplot) {
    if (squelch_state) {
        if (magnitude_squared < (SQUELCH_THRESH * SQUELCH_THRESH)) {
            squelch_count--;
            if(squelch_count <= 0) {
                squelch_state = false;

                if (debugplot) fprintf(stderr, "SQUELCH CLOSE");
                rfm_reset();
            }
        } else {
            squelch_count = SQUELCH_NUM;
        }

        return true;
    } else {
        if (magnitude_squared > (SQUELCH_THRESH * SQUELCH_THRESH)) {
            squelch_count++;
            if (squelch_count >= SQUELCH_NUM) {
                squelch_state = true;

                if (debugplot) fprintf(stderr, "SQUELCH OPEN");
            }
        } else {
            squelch_count = 0;
        }

        return false;
    }
}