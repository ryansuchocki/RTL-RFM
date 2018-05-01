
    #define TAU INT16_MAX * 2

    typedef struct IQPair
    {
        int16_t i;
        int16_t q;
    } IQPair;

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

    #ifdef NOMACROS
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

        #define IQPAIR_CONJUGATE complex_conjugate
        #define IQPAIR_PRODUCT complex_product
    #endif