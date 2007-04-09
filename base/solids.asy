import three;
import light;
import graph3;

// A solid geometry package.

// Try to find a bounding tangent line between two paths.
real[] tangent(path p, path q, bool side) 
{
  if((cyclic(p) && inside(p,point(q,0)) || 
      cyclic(q) && inside(q,point(p,0))) &&
     intersect(p,q).length == 0) return new real[];

  real time(path p) {
    pair m=min(p);
    pair M=max(p);
    path edge=side ? (m.x,m.y)--(M.x,m.y) : (m.x,M.y)--(M.x,M.y);
    return intersect(p,edge)[0];
  }

  static real epsilon=sqrt(realEpsilon);
  
  for(int i=0; i < 100; ++i) {
    real ta=time(p);
    real tb=time(q);
    pair a=point(p,ta);
    pair b=point(q,tb);
    real angle=angle(b-a,warn=false);
    if(abs(angle) <= epsilon || abs(abs(0.5*angle)-pi) <= epsilon) {
      return new real[] {ta,tb};
    }
    transform t=rotate(-degrees(angle));
    p=t*p;
    q=t*q;
  }
  return new real[];
}

path line(path p, path q, real[] t) 
{
  return point(p,t[0])--point(q,t[1]);
}

// Return a generalized cylinder of height h constructed from area base in
// the XY plane and aligned with axis.
path[] cylinder(path3 base, real h, triple axis=Z,
                projection P=currentprojection) 
{
  base=rotate(-colatitude(axis),cross(axis,Z))*base;
  path3 top=shift(h*axis)*base;
  path Base=project(base,P);
  path Top=project(top,P);
  real[] t1=tangent(Base,Top,true);
  real[] t2=tangent(Base,Top,false);
  path p=subpath(Base,t1[0],t2[0]);
  path q=subpath(Base,t2[0],t1[0]);
  return base^^project(top,P)^^line(Base,Top,t1)^^line(Base,Top,t2);
}

// The three-dimensional "wireframe" used to visualize a volume of revolution
struct skeleton {
  // transverse skeleton (perpendicular to axis of revolution)
  path3[] front;
  path3[] back;
  // longitudinal skeleton (parallel to axis of revolution)
  path3[] longitudinal;
}

// A surface of revolution generated by rotating a planar path3 g, assumed
// to be straight between nodes, from angle1 to angle2 about c--c+axis.
struct revolution {
  triple c;
  path3 g;
  triple axis;
  real angle1,angle2;
  
  void init(triple c=O, path3 g, triple axis=Z, real angle1=0,
            real angle2=360) {
    this.c=c;
    this.g=g;
    this.axis=unit(axis);
    this.angle1=angle1;
    this.angle2=angle2;
  }
  
  revolution copy() {
    revolution r=new revolution;
    r.init(c,g,axis,angle1,angle2);
    return r;
  }
  
  private real scalefactor() {return abs(c)+max(abs(min(g)),abs(max(g)));}

  // Fill on picture pic the surface of rotation generated by rotating g
  // from angle1 to angle2 sampled n times about the line c--c+axis, using
  // the spatially dependent pen color(triple).
  void fill(picture pic=currentpicture, int n=32, pen color(triple),
            projection P=currentprojection) {
    real s=(angle2-angle1)/n;
  
    triple normal=normal(g);
    if(abs(normal) <= epsilon) normal=unit(cross(dir(g,0),axis));
    if(abs(normal) <= epsilon) normal=unit(cross(point(g,0)-c,axis));

    triple perp=cross(normal,axis);
  
    int L=length(g);
    triple[] point=new triple[L+1];
    triple[] midpoint=new triple[L+1];
  
    for(int i=0; i <= L; ++i) {
      point[i]=point(g,i);
      midpoint[i]=point(g,i+0.5);
    }
  
    triple surface(triple v, real j) {
      triple center=c+dot(v-c,axis)*axis;
      return center+abs(v-center)*(Cos(j)*perp+Sin(j)*normal);
    }
  
    triple vertex(int i, real j) {return surface(point[i],j);}
    triple center(int i, real j) {return surface(midpoint[i],j);}

    int[] edges={0,0,0,2};
    real depth[][];
  
    begingroup(pic);
    for(int i=0; i < L; ++i) {
      triple camera=P.camera;
      if(P.infinity)
        camera *= scalefactor();
      real phi=angle1;
      for(int j=0; j < n; ++j, phi += s) {
        real d=abs(camera-center(i,phi+0.5s));
        depth.push(new real[] {d,i,phi});
      }
    }
  
    depth=sort(depth);
  
    while(depth.length > 0) {
      real[] a=depth.pop();
      int i=round(a[1]);
      real j=a[2];
      triple[] v={vertex(i,j),vertex(i+1,j),vertex(i+1,j+s),vertex(i,j+s)};
      pen[] p={color(v[0]),color(v[1]),color(v[2]),color(v[3])};
      gouraudshade(pic,project(v[0]--v[1]--v[2]--v[3]--cycle3,P),p,v,edges);
    }
    endgroup(pic);
  }
  
  void fill(picture pic=currentpicture, int n=32, pen p=currentpen,
            projection P=currentprojection) {
    pen color(triple x) {return currentlight.intensity(x-c)*p;}
    fill(pic,n,color,P);
  }
  
  path3 slice(real position, int ngraph) {
    triple v=point(g,position);
    triple center=c+dot(v-c,axis)*axis;
    //    return Arc(center,abs(v-center),90,angle1,90,angle2,axis,ngraph);
    return Circle(center,abs(v-center),axis,ngraph);
  }
  
  // add transverse slice to skeleton s
  void transverse(skeleton s, real t, int ngraph=32,
                  projection P=currentprojection) {
    static real epsilon=sqrt(realEpsilon);
    path3 S=slice(t,ngraph);
    triple camera=P.camera;
    if(P.infinity)
      camera *= scalefactor();
    if((t <= epsilon && dot(axis,camera) < 0) ||
       (t >= length(g)-epsilon && dot(axis,camera) >= 0))
      s.front.push(S);
    else {
      path3 Sp=slice(t+epsilon,ngraph);
      path3 Sm=slice(t-epsilon,ngraph);
      path sp=project(Sp,P);
      path sm=project(Sm,P);
      real[] t1=tangent(sp,sm,true);
      real[] t2=tangent(sp,sm,false);
      if(t1.length > 1 && t2.length > 1) {
        real t1=t1[0];
        real t2=t2[0];
        int len=length(S);
        if(t2 < t1) t2 += len;
        path3 p1=subpath(S,t1,t2);
        path3 p2=subpath(S,t2,t1+len);
        if(dot(point(p1,0.5*length(p1))-c,camera) >= 0) {
          s.front.push(p1);
          s.back.push(p2);
        } else {
          s.front.push(p2);
          s.back.push(p1);
        }
      }
    }
  }

  // add m evenly spaced transverse slices to skeleton s
  void transverse(skeleton s, int m=0, int ngraph=32,
                  projection P=currentprojection) {
    int N=size(g);
    int n=(m == 0) ? N : m;
    real factor=m == 1 ? 0 : 1/(m-1);
    for(int i=0; i < n; ++i) {
      real t=(m == 0) ? i : reltime(g,i*factor);
      transverse(s,t,ngraph,P);
    }
  }

  // add longitudinal curves to skeleton
  void longitudinal(skeleton s, int ngraph=32, projection P=currentprojection) {
    real t, d=0;
    // Find a point on g of maximal distance from the axis.
    int N=size(g);
    for(int i=0; i < N; ++i) {
      triple v=point(g,i);
      triple center=c+dot(v-c,axis)*axis;
      real r=abs(v-center);
      if(r > d) {
        t=i;
        d=r;
      }
    }
    triple v=point(g,t);
    path3 S=slice(t,ngraph);
    path3 Sm=slice(t+epsilon,ngraph);
    path3 Sp=slice(t-epsilon,ngraph);
    path sp=project(Sp,P);
    path sm=project(Sm,P);
    real[] t1=tangent(sp,sm,true);
    real[] t2=tangent(sp,sm,false);
    transform3 T=align(axis);
    real ref=longitude(T*(v-c),warn=false);
    real angle(real t) {return longitude(T*(point(S,t)-c),warn=false)-ref;}
    if(t1.length > 1)
      s.longitudinal.push(rotate(angle(t1[0]),c,c+axis)*g);
    if(t2.length > 1)
      s.longitudinal.push(rotate(angle(t2[0]),c,c+axis)*g);
  }
  
  skeleton skeleton(int m=0, int ngraph=32, projection P=currentprojection) {
    skeleton s;
    transverse(s,m,ngraph,P);
    longitudinal(s,ngraph,P);
    return s;
  }

  // Draw on picture pic the skeleton of the surface of rotation. Draw
  // the front portion of each of the m transverse slices with pen p and
  // the back portion with pen backpen.
  void draw(picture pic=currentpicture, int m=0, pen p=currentpen,
            pen backpen=p, bool longitudinal=true, pen longitudinalpen=p,
            projection P=currentprojection) {
    skeleton s=skeleton(m,P);
    begingroup(pic);
    draw(pic,s.back,linetype("8 8",8)+backpen);
    draw(pic,s.front,p);
    if(longitudinal) draw(pic,s.longitudinal,longitudinalpen);
    endgroup(pic);
  }
  
  void filldraw(picture pic=currentpicture, int n=32,
                pen fillpen=currentpen,  int m=2, pen drawpen=currentpen,
                bool longitudinal=false, projection P=currentprojection) {
    fill(pic,n,fillpen,P);
    draw(pic,m,drawpen,longitudinal,P);
  }
}

revolution revolution(triple c=O, path3 g, triple axis=Z, real angle1=0,
                      real angle2=360) 
{
  revolution r;
  r.init(c,g,axis,angle1,angle2);
  return r;
}

revolution operator * (transform3 t, revolution r)
{
  triple trc=t*r.c;
  return revolution(trc,t*r.g,t*(r.c+r.axis)-trc,r.angle1,r.angle2);
}

// Return the surface of rotation obtain by rotating the path3 (x,0,f(x))
// sampled n times between x=a and x=b about an axis lying in the XZ plane.
revolution revolution(triple c=O, real f(real x), real a, real b, int n=32,
                      triple axis=Z)
{
  real width=n == 0 ? 0 : (b-a)/n;
  guide3 g;
  for(int i=0; i <= n; ++i) {
    real x=a+width*i;
    g=g--(x,0,f(x));
  }
  return revolution(c,g,axis);
}

// Return a vector perpendicular to axis.
triple perp(triple axis)
{
  triple v=cross(axis,X);
  if(v == O) v=cross(axis,Y);
  return v;
}

// Return a right circular cylinder of height h in the direction of axis
// based on a circle centered at c with radius r.
revolution cylinder(triple c=O, real r, real h, triple axis=Z)
{
  triple C=c+r*perp(axis);
  axis=h*unit(axis);
  return revolution(c,C--C+axis,axis);
}

// Return a right circular cone of height h in the direction of axis
// based on a circle centered at c with radius r.
revolution cone(triple c=O, real r, real h, triple axis=Z)
{
  axis=unit(axis);
  return revolution(c,c+r*perp(axis)--c+h*axis,axis);
}

// Return a sphere of radius r centered at c sampled n times.
revolution sphere(triple c=O, real r, int n=32)
{
  return revolution(c,Arc(c,r,90,0,90,180,Y,n),Z);
}
