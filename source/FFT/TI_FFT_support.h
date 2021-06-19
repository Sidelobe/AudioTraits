// From TI's FFT Demo/Benchmark project
// only modifications: commenting out platform specific pragmas and silencing double->float warnings

#ifndef TI_FFT_support_h
#define TI_FFT_support_h

#include <math.h>

#define PI M_PI

/** Bit-Reverse Table */
static const std::array<unsigned char, 64> brev {
    0x0, 0x20, 0x10, 0x30, 0x8, 0x28, 0x18, 0x38,
    0x4, 0x24, 0x14, 0x34, 0xc, 0x2c, 0x1c, 0x3c,
    0x2, 0x22, 0x12, 0x32, 0xa, 0x2a, 0x1a, 0x3a,
    0x6, 0x26, 0x16, 0x36, 0xe, 0x2e, 0x1e, 0x3e,
    0x1, 0x21, 0x11, 0x31, 0x9, 0x29, 0x19, 0x39,
    0x5, 0x25, 0x15, 0x35, 0xd, 0x2d, 0x1d, 0x3d,
    0x3, 0x23, 0x13, 0x33, 0xb, 0x2b, 0x1b, 0x3b,
    0x7, 0x27, 0x17, 0x37, 0xf, 0x2f, 0x1f, 0x3f
};

/** Function for generating Specialized sequence of twiddle factors */
static void tw_gen (float *w, int n)
{
    int i, j, k;
    double x_t, y_t, theta1, theta2, theta3;

    for (j = 1, k = 0; j <= n >> 2; j = j << 2)
    {
        for (i = 0; i < n >> 2; i += j)
        {
            theta1 = 2 * PI * i / n;
            x_t = cos (theta1);
            y_t = sin (theta1);
            w[k] = (float) x_t;
            w[k + 1] = (float) y_t;

            theta2 = 4 * PI * i / n;
            x_t = cos (theta2);
            y_t = sin (theta2);
            w[k + 2] = (float) x_t;
            w[k + 3] = (float) y_t;

            theta3 = 6 * PI * i / n;
            x_t = cos (theta3);
            y_t = sin (theta3);
            w[k + 4] = (float) x_t;
            w[k + 5] = (float) y_t;
            k += 6;
        }
    }
}

static void split_gen (float *pATable, float *pBTable, int n)
{
    int i;

    for (i = 0; i < n; i++)
    {
        pATable[2 * i] = (float) (0.5 * (1.0 - sin (2 * PI / (double) (2 * n) * (double) i)));
        pATable[2 * i + 1] = (float) (0.5 * (-1.0 * cos (2 * PI / (double) (2 * n) * (double) i)));
        pBTable[2 * i] = (float) (0.5 * (1.0 + sin (2 * PI / (double) (2 * n) * (double) i)));
        pBTable[2 * i + 1] = (float) (0.5 * (1.0 * cos (2 * PI / (double) (2 * n) * (double) i)));
    }
}

static void FFT_Split (int n, float *pIn, float *pATable, float *pBTable, float *pOut)
{
    int i;
    float Tr, Ti;

//    _nassert ((int) pIn % 8 == 0);
//    _nassert ((int) pOut % 8 == 0);
//    _nassert ((int) pATable % 8 == 0);
//    _nassert ((int) pBTable % 8 == 0);

    pIn[2 * n] = pIn[0];
    pIn[2 * n + 1] = pIn[1];

//#pragma UNROLL(2)
    for (i = 0; i < n; i++)
    {
        Tr = (pIn[2 * i] * pATable[2 * i] - pIn[2 * i + 1] * pATable[2 * i + 1] + pIn[2 * n - 2 * i] * pBTable[2 * i] + pIn[2 * n - 2 * i + 1] * pBTable[2 * i + 1]);

        Ti = (pIn[2 * i + 1] * pATable[2 * i] + pIn[2 * i] * pATable[2 * i + 1] + pIn[2 * n - 2 * i] * pBTable[2 * i + 1] - pIn[2 * n - 2 * i + 1] * pBTable[2 * i]);

        pOut[2 * i] = Tr;
        pOut[2 * i + 1] = Ti;
        // Use complex conjugate symmetry properties to get the rest..
        pOut[4 * n - 2 * i] = Tr;
        pOut[4 * n - 2 * i + 1] = -Ti;

    }
    pOut[2 * n] = pIn[0] - pIn[1];
    pOut[2 * n + 1] = 0;

}

static void IFFT_Split (int n, const float *pIn, float *pATable, float *pBTable, float *pOut)
{
    int i;
    float Tr, Ti;

//    _nassert ((int) pIn % 8 == 0);
//    _nassert ((int) pOut % 8 == 0);
//    _nassert ((int) pATable % 8 == 0);
//    _nassert ((int) pBTable % 8 == 0);

//#pragma UNROLL(2)
    for (i = 0; i < n; i++)
    {
        Tr = (pIn[2 * i] * pATable[2 * i] + pIn[2 * i + 1] * pATable[2 * i + 1] + pIn[2 * n - 2 * i] * pBTable[2 * i] - pIn[2 * n - 2 * i + 1] * pBTable[2 * i + 1]);

        Ti = (pIn[2 * i + 1] * pATable[2 * i] - pIn[2 * i] * pATable[2 * i + 1] - pIn[2 * n - 2 * i] * pBTable[2 * i + 1] - pIn[2 * n - 2 * i + 1] * pBTable[2 * i]);

        pOut[2 * i] = Tr;
        pOut[2 * i + 1] = Ti;
    }

}

#endif /* TI_FFT_support_h */
