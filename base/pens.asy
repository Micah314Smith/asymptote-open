include fonts;
			  
public pen currentpen;

pen nullpen=linewidth(0);

pen linetype(string s) 
{
  return linetype(s,true);
}

pen solid=linetype("");
pen dotted=linetype("0 4");
pen dashed=linetype("8 8");
pen longdashed=linetype("24 8");
pen dashdotted=linetype("8 8 0 8");
pen longdashdotted=linetype("24 8 0 8");

void defaultpen(real w) {defaultpen(linewidth(w));}
pen operator +(pen p, real w) {return p+linewidth(w);}
pen operator +(real w, pen p) {return linewidth(w)+p;}

pen Dotted=dotted+1.0;
pen Dotted(pen p) {return dotted+2*linewidth(p);}

pen squarecap=linecap(0);
pen roundcap=linecap(1);
pen extendcap=linecap(2);

pen miterjoin=linejoin(0);
pen roundjoin=linejoin(1);
pen beveljoin=linejoin(2);

pen zerowinding=fillrule(0);
pen evenodd=fillrule(1);
pen zerowindingoverlap=fillrule(2);
pen evenoddoverlap=fillrule(3);

pen nobasealign=basealign(0);
pen basealign=basealign(1);

pen invisible=invisible();
pen black=gray(0);
pen lightgray=gray(0.9);
pen lightgrey=lightgray;
pen gray=gray(0.5);
pen grey=gray;
pen white=gray(1);

pen red=rgb(1,0,0);
pen green=rgb(0,1,0);
pen blue=rgb(0,0,1);

pen cmyk=cmyk(0,0,0,0);
pen Cyan=cmyk(1,0,0,0);
pen Magenta=cmyk(0,1,0,0);
pen Yellow=cmyk(0,0,1,0);
pen Black=cmyk(0,0,0,1);

pen yellow=red+green;
pen magenta=red+blue;
pen cyan=blue+green;

pen brown=red+black;
pen darkgreen=green+black;
pen darkblue=blue+black;

pen orange=red+yellow;
pen purple=magenta+blue;

pen chartreuse=brown+green;
pen fuchsia=red+darkblue;
pen salmon=red+darkgreen+darkblue;
pen lightblue=darkgreen+blue;
pen lavender=brown+darkgreen+blue;
pen pink=red+darkgreen+blue;

pen cmyk(pen p) {
  return p+cmyk;
}

real linewidth() 
{
  return linewidth(currentpen);
}

// Options for handling label overwriting
int Allow=0;
int Suppress=1;
int SuppressQuiet=2;
int Move=3;
int MoveQuiet=4;

pen[] colorPen={red,blue,green,magenta,cyan,orange,purple,brown,darkblue,
		darkgreen,chartreuse,fuchsia,salmon,lightblue,black,lavender,
		pink,yellow,gray};
colorPen.cyclic(true);

pen[] monoPen={solid,dashed,dotted,longdashed,dashdotted,longdashdotted};
monoPen.cyclic(true);

public bool mono=false;

pen Pen(int n) 
{
  return mono ? monoPen[n] : colorPen[n];
}

real dotsize(pen p=currentpen) 
{
  return dotfactor*linewidth(p);
}

real arrowsize(pen p=currentpen) 
{
  return arrowfactor*linewidth(p);
}

real arcarrowsize(pen p=currentpen) 
{
  return arcarrowfactor*linewidth(p);
}

real barsize(pen p=currentpen) 
{
  return barfactor*linewidth(p);
}

pen fontsize(real size) 
{
  return fontsize(size,1.2*size);
}

real labelmargin(pen p=currentpen)
{
  return labelmargin*fontsize(p);
}

pen interp(pen a, pen b, real t) 
{
  return (1-t)*a+t*b;
}

pen squarepen=makepen(shift(-0.5,-0.5)*unitsquare);
