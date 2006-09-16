private import graph;

typedef bounds range(picture pic, real min, real max);

range Range(bool automin=false, real min=-infinity,
            bool automax=false, real max=infinity) 
{
  return new bounds(picture pic, real dmin, real dmax) {
    // autoscale routine finds reasonable limits
    bounds mz=autoscale(pic.scale.z.T(dmin),
                        pic.scale.z.T(dmax),
                        pic.scale.z.scale);
    // If automin/max, use autoscale result, else
    //   if min/max is finite, use specified value, else
    //   use minimum/maximum data value
    real pmin=automin ? pic.scale.z.Tinv(mz.min) : (finite(min) ? min : dmin);
    real pmax=automax ? pic.scale.z.Tinv(mz.max) : (finite(max) ? max : dmax);
    return bounds(pmin,pmax);
  };
}

range Automatic=Range(true,true);
range Full=Range();

void image(frame f, real[][] data, pair initial, pair final, pen[] palette,
           bool transpose=(initial.x < final.x && initial.y < final.y) ?
           true : false, transform t=identity())
{
  _image(f,transpose ? transpose(data) : data,initial,final,palette,t);
}

// Reduce color palette to approximate range of data relative to "display"
// range => errors of 1/palette.length in resulting color space.
pen[] adjust(picture pic, real min, real max, real rmin, real rmax,
             pen[] palette) 
{
  real dmin=pic.scale.z.T(min);
  real dmax=pic.scale.z.T(max);
  int minindex=floor((dmin-rmin)/(rmax-rmin)*palette.length);
  if(minindex < 0) minindex=0;
  int maxindex=floor((dmax-rmin)/(rmax-rmin)*palette.length);
  if(maxindex > palette.length) maxindex=palette.length;
  if(minindex > 0 || maxindex < palette.length) {
    pen[] newpalette;
    for(int i=minindex; i < maxindex; ++i)
      newpalette.push(palette[i]);
    return newpalette;
  }
  return palette;
}

bounds image(picture pic=currentpicture, real[][] f, range range=Full,
             pair initial, pair final, pen[] palette,
             bool transpose=(initial.x < final.x && initial.y < final.y) ?
             true : false) 
{
  f=transpose ? transpose(f) : copy(f);
  palette=copy(palette);

  real m=min(f);
  real M=max(f);
  bounds range=range(pic,m,M);
  real rmin=pic.scale.z.T(range.min);
  real rmax=pic.scale.z.T(range.max);
  palette=adjust(pic,m,M,rmin,rmax,palette);

  int n=f.length;
  int m=n > 0 ? f[0].length : 0;
  // Crop data to allowed range and scale
  for(int i=0; i < n; ++i) {
    real[] fi=f[i];
    for(int j=0; j < m; ++j) {
      real v=fi[j];
      v=max(v,range.min);
      v=min(v,range.max);
      fi[j]=pic.scale.z.T(v);
    }
  }

  initial=Scale(pic,initial);
  final=Scale(pic,final);

  pic.add(new void(frame F, transform t) {
      _image(F,f,initial,final,palette,t);
    });
  pic.addBox(initial,final);
  return range; // Return range used for color space
}

bounds image(picture pic=currentpicture, real f(real,real),
             range range=Full, pair initial, pair final,
             int nx=ngraph, int ny=nx, pen[] palette)
{
  // Generate data, taking scaling into account
  real xmin=pic.scale.x.T(initial.x);
  real xmax=pic.scale.x.T(final.x);
  real ymin=pic.scale.y.T(initial.y);
  real ymax=pic.scale.y.T(final.y);
  real[][] data=new real[ny][nx];
  for(int j=0; j < ny; ++j) {
    real[] dataj=data[j];
    real y=pic.scale.y.Tinv(interp(ymin,ymax,(j+0.5)/nx));
    for(int i=0; i < nx; ++i) {
      // Take center point of each bin
      dataj[i]=f(pic.scale.x.Tinv(interp(xmin,xmax,(i+0.5)/ny)),y);
    }
  }
  return image(pic,data,range,initial,final,palette,false);
}

bounds image(picture pic=currentpicture, pair[] z, real[] f,
             range range=Full, pen[] palette)
{
  if(z.length != f.length)
    abort("z and f arrays have different lengths");

  real m=min(f);
  real M=max(f);
  bounds range=range(pic,m,M);
  real rmin=pic.scale.z.T(range.min);
  real rmax=pic.scale.z.T(range.max);
  palette=adjust(pic,m,M,rmin,rmax,palette);

  int n=f.length;
  // Crop data to allowed range and scale
  for(int i=0; i < n; ++i) {
    real v=f[i];
    v=max(v,range.min);
    v=min(v,range.max);
    f[i]=pic.scale.z.T(v);
  }

  int[] edges={0,0,1};
  int N=palette.length-1;

  int[][] trn=triangulate(z);
  real step=rmax == rmin? 0.0 : (palette.length-1)/(rmax-rmin);
  for(int i=0; i < trn.length; ++i) {
    int[] trni=trn[i];
    int i0=trni[0], i1=trni[1], i2=trni[2];
    pair[] Z={z[i0],z[i1],z[i2]};
    pen color(int i) {
      return palette[round((f[i]-rmin)*step)];
    }
    pen[] p={color(i0),color(i1),color(i2)};
    gouraudshade(pic,Z[0]--Z[1]--Z[2]--cycle,p,Z,edges);
  }
  return range; // Return range used for color space
}

bounds image(picture pic=currentpicture, real[] x, real[] y, real[] f,
             range range=Full, pen[] palette)
{
  int n=x.length;
  if(n != y.length)
    abort("x and y arrays have different lengths");

  pair[] z=new pair[n];

  for(int i=0; i < n; ++i)
    z[i]=(x[i],y[i]);
    
  return image(pic,z,f,range,palette);
}

typedef ticks paletteticks(int sign=-1);

paletteticks PaletteTicks(Label format="", ticklabel ticklabel=null,
                          bool beginlabel=true, bool endlabel=true,
                          int N=0, int n=0, real Step=0, real step=0,
                          pen pTick=nullpen, pen ptick=nullpen)
{
  return new ticks(int sign=-1) {
    format.align(sign > 0 ? RightSide : LeftSide);
    return Ticks(sign,format,ticklabel,beginlabel,endlabel,N,n,Step,step,
                 true,true,extend=true,pTick,ptick);
  };
} 

paletteticks PaletteTicks=PaletteTicks();

void palette(picture pic=currentpicture, Label L="", bounds range, 
             pair initial, pair final, axis axis=Right, pen[] palette, 
             pen p=currentpen, paletteticks ticks=PaletteTicks)
{
  real initialz=pic.scale.z.T(range.min);
  real finalz=pic.scale.z.T(range.max);
  bounds mz=autoscale(initialz,finalz,pic.scale.z.scale);
  
  axisT axis;
  axis(pic,axis);
  real angle=degrees(axis.align);

  initial=Scale(pic,initial);
  final=Scale(pic,final);

  pair lambda=final-initial;
  bool vertical=(floor((angle+45)/90) % 2 == 0);
  pair perp,par;

  if(vertical) {perp=E; par=N;} else {perp=N; par=E;}

  guide g=(final-dot(lambda,par)*par)--final;
  guide g2=initial--final-dot(lambda,perp)*perp;

  if(sgn(dot(lambda,perp)*dot(axis.align,perp)) == -1) {
    guide tmp=g;
    g=g2;
    g2=tmp;
  }

  palette=copy(palette);
  Label L=L.copy();
  if(L.defaultposition) L.position(0.5);
  L.align(axis.align);
  L.p(p);
  if(vertical && L.defaultangle) {
    frame f;
    add(f,Label(L.s,(0,0),L.p));
    L.angle(length(max(f)-min(f)) > ylabelwidth*fontsize(L.p) ? 90 : 0);
  }
  real[][] pdata=new real[][] {sequence(palette.length-1)};
  if(vertical) pdata=transpose(pdata);
  
  pic.add(new void(frame f, transform t) {
      pair Z0=t*initial;
      pair Z1=t*final;
      pair initial=Z0;
      _image(f,pdata,inverse(t)*initial,final,palette,t);
      guide G=Z0--(Z0.x,Z1.y)--Z1--(Z1.x,Z0.y)--cycle;
      draw(f,G,p);
    });
  
  pic.addBox(initial,final);
  
  ticklocate locate=ticklocate(initialz,finalz,pic.scale.z);
  axis(pic,L,g,g2,p,ticks(sgn(axis.side.x*dot(lambda,par))),locate,
       mz.divisor,Above);
}

// A grayscale palette
pen[] Grayscale(int NColors=256)
{
  real ninv=1.0/(NColors-1.0);
  return sequence(new pen(int i) {return gray(i*ninv);},NColors);
}

// A rainbow palette
pen[] Rainbow(int NColors=32766)
{
  if(settings.gray) return Grayscale(NColors);
  
  int offset=1;
  int nintervals=5;
  int n=quotient(NColors-1,nintervals);
                
  pen[] Palette;
  if(n == 0) return Palette;
  
  Palette=new pen[n*nintervals+offset];
  real ninv=1.0/n;

  int N2=2n;
  int N3=3n;
  int N4=4n;
  for(int i=0; i < n; ++i) {
    real ininv=i*ninv;
    real ininv1=1.0-ininv;
    Palette[i]=rgb(ininv1,0.0,1.0);
    Palette[n+i]=rgb(0.0,ininv,1.0);
    Palette[N2+i]=rgb(0.0,1.0,ininv1);
    Palette[N3+i]=rgb(ininv,1.0,0.0);    
    Palette[N4+i]=rgb(1.0,ininv1,0.0);
  }
  Palette[N4+n]=rgb(1.0,0.0,0.0);
  
  return Palette;
}

private pen[] BWRainbow(int NColors, bool two)
{
  if(settings.gray) return Grayscale(NColors);
  
  int offset=1;
  int nintervals=6;
  int divisor=3;
  
  if(two) nintervals += 6;
  
  int num=NColors-offset;
  int n=quotient(num,nintervals*divisor)*divisor;
  NColors=n*nintervals+offset;
                
  pen[] Palette;
  if(n == 0) return Palette;
  
  Palette=new pen[NColors];
  real ninv=1.0/n;

  int N1,N2,N3,N4,N5;
  int k=0;
  
  if(two) {
    N1=n;
    N2=2n;
    N3=3n;
    N4=4n;
    N5=5n;
    for(int i=0; i < n; ++i) {
      real ininv=i*ninv;
      real ininv1=1.0-ininv;
      Palette[i]=rgb(ininv1,0.0,1.0);
      Palette[N1+i]=rgb(0.0,ininv,1.0);
      Palette[N2+i]=rgb(0.0,1.0,ininv1);
      Palette[N3+i]=rgb(ininv,1.0,0.0);
      Palette[N4+i]=rgb(1.0,ininv1,0.0);
      Palette[N5+i]=rgb(1.0,0.0,ininv);
    }
    k += 6n;
  }
  
  if(two)
    for(int i=0; i < n; ++i) 
      Palette[k+i]=rgb(1.0-i*ninv,0.0,1.0);
  else {
    int n3=quotient(n,3);
    int n23=2*n3;
    real third=n3*ninv;
    real twothirds=n23*ninv;
    N1=k;
    N2=k+n3;
    N3=k+n23;
    for(int i=0; i < n3; ++i) {
      real ininv=i*ninv;
      Palette[N1+i]=rgb(ininv,0.0,ininv);
      Palette[N2+i]=rgb(third,0.0,third+ininv);
      Palette[N3+i]=rgb(third-ininv,0.0,twothirds+ininv);
    }
  }
  k += n;

  N1=k;
  N2=N1+n;
  N3=N2+n;
  N4=N3+n;
  N5=N4+n;
  for(int i=0; i < n; ++i) {
    real ininv=i*ninv;
    real ininv1=1.0-ininv;
    Palette[N1+i]=rgb(0.0,ininv,1.0);
    Palette[N2+i]=rgb(0.0,1.0,ininv1);
    Palette[N3+i]=rgb(ininv,1.0,0.0);    
    Palette[N4+i]=rgb(1.0,ininv1,0.0);
    Palette[N5+i]=rgb(1.0,ininv,ininv);
  }
  k=N5+n;
  Palette[k]=rgb(1.0,1.0,1.0);
  
  return Palette;
}

// A rainbow palette tapering off to black/white at the spectrum ends,
pen[] BWRainbow(int NColors=32761)
{
  return BWRainbow(NColors,false);
}

// A double rainbow palette tapering off to black/white at the spectrum ends,
// with a linearly scaled intensity.
pen[] BWRainbow2(int NColors=32761)
{
  pen[] Palette=BWRainbow(NColors,true);
  int n=Palette.length;
  real ninv=1.0/n;
  for(int i=0; i < n; ++i)
    Palette[i]=i*ninv*Palette[i];
  return Palette;
}

pen[] cmyk(pen[] Palette) 
{
  int n=Palette.length;
  for(int i=0; i < n; ++i)
    Palette[i]=cmyk+Palette[i];
  return Palette;
}
