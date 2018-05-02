#include "rtl_rfm.h"

void decimate(IQDecimator *d, IQPair sample)
{
    d->acci += sample.i;
    d->accq += sample.q;
    d->count++;

    if (d->count == d->downsample)
    {
        d->samplehandler((IQPair) {
            .i = d->acci / d->downsample,
            .q = d->accq / d->downsample
        }); // divide and convert to signed

        d->count = d->acci = d->accq = 0;
    }
}

#ifdef CHECKNOMACRO
    IQPair complex_conjugate(IQPair arg)
    {
        return (IQPair) {.q=arg.q, .i=-arg.i};
    }

    IQPair complex_product(IQPair arg1, IQPair arg2)
    {
        return (IQPair)
        {
            .q = (arg1.q * arg2.q) - (arg1.i * arg2.i),
            .i = (arg1.q * arg2.i) + (arg1.i * arg2.q)
        };
    }
#endif