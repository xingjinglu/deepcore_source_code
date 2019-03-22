__global__ void dk_scgemm_16x128( 
          char*              d_c, 
    const char* __restrict__ d_a, 
    const char* __restrict__ d_b, 
    float scale, unsigned int nbx, 
    int anr, int bnr, int cnc, int lda, int ldb )
{
    __shared__ char smem[1024+8192];
    float2 c[16]={{0.f,0.f}};
    float4 a[2], b[2];
    unsigned int bx=blockIdx.x;
    unsigned int z=blockIdx.y;
    unsigned int x=bx%nbx;
    unsigned int y=bx/nbx;
    unsigned int tid=threadIdx.x;
    unsigned int lane=tid&31;
    unsigned int slot=tid>>5;
    unsigned int hcnc=cnc>>1;
    unsigned int u=lane&3;
    unsigned int v=lane>>2;
    unsigned int su=(x<<4)|(tid&15);
    unsigned int sv=(y<<6)|(tid&63);
    unsigned int ox=(x<<4)|(tid&15);
    unsigned int oy=(y<<7)|(slot<<5)|(lane>>4);    
    unsigned int elda=lda<<3;
    unsigned int dldb=ldb<<1;
    d_a+=(z*bnr+(tid>>4))*lda+((su< anr?su:( anr-1))<<3);
    d_b+=(z*bnr+(tid>>6))*ldb+((sv<hcnc?sv:(hcnc-1))<<4);
    d_c+=(z*cnc+oy)*lda+(ox<<3);    
    float2* __restrict__ asst=(float2*)&smem[tid<<3];
    float4* __restrict__ bsst=(float4*)&smem[0x400+(tid<<4)];
    float4* __restrict__ asld=(float4*)&smem[u<<4];
    float4* __restrict__ bsld=(float4*)&smem[0x400+((slot<<8)|(v<<4))];
    float2 p0=*((const float2* __restrict__)d_a);
    float4 p1=*((const float4* __restrict__)d_b); d_b+=dldb;
    float4 p2=*((const float4* __restrict__)d_b); d_b+=dldb;
    float4 p3=*((const float4* __restrict__)d_b); d_b+=dldb;
    float4 p4=*((const float4* __restrict__)d_b);

    for( int k=bnr-8; k>0; k-=8 )
    {
        asst[0*128]=p0;
        bsst[0*128]=p1;
        bsst[1*128]=p2;
        bsst[2*128]=p3;
        bsst[3*128]=p4;
        float2 q0=*((const float2* __restrict__)(d_a+=elda));
        float4 q1=*((const float4* __restrict__)(d_b+=dldb));
        float4 q2=*((const float4* __restrict__)(d_b+=dldb));
        float4 q3=*((const float4* __restrict__)(d_b+=dldb));
        float4 q4=*((const float4* __restrict__)(d_b+=dldb));
        __syncthreads();
    #pragma unroll
        for( int i=0; i<8; ++i ){
            a[0]=asld[i* 8+0];
            a[1]=asld[i* 8+4];
            b[0]=bsld[i*64+0];
            b[1]=bsld[i*64+8];
            CFMA4x4(c,a,b)
        } __syncthreads();
        p0=q0;
        p1=q1;
        p2=q2;
        p3=q3;
        p4=q4;
    }
    asst[0*128]=p0;
    bsst[0*128]=p1;
    bsst[1*128]=p2;
    bsst[2*128]=p3;
    bsst[3*128]=p4;
    __syncthreads();
#pragma unroll
    for( int i=0; i<8; ++i ){
        a[0]=asld[i* 8+0];
        a[1]=asld[i* 8+4];
        b[0]=bsld[i*64+0];
        b[1]=bsld[i*64+8];
        CFMA4x4(c,a,b)
    }
    scgemm_epilog16x32( d_c, &smem[slot<<11], c, lane, u, v, anr-ox, cnc-oy, lda, scale );
}