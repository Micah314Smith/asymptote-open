size(15cm,12cm,IgnoreAspect);
real minpercent=20;
real ignorebelow=0;
string data="diatom.csv";

import graph;

defaultpen(fontsize(8)+overwrite(MoveQuiet));

file in=line(csv(input(data)));

string depthlabel=in;
string yearlabel=in;
string[] taxa=in;

real[] depth;
int[] year;
real[][] percentage;

while(true) {
  real d=in;
  if(eof(in)) break;
  depth.push(d);
  year.push(in);
  percentage.push(in);
}

percentage=transpose(percentage);
real depthmin=-min(depth);
real depthmax=-max(depth);

int n=percentage.length;

int final;
for(int taxon=0; taxon < n; ++taxon) {
  real[] P=percentage[taxon];
  if(max(P) < ignorebelow) continue;
  final=taxon;
}  

real location=0;
for(int taxon=0; taxon < n; ++taxon) {
  real[] P=percentage[taxon];
  real maxP=max(P);
  if(maxP < ignorebelow) continue;
  picture pic;
  real x=1;
  if(maxP < minpercent) x=minpercent/maxP;
  if(maxP > 100) x=50/maxP;
  scale(pic,Linear(x),Linear(false,-1));
  filldraw(pic,(0,depthmin)--graph(pic,P,depth)--(0,depthmax)--cycle,
	   gray(0.9));
  xaxis(pic,Bottom,LeftTicks("$%.3g$",beginlabel=false,0,2),Above);
  xaxis(pic,rotate(45)*Label(TeXify(taxa[taxon])),Top,Above);
  if(taxon == 0) yaxis(pic,depthlabel,Left,RightTicks(0,10),Above);
  if(taxon == final) yaxis(pic,Right,LeftTicks("%",0,10),Above);
 
  add(shift(location,0)*pic);
  location += pic.userMax.x;
}

for(int i=0; i < year.length; ++i)
  if(year[i] != 0) label((string) year[i],(location,-depth[i]),E);

label("\%",(0.5*location,point(S).y),5*S);
