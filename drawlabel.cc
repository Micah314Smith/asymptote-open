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
#include "lexical.h"

using namespace settings;

namespace camp {
  
string texready=string("(Please type a command or say `\\end')\n*");
pen drawElement::lastpen;

void drawLabel::labelwarning(const char *action) 
{
  cerr << "warning: label \"" << label 
       << "\" " << action << " to avoid overwriting" << endl;
}
 
int drawLabel::wait(iopipestream &tex, const char *s, const char **abort,
		    bool ignore)
{
  int rc=tex.wait(s,abort);
  if(rc > 0) {
    if(fataltex[rc-1]) {
      tex.pipeclose();
      ignore=false;
    } else {
      tex << "\n";
      tex.wait(s,abort);
      tex << "\n";
      tex.wait(s,abort);
    }
    if(!ignore) {
      string s=tex.message();
      tex.shred();
      reportError(s);
    }
   }
  tex.shred();
  return rc;
}
 
bool drawLabel::texbounds(iopipestream& tex, string& s, const char **abort,
			  bool warn)
{
  string texbuf;
  tex << "\\setbox\\ASYbox=\\hbox{" << stripblanklines(s) << "}\n\n";
  int rc=wait(tex,texready.c_str(),abort,getSetting<bool>("inlinetex"));
   if(rc) {
     tex << "\\show 0\n";
     tex.wait("\n*");
     if(warn) {
       if(getSetting<bool>("debug")) {
	 ostringstream buf;
	 buf << "Cannot determine size of label \"" << s << "\"";
	 reportWarning(buf);
       }
       return false;
     }
     return false;
   }

  tex << "\\showthe\\wd\\ASYbox\n";
  tex >> texbuf;
  
  string cannotread="Cannot read label ";
  if(texbuf[0] == '>' && texbuf[1] == ' ')
    try {
      width=lexical::cast<double>(texbuf.c_str()+2,true)*tex2ps;
    } catch(lexical::bad_cast&) {
      reportError(cannotread+"width");
    }
  else reportError(cannotread+"width");
  tex << "\n";
  wait(tex,"\n*",abort);
  
  tex << "\\showthe\\ht\\ASYbox\n";
  tex >> texbuf;
  if(texbuf[0] == '>' && texbuf[1] == ' ')
    try {
    height=lexical::cast<double>(texbuf.c_str()+2,true)*tex2ps;
    } catch(lexical::bad_cast&) {
      reportError(cannotread+"height");
    }
  else reportError(cannotread+"height");
  tex << "\n";
  wait(tex,"\n*",abort);
  
  tex << "\\showthe\\dp\\ASYbox\n";
  tex >> texbuf;
  if(texbuf[0] == '>' && texbuf[1] == ' ')
    try {
    depth=lexical::cast<double>(texbuf.c_str()+2,true)*tex2ps;
    } catch(lexical::bad_cast&) {
      reportError(cannotread+"depth");
    }
  else reportError(cannotread+"depth");
  tex << "\n";
  wait(tex,"\n*",abort);
     
  return true;
}   

inline double urand()
{			  
  static const double factor=2.0/RAND_MAX;
  return rand()*factor-1.0;
}

void drawLabel::getbounds(iopipestream& tex, const string& texengine)
{
  if(havebounds) return;
  havebounds=true;
  
  const char **abort=texabort(texengine);
  if(pentype.size() != lastpen.size() ||
     pentype.Lineskip() != lastpen.Lineskip()) {
    if(latex(texengine)) {
      tex <<  "\\fontsize{" << pentype.size() << "}{" << pentype.Lineskip()
	  << "}\\selectfont\n";
      wait(tex,"\n*",abort);
    }
  }
    
  string font=pentype.Font();
  if(font != lastpen.Font()) {
    tex <<  font << "\n";
    wait(tex,"\n*",abort);
  }
    
  lastpen=pentype;
    
  bool nullsize=size.empty();
  if(!texbounds(tex,label,abort,nullsize) && !nullsize)
    texbounds(tex,size,abort,false);
  enabled=true;
    
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

void drawLabel::bounds(bbox& b, iopipestream& tex, boxvector& labelbounds,
		       bboxlist&)
{
  string texengine=getSetting<string>("tex");
  if(texengine == "none") {b += position; return;}
  
  getbounds(tex,texengine);
  
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

void drawLabel::checkbounds()
{
  if(!havebounds)
    reportError("drawLabel::write called before bounds");
}

bool drawLabel::write(texfile *out, const bbox&)
{
  checkbounds();
  if(suppress || pentype.invisible() || !enabled) return true;
  out->setpen(pentype);
  out->put(label,T,position,texAlign);
  return true;
}

drawElement *drawLabel::transformed(const transform& t)
{
  return new drawLabel(label,size,t*T,t*position,
		       length(align)*unit(shiftless(t)*align),pentype);
}

void drawLabelPath::bounds(bbox& b, iopipestream& tex, boxvector&, bboxlist&)
{
  string texengine=getSetting<string>("tex");
  if(texengine == "none") {b += position; return;}
    
  getbounds(tex,texengine);
  double L=p.arclength();
  
  double s1,s2;
  if(justify == "l") {
    s1=0.0;
    s2=width;
  } else if(justify == "r") {
    s1=L-width;
    s2=L;
  } else {
    double s=0.5*L;
    double h=0.5*width;
    s1=s-h;
    s2=s+h;
  }
  
  double Sx=shift.getx();
  double Sy=shift.gety();
  s1 += Sx;
  s2 += Sx;
  
  if(width > L || (!p.cyclic() && (s1 < 0 || s2 > L))) {
    ostringstream buf;
    buf << "Cannot fit label \"" << label << "\" to path";
    reportError(buf);
  }
  
  path q=p.subpath(p.arctime(s1),p.arctime(s2));
  
  b += q.bounds(Sy,Sy+height);
  Box=b;
}
  
bool drawLabelPath::write(texfile *out, const bbox&)
{
  bbox b=Box;
  double Hoffset=getSetting<bool>("inlinetex") ? b.right : b.left;
  b.shift(pair(-Hoffset,-b.bottom));
  
  checkbounds();
  if(drawLabel::pentype.invisible()) return true;
  out->setpen(drawLabel::pentype);
  out->verbatimline("\\psset{unit=1pt}%");
  out->verbatim("\\pstextpath[");
  out->verbatim(justify);
  out->verbatim("]");
  out->writepair(shift);
  out->verbatim("{\\pstVerb{");
  out->beginraw();
  writeshiftedpath(out);
  out->endraw();
  out->verbatim("}}{");
  out->verbatim(label);
  out->verbatimline("}");
  return true;
}

drawElement *drawLabelPath::transformed(const transform& t)
{
  return new drawLabelPath(label,size,transpath(t),justify,shift,
			   transpen(t));
}

} //namespace camp
