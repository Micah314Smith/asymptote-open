// Asymptote mathematics routines

public real perpsize=arrowsize;

real radians(real degrees)
{
  return degrees*pi/180;
}

real degrees(real radians) 
{
  return radians*180/pi;
}

int quadrant(real degrees)
{
  return floor(degrees/90) % 4;
}

// Roots of unity. 
pair zeta(int n, int k = 1)
{
  return expi(2pi*k/n);
}

real Sin(real deg) {return sin(radians(deg));}
real Cos(real deg) {return cos(radians(deg));}
real Tan(real deg) {return tan(radians(deg));}
real csc(real x) {return 1/sin(x);}
real sec(real x) {return 1/cos(x);}
real cot(real x) {return tan(pi/2-x);}
real frac(real x) {return x-(int)x;}

struct vector {
  public real x,y,z;
  void vector(real x, real y, real z) {this.x=x; this.y=y; this.z=z;}
}

void write(file out, vector v)
{
  write(out,"(");
  write(out,v.x); write(out,","); write(out,v.y); write(out,",");
  write(out,v.z);
  write(out,")");
}

void write(vector v)
{
  write(stdout,v); write(stdout,endl);
}

vector vector(real x, real y, real z)
{
  vector v=new vector;
  v.vector(x,y,z);
  return v;
}

real length(vector a)
{
  return sqrt(a.x^2+a.y^2+a.z^2);
}

vector operator - (vector a)
{
  return vector(-a.x,-a.y,-a.z);
}

vector operator + (vector a, vector b)
{
  return vector(a.x+b.x,a.y+b.y,a.z+b.z);
}

vector operator - (vector a, vector b)
{
  return vector(a.x-b.x,a.y-b.y,a.z-b.z);
}

vector operator * (vector a, real s)
{
  return vector(a.x*s,a.y*s,a.z*s);
}

vector operator * (real s,vector a)
{
  return a*s;
}

vector operator / (vector a, real s)
{
  return vector(a.x/s,a.y/s,a.z/s);
}

real dot(vector a, vector b)
{
  return a.x*b.x+a.y*b.y+a.z*b.z;
}

vector cross(vector a, vector b)
{
  return vector(a.y*b.z-a.z*b.y,
		a.z*b.x-a.x*b.z,
		a.x*b.y-b.x*a.y);
}

// Compute normal vector to the plane defined by the first 3 vectors of p.
vector normal(vector[] p)
{
  if(p.length < 3) abort("3 vectors are required");
  return cross(p[1]-p[0],p[2]-p[0]);
}

vector unit(vector p)
{
  return p/length(p);
}

vector unitnormal(vector[] p)
{
  return unit(normal(p));
}

// Return any point on the intersection of the two planes with normals
// n0 and n1 passing through points P0 and P1, respectively.
// If the planes are parallel return vector(infinity,infinity,infinity).
vector intersection(vector n0, vector P0, vector n1, vector P1)
{
  real Dx=n0.y*n1.z-n1.y*n0.z;
  real Dy=n0.z*n1.x-n1.z*n0.x;
  real Dz=n0.x*n1.y-n1.x*n0.y;
  if(abs(Dx) > abs(Dy) && abs(Dx) > abs(Dz)) {
    Dx=1/Dx;
    real d0=n0.y*P0.y+n0.z*P0.z;
    real d1=n1.y*P1.y+n1.z*P1.z+n1.x*(P1.x-P0.x);
    real y=(d0*n1.z-d1*n0.z)*Dx;
    real z=(d0*n0.y-d1*n1.y)*Dx;
    return vector(P0.x,y,z);
  } else if(abs(Dy) > abs(Dz)) {
    Dy=1/Dy;
    real d0=n0.z*P0.z+n0.x*P0.x;
    real d1=n1.z*P1.z+n1.x*P1.x+n1.y*(P1.y-P0.y);
    real z=(d0*n1.x-d1*n0.x)*Dy;
    real x=(d1*n0.z-d0*n1.z)*Dy;
    return vector(x,P0.y,z);
  } else {
    if(Dz == 0) return vector(infinity,infinity,infinity);
    Dz=1/Dz;
    real d0=n0.x*P0.x+n0.y*P0.y;
    real d1=n1.x*P1.x+n1.y*P1.y+n1.z*(P1.z-P0.z);
    real x=(d0*n1.y-d1*n0.y)*Dz;
    real y=(d1*n0.x-d0*n1.x)*Dz;
    return vector(x,y,P0.z);
  }
}

// Given a real array A, return its partial (optionally dx-weighted) sums.
real[] partialsum(real[] A, real[] dx=null) 
{
  real[] B=new real[];
  B[0]=0;
  if(alias(dx,null))
    for(int i=0; i < A.length; ++i) B[i+1]=B[i]+A[i];
  else
    for(int i=0; i < A.length; ++i) B[i+1]=B[i]+A[i]*dx[i];
  return B;
}

void perpendicular(picture pic=currentpicture, pair z1, pair z2, real
		   size=perpsize, pen p=currentpen)
{
  pair v=perpsize*unit(z2-z1);
  picture apic=new picture;
  _draw(apic,v--v+I*v--I*v,p);
  addabout(pic,apic,z1);
}

bool straight(guide p)
{
  for(int i=0; i < length(p); ++i)
    if(!straight(p,i)) return false;
  return true;
}

bool polygon(guide p)
{
  return cyclic(p) && straight(p);
}

// Returns true iff the point z lies in the region bounded by the cyclic
// polygon p.
bool inside(pair z, guide p)
{
  if(!polygon(p)) abort("Polygon must be a straight cyclic path");
  bool c=false;
  int n=length(p);
  for(int i=0; i < n; ++i) {
    pair pi=point(p,i);
    pair pj=point(p,i+1);
    if(((pi.y <= z.y && z.y < pj.y) || (pj.y <= z.y && z.y < pi.y)) &&
       z.x < pi.x+(pj.x-pi.x)*(z.y-pi.y)/(pj.y-pi.y)) c=!c;
  }
  return c;
}

// Returns true iff the line a--b intersects the cyclic polygon p.
bool intersect(pair a, pair b, path p)
{
  if(!polygon(p)) abort("Polygon must be a straight cyclic path");
  int n=length(p);
  for(int i=0; i < n; ++i) {
    pair A=point(p,i);
    pair B=point(p,i+1);
    real de=(b.x-a.x)*(A.y-B.y)-(A.x-B.x)*(b.y-a.y);
    if(de != 0) {
      de=1/de;
      real t=((A.x-a.x)*(A.y-B.y)-(A.x-B.x)*(A.y-a.y))*de;
      real T=((b.x-a.x)*(A.y-a.y)-(A.x-a.x)*(b.y-a.y))*de;
      if(0 <= t && t <= 1 && 0 <= T && T <= 1) return true;
    }
  }
  return false;
}

real[][] zero(int n)
{
  real[][] m;
  for(int i=0; i < n; ++i)
    m[i]=sequence(new real(int x){return 0;},n);
  return m;
}

real[][] identity(int n)
{
  real[][] m;
  for(int i=0; i < n; ++i)
    m[i]=sequence(new real(int x){return x == i ? 1 : 0;},n);
  return m;
}

real[][] operator + (real[][] a, real[][] b)
{
  real[][] m;
  for(int i=0; i < a.length; ++i)
    m[i]=a[i]+b[i];
  return m;
}

real[][] operator - (real[][] a, real[][] b)
{
  real[][] m;
  for(int i=0; i < a.length; ++i)
    m[i]=a[i]-b[i];
  return m;
}

real[][] operator * (real[][] a, real[][] b)
{
  int n=a.length;
  real[][] m=new real[n][b[0].length];
  for(int i=0; i < n; ++i) {
    real[] ai=a[i];
    real[] mi=m[i];
    if(ai.length != b.length) 
      abort("Multiplication of incommensurate matrices is undefined");
    for(int j=0; j < b[0].length; ++j) {
      real sum;
      for(int k=0; k < b.length; ++k)
	sum += ai[k]*b[k][j];
      mi[j]=sum;
    }
  }
  return m;
}

real[][] operator * (real[][] a, real[] b)
{
  return a*transpose(new real[][] {b});
}

real[][] operator * (real[] b,real[][] a)
{
  return new real[][] {b}*a;
}

real[][] operator * (real[][] a, real b)
{
  real[][] m;
  for(int i=0; i < a.length; ++i)
    m[i]=a[i]*b;
  return m;
}

real[][] operator * (real b, real[][] a)
{
  return a*b;
}

real[][] operator / (real[][] a, real b)
{
  return a*(1/b);
}

real determinant(real[][] m)
{
  if(m.length != 2)
    abort("determinant of general matrix not yet implemented");
  if(m[0].length != m.length || m[1].length != m.length)
    abort("matrix not square");
  return m[0][0]*m[1][1]-m[0][1]*m[1][0];
}

real[][] inverse(real[][] m)
{
  if(m.length != 2)
    abort("inverse of general matrix not yet implemented");
  return new real[][] {{m[1][1],-m[0][1]},{-m[1][0],m[0][0]}}/determinant(m);
}
