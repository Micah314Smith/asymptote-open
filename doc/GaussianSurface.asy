import graph3;

size(200,0);

currentprojection=perspective(10,8,4);

real f(pair z) {return 0.5+exp(-abs(z)^2);}

draw((-1,-1,0)--(1,-1,0)--(1,1,0)--(-1,1,0)--cycle);

draw(arc(0.12Z,0.2,90,60,90,15),Arrow3);

surface s=surface(f,(-1,-1),(1,1));
  
xaxis3(Label("$x$",1),red,Arrow3);
yaxis3(Label("$y$",1),red,Arrow3);
zaxis3(red,Arrow3);

label("$O$",(0,0,0),S,red);

draw(s,meshpen=black,nolight);
