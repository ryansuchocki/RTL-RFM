#ifndef _IQ16_GUARD
    #define _IQ16_GUARD

    //#define CHECKNOMACRO

    #define TAU INT16_MAX * 2

    typedef struct IQPair
    {
        int16_t i;
        int16_t q;
    } IQPair;

    #ifdef CHECKNOMACRO
        IQPair complex_conjugate(IQPair arg);
        IQPair complex_product(IQPair arg1, IQPair arg2);

        #define IQPAIR_CONJUGATE complex_conjugate
        #define IQPAIR_PRODUCT complex_product
    #else
        #define IQPAIR_CONJUGATE(x)\
        ((IQPair) {\
            .q=x.q,\
            .i=-x.i\
        })

        #define IQPAIR_PRODUCT(x, y)\
        ((IQPair) {\
            .q=(x.q*y.q)-(x.i*y.i),\
            .i=(x.q*y.i)+(x.i*y.q)\
        })
    #endif

    typedef void (*SampleHandler)(IQPair sample);

    typedef struct IQDecimatorS
    {
        uint32_t acci, accq;
        uint16_t count, downsample;
        SampleHandler samplehandler;
    } IQDecimator;

    void decimate(IQDecimator *d, IQPair sample);

#endif