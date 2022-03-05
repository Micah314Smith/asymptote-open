layout(binding=0, std430) buffer offsetBuffer {
  uint offset[];
};

#ifdef GPUINDEXING
uniform uint offset2;

layout(binding=1, std430) buffer sum1Buffer {
  uint sum1[];
};

layout(binding=3, std430) buffer sum3Buffer {
  uint sum3[];
};
#endif

layout(binding=4, std430) buffer countBuffer {
  uint count[];
};

layout(binding=5, std430) buffer fragmentBuffer {
  vec4 fragment[];
};

layout(binding=6, std430) buffer depthBuffer {
  float depth[];
};

layout(binding=7, std430) buffer opaqueBuffer {
  vec4 opaqueColor[];
};

layout(binding=8, std430) buffer opaqueDepthBuffer {
  float opaqueDepth[];
};

layout(binding=9, std430) buffer maxBuffer {
  uint maxSize;
};

out vec4 outColor;

uniform uint width;
uniform uint m1;
uniform uint r;
uniform vec4 background;

vec4 blend(vec4 outColor, vec4 color)
{
  return mix(outColor,color,color.a);
}

void main()
{
  uint headIndex=uint(gl_FragCoord.y)*width+uint(gl_FragCoord.x);
  uint size=count[headIndex];
  float OpaqueDepth=opaqueDepth[headIndex];
  if(size == 0u) {
#ifdef GPUINDEXING
    offset[headIndex]=0u;
#endif
    opaqueDepth[headIndex]=0.0;
    discard;
  }

  outColor=OpaqueDepth != 0.0 ? opaqueColor[headIndex] : background;

#ifdef GPUINDEXING
  uint p=headIndex < r*(m1+1u) ? headIndex/(m1+1u) : (headIndex-r)/m1;
  uint listIndex=sum1[p]+sum1[offset2+p/m2]+sum3[p/(m2*m2)]+
#else
  uint listIndex=
#endif
    offset[headIndex]-size;

  uint k=0u;
  if(OpaqueDepth != 0.0)
    while(k < size && depth[listIndex+k] >= OpaqueDepth)
      ++k;

  // Sort the fragments with respect to descending depth
  if(size-k <= ARRAYSIZE) {
    if(k < size) {
      uint Index[ARRAYSIZE];
      float Depth[ARRAYSIZE];
      Index[0]=k;
      Depth[0]=depth[listIndex+k];
      ++k;
      uint i=1u;
      while(true) {
        if(OpaqueDepth != 0.0)
          while(k < size && depth[listIndex+k] >= OpaqueDepth)
            ++k;
        if(k == size) break;
        float d=depth[listIndex+k];
        uint j=i;
        while(j > 0u && d > Depth[j-1u]) {
          Index[j]=Index[j-1u];
          Depth[j]=Depth[j-1u];
          --j;
        }
        Index[j]=k++;
        Depth[j]=d;
        ++i;
      }
      for(uint j=0u; j < i; ++j)
        outColor=blend(outColor,fragment[listIndex+Index[j]]);
    }
  } else {
    atomicMax(maxSize,size);
    for(uint i=k+1u; i < size; i++) {
      vec4 temp=fragment[listIndex+i];
      float d=depth[listIndex+i];
      uint j=i;
      while(j > 0u && d > depth[listIndex+j-1u]) {
        fragment[listIndex+j]=fragment[listIndex+j-1u];
        depth[listIndex+j]=depth[listIndex+j-1u];
        --j;
      }
      fragment[listIndex+j]=temp;
      depth[listIndex+j]=d;
    }

    uint stop=listIndex+size;
    if(OpaqueDepth == 0.0)
      for(uint i=listIndex+k; i < stop; i++)
        outColor=blend(outColor,fragment[i]);
    else
      for(uint i=listIndex+k; i < stop; i++) {
        if(depth[i] < OpaqueDepth)
          outColor=blend(outColor,fragment[i]);
      }
  }

  count[headIndex]=0u;
  opaqueDepth[headIndex]=0.0;
#ifdef GPUINDEXING
  offset[headIndex]=0u;
#endif
}
