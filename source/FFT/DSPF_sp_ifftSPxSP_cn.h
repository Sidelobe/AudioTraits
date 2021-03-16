#ifndef _DSPF_SP_ifftSPxSP_CN_H_
#define _DSPF_SP_ifftSPxSP_CN_H_

/* ========================================================================*/
/*  TEXAS INSTRUMENTS, INC.                                                */
/*                                                                         */
/*  NAME                                                                   */
/*      DSPF_sp_ifftSPxSP -- Single Precision floating point mixed radix   */
/*      inverse FFT with complex input                                     */
/*                                                                         */
/*  USAGE                                                                  */
/*          This routine is C-callable and can be called as:               */
/*                                                                         */
/*          void DSPF_sp_ifftSPxSP(                                        */
/*              int N, float * ptr_x, float * ptr_w, float * ptr_y,        */
/*              unsigned char * brev, int n_min, int offset, int n_max);   */
/*                                                                         */
/*          N = length of ifft in complex samples, power of 2 such that    */
/*              N>=8 and N <= 16384.                                       */
/*          ptr_x = pointer to complex data input (normal order)           */
/*          ptr_w = pointer to complex twiddle factor (see below)          */
/*          ptr_y = pointer to complex output data (normal order)          */
/*          brev = pointer to bit reverse table containing 64 entries      */
/*          n_min = smallest ifft butterfly used in computation            */
/*                  used for decomposing ifft into subiffts, see notes     */
/*          offset = index in complex samples of sub-ifft from start of    */
/*                   main ifft                                             */
/*          n_max = size of main ifft in complex samples                   */
/*                                                                         */
/*  DESCRIPTION                                                            */
/*         The benchmark performs a mixed radix forwards ifft using        */
/*         a special sequece of coefficients generated in the following    */
/*         way:                                                            */
/*                                                                         */
/*         //generate vector of twiddle factors for optimized algorithm//  */
/*          void tw_gen(float * w, int N)                                  */
/*          {                                                              */
/*            int j, k;                                                    */
/*            double x_t, y_t, theta1, theta2, theta3;                     */
/*            const double PI = 3.141592654;                               */
/*                                                                         */
/*            for (j=1, k=0; j <= N>>2; j = j<<2)                          */
/*            {                                                            */
/*                for (i=0; i < N>>2; i+=j)                                */
/*                {                                                        */
/*                    theta1 = 2*PI*i/N;                                   */
/*                    x_t = cos(theta1);                                   */
/*                    y_t = sin(theta1);                                   */
/*                    w[k]   =  (float)x_t;                                */
/*                    w[k+1] =  (float)y_t;                                */
/*                                                                         */
/*                    theta2 = 4*PI*i/N;                                   */
/*                    x_t = cos(theta2);                                   */
/*                    y_t = sin(theta2);                                   */
/*                    w[k+2] =  (float)x_t;                                */
/*                    w[k+3] =  (float)y_t;                                */
/*                                                                         */
/*                    theta3 = 6*PI*i/N;                                   */
/*                    x_t = cos(theta3);                                   */
/*                    y_t = sin(theta3);                                   */
/*                    w[k+4] =  (float)x_t;                                */
/*                    w[k+5] =  (float)y_t;                                */
/*                    k+=6;                                                */
/*                }                                                        */
/*            }                                                            */
/*          }                                                              */
/*        This redundant set of twiddle factors is size 2*N float samples. */
/*        The function is accurate to about 130dB of signal to noise ratio */
/*        to the IDFT function below:                                      */
/*                                                                         */
/*          void idft(int n, float x[], float y[])                         */
/*          {                                                              */
/*            int k,i, index;                                              */
/*            const float PI = 3.14159654;                                 */
/*            float * p_x;                                                 */
/*            float arg, fx_0, fx_1, fy_0, fy_1, co, si;                   */
/*                                                                         */
/*            for(k = 0; k<n; k++)                                         */
/*            {                                                            */
/*              p_x = x;                                                   */
/*              fy_0 = 0;                                                  */
/*              fy_1 = 0;                                                  */
/*              for(i=0; i<n; i++)                                         */
/*              {                                                          */
/*                fx_0 = p_x[0];                                           */
/*                fx_1 = p_x[1];                                           */
/*                p_x += 2;                                                */
/*                index = (i*k) % n;                                       */
/*                arg = 2*PI*index/n;                                      */
/*                co = cos(arg);                                           */
/*                si = sin(arg);                                           */
/*                fy_0 += ((fx_0 * co) - (fx_1 * si));                     */
/*                fy_1 += ((fx_1 * co) + (fx_0 * si));                     */
/*              }                                                          */
/*              y[2*k] = fy_0/n;                                           */
/*              y[2*k+1] = fy_1/n;                                         */
/*            }                                                            */
/*         }                                                               */
/*                                                                         */
/*         The function takes the table and input data and calculates the  */
/*         ifft producing the frequency domain data in the Y array. the    */
/*         output is scaled by a scaling factor of 1/N.                    */
/*                                                                         */
/*         As the ifft allows every input point to effect every output     */
/*         point in a cache based system such as the c6711, this causes    */
/*         cache thrashing. This is mitigated by allowing the main ifft    */
/*         of size N to be divided into several steps, allowing as much    */
/*         data reuse as possible.                                         */
/*                                                                         */
/*         For example the following function:                             */
/*                                                                         */
/*         DSPF_sp_ifftSPxSP(1024, &x[0],&w[0],y,brev,4,  0,1024)          */
/*                                                                         */
/*         is equvalent to:                                                */
/*                                                                         */
/*         DSPF_sp_ifftSPxSP(1024,&x[2*0],&w[0],y,brev,256,0,1024)         */
/*         DSPF_sp_ifftSPxSP(256,&x[2*0],&w[2*768],y,brev,4,0,1024)        */
/*         DSPF_sp_ifftSPxSP(256,&x[2*256],&w[2*768],y,brev,4,256,1024)    */
/*         DSPF_sp_ifftSPxSP(256,&x[2*512],&w[2*768],y,brev,4,512,1024)    */
/*         DSPF_sp_ifftSPxSP(256,&x[2*768],&w[2*768],y,brev,4,768,1024)    */
/*                                                                         */
/*         Notice how the 1st ifft function is called on the entire 1K     */
/*         data set it covers the 1st pass of the ifft until the butterfly */
/*         size is 256. The following 4 iffts do 256 pt iffts 25% of the   */
/*         size. These continue down to the end when the buttefly is of    */
/*         size 4. They use an index to the main twiddle factor array of   */
/*         0.75*2*N. This is because the twiddle factor array is composed  */
/*         of successively decimated versions of the main array.           */
/*                                                                         */
/*         N not equal to a power of 4 can be used, i.e. 512. In this case */
/*         to decompose the ifft the following would be needed :           */
/*                                                                         */
/*         DSPF_sp_ifftSPxSP(512, &x[0],&w[0],y,brev,2,  0,512)            */
/*                                                                         */
/*         is equvalent to:                                                */
/*                                                                         */
/*         DSPF_sp_ifftSPxSP(512, &x[2*0],  &w[0] ,   y,brev,128,  0,512)  */
/*         DSPF_sp_ifftSPxSP(128, &x[2*0],  &w[2*384],y,brev,4,    0,512)  */
/*         DSPF_sp_ifftSPxSP(128, &x[2*128],&w[2*384],y,brev,4,  128,512)  */
/*         DSPF_sp_ifftSPxSP(128, &x[2*256],&w[2*384],y,brev,4,  256,512)  */
/*         DSPF_sp_ifftSPxSP(128, &x[2*384],&w[2*384],y,brev,4,  384,512)  */
/*                                                                         */
/*         The twiddle factor array is composed of log4(N) sets of twiddle */
/*         factors, (3/4)*N, (3/16)*N, (3/64)*N, etc.  The index into this */
/*         array for each stage of the ifft is calculated by summing these */
/*         indices up appropriately.                                       */
/*         For multiple iffts they can share the same table by calling the */
/*         small iffts from further down in the twiddle factor array. In   */
/*         the same way as the decomposition works for more data reuse.    */
/*                                                                         */
/*         Thus, the above decomposition can be summarized for a general N */
/*         radix "rad" as follows:                                         */
/*         DSPF_sp_ifftSPxSP(N,  &x[0],      &w[0],      y,brev,N/4,0,   N)*/
/*         DSPF_sp_ifftSPxSP(N/4,&x[0],      &w[2*3*N/4],y,brev,rad,0,   N)*/
/*         DSPF_sp_ifftSPxSP(N/4,&x[2*N/4],  &w[2*3*N/4],y,brev,rad,N/4, N)*/
/*         DSPF_sp_ifftSPxSP(N/4,&x[2*N/2],  &w[2*3*N/4],y,brev,rad,N/2, N)*/
/*        DSPF_sp_ifftSPxSP(N/4,&x[2*3*N/4],&w[2*3*N/4],y,brev,rad,3*N/4,N)*/
/*                                                                         */
/*         As discussed previously, N can be either a power of 4 or 2.     */
/*         If N is a power of 4, then rad = 4, and if N is a power of 2    */
/*         and not a power of 4, then rad = 2. "rad" is used to control    */
/*         how many stages of decomposition are performed. It is also      */
/*         used to determine whether a radix-4 or radix-2 decomposition    */
/*         should be performed at the last stage. Hence when "rad" is set  */
/*         to "N/4" the first stage of the transform alone is performed    */
/*         and the code exits. To complete the FFT, four other calls are   */
/*         required to perform N/4 size FFTs.In fact, the ordering of      */
/*         these 4 FFTs amongst themselves does not matter and hence from  */
/*         a cache perspective, it helps to go through the remaining 4     */
/*         FFTs in exactly the opposite order to the first. This is        */
/*         illustrated as follows:                                         */
/*                                                                         */
/*        DSPF_sp_ifftSPxSP(N,  &x[0],      &w[0],      y,brev,N/4,0,    N)*/
/*        DSPF_sp_ifftSPxSP(N/4,&x[2*3*N/4],&w[2*3*N/4],y,brev,rad,3*N/4,N)*/
/*        DSPF_sp_ifftSPxSP(N/4,&x[2*N/2],  &w[2*3*N/4],y,brev,rad,N/2,  N)*/
/*        DSPF_sp_ifftSPxSP(N/4,&x[2*N/4],  &w[2*3*N/4],y,brev,rad,N/4,  N)*/
/*        DSPF_sp_ifftSPxSP(N/4,&x[0],      &w[2*3*N/4],y,brev,rad,0,    N)*/
/*         In addition this function can be used to minimize call overhead,*/
/*         by completing the FFT with one function call invocation as      */
/*         shown below:                                                    */
/*        DSPF_sp_ifftSPxSP(N,  &x[0],      &w[0],      y, brev, rad, 0,N) */
/*                                                                         */
/* ------------------------------------------------------------------------*/
/*            Copyright (c) 2002 Texas Instruments, Incorporated.          */
/*                           All Rights Reserved.                          */
/* ========================================================================*/

void DSPF_sp_ifftSPxSP(int N, float * ptr_x, float * ptr_w, float * ptr_y,
                       unsigned char * brev, int n_min, int offset, int n_max)
{
    int  i, j, k, l1, l2, h2, predj;
    int  tw_offset, stride, fft_jmp;
    
    float x0, x1, x2, x3,x4,x5,x6,x7;
    float xt0, yt0, xt1, yt1, xt2, yt2, yt3;
    float yt4, yt5, yt6, yt7;
    float si1,si2,si3,co1,co2,co3;
    float xh0,xh1,xh20,xh21,xl0,xl1,xl20,xl21;
    float x_0, x_1, x_l1, x_l1p1, x_h2 , x_h2p1, x_l2, x_l2p1;
    float xl0_0, xl1_0, xl0_1, xl1_1;
    float xh0_0, xh1_0, xh0_1, xh1_1;
    float *x,*w;
    int   k0, k1, j0, j1, l0, radix;
    float * y0, * ptr_x0, * ptr_x2;
    float scale;
    
    radix = n_min;
    
    stride = N; /* n is the number of complex samples*/
    tw_offset = 0;
    while (stride > radix)
    {
        j = 0;
        fft_jmp = stride + (stride>>1);
        h2 = stride>>1;
        l1 = stride;
        l2 = stride + (stride>>1);
        x = ptr_x;
        w = ptr_w + tw_offset;
        
        for (i = 0; i < N; i += 4)
        {
            co1 = w[j];
            si1 = w[j+1];
            co2 = w[j+2];
            si2 = w[j+3];
            co3 = w[j+4];
            si3 = w[j+5];
            
            x_0    = x[0];
            x_1    = x[1];
            x_h2   = x[h2];
            x_h2p1 = x[h2+1];
            x_l1   = x[l1];
            x_l1p1 = x[l1+1];
            x_l2   = x[l2];
            x_l2p1 = x[l2+1];
            
            xh0  = x_0    + x_l1;
            xh1  = x_1    + x_l1p1;
            xl0  = x_0    - x_l1;
            xl1  = x_1    - x_l1p1;
            
            xh20 = x_h2   + x_l2;
            xh21 = x_h2p1 + x_l2p1;
            xl20 = x_h2   - x_l2;
            xl21 = x_h2p1 - x_l2p1;
            
            ptr_x0 = x;
            ptr_x0[0] = xh0 + xh20;
            ptr_x0[1] = xh1 + xh21;
            
            ptr_x2 = ptr_x0;
            x += 2;
            j += 6;
            predj = (j - fft_jmp);
            if (!predj) x += fft_jmp;
            if (!predj) j = 0;
            
            xt0 = xh0 - xh20; //xt0 = xh0 - xh20;
            yt0 = xh1 - xh21; //yt0 = xh1 - xh21;
            xt1 = xl0 - xl21; //xt1 = xl0 + xl21;
            yt2 = xl1 - xl20; //yt2 = xl1 + xl20;
            xt2 = xl0 + xl21; //xt2 = xl0 - xl21;
            yt1 = xl1 + xl20; //yt1 = xl1 - xl20;
            
            ptr_x2[l1  ] = xt1 * co1 - yt1 * si1; //ptr_x2[l1  ] = xt1 * co1 + yt1 * si1;
            ptr_x2[l1+1] = yt1 * co1 + xt1 * si1; //ptr_x2[l1+1] = yt1 * co1 - xt1 * si1;
            ptr_x2[h2  ] = xt0 * co2 - yt0 * si2; //ptr_x2[h2  ] = xt0 * co2 + yt0 * si2;
            ptr_x2[h2+1] = yt0 * co2 + xt0 * si2; //ptr_x2[h2+1] = yt0 * co2 - xt0 * si2;
            ptr_x2[l2  ] = xt2 * co3 - yt2 * si3; //ptr_x2[l2  ] = xt2 * co3 + yt2 * si3;
            ptr_x2[l2+1] = yt2 * co3 + xt2 * si3; //ptr_x2[l2+1] = yt2 * co3 - xt2 * si3;
        }
        tw_offset += fft_jmp;
        stride = stride>>2;
    }/* end while*/
    
    j = offset>>2;
    
    ptr_x0 = ptr_x;
    y0 = ptr_y;
    /*l0 = _norm(n_max) - 17;    get size of fft */
    l0=0;
    for(k=30;k>=0;k--)
        if( (n_max & (1 << k)) == 0 )
            l0++;
        else
            break;
    l0=l0-17;
    scale = 1/(float)n_max;
    if (radix <= 4) for (i = 0; i < N; i += 4)
    {
        /* reversal computation*/
        
        j0 = (j     ) & 0x3F;
        j1 = (j >> 6);
        k0 = brev[j0];
        k1 = brev[j1];
        k = (k0 << 6) +  k1;
        k = k >> l0;
        j++;        /* multiple of 4 index*/
        
        x0   = ptr_x0[0];  x1 = ptr_x0[1];
        x2   = ptr_x0[2];  x3 = ptr_x0[3];
        x4   = ptr_x0[4];  x5 = ptr_x0[5];
        x6   = ptr_x0[6];  x7 = ptr_x0[7];
        ptr_x0 += 8;
        
        xh0_0  = x0 + x4;
        xh1_0  = x1 + x5;
        xh0_1  = x2 + x6;
        xh1_1  = x3 + x7;
        
        if (radix == 2) {
            xh0_0 = x0;
            xh1_0 = x1;
            xh0_1 = x2;
            xh1_1 = x3;
        }
        
        yt0  = xh0_0 + xh0_1;
        yt1  = xh1_0 + xh1_1;
        yt4  = xh0_0 - xh0_1;
        yt5  = xh1_0 - xh1_1;
        
        xl0_0  = x0 - x4;
        xl1_0  = x1 - x5;
        xl0_1  = x2 - x6;
        xl1_1  = x7 - x3; //xl1_1  = x3 - x7;
        
        if (radix == 2) {
            xl0_0 = x4;
            xl1_0 = x5;
            xl1_1 = x6;
            xl0_1 = x7;
        }
        
        yt2  = xl0_0 + xl1_1;
        yt3  = xl1_0 + xl0_1; // yt3  = xl1_0 + (- xl0_1);
        yt6  = xl0_0 - xl1_1;
        yt7  = xl1_0 - xl0_1; // yt7  = xl1_0 - (- xl0_1);
        
        y0[k] = yt0*scale; y0[k+1] = yt1*scale;
        k += n_max>>1;
        y0[k] = yt2*scale; y0[k+1] = yt3*scale;
        k += n_max>>1;
        y0[k] = yt4*scale; y0[k+1] = yt5*scale;
        k += n_max>>1;
        y0[k] = yt6*scale; y0[k+1] = yt7*scale;
    }
}

/* ========================================================================*/
/*  End of file: sp_ifftSPxSP.c                                            */
/* ------------------------------------------------------------------------*/
/*          Copyright (C) 2002 Texas Instruments, Incorporated.            */
/*                          All Rights Reserved.                           */
/* ========================================================================*/

#endif // _DSPF_SP_ifftSPxSP_CN_H_
