#include "rtl_rfm.h"
#include "squelch.h"

#define SQUELCH_THRESH 10
#define SQUELCH_NUM 16

bool squelch_state = false; // 0 is squelched, 1 is receiving
int squelch_count = 0;

int32_t magnitude_squared;
extern bool debugplot;

bool squelch(IQPair sample, CB_VOID open_cb)
{
    magnitude_squared = sample.i * sample.i + sample.q * sample.q;

    if (squelch_state) {
        if (magnitude_squared < (SQUELCH_THRESH * SQUELCH_THRESH)) {
            squelch_count--;
            if(squelch_count <= 0) {
                squelch_state = false;

                if (debugplot) fprintf(stderr, "SQUELCH CLOSE");
                open_cb();
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