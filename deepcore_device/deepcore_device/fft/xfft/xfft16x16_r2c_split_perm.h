__global__ void dk_xfft16x16_r2c_split_perm( 
      float2*              d_c, 
const __half* __restrict__ d_r, 
const float * __restrict__ d_RF, 
      int                  nx, 
      int                  ny, 
      int                  ldc, 
      int                  ldr, 
      int                  n, 
      int                  grid_x, 
      int                  grid_y, 
      int                  sx, 
      int                  sy, 
      int                  is_grad )
{
    const int brev[]={0,4,2,6,1,5,3,7};
    __shared__ float smem[16*145];
    __shared__ float2 s_RF[8];
    float2 c[9];
    int bx=blockIdx.x;
    int by=blockIdx.y;
    int gdy=gridDim.y;
    int tid=threadIdx.x;
    int x=tid&15;
    int y=tid>>4;
    int u=x&1;
    int v=x>>1;
    int icell=(bx<<4)+y;
    int n_cells_per_map=grid_x*grid_y;
    int idx=is_grad?by:icell;
    int idy=is_grad?icell:by;
    int map_id=idx/n_cells_per_map;
    int map_cell_id=idx%n_cells_per_map;
    int ox=(map_cell_id%grid_x)*sx;
    int oy=(map_cell_id/grid_x)*sy;
    float* spx=&smem[y*144+x];
    float* spy=&smem[y*144+v*18+u];
    d_c+=(y*gdy+by)*ldc+(bx<<4)+x;
    d_r+=idy*ldr+(map_id*ny+oy)*nx+(ox+=x);
    if(y==0){ ((float*)s_RF)[x]=d_RF[x]; }
    CLEAR8C(c)
    int vay=ny-oy;
    if((icell<n)&(ox<nx)){
    #pragma unroll
        for( int i=0; i<8; ++i ){
            if((i*2+0)<vay){ c[i].x=__half2float(d_r[0]); } d_r+=nx;
            if((i*2+1)<vay){ c[i].y=__half2float(d_r[0]); } d_r+=nx;
        } 
    } __syncthreads();
    s_vfft16( c, spx, spy, s_RF, brev );
    s_hfft16( c, &smem[y*144+v*18+u*8], spx, s_RF, brev, x, u );
    s_store9( d_c, &smem[y*145+x], &smem[x*145+y], c, 16*ldc*gdy );
}
__global__ void dk_xfft16x16_r2c_split_perm_pad( 
      float2*              d_c, 
const __half* __restrict__ d_r, 
const float * __restrict__ d_RF, 
      int                  nx, 
      int                  ny, 
      int                  ldc, 
      int                  ldr, 
      int                  n, 
      int                  grid_x, 
      int                  grid_y, 
      int                  sx, 
      int                  sy, 
      int                  is_grad,
      int                  pad_x, 
      int                  pad_y )
{
    const int brev[]={0,4,2,6,1,5,3,7};
    __shared__ float smem[16*145];
    __shared__ float2 s_RF[8];
    float2 c[9];
    int bx=blockIdx.x;
    int by=blockIdx.y;
    int gdy=gridDim.y;
    int tid=threadIdx.x;
    int x=tid&15;
    int y=tid>>4;
    int u=x&1;
    int v=x>>1;
    int icell=(bx<<4)+y;
    int idx=is_grad?by:icell;
    int idy=is_grad?icell:by;
    int n_cells_per_map=grid_x*grid_y;
    int map_id=idx/n_cells_per_map;
    int map_cell_id=idx%n_cells_per_map;
    int cell_x=map_cell_id%grid_x;
    int cell_y=map_cell_id/grid_x;
    int ox=cell_x*sx-pad_x+x;
    int oy=cell_y*sy-pad_y;
    float* spx=&smem[y*144+x];
    float* spy=&smem[y*144+v*18+u];
    d_c+=(y*gdy+by)*ldc+(bx<<4)+x;
    d_r+=idy*ldr+(map_id*ny+(oy+((cell_y==0)?pad_y:0)))*nx+ox;
    if(y==0){ ((float*)s_RF)[x]=d_RF[x]; }
    CLEAR8C(c)
    if((icell<n)&(ox>=0)&(ox<nx)){
    #pragma unroll
        for( int i=0; i<8; ++i ){
            if((oy>=0)&(oy<ny)){ c[i].x=__half2float(d_r[0]); d_r+=nx; } ++oy;
            if((oy>=0)&(oy<ny)){ c[i].y=__half2float(d_r[0]); d_r+=nx; } ++oy;
        }
    } __syncthreads();
    s_vfft16( c, spx, spy, s_RF, brev );
    s_hfft16( c, &smem[y*144+v*18+u*8], spx, s_RF, brev, x, u );
    s_store9( d_c, &smem[y*145+x], &smem[x*145+y], c, 16*ldc*gdy );
}