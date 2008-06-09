import contour;

size(200);

int n=100;

pair[] points=new pair[n];
real[] values=new real[n];

real f(real a, real b) {return a^2+b^2;}

real r() {return 1.1*(rand()/randMax*2-1);}

for(int i=0; i < n; ++i) {
  points[i]=(r(),r());
  values[i]=f(points[i].x,points[i].y);
}

draw(contour(points,values,new real[]{0.25,0.5,1},operator ..),blue);
