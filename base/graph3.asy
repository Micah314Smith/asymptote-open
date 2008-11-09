// Three-dimensional graphing routines

private import math;
import graph;
import three;

triple zero3(real) {return O;}

typedef triple direction3(real);
direction3 Dir(triple dir) {return new triple(real) {return dir;};}

ticklocate ticklocate(real a, real b, autoscaleT S=defaultS,
                      real tickmin=-infinity, real tickmax=infinity,
                      real time(real)=null, direction3 dir) 
{
  if((valuetime) time == null) time=linear(S.T(),a,b);
  ticklocate locate;
  locate.a=a;
  locate.b=b;
  locate.S=S.copy();
  if(finite(tickmin)) locate.S.tickMin=tickmin;
  if(finite(tickmax)) locate.S.tickMax=tickmax;
  locate.time=time;
  locate.dir=zero;
  locate.dir3=dir;
  return locate;
}
                             
private struct locateT {
  real t;         // tick location time
  triple V;       // tick location in frame coordinates
  triple pathdir; // path direction in frame coordinates
  triple dir;     // tick direction in frame coordinates
  
  void dir(transform3 T, path3 g, ticklocate locate, real t) {
    pathdir=unit(T*dir(g,t));
    triple Dir=locate.dir3(t);
    dir=unit(Dir);
  }
  // Locate the desired position of a tick along a path.
  void calc(transform3 T, path3 g, ticklocate locate, real val) {
    t=locate.time(val);
    V=T*point(g,t);
    dir(T,g,locate,t);
  }
}

void drawtick(picture pic, transform3 T, path3 g, path3 g2,
              ticklocate locate, real val, real Size, int sign, pen p,
              bool extend)
{
  locateT locate1,locate2;
  locate1.calc(T,g,locate,val);
  path3 G;
  if(extend && size(g2) > 0) {
    locate2.calc(T,g2,locate,val);
    G=locate1.V--locate2.V;
  } else
    G=(sign == 0) ?
      locate1.V-Size*locate1.dir--locate1.V+Size*locate1.dir :
      locate1.V--locate1.V+Size*sign*locate1.dir;
  draw(pic,G,p);
}

triple ticklabelshift(triple align, pen p=currentpen) 
{
  return 0.25*unit(align)*labelmargin(p);
}

// Signature of routines that draw labelled paths with ticks and tick labels.
typedef void ticks3(picture, transform3, Label, path3, path3, pen,
                    arrowbar3, ticklocate, int[], bool opposite=false,
                    bool primary=true);

// Label a tick on a frame.
void labeltick(picture pic, transform3 T, path3 g,
               ticklocate locate, real val, int sign, real Size,
               ticklabel ticklabel, Label F, real norm=0)
{
  locateT locate1;
  locate1.calc(T,g,locate,val);
  triple align=F.align.dir3;
  if(align == O) align=sign*locate1.dir;

  triple shift=align*labelmargin(F.p);
  if(dot(align,sign*locate1.dir) >= 0)
    shift=sign*(Size)*locate1.dir;

  real label;
  if(locate.S.scale.logarithmic)
    label=locate.S.scale.Tinv(val);
  else {
    label=val;
    if(abs(label) < zerotickfuzz*norm) label=0;
    // Fix epsilon errors at +/-1e-4
    // default format changes to scientific notation here
    if(abs(abs(label)-1e-4) < epsilon) label=sgn(label)*1e-4;
  }

  string s=ticklabel(label);
  triple v=locate1.V+shift;
  if(s != "") {
    s=baseline(s,align,"$10^4$");
    label(pic,F.defaulttransform ? s : F.T3*s,v,align,F.p);
  }
}  

// Add axis label L to frame f.
void labelaxis(picture pic, transform3 T, Label L, path3 g, 
               ticklocate locate=null, int sign=1, bool ticklabels=false)
{
  triple m=pic.min(identity4);
  triple M=pic.max(identity4);
  triple align=L.align.dir3;
  Label L=L.copy();

  pic.add(new void(frame f, transform3 T, picture pic2, projection P) {
      path3 g=T*g;
      real t=relative(L,g);
      triple v=point(g,t);
      picture F;
      if(L.align.dir3 == O) align=invert(L.align.dir,v,P);

      if(ticklabels && locate != null && piecewisestraight(g)) {
        locateT locate1;
        locate1.dir(T,g,locate,t);
        triple pathdir=locate1.pathdir;

        triple perp=cross(pathdir,P.vector());
        if(align == O)
          align=unit(sgn(dot(sign*locate1.dir,perp))*perp);
        path[] g=project(box(T*m,T*M),P);
        pair z=project(v,P);
        pair Ppathdir=project(v+pathdir,P)-z;
        pair Perp=unit(I*Ppathdir);
        real angle=degrees(Ppathdir,warn=false);
        transform S=rotate(-angle,z);
        path[] G=S*g;
        pair Palign=project(v+align,P)-z;
        pair PalignPerp=dot(Palign,Perp)*Perp;
        pair Align=rotate(-angle)*PalignPerp;
        real factor=abs(PalignPerp);
        if(factor != 0) factor=1/sqrt(factor);
        pair offset=unit(Palign)*factor*
          abs((Align.y >= 0 ? max(G).y : (Align.y < 0 ? min(G).y : 0))-z.y);
        triple normal=cross(pathdir,align);
        if(normal != O) v=invert(z+offset,normal,v,P);
      }

      label(F,L,v);
      add(f,F.fit3(identity4,pic2,P));
    },exact=false);

  path3[] G=path3(texpath(L));
  G=L.align.is3D ? align(G,O,align,L.p) : L.T3*G;
  triple v=point(g,relative(L,g));
  pic.addBox(v,v,min(G),max(G));
}

// Tick construction routine for a user-specified array of tick values.
ticks3 Ticks3(int sign, Label F="", ticklabel ticklabel=null,
              bool beginlabel=true, bool endlabel=true,
              real[] Ticks=new real[], real[] ticks=new real[], int N=1,
              bool begin=true, bool end=true,
              real Size=0, real size=0, bool extend=false,
              pen pTick=nullpen, pen ptick=nullpen)
{
  return new void(picture pic, transform3 t, Label L,
                  path3 g, path3 g2, pen p, arrowbar3 arrow, ticklocate locate,
                  int[] divisor, bool opposite, bool primary) {
    // Use local copy of context variables:
    int Sign=opposite ? -1 : 1;
    int sign=Sign*sign;
    pen pTick=pTick;
    pen ptick=ptick;
    ticklabel ticklabel=ticklabel;
    
    real Size=Size;
    real size=size;
    if(Size == 0) Size=Ticksize;
    if(size == 0) size=ticksize;
    
    Label L=L.copy();
    Label F=F.copy();
    L.p(p);
    F.p(p);
    if(pTick == nullpen) pTick=p;
    if(ptick == nullpen) ptick=pTick;
    
    bool ticklabels=false;
    path3 G=t*g;
    path3 G2=t*g2;
    
    scalefcn T;
    
    real a,b;
    if(locate.S.scale.logarithmic) {
      a=locate.S.postscale.Tinv(locate.a);
      b=locate.S.postscale.Tinv(locate.b);
      T=locate.S.scale.T;
    } else {
      a=locate.S.Tinv(locate.a);
      b=locate.S.Tinv(locate.b);
      T=identity;
    }
    
    if(a > b) {real temp=a; a=b; b=temp;}

    real norm=max(abs(a),abs(b));
    
    string format=F.s == "" ? autoformat(norm...Ticks) :
      (F.s == trailingzero ? autoformat(true,norm...Ticks) : F.s);
    if(F.s == "%") F.s="";
    if(ticklabel == null) {
      if(locate.S.scale.logarithmic) {
        int base=round(locate.S.scale.Tinv(1));
        ticklabel=format == "%" ? Format("") : DefaultLogFormat(base);
      } else ticklabel=Format(format);
    }

    begingroup3(pic);
    if(primary) draw(pic,G,p,arrow);
    else draw(pic,G,p);

    for(int i=(begin ? 0 : 1); i < (end ? Ticks.length : Ticks.length-1); ++i) {
      real val=T(Ticks[i]);
      if(val >= a && val <= b)
        drawtick(pic,t,g,g2,locate,val,Size,sign,pTick,extend);
    }
    for(int i=0; i < ticks.length; ++i) {
      real val=T(ticks[i]);
      if(val >= a && val <= b)
        drawtick(pic,t,g,g2,locate,val,size,sign,ptick,extend);
    }
    endgroup3(pic);
    
    if(N == 0) N=1;
    if(Size > 0 && primary) {
      for(int i=(beginlabel ? 0 : 1);
          i < (endlabel ? Ticks.length : Ticks.length-1); i += N) {
        real val=T(Ticks[i]);
        if(val >= a && val <= b) {
          ticklabels=true;
          labeltick(pic,t,g,locate,val,Sign,Size,ticklabel,F,norm);
        }
      }
    }
    if(L.s != "" && primary) 
      labelaxis(pic,t,L,G,locate,Sign,ticklabels);
  };
}

// Automatic tick construction routine.
ticks3 Ticks3(int sign, Label F="", ticklabel ticklabel=null,
              bool beginlabel=true, bool endlabel=true,
              int N, int n=0, real Step=0, real step=0,
              bool begin=true, bool end=true, tickmodifier modify=None,
              real Size=0, real size=0, bool extend=false,
              pen pTick=nullpen, pen ptick=nullpen)
{
  return new void(picture pic, transform3 T, Label L,
                  path3 g, path3 g2, pen p, arrowbar3 arrow, ticklocate locate,
                  int[] divisor, bool opposite, bool primary) {
    path3 G=T*g;
    real limit=Step == 0 ? axiscoverage*arclength(G) : 0;
    tickvalues values=modify(generateticks(sign,F,ticklabel,N,n,Step,step,
                                           Size,size,identity(),1,
                                           project(G,currentprojection),
                                           limit,p,locate,divisor,
                                           opposite));
    Ticks3(sign,F,ticklabel,beginlabel,endlabel,values.major,values.minor,
           values.N,begin,end,Size,size,extend,pTick,ptick)
      (pic,T,L,g,g2,p,arrow,locate,divisor,opposite,primary);
  };
}

ticks3 NoTicks3()
{
  return new void(picture pic, transform3 T, Label L, path3 g,
                  path3, pen p, arrowbar3 arrow, ticklocate, int[],
                  bool opposite, bool primary) {
    path3 G=T*g;
    if(primary) draw(pic,G,p,arrow);
    else draw(pic,G,p);
    if(L.s != "" && primary) {
      Label L=L.copy();
      L.p(p);
      labelaxis(pic,T,L,G,opposite ? -1 : 1);
    }
  };
}

ticks3 InTicks(Label format="", ticklabel ticklabel=null,
               bool beginlabel=true, bool endlabel=true,
               int N=0, int n=0, real Step=0, real step=0,
               bool begin=true, bool end=true, tickmodifier modify=None,
               real Size=0, real size=0, bool extend=false,
               pen pTick=nullpen, pen ptick=nullpen)
{
  return Ticks3(-1,format,ticklabel,beginlabel,endlabel,N,n,Step,step,
                begin,end,modify,Size,size,extend,pTick,ptick);
}

ticks3 OutTicks(Label format="", ticklabel ticklabel=null,
                bool beginlabel=true, bool endlabel=true,
                int N=0, int n=0, real Step=0, real step=0,
                bool begin=true, bool end=true, tickmodifier modify=None,
                real Size=0, real size=0, bool extend=false,
                pen pTick=nullpen, pen ptick=nullpen)
{
  return Ticks3(1,format,ticklabel,beginlabel,endlabel,N,n,Step,step,
                begin,end,modify,Size,size,extend,pTick,ptick);
}

ticks3 InOutTicks(Label format="", ticklabel ticklabel=null,
                  bool beginlabel=true, bool endlabel=true,
                  int N=0, int n=0, real Step=0, real step=0,
                  bool begin=true, bool end=true, tickmodifier modify=None,
                  real Size=0, real size=0, bool extend=false,
                  pen pTick=nullpen, pen ptick=nullpen)
{
  return Ticks3(0,format,ticklabel,beginlabel,endlabel,N,n,Step,step,
                begin,end,modify,Size,size,extend,pTick,ptick);
}

ticks3 InTicks(Label format="", ticklabel ticklabel=null, 
               bool beginlabel=true, bool endlabel=true, 
               real[] Ticks, real[] ticks=new real[],
               real Size=0, real size=0, bool extend=false,
               pen pTick=nullpen, pen ptick=nullpen)
{
  return Ticks3(-1,format,ticklabel,beginlabel,endlabel,
                Ticks,ticks,Size,size,extend,pTick,ptick);
}

ticks3 OutTicks(Label format="", ticklabel ticklabel=null, 
                bool beginlabel=true, bool endlabel=true, 
                real[] Ticks, real[] ticks=new real[],
                real Size=0, real size=0, bool extend=false,
                pen pTick=nullpen, pen ptick=nullpen)
{
  return Ticks3(1,format,ticklabel,beginlabel,endlabel,
                Ticks,ticks,Size,size,extend,pTick,ptick);
}

ticks3 InOutTicks(Label format="", ticklabel ticklabel=null, 
                  bool beginlabel=true, bool endlabel=true, 
                  real[] Ticks, real[] ticks=new real[],
                  real Size=0, real size=0, bool extend=false,
                  pen pTick=nullpen, pen ptick=nullpen)
{
  return Ticks3(0,format,ticklabel,beginlabel,endlabel,
                Ticks,ticks,Size,size,extend,pTick,ptick);
}

ticks3 NoTicks3=NoTicks3(),
  InTicks=InTicks(),
  OutTicks=OutTicks(),
  InOutTicks=InOutTicks();

triple tickMin3(picture pic)
{
  return minbound(pic.userMin,(pic.scale.x.tickMin,pic.scale.y.tickMin,
                               pic.scale.z.tickMin));
}
  
triple tickMax3(picture pic)
{
  return maxbound(pic.userMax,(pic.scale.x.tickMax,pic.scale.y.tickMax,
                               pic.scale.z.tickMax));
}
                                               
axis Bounds(int type=Both, int type2=Both, triple align=O, bool extend=false)
{
  return new void(picture pic, axisT axis) {
    axis.type=type;
    axis.type2=type2;
    axis.position=0.5;
    axis.align=align;
    axis.extend=extend;
  };
}

axis YZEquals(real y, real z, triple align=O, bool extend=false)
{
  return new void(picture pic, axisT axis) {
    axis.type=Value;
    axis.type2=Value;
    axis.value=pic.scale.y.T(y);
    axis.value2=pic.scale.z.T(z);
    axis.position=1;
    axis.align=align;
    axis.extend=extend;
  };
}

axis XZEquals(real x, real z, triple align=O, bool extend=false)
{
  return new void(picture pic, axisT axis) {
    axis.type=Value;
    axis.type2=Value;
    axis.value=pic.scale.x.T(x);
    axis.value2=pic.scale.z.T(z);
    axis.position=1;
    axis.align=align;
    axis.extend=extend;
  };
}

axis XYEquals(real x, real y, triple align=O, bool extend=false)
{
  return new void(picture pic, axisT axis) {
    axis.type=Value;
    axis.type2=Value;
    axis.value=pic.scale.x.T(x);
    axis.value2=pic.scale.y.T(y);
    axis.position=1;
    axis.align=align;
    axis.extend=extend;
  };
}

axis YZZero(triple align=O, bool extend=false)
{
  return new void(picture pic, axisT axis) {
    axis.type=Value;
    axis.type2=Value;
    axis.value=pic.scale.y.T(pic.scale.y.scale.logarithmic ? 1 : 0);
    axis.value2=pic.scale.z.T(pic.scale.z.scale.logarithmic ? 1 : 0);
    axis.position=1;
    axis.align=align;
    axis.extend=extend;
  };
}

axis XZZero(triple align=O, bool extend=false)
{
  return new void(picture pic, axisT axis) {
    axis.type=Value;
    axis.type2=Value;
    axis.value=pic.scale.x.T(pic.scale.x.scale.logarithmic ? 1 : 0);
    axis.value2=pic.scale.z.T(pic.scale.z.scale.logarithmic ? 1 : 0);
    axis.position=1;
    axis.align=align;
    axis.extend=extend;
  };
}

axis XYZero(triple align=O, bool extend=false)
{
  return new void(picture pic, axisT axis) {
    axis.type=Value;
    axis.type2=Value;
    axis.value=pic.scale.x.T(pic.scale.x.scale.logarithmic ? 1 : 0);
    axis.value2=pic.scale.y.T(pic.scale.y.scale.logarithmic ? 1 : 0);
    axis.position=1;
    axis.align=align;
    axis.extend=extend;
  };
}

axis
Bounds=Bounds(),
  YZZero=YZZero(),
  XZZero=XZZero(),
  XYZero=XYZero();

// Draw a general three-dimensional axis.
void axis(picture pic=currentpicture, Label L="", path3 g, path3 g2=nullpath3,
          pen p=currentpen, ticks3 ticks, ticklocate locate,
          arrowbar3 arrow=None, int[] divisor=new int[], bool above=false,
          bool opposite=false) 
{
  Label L=L.copy();
  real t=reltime(g,0.5);
  if(L.defaultposition) L.position(t);
  divisor=copy(divisor);
  locate=locate.copy();
  
  pic.add(new void (picture f, transform3 t, transform3 T, triple, triple) {
      picture d;
      ticks(d,t,L,g,g2,p,arrow,locate,divisor,opposite,true);
      add(f,t*T*inverse(t)*d);
    },above=above);
  
  addPath(pic,g,p);
  
  if(L.s != "") {
    frame f;
    Label L0=L.copy();
    L0.position(0);
    add(f,L0);
    triple pos=point(g,L.relative()*length(g));
    pic.addBox(pos,pos,min3(f),max3(f));
  }
}

real xtrans(transform3 t, real x)
{
  return (t*(x,0,0)).x;
}

real ytrans(transform3 t, real y)
{
  return (t*(0,y,0)).y;
}

real ztrans(transform3 t, real z)
{
  return (t*(0,0,z)).z;
}

private triple defaultdir(triple X, triple Y, triple Z, bool opposite=false,
                          projection P) {
  triple u=cross(P.vector(),Z);
  return abs(dot(u,X)) > abs(dot(u,Y)) ? -X : (opposite ? Y : -Y);
}

// An internal routine to draw an x axis at a particular y value.
void xaxis3At(picture pic=currentpicture, Label L="", axis axis,
              real xmin=-infinity, real xmax=infinity, pen p=currentpen,
              ticks3 ticks=NoTicks3, arrowbar3 arrow=None, bool above=true,
              bool opposite=false, bool opposite2=false, bool primary=true)
{
  int type=axis.type;
  int type2=axis.type2;
  triple dir=axis.align.dir3 == O ?
    defaultdir(Y,Z,X,opposite^opposite2,currentprojection) : axis.align.dir3;
  Label L=L.copy();
  if(L.align.dir3 == O && L.align.dir == 0) L.align(opposite ? -dir : dir);

  real y=axis.value;
  real z=axis.value2;
  real y2,z2;
  int[] divisor=copy(axis.xdivisor);

  pic.add(new void(picture f, transform3 t, transform3 T, triple lb,
                   triple rt) {
            transform3 tinv=inverse(t);
            triple a=xmin == -infinity ? tinv*(lb.x-min3(p).x,ytrans(t,y),
                                               ztrans(t,z)) : (xmin,y,z);
            triple b=xmax == infinity ? tinv*(rt.x-max3(p).x,ytrans(t,y),
                                              ztrans(t,z)) : (xmax,y,z);
            triple a2=xmin == -infinity ? tinv*(lb.x-min3(p).x,ytrans(t,y2),
                                                ztrans(t,z2)) : (xmin,y2,z2);
            triple b2=xmax == infinity ? tinv*(rt.x-max3(p).x,ytrans(t,y2),
                                               ztrans(t,z2)) : (xmax,y2,z2);

            if(xmin == -infinity || xmax == infinity) {
              bounds mx=autoscale(a.x,b.x,pic.scale.x.scale);
              pic.scale.x.tickMin=mx.min;
              pic.scale.x.tickMax=mx.max;
              divisor=mx.divisor;
            }
      
            triple fuzz=X*epsilon*max(abs(a.x),abs(b.x));
            a -= fuzz;
            b += fuzz;

            picture d;
            ticks(d,t,L,a--b,finite(y2) ? a2--b2 : nullpath3,p,arrow,
                  ticklocate(a.x,b.x,pic.scale.x,Dir(dir)),divisor,
                  opposite,primary);
            add(f,t*T*tinv*d);
          },above=above);

  void bounds() {
    if(type == Min)
      y=pic.scale.y.automin() ? tickMin3(pic).y : pic.userMin.y;
    else if(type == Max)
      y=pic.scale.y.automax() ? tickMax3(pic).y : pic.userMax.y;
    else if(type == Both) {
      y2=pic.scale.y.automax() ? tickMax3(pic).y : pic.userMax.y;
      y=opposite ? y2 : 
        (pic.scale.y.automin() ? tickMin3(pic).y : pic.userMin.y);
    }

    if(type2 == Min)
      z=pic.scale.z.automin() ? tickMin3(pic).z : pic.userMin.z;
    else if(type2 == Max)
      z=pic.scale.z.automax() ? tickMax3(pic).z : pic.userMax.z;
    else if(type2 == Both) {
      z2=pic.scale.z.automax() ? tickMax3(pic).z : pic.userMax.z;
      z=opposite2 ? z2 : 
        (pic.scale.z.automin() ? tickMin3(pic).z : pic.userMin.z);
    }

    real Xmin=finite(xmin) ? xmin : pic.userMin.x;
    real Xmax=finite(xmax) ? xmax : pic.userMax.x;

    triple a=(Xmin,y,z);
    triple b=(Xmax,y,z);
    triple a2=(Xmin,y2,z2);
    triple b2=(Xmax,y2,z2);

    if(finite(a)) {
      pic.addPoint(a,min3(p));
      pic.addPoint(a,max3(p));
    }
  
    if(finite(b)) {
      pic.addPoint(b,min3(p));
      pic.addPoint(b,max3(p));
    }

    if(finite(a) && finite(b)) {
      picture d;
      ticks(d,pic.scaling3(warn=false),L,
            (a.x,0,0)--(b.x,0,0),(a2.x,0,0)--(b2.x,0,0),p,arrow,
            ticklocate(a.x,b.x,pic.scale.x,Dir(dir)),divisor,
            opposite,primary);
      frame f;
      if(L.s != "") {
        Label L0=L.copy();
        L0.position(0);
        add(f,L0);
      }
      triple pos=a+L.relative()*(b-a);
      triple m=min3(d);
      triple M=max3(d);
      pic.addBox(pos,pos,(min3(f).x,m.y,m.z),(max3(f).x,m.y,m.z));
    }
  }

  // Process any queued y and z axes bound calculation requests.
  for(int i=0; i < pic.scale.y.bound.length; ++i)
    pic.scale.y.bound[i]();
  for(int i=0; i < pic.scale.z.bound.length; ++i)
    pic.scale.z.bound[i]();

  pic.scale.y.bound.delete();
  pic.scale.z.bound.delete();

  bounds();

  // Request another x bounds calculation before final picture scaling.
  pic.scale.x.bound.push(bounds);
}

// An internal routine to draw an x axis at a particular y value.
void yaxis3At(picture pic=currentpicture, Label L="", axis axis,
              real ymin=-infinity, real ymax=infinity, pen p=currentpen,
              ticks3 ticks=NoTicks3, arrowbar3 arrow=None, bool above=true,
              bool opposite=false, bool opposite2=false, bool primary=true)
{
  int type=axis.type;
  int type2=axis.type2;
  triple dir=axis.align.dir3 == O ?
    defaultdir(X,Z,Y,opposite^opposite2,currentprojection) : axis.align.dir3;
  Label L=L.copy();
  if(L.align.dir3 == O && L.align.dir == 0) L.align(opposite ? -dir : dir);

  real x=axis.value;
  real z=axis.value2;
  real x2,z2;
  int[] divisor=copy(axis.ydivisor);

  pic.add(new void(picture f, transform3 t, transform3 T, triple lb,
                   triple rt) {
            transform3 tinv=inverse(t);
            triple a=ymin == -infinity ? tinv*(xtrans(t,x),lb.y-min3(p).y,
                                               ztrans(t,z)) : (x,ymin,z);
            triple b=ymax == infinity ? tinv*(xtrans(t,x),rt.y-max3(p).y,
                                              ztrans(t,z)) : (x,ymax,z);
            triple a2=ymin == -infinity ? tinv*(xtrans(t,x2),lb.y-min3(p).y,
                                                ztrans(t,z2)) : (x2,ymin,z2);
            triple b2=ymax == infinity ? tinv*(xtrans(t,x2),rt.y-max3(p).y,
                                               ztrans(t,z2)) : (x2,ymax,z2);

            if(ymin == -infinity || ymax == infinity) {
              bounds my=autoscale(a.y,b.y,pic.scale.y.scale);
              pic.scale.y.tickMin=my.min;
              pic.scale.y.tickMax=my.max;
              divisor=my.divisor;
            }
      
            triple fuzz=Y*epsilon*max(abs(a.y),abs(b.y));
            a -= fuzz;
            b += fuzz;

            picture d;
            ticks(d,t,L,a--b,finite(x2) ? a2--b2 : nullpath3,p,arrow,
                  ticklocate(a.y,b.y,pic.scale.y,Dir(dir)),divisor,
                  opposite,primary);
            add(f,t*T*tinv*d);
          },above=above);

  void bounds() {
    if(type == Min)
      x=pic.scale.x.automin() ? tickMin3(pic).x : pic.userMin.x;
    else if(type == Max)
      x=pic.scale.x.automax() ? tickMax3(pic).x : pic.userMax.x;
    else if(type == Both) {
      x2=pic.scale.x.automax() ? tickMax3(pic).x : pic.userMax.x;
      x=opposite ? x2 : 
        (pic.scale.x.automin() ? tickMin3(pic).x : pic.userMin.x);
    }

    if(type2 == Min)
      z=pic.scale.z.automin() ? tickMin3(pic).z : pic.userMin.z;
    else if(type2 == Max)
      z=pic.scale.z.automax() ? tickMax3(pic).z : pic.userMax.z;
    else if(type2 == Both) {
      z2=pic.scale.z.automax() ? tickMax3(pic).z : pic.userMax.z;
      z=opposite2 ? z2 : 
        (pic.scale.z.automin() ? tickMin3(pic).z : pic.userMin.z);
    }

    real Ymin=finite(ymin) ? ymin : pic.userMin.y;
    real Ymax=finite(ymax) ? ymax : pic.userMax.y;

    triple a=(x,Ymin,z);
    triple b=(x,Ymax,z);
    triple a2=(x2,Ymin,z2);
    triple b2=(x2,Ymax,z2);

    if(finite(a)) {
      pic.addPoint(a,min3(p));
      pic.addPoint(a,max3(p));
    }
  
    if(finite(b)) {
      pic.addPoint(b,min3(p));
      pic.addPoint(b,max3(p));
    }

    if(finite(a) && finite(b)) {
      picture d;
      ticks(d,pic.scaling3(warn=false),L,
            (0,a.y,0)--(0,b.y,0),(0,a2.y,0)--(0,a2.y,0),p,arrow,
            ticklocate(a.y,b.y,pic.scale.y,Dir(dir)),divisor,
            opposite,primary);
      frame f;
      if(L.s != "") {
        Label L0=L.copy();
        L0.position(0);
        add(f,L0);
      }
      triple pos=a+L.relative()*(b-a);
      triple m=min3(d);
      triple M=max3(d);
      pic.addBox(pos,pos,(m.x,min3(f).y,m.z),(m.x,max3(f).y,m.z));
    }
  }

  // Process any queued x and z axis bound calculation requests.
  for(int i=0; i < pic.scale.x.bound.length; ++i)
    pic.scale.x.bound[i]();
  for(int i=0; i < pic.scale.z.bound.length; ++i)
    pic.scale.z.bound[i]();

  pic.scale.x.bound.delete();
  pic.scale.z.bound.delete();

  bounds();

  // Request another y bounds calculation before final picture scaling.
  pic.scale.y.bound.push(bounds);
}

// An internal routine to draw an x axis at a particular y value.
void zaxis3At(picture pic=currentpicture, Label L="", axis axis,
              real zmin=-infinity, real zmax=infinity, pen p=currentpen,
              ticks3 ticks=NoTicks3, arrowbar3 arrow=None, bool above=true,
              bool opposite=false, bool opposite2=false, bool primary=true)
{
  int type=axis.type;
  int type2=axis.type2;
  triple dir=axis.align.dir3 == O ?
    defaultdir(X,Y,Z,opposite^opposite2,currentprojection) : axis.align.dir3;
  Label L=L.copy();
  if(L.align.dir3 == O && L.align.dir == 0) L.align(opposite ? -dir : dir);

  real x=axis.value;
  real y=axis.value2;
  real x2,y2;
  int[] divisor=copy(axis.zdivisor);

  pic.add(new void(picture f, transform3 t, transform3 T, triple lb,
                   triple rt) {
            transform3 tinv=inverse(t);
            triple a=zmin == -infinity ? tinv*(xtrans(t,x),ytrans(t,y),
                                               lb.z-min3(p).z) : (x,y,zmin);
            triple b=zmax == infinity ? tinv*(xtrans(t,x),ytrans(t,y),
                                              rt.z-max3(p).z) : (x,y,zmax);
            triple a2=zmin == -infinity ? tinv*(xtrans(t,x2),ytrans(t,y2),
                                                lb.z-min3(p).z) : (x2,y2,zmin);
            triple b2=zmax == infinity ? tinv*(xtrans(t,x2),ytrans(t,y2),
                                               rt.z-max3(p).z) : (x2,y2,zmax);

            if(zmin == -infinity || zmax == infinity) {
              bounds mz=autoscale(a.z,b.z,pic.scale.z.scale);
              pic.scale.z.tickMin=mz.min;
              pic.scale.z.tickMax=mz.max;
              divisor=mz.divisor;
            }
      
            triple fuzz=Z*epsilon*max(abs(a.z),abs(b.z));
            a -= fuzz;
            b += fuzz;

            picture d;
            ticks(d,t,L,a--b,finite(x2) ? a2--b2 : nullpath3,p,arrow,
                  ticklocate(a.z,b.z,pic.scale.z,Dir(dir)),divisor,
                  opposite,primary);
            add(f,t*T*tinv*d);
          },above=above);

  void bounds() {
    if(type == Min)
      x=pic.scale.x.automin() ? tickMin3(pic).x : pic.userMin.x;
    else if(type == Max)
      x=pic.scale.x.automax() ? tickMax3(pic).x : pic.userMax.x;
    else if(type == Both) {
      x2=pic.scale.x.automax() ? tickMax3(pic).x : pic.userMax.x;
      x=opposite ? x2 : 
        (pic.scale.x.automin() ? tickMin3(pic).x : pic.userMin.x);
    }

    if(type2 == Min)
      y=pic.scale.y.automin() ? tickMin3(pic).y : pic.userMin.y;
    else if(type2 == Max)
      y=pic.scale.y.automax() ? tickMax3(pic).y : pic.userMax.y;
    else if(type2 == Both) {
      y2=pic.scale.y.automax() ? tickMax3(pic).y : pic.userMax.y;
      y=opposite2 ? y2 : 
        (pic.scale.y.automin() ? tickMin3(pic).y : pic.userMin.y);
    }

    real Zmin=finite(zmin) ? zmin : pic.userMin.z;
    real Zmax=finite(zmax) ? zmax : pic.userMax.z;

    triple a=(x,y,Zmin);
    triple b=(x,y,Zmax);
    triple a2=(x2,y2,Zmin);
    triple b2=(x2,y2,Zmax);

    if(finite(a)) {
      pic.addPoint(a,min3(p));
      pic.addPoint(a,max3(p));
    }
  
    if(finite(b)) {
      pic.addPoint(b,min3(p));
      pic.addPoint(b,max3(p));
    }

    if(finite(a) && finite(b)) {
      picture d;
      ticks(d,pic.scaling3(warn=false),L,
            (0,0,a.z)--(0,0,b.z),(0,0,a2.z)--(0,0,a2.z),p,arrow,
            ticklocate(a.z,b.z,pic.scale.z,Dir(dir)),divisor,
            opposite,primary);
      frame f;
      if(L.s != "") {
        Label L0=L.copy();
        L0.position(0);
        add(f,L0);
      }
      triple pos=a+L.relative()*(b-a);
      triple m=min3(d);
      triple M=max3(d);
      pic.addBox(pos,pos,(m.x,m.y,min3(f).z),(m.x,m.y,max3(f).z));
    }
  }

  // Process any queued x and y axes bound calculation requests.
  for(int i=0; i < pic.scale.x.bound.length; ++i)
    pic.scale.x.bound[i]();
  for(int i=0; i < pic.scale.y.bound.length; ++i)
    pic.scale.y.bound[i]();

  pic.scale.x.bound.delete();
  pic.scale.y.bound.delete();

  bounds();

  // Request another z bounds calculation before final picture scaling.
  pic.scale.z.bound.push(bounds);
}

// Internal routine to autoscale the user limits of a picture.
void autoscale3(picture pic=currentpicture, axis axis)
{
  bool set=pic.scale.set;
  autoscale(pic,axis);

  if(!set) {
    bounds mz;
    if(pic.userSetz) {
      mz=autoscale(pic.userMin.z,pic.userMax.z,pic.scale.z.scale);
      if(pic.scale.z.scale.logarithmic &&
         floor(pic.userMin.z) == floor(pic.userMax.z)) {
        if(pic.scale.z.automin())
          pic.userMinz(floor(pic.userMin.z));
        if(pic.scale.z.automax())
          pic.userMaxz(ceil(pic.userMax.z));
      }
    } else {mz.min=mz.max=0; pic.scale.set=false;}
    
    pic.scale.z.tickMin=mz.min;
    pic.scale.z.tickMax=mz.max;
    axis.zdivisor=mz.divisor;
  }
}

// Draw an x axis in three dimensions.
void xaxis3(picture pic=currentpicture, Label L="", axis axis=YZZero,
            real xmin=-infinity, real xmax=infinity, pen p=currentpen,
            ticks3 ticks=NoTicks3, arrowbar3 arrow=None, bool above=false)
{
  if(xmin > xmax) return;
  
  if(pic.scale.x.automin && xmin > -infinity) pic.scale.x.automin=false;
  if(pic.scale.x.automax && xmax < infinity) pic.scale.x.automax=false;

  if(!pic.scale.set) {
    axis(pic,axis);
    autoscale3(pic,axis);
  }
  
  bool newticks=false;
  
  if(xmin != -infinity) {
    xmin=pic.scale.x.T(xmin);
    newticks=true;
  }
  
  if(xmax != infinity) {
    xmax=pic.scale.x.T(xmax);
    newticks=true;
  }
  
  if(newticks && pic.userSetx && ticks != NoTicks3) {
    if(xmin == -infinity) xmin=pic.userMin.x;
    if(xmax == infinity) xmax=pic.userMax.x;
    bounds mx=autoscale(xmin,xmax,pic.scale.x.scale);
    pic.scale.x.tickMin=mx.min;
    pic.scale.x.tickMax=mx.max;
    axis.xdivisor=mx.divisor;
  }
  
  axis(pic,axis);
  
  if(xmin == -infinity && !axis.extend) {
    if(pic.scale.set && pic.scale.x.automin())
      xmin=pic.scale.x.tickMin;
    else xmin=pic.userMin.x;
  }
  
  if(xmax == infinity && !axis.extend) {
    if(pic.scale.set && pic.scale.x.automax())
      xmax=pic.scale.x.tickMax;
    else xmax=pic.userMax.x;
  }

  if(L.defaultposition) {
    L=L.copy();
    L.position(axis.position);
  }
  
  bool back=false;
  if(axis.type == Both) {
    triple v=currentprojection.vector();
    back=dot((0,pic.userMax.y-pic.userMin.y,0),v)*sgn(v.z) > 0;
  }

  xaxis3At(pic,L,axis,xmin,xmax,p,ticks,arrow,above,false,false,!back);
  if(axis.type == Both)
    xaxis3At(pic,L,axis,xmin,xmax,p,ticks,arrow,above,true,false,back);
  if(axis.type2 == Both) {
    xaxis3At(pic,L,axis,xmin,xmax,p,ticks,arrow,above,false,true,false);
    if(axis.type == Both)
      xaxis3At(pic,L,axis,xmin,xmax,p,ticks,arrow,above,true,true,false);
  }
}

// Draw a y axis in three dimensions.
void yaxis3(picture pic=currentpicture, Label L="", axis axis=XZZero,
            real ymin=-infinity, real ymax=infinity, pen p=currentpen,
            ticks3 ticks=NoTicks3, arrowbar3 arrow=None, bool above=false)
{
  if(ymin > ymax) return;

  if(pic.scale.y.automin && ymin > -infinity) pic.scale.y.automin=false;
  if(pic.scale.y.automax && ymax < infinity) pic.scale.y.automax=false;
  
  if(!pic.scale.set) {
    axis(pic,axis);
    autoscale3(pic,axis);
  }
  
  bool newticks=false;
  
  if(ymin != -infinity) {
    ymin=pic.scale.y.T(ymin);
    newticks=true;
  }
  
  if(ymax != infinity) {
    ymax=pic.scale.y.T(ymax);
    newticks=true;
  }
  
  if(newticks && pic.userSety && ticks != NoTicks3) {
    if(ymin == -infinity) ymin=pic.userMin.y;
    if(ymax == infinity) ymax=pic.userMax.y;
    bounds my=autoscale(ymin,ymax,pic.scale.y.scale);
    pic.scale.y.tickMin=my.min;
    pic.scale.y.tickMax=my.max;
    axis.ydivisor=my.divisor;
  }
  
  axis(pic,axis);
  
  if(ymin == -infinity && !axis.extend) {
    if(pic.scale.set && pic.scale.y.automin())
      ymin=pic.scale.y.tickMin;
    else ymin=pic.userMin.y;
  }
  
  if(ymax == infinity && !axis.extend) {
    if(pic.scale.set && pic.scale.y.automax())
      ymax=pic.scale.y.tickMax;
    else ymax=pic.userMax.y;
  }

  if(L.defaultposition) {
    L=L.copy();
    L.position(axis.position);
  }
  
  bool back=false;
  if(axis.type == Both) {
    triple v=currentprojection.vector();
    back=dot((pic.userMax.x-pic.userMin.x,0,0),v)*sgn(v.z) > 0;
  }

  yaxis3At(pic,L,axis,ymin,ymax,p,ticks,arrow,above,false,false,!back);

  if(axis.type == Both)
    yaxis3At(pic,L,axis,ymin,ymax,p,ticks,arrow,above,true,false,back);
  if(axis.type2 == Both) {
    yaxis3At(pic,L,axis,ymin,ymax,p,ticks,arrow,above,false,true,false);
    if(axis.type == Both)
      yaxis3At(pic,L,axis,ymin,ymax,p,ticks,arrow,above,true,true,false);
  }
}
// Draw a z axis in three dimensions.
void zaxis3(picture pic=currentpicture, Label L="", axis axis=XYZero,
            real zmin=-infinity, real zmax=infinity, pen p=currentpen,
            ticks3 ticks=NoTicks3, arrowbar3 arrow=None, bool above=false)
{
  if(zmin > zmax) return;

  if(pic.scale.z.automin && zmin > -infinity) pic.scale.z.automin=false;
  if(pic.scale.z.automax && zmax < infinity) pic.scale.z.automax=false;
  
  if(!pic.scale.set) {
    axis(pic,axis);
    autoscale3(pic,axis);
  }
  
  bool newticks=false;
  
  if(zmin != -infinity) {
    zmin=pic.scale.z.T(zmin);
    newticks=true;
  }
  
  if(zmax != infinity) {
    zmax=pic.scale.z.T(zmax);
    newticks=true;
  }
  
  if(newticks && pic.userSetz && ticks != NoTicks3) {
    if(zmin == -infinity) zmin=pic.userMin.z;
    if(zmax == infinity) zmax=pic.userMax.z;
    bounds mz=autoscale(zmin,zmax,pic.scale.z.scale);
    pic.scale.z.tickMin=mz.min;
    pic.scale.z.tickMax=mz.max;
    axis.zdivisor=mz.divisor;
  }
  
  axis(pic,axis);
  
  if(zmin == -infinity && !axis.extend) {
    if(pic.scale.set && pic.scale.z.automin())
      zmin=pic.scale.z.tickMin;
    else zmin=pic.userMin.z;
  }
  
  if(zmax == infinity && !axis.extend) {
    if(pic.scale.set && pic.scale.z.automax())
      zmax=pic.scale.z.tickMax;
    else zmax=pic.userMax.z;
  }

  if(L.defaultposition) {
    L=L.copy();
    L.position(axis.position);
  }
  
  bool back=false;
  if(axis.type == Both) {
    triple v=currentprojection.vector();
    back=dot((pic.userMax.x-pic.userMin.x,0,0),v)*sgn(v.y) > 0;
  }

  zaxis3At(pic,L,axis,zmin,zmax,p,ticks,arrow,above,false,false,!back);
  if(axis.type == Both)
    zaxis3At(pic,L,axis,zmin,zmax,p,ticks,arrow,above,true,false,back);
  if(axis.type2 == Both) {
    zaxis3At(pic,L,axis,zmin,zmax,p,ticks,arrow,above,false,true,false);
    if(axis.type == Both)
      zaxis3At(pic,L,axis,zmin,zmax,p,ticks,arrow,above,true,true,false);
  }
}

// Set the z limits of a picture.
void zlimits(picture pic=currentpicture, real min=-infinity, real max=infinity,
             bool crop=NoCrop)
{
  if(min > max) return;
  
  pic.scale.z.automin=min <= -infinity;
  pic.scale.z.automax=max >= infinity;
  
  bounds mz;
  if(pic.scale.z.automin() || pic.scale.z.automax())
    mz=autoscale(pic.userMin.z,pic.userMax.z,pic.scale.z.scale);
  
  if(pic.scale.z.automin) {
    if(pic.scale.z.automin()) pic.userMinz(mz.min);
  } else pic.userMinz(pic.scale.z.T(min));
  
  if(pic.scale.z.automax) {
    if(pic.scale.z.automax()) pic.userMaxz(mz.max);
  } else pic.userMaxz(pic.scale.z.T(max));
}

// Restrict the x, y, and z limits to box(min,max).
void limits(picture pic=currentpicture, triple min, triple max)
{
  xlimits(pic,min.x,max.x);
  ylimits(pic,min.y,max.y);
  zlimits(pic,min.z,max.z);
}
  
// Draw x, y and z axes.
void axes3(picture pic=currentpicture,
           Label xlabel="", Label ylabel="", Label zlabel="", 
	   triple min=(-infinity,-infinity,-infinity),
	   triple max=(infinity,infinity,infinity),
           pen p=currentpen, arrowbar3 arrow=None)
{
  xaxis3(pic,xlabel,min.x,max.x,p,arrow);
  yaxis3(pic,ylabel,min.y,max.y,p,arrow);
  zaxis3(pic,zlabel,min.z,max.z,p,arrow);
}

triple Scale(picture pic=currentpicture, triple v)
{
  return (pic.scale.x.T(v.x),pic.scale.y.T(v.y),pic.scale.z.T(v.z));
}

real ScaleZ(picture pic=currentpicture, real z)
{
  return pic.scale.z.T(z);
}

// Draw a tick of length size at triple v in direction dir using pen p.
void tick(picture pic=currentpicture, triple v, triple dir, real size=Ticksize,
          pen p=currentpen)
{
  pic.add(new void (picture f, transform3 t) {
      triple tv=t*v;
      draw(f,tv--tv+unit(dir)*size,p);
    });
  pic.addPoint(v,p);
  pic.addPoint(v,unit(dir)*size,p);
}

void xtick(picture pic=currentpicture, triple v, triple dir=Y,
           real size=Ticksize, pen p=currentpen)
{
  tick(pic,Scale(pic,v),dir,size,p);
}

void xtick3(picture pic=currentpicture, real x, triple dir=Y,
            real size=Ticksize, pen p=currentpen)
{
  xtick(pic,(x,pic.scale.y.scale.logarithmic ? 1 : 0,
             pic.scale.z.scale.logarithmic ? 1 : 0),dir,size,p);
}

void ytick(picture pic=currentpicture, triple v, triple dir=X,
           real size=Ticksize, pen p=currentpen) 
{
  xtick(pic,v,dir,size,p);
}

void ytick(picture pic=currentpicture, real y, triple dir=X,
           real size=Ticksize, pen p=currentpen)
{
  xtick(pic,(pic.scale.x.scale.logarithmic ? 1 : 0,y,
             pic.scale.z.scale.logarithmic ? 1 : 0),dir,size,p);
}

void ztick(picture pic=currentpicture, triple v, triple dir=X,
           real size=Ticksize, pen p=currentpen) 
{
  xtick(pic,v,dir,size,p);
}

void ztick(picture pic=currentpicture, real z, triple dir=X,
           real size=Ticksize, pen p=currentpen)
{
  xtick(pic,(pic.scale.x.scale.logarithmic ? 1 : 0,
             pic.scale.y.scale.logarithmic ? 1 : 0,z),dir,size,p);
}

void tick(picture pic=currentpicture, Label L, real value, triple v,
          triple dir, string format="", real size=Ticksize, pen p=currentpen)
{
  Label L=L.copy();
  L.align(L.align,-dir);
  if(shift(L.T3)*O == O) {
    L.T3=shift(dot(dir,L.align.dir3) > 0 ? dir*size :
               ticklabelshift(L.align.dir3,p))*L.T3;
  }
  L.p(p);
  if(L.s == "") L.s=format(format == "" ? defaultformat : format,value);
  L.s=baseline(L.s,L.align,"$10^4$");
  label(pic,L,v);
  xtick(pic,v,dir,size,p);
}

void xtick(picture pic=currentpicture, Label L, triple v, triple dir=Y,
           string format="", real size=Ticksize, pen p=currentpen)
{
  tick(pic,L,v.x,v,dir,format,size,p);
}

void xtick3(picture pic=currentpicture, Label L, real x, triple dir=Y,
            string format="", real size=Ticksize, pen p=currentpen)
{
  xtick(pic,L,(x,pic.scale.y.scale.logarithmic ? 1 : 0,
               pic.scale.z.scale.logarithmic ? 1 : 0),dir,size,p);
}

void ytick(picture pic=currentpicture, Label L, triple v, triple dir=X,
           string format="", real size=Ticksize, pen p=currentpen)
{
  tick(pic,L,v.y,v,dir,format,size,p);
}

void ytick3(picture pic=currentpicture, Label L, real y, triple dir=X,
            string format="", real size=Ticksize, pen p=currentpen)
{
  ytick(pic,L,(pic.scale.x.scale.logarithmic ? 1 : 0,y,
               pic.scale.z.scale.logarithmic ? 1 : 0),dir,format,size,p);
}

void ztick(picture pic=currentpicture, Label L, triple v, triple dir=X,
           string format="", real size=Ticksize, pen p=currentpen)
{
  tick(pic,L,v.z,v,dir,format,size,p);
}

void ztick3(picture pic=currentpicture, Label L, real z, triple dir=X,
            string format="", real size=Ticksize, pen p=currentpen)
{
  ztick(pic,L,(pic.scale.x.scale.logarithmic ? 1 : 0,
               pic.scale.z.scale.logarithmic ? 1 : 0,z),dir,format,size,p);
}

private void label(picture pic, Label L, triple v, real x, align align,
                   string format, pen p)
{
  Label L=L.copy();
  L.align(align);
  L.p(p);
  if(shift(L.T3)*O == O)
    L.T3=shift(ticklabelshift(L.align.dir3,L.p))*L.T3;
  if(L.s == "") L.s=format(format == "" ? defaultformat : format,x);
  L.s=baseline(L.s,L.align,"$10^4$");
  label(pic,L,v);
}

void labelx(picture pic=currentpicture, Label L="", triple v,
            align align=-Y, string format="", pen p=nullpen)
{
  label(pic,L,Scale(pic,v),v.x,align,format,p);
}

void labelx3(picture pic=currentpicture, Label L="", real x,
             align align=-Y, string format="", pen p=nullpen)
{
  labelx(pic,L,(x,pic.scale.y.scale.logarithmic ? 1 : 0,
                pic.scale.z.scale.logarithmic ? 1 : 0),align,format,p);
}

void labely(picture pic=currentpicture, Label L="", triple v,
            align align=-X, string format="", pen p=nullpen)
{
  label(pic,L,Scale(pic,v),v.y,align,format,p);
}

void labely3(picture pic=currentpicture, Label L="", real y,
             align align=-X, string format="", pen p=nullpen)
{
  labely(pic,L,(pic.scale.x.scale.logarithmic ? 1 : 0,y,
                pic.scale.z.scale.logarithmic ? 1 : 0),align,format,p);
}

void labelz(picture pic=currentpicture, Label L="", triple v,
            align align=-X, string format="", pen p=nullpen)
{
  label(pic,L,Scale(pic,v),v.z,align,format,p);
}

void labelz3(picture pic=currentpicture, Label L="", real z,
             align align=-X, string format="", pen p=nullpen)
{
  labelz(pic,L,(pic.scale.x.scale.logarithmic ? 1 : 0,
                pic.scale.y.scale.logarithmic ? 1 : 0,z),align,format,p);
}

typedef guide3 graph(triple F(real), real, real, int);

graph graph(interpolate3 join)
{
  return new guide3(triple f(real), real a, real b, int n) {
    real width=b-a;
    return n == 0 ? join(f(a)) :
      join(...sequence(new guide3(int i) {
            return f(a+(i/n)*width);
          },n+1));
  };
}

guide3 Straight(... guide3[])=operator --;
guide3 Spline(... guide3[])=operator ..;
                       
guide3 graph(picture pic=currentpicture, real x(real), real y(real),
             real z(real), real a, real b, int n=ngraph,
             interpolate3 join=operator --)
{
  return graph(join)(new triple(real t) {return Scale(pic,(x(t),y(t),z(t)));},
                     a,b,n);
}

guide3 graph(picture pic=currentpicture, triple v(real), real a, real b,
             int n=ngraph, interpolate3 join=operator --)
{
  return graph(join)(new triple(real t) {return Scale(pic,v(t));},a,b,n);
}

int[] conditional(triple[] v, bool[] cond)
{
  if(cond.length > 0) {
    checklengths(cond.length,v.length,conditionlength);
    return cond ? sequence(cond.length) : null;
  } else return sequence(v.length);
}

guide3 graph(picture pic=currentpicture, triple[] v, bool[] cond={},
             interpolate3 join=operator --)
{
  int[] I=conditional(v,cond);
  int k=0;
  return graph(join)(new triple(real) {
      int i=I[k]; ++k;
      return Scale(pic,v[i]);}
    ,0,0,I.length-1);
}

guide3 graph(picture pic=currentpicture, real[] x, real[] y, real[] z,
             bool[] cond={}, interpolate3 join=operator --)
{
  checklengths(x.length,y.length);
  checklengths(x.length,z.length);
  int[] I=conditional(x,cond);
  int k=0;
  return graph(join)(new triple(real) {
      int i=I[k]; ++k;
      return Scale(pic,(x[i],y[i],z[i]));
    },0,0,I.length-1);
}

// The graph of a function along a path.
guide3 graph(triple F(path, real), path p, int n=1,
             interpolate3 join=operator --)
{
  guide3 g=join(...sequence(new guide3(int i) {
        return F(p,i/n);
      },n*length(p)));
  return cyclic(p) ? join(g,cycle) : join(g,F(p,length(p)));
}

guide3 graph(triple F(pair), path p, int n=1, interpolate3 join=operator --)
{
  return graph(new triple(path p, real position) 
               {return F(point(p,position));},p,n,join);
}

guide3 graph(picture pic=currentpicture, real f(pair), path p, int n=1,
             interpolate3 join=operator --) 
{
  return graph(new triple(pair z) {return Scale(pic,(z.x,z.y,f(z)));},p,n,
               join);
}

guide3 graph(real f(pair), path p, int n=1, real T(pair),
             interpolate3 join=operator --)
{
  return graph(new triple(pair z) {pair w=T(z); return (w.x,w.y,f(w));},p,n,
               join);
}

// Connect points in v into segments corresponding to consecutive true elements
// of b using interpolation operator join. 
path3[] segment(triple[] v, bool[] b, interpolate3 join=operator --)
{
  checklengths(v.length,b.length,conditionlength);
  int[][] segment=segment(b);
  return sequence(new path3(int i) {return join(... v[segment[i]]);},
                  segment.length);
}

// return the surface described by a matrix f
surface surface(triple[][] f, bool[][] cond={})
{
  if(!rectangular(f)) abort("matrix is not rectangular");
  
  int nx=f.length-1;
  int ny=nx > 0 ? f[0].length-1 : 0;
  
  bool all=cond.length == 0;

  int count;
  if(all)
    count=nx*ny;
  else {
    count=0;
    for(int i=0; i < nx; ++i) {
      bool[] condi=cond[i];
      for(int j=0; j < ny; ++j)
        if(condi[j]) ++count;
    }
  }

  surface s=surface(count);
  int k=-1;
  for(int i=0; i < nx; ++i) {
    bool[] condi=all ? null : cond[i];
    triple[] fi=f[i];
    triple[] fp=f[i+1];
    for(int j=0; j < ny; ++j) {
      if(all || condi[j])
        s.s[++k]=patch(new triple[] {fi[j],fp[j],fp[j+1],fi[j+1]});
    }
  }
  return s;
}

private surface bispline(real[][] z, real[][] p, real[][] q, real[][] r,
                         real[] x, real[] y, bool[][] cond={})
{ // z[i][j] is the value at (x[i],y[j])
  // p and q are the first derivatives with respect to x and y, respectively
  // r is the second derivative ddu/dxdy
  int n=x.length-1;
  int m=y.length-1;

  bool all=cond.length == 0;

  int count;
  if(all)
    count=n*m;
  else {
    count=0;
    for(int i=0; i < n; ++i) {
      bool[] condi=cond[i];
      for(int j=0; j < m; ++j)
        if(condi[j]) ++count;
    }
  }

  surface g=surface(count);
  int k=-1;
  for(int i=0; i < n; ++i) {
    bool[] condi=all ? null : cond[i];
    real xi=x[i];
    real[] zi=z[i];
    real[] zp=z[i+1];
    real[] ri=r[i];
    real[] rp=r[i+1];
    real[] pi=p[i];
    real[] pp=p[i+1];
    real[] qi=q[i];
    real[] qp=q[i+1];
    real xp=x[i+1];
    real hx=(xp-xi)/3;
    for(int j=0; j < m; ++j) {
      real yj=y[j];
      real yp=y[j+1];
      if(all || condi[j]) {
        triple[][] P={
          {O,O,O,O},
          {O,O,O,O},
          {O,O,O,O},
          {O,O,O,O}};
        real hy=(yp-yj)/3;
        real hxy=hx*hy;
        // first x and y  directions
        for(int k=0 ; k < 4 ; ++k) {
          P[k][0] += xi*X;
          P[0][k] += yj*Y;
          P[k][1] += (xp+2*xi)/3*X;
          P[1][k] += (yp+2*yj)/3*Y;
          P[k][2] += (2*xp+xi)/3*X;
          P[2][k] += (2*yp+yj)/3*Y;
          P[k][3] += xp*X;
          P[3][k] += yp*Y;
        }
        // now z, first the value 
        P[0][0] += zi[j]*Z;
        P[0][3] += zp[j]*Z;
        P[3][0] += zi[j+1]*Z;
        P[3][3] += zp[j+1]*Z;
        // 2nd, first derivative
        P[0][1] += (P[0][0].z+hx*pi[j])*Z;
        P[3][1] += (P[3][0].z+hx*pi[j+1])*Z;
        P[0][2] += (P[0][3].z-hx*pp[j])*Z;
        P[3][2] += (P[3][3].z-hx*pp[j+1])*Z;
        P[1][0] += (P[0][0].z+hy*qi[j])*Z;
        P[1][3] += (P[0][3].z+hy*qp[j])*Z;
        P[2][0] += (P[3][0].z-hy*qi[j+1])*Z;
        P[2][3] += (P[3][3].z-hy*qp[j+1])*Z;
        // 3nd, second derivative
        P[1][1] += (P[1][0].z+P[0][1].z-P[0][0].z+hxy*ri[j])*Z;
        P[2][1] += (P[2][0].z+P[3][1].z-P[3][0].z-hxy*ri[j+1])*Z;
        P[1][2] += (P[0][2].z+P[1][3].z-P[0][3].z-hxy*rp[j])*Z;
        P[2][2] += (P[3][2].z+P[2][3].z-P[3][3].z+hxy*rp[j+1])*Z;
        g.s[++k]=patch(P);
      }
    }
  }
  return g;
}

// return the surface described by a real matrix f, interpolated with
// splinetype.
surface surface(real[][] f, real[] x, real[] y,
                splinetype splinetype=null, bool[][] cond={})
{
  if(splinetype == null)
    splinetype=(x[0] == x[x.length-1] && y[0] == y[y.length-1]) ? 
      periodic : notaknot;
  int n=x.length; int m=y.length;
  real[][] ft=transpose(f);
  real[][] tp=new real[m][];
  for(int j=0; j < m ; ++j)
    tp[j]=splinetype(x,ft[j]);
  real[][] q=new real[n][];
  for(int i=0; i < n ; ++i)
    q[i]=splinetype(y,f[i]);
  real[][] qt=transpose(q);
  real[] d1=splinetype(x,qt[0]);
  real[] d2=splinetype(x,qt[m-1]);
  real[][] r=new real[n][];
  for(int i=0; i < n ; ++i)
    r[i]=clamped(d1[i],d2[i])(y,f[i]);
  return bispline(f,transpose(tp),q,r,x,y,cond);
}

// return the surface described by a real matrix f, interpolated with
// splinetype.
surface surface(real[][] f, pair a, pair b, splinetype splinetype,
                bool[][] cond={})
{
  if(!rectangular(f)) abort("matrix is not rectangular");

  int nx=f.length-1;
  int ny=nx > 0 ? f[0].length-1 : 0;

  if(nx == 0 || ny == 0) return nullsurface;

  real[] x=uniform(a.x,b.x,nx);
  real[] y=uniform(a.y,b.y,ny);
  return surface(f,x,y,splinetype,cond);
}

// return the surface described by a real matrix f, interpolated linearly.
surface surface(real[][] f, pair a, pair b, bool[][] cond={})
{
  if(!rectangular(f)) abort("matrix is not rectangular");

  int nx=f.length-1;
  int ny=nx > 0 ? f[0].length-1 : 0;

  if(nx == 0 || ny == 0) return nullsurface;

  triple[][] v=new triple[nx+1][ny+1];
  for(int i=0; i <= nx; ++i) {
    real x=interp(a.x,b.x,i/nx);
    for(int j=0; j <= ny; ++j) {
      v[i][j]=(x,interp(a.y,b.y,j/ny),f[i][j]);
    }
  }
  return surface(v,cond);
}

// return the surface described by a parametric function f over box(a,b),
// interpolated linearly.
surface surface(triple f(pair z), pair a, pair b, int nu=nmesh, int nv=nu,
                bool cond(pair z)=null)
{
  if(nu <= 0 || nv <= 0) return nullsurface;

  bool[][] active;
  bool all=cond == null;
  if(!all) active=new bool[nu+1][nv+1];

  real du=1/nu;
  real dv=1/nv;
  pair Idv=(0,dv);
  pair dz=(du,dv);

  triple[][] v=new triple[nu+1][nv+1];

  for(int i=0; i <= nu; ++i) {
    bool[] activei=all ? null : active[i];
    real x=interp(a.x,b.x,i*du);
    for(int j=0; j <= nv; ++j) {
      pair z=(x,interp(a.y,b.y,j*dv));
      v[i][j]=f(z);
      if(!all)
        activei[j]=cond(z) || cond(z+du) || cond(z+Idv) || cond(z+dz);
    }
  }
  return surface(v,active);
}
  
// return the surface described by a real function f over box(a,b),
// interpolated linearly.
surface surface(real f(pair z), pair a, pair b, int nx=nmesh, int ny=nx,
                bool cond(pair z)=null)
{
  return surface(new triple(pair z) {return (z.x,z.y,f(z));},a,b,nx,ny,cond);
}

// return the surface described by a real function f over box(a,b),
// interpolated with splinetype.
surface surface(real f(pair z), pair a, pair b, int nx=nmesh, int ny=nx,
                splinetype splinetype, bool cond(pair z)=null)
{
  bool[][] active;
  bool all=cond == null;
  if(!all) active=new bool[nx+1][ny+1];

  real dx=1/nx;
  real dy=1/ny;
  pair Idy=(0,dy);
  pair dz=(dx,dy);

  real[][] F=new real[nx+1][ny+1];
  real[] x=uniform(a.x,b.x,nx);
  real[] y=uniform(a.y,b.y,ny);
  for(int i=0; i <= nx; ++i) {
    bool[] activei=all ? null : active[i];
    real x=x[i];
    for(int j=0; j <= ny; ++j) {
      pair z=(x,y[j]);
      F[i][j]=f(z);
      if(!all)
        activei[j]=cond(z) || cond(z+dx) || cond(z+Idy) || cond(z+dz);
    }
  }
  return surface(F,x,y,splinetype,active);
}

guide3[][] lift(real f(real x, real y), guide[][] g,
                interpolate3 join=operator --)
{
  guide3[][] G=new guide3[g.length][];
  for(int cnt=0; cnt < g.length; ++cnt) {
    guide[] gcnt=g[cnt];
    guide3[] Gcnt=new guide3[gcnt.length];
    for(int i=0; i < gcnt.length; ++i) {
      guide gcnti=gcnt[i];
      guide3 Gcnti=join(...sequence(new guide3(int j) {
            pair z=point(gcnti,j);
            return (z.x,z.y,f(z.x,z.y));
          },size(gcnti)));
      if(cyclic(gcnti)) Gcnti=Gcnti..cycle;
      Gcnt[i]=Gcnti;
    }
    G[cnt]=Gcnt;
  }
  return G;
}

guide3[][] lift(real f(pair z), guide[][] g, interpolate3 join=operator --)
{
  return lift(new real(real x, real y) {return f((x,y));},g,join);
}

void draw(picture pic=currentpicture, Label[] L=new Label[],
          guide3[][] g, pen[] p)
{
  pen thin=is3D() ? thin() : defaultpen;
  begingroup3(pic);
  for(int cnt=0; cnt < g.length; ++cnt) {
    guide3[] gcnt=g[cnt];
    pen pcnt=thin+p[cnt];
    for(int i=0; i < gcnt.length; ++i)
      draw(pic,gcnt[i],pcnt);
    if(L.length > 0) {
      Label Lcnt=L[cnt];
      for(int i=0; i < gcnt.length; ++i) {
        if(Lcnt.s != "" && size(gcnt[i]) > 1)
          label(pic,Lcnt,gcnt[i],pcnt);
      }
    }
  }
  endgroup3(pic);
}

void draw(picture pic=currentpicture, Label[] L=new Label[],
          guide3[][] g, pen p=currentpen)
{
  draw(pic,L,g,sequence(new pen(int) {return p;},g.length));
}

picture vectorfield(path3 vector(pair z), triple f(pair z),
                    pair a, pair b, int nx=nmesh, int ny=nx,
                    bool autoscale=true,
                    pen p=currentpen, arrowbar3 arrow=Arrow3)
{
  picture pic;
  real dx=1/nx;
  real dy=1/ny;
  real scale;
  if(autoscale) {
    real size(pair z) {
      path3 g=vector(z);
      return abs(point(g,size(g)-1)-point(g,0));
    }
    real max=size((0,0));
    for(int i=0; i <= nx; ++i) {
      real x=interp(a.x,b.x,i*dx);
      for(int j=0; j <= ny; ++j)
        max=max(max,size((x,interp(a.y,b.y,j*dy))));
    }
    pair lambda=(abs(f((b.x,a.y))-f(a)),abs(f((a.x,b.y))-f(a)));
    scale=min(lambda.x/nx,lambda.y/ny)/max;
  } else scale=1;
  for(int i=0; i <= nx; ++i) {
    real x=interp(a.x,b.x,i*dx);
    for(int j=0; j <= ny; ++j) {
      real y=interp(a.y,b.y,j*dy);
      pair z=(x,y);
      draw(pic,shift(f(z))*scale3(scale)*vector(z),p,arrow);
    }
  }
  return pic;
}

triple polar(real r, real theta, real phi)
{
  return r*expi(theta,phi);
}

guide3 polargraph(real r(real,real), real theta(real), real phi(real),
                  int n=ngraph, interpolate3 join=operator --)
{
  return graph(join)(new triple(real t) {
      return polar(r(theta(t),phi(t)),theta(t),phi(t));
    },0,1,n);
}

// True arc
path3 Arc(triple c, triple v1, triple v2, triple normal=O, bool direction=CCW,
          int n=nCircle)
{
  v1 -= c;
  real r=abs(v1);
  v1=unit(v1);
  v2=unit(v2-c);

  if(normal == O) {
    normal=cross(v1,v2);
    if(normal == O) abort("explicit normal required for these endpoints");
  }

  transform3 T=align(unit(normal));
  transform3 Tinv=transpose(T);
  v1=Tinv*v1;
  v2=Tinv*v2;

  static real epsilon=sqrt(realEpsilon);
  real fuzz=epsilon*max(abs(v1),abs(v2));
  if(abs(v1.z) > fuzz || abs(v2.z) > fuzz)
    abort("invalid normal vector");

  real phi1=radians(longitude(v1,warn=false));
  real phi2=radians(longitude(v2,warn=false));
  if(phi1 >= phi2 && direction) phi1 -= 2pi;
  if(phi2 >= phi1 && !direction) phi2 -= 2pi;

  real piby2=pi/2;
  return shift(c)*T*polargraph(new real(real theta, real phi) {return r;},
                               new real(real t) {return piby2;},
                               new real(real t) {return interp(phi1,phi2,t);},
                               n,operator ..);
}

path3 Arc(triple c, real r, real theta1, real phi1, real theta2, real phi2,
          triple normal=O, bool direction, int n=nCircle)
{
  return Arc(c,c+r*dir(theta1,phi1),c+r*dir(theta2,phi2),normal,direction,n);
}

path3 Arc(triple c, real r, real theta1, real phi1, real theta2, real phi2,
          triple normal=O, int n=nCircle)
{
  return Arc(c,r,theta1,phi1,theta2,phi2,normal,
             theta2 > theta1 || (theta2 == theta1 && phi2 >= phi1) ? CCW : CW,
             n);
}

// True circle
path3 Circle(triple c, real r, triple normal=Z, int n=nCircle)
{
  return Arc(c,r,90,0,90,360,normal,n)&cycle;
}
