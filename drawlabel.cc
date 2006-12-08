/*****
 * drawlabel.cc
 * John Bowman 2003/04/07
 *
 * Add a label to a picture.
 *****/

#include <sstream>

#include "drawlabel.h"
#include "settings.h"
#include "util.h"

namespace camp {
  
extern string texready;
pen drawElement::lastpen;

void drawLabel::labelwarning(const char *action) 
{
  cerr << "warning: label \"" << label 
	       << "\" " << action << " to avoid overwriting" << endl;
}
 
void recover(iopipestream &tex, const char **abort)
{
  tex << "\n";
  tex.wait("\n*",abort);
  tex << "\n\n";
  tex.wait("\n*",abort);
}
  
bool drawLabel::texbounds(iopipestream& tex, string& s, bool warn,
			  const char **abort)
{
  string texbuf;
  tex << "\\setbox\\ASYbox=\\hbox{" << stripblanklines(s) << "}\n\n";
  int rc=tex.wait(texready.c_str(),abort);
  if(rc) {
    if(settings::fataltex[rc]) camp::reportError(*tex.message()); // Fatal error
    if(settings::getSetting<bool>("inlinetex")) {
      if(warn) {
	recover(tex,abort);
	if(settings::getSetting<bool>("debug")) {
	  ostringstream buf;
	  buf << "Cannot determine size of label \"" << s << "\"";
	  reportWarning(buf);
	}
	return false;
      }
      tex << "\\show 0\n";
      tex << "\n";
      tex.wait("\n*",abort);
      return false;
    } else {
      recover(tex,abort);
      camp::reportError(*tex.message());
    }
  }
  
  tex << "\\showthe\\wd\\ASYbox\n";
  tex >> texbuf;
  if(texbuf[0] == '>' && texbuf[1] == ' ')
    width=atof(texbuf.c_str()+2)*tex2ps;
  else reportError("Cannot read label width");
  tex << "\n";
  tex.wait("\n*",abort);
  
  tex << "\\showthe\\ht\\ASYbox\n";
  tex >> texbuf;
  if(texbuf[0] == '>' && texbuf[1] == ' ')
    height=atof(texbuf.c_str()+2)*tex2ps;
  else reportError("Cannot read label height");
  tex << "\n";
  tex.wait("\n*",abort);
  
  tex << "\\showthe\\dp\\ASYbox\n";
  tex >> texbuf;
  if(texbuf[0] == '>' && texbuf[1] == ' ')
    depth=atof(texbuf.c_str()+2)*tex2ps;
  else reportError("Cannot read label depth");
  tex << "\n";
  tex.wait("\n*",abort);
     
  return true;
}   

inline double urand()
{			  
  static const double factor=2.0/RAND_MAX;
  return rand()*factor-1.0;
}

void drawLabel::bounds(bbox& b, iopipestream& tex, boxvector& labelbounds,
		       bboxlist&)
{
  mem::string texengine=settings::getSetting<mem::string>("tex");
  if(texengine == "none") {b += position; return;}
  
  if(!havebounds) {
    havebounds=true;
    const char **abort=settings::texabort(texengine);
    if(pentype.size() != lastpen.size() ||
       pentype.Lineskip() != lastpen.Lineskip()) {
      if(settings::latex(texengine)) {
	tex <<  "\\fontsize{" << pentype.size() << "}{" << pentype.Lineskip()
	    << "}\\selectfont\n";
	tex.wait("\n*",abort);
      }
    }
    
    mem::string font=pentype.Font();
    if(font != lastpen.Font()) {
      tex <<  font << "\n";
      tex.wait("\n*",abort);
    }
    
    lastpen=pentype;
    
    bool nullsize=size == "";
    if(!texbounds(tex,label,nullsize,abort) && !nullsize)
      texbounds(tex,size,false,abort);
    
    Align=inverse(T)*align;
    double scale0=max(fabs(Align.getx()),fabs(Align.gety()));
    if(scale0) Align *= 0.5/scale0;
    Align -= pair(0.5,0.5);
    double Depth=(pentype.Baseline() == NOBASEALIGN) ? depth : 0.0;
    texAlign=Align;
    if(Depth > 0) texAlign += pair(0.0,Depth/(height+Depth));
    Align.scale(width,height+Depth);
    Align += pair(0.0,Depth-depth);
    Align=T*Align;
  }

  // alignment point
  pair p=position+Align;
  const double vertical=height+depth;
  const double fuzz=pentype.size()*0.05+0.3;
  pair A=p+T*pair(-fuzz,-fuzz);
  pair B=p+T*pair(-fuzz,vertical+fuzz);
  pair C=p+T*pair(width+fuzz,vertical+fuzz);
  pair D=p+T*pair(width+fuzz,-fuzz);
  
  if(pentype.Overwrite() != ALLOW && label != "") {
    size_t n=labelbounds.size();
    box Box=box(A,B,C,D);
    for(size_t i=0; i < n; i++) {
      if(labelbounds[i].intersect(Box)) {
	switch(pentype.Overwrite()) {
	case SUPPRESS:
	  labelwarning("suppressed");
	case SUPPRESSQUIET:
	  suppress=true; 
	  return;
	case MOVE:
	  labelwarning("moved");
	default:
	  break;
	}

	pair Align=(align == pair(0,0)) ? unit(pair(urand(),urand())) :
	  unit(align);
	double s=0.1*pentype.size();
	double dx=0, dy=0;
	if(Align.getx() > 0.1) dx=labelbounds[i].xmax()-Box.xmin()+s;
	if(Align.getx() < -0.1) dx=labelbounds[i].xmin()-Box.xmax()-s;
	if(Align.gety() > 0.1) dy=labelbounds[i].ymax()-Box.ymin()+s;
	if(Align.gety() < -0.1) dy=labelbounds[i].ymin()-Box.ymax()-s;
	pair offset=pair(dx,dy);
	position += offset;
	A += offset;
	B += offset;
	C += offset;
	D += offset;
	Box=box(A,B,C,D);
	i=0;
      }
    }
    labelbounds.resize(n+1);
    labelbounds[n]=Box;
  }
  
  Box=bbox();
  Box += A;
  Box += B;
  Box += C;
  Box += D;
  
  b += Box;
}

drawElement *drawLabel::transformed(const transform& t)
{
  return new drawLabel(label,size,t*T,t*position,
		       length(align)*unit(shiftless(t)*align),pentype);
}

} //namespace camp
