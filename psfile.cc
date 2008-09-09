/*****
 * psfile.cc
 * Andy Hammerlindl 2002/06/10
 *
 * Encapsulates the writing of commands to a PostScript file.
 * Allows identification and removal of redundant commands.
 *****/

#include <ctime>
#include <iomanip>
#include <sstream>
#include <zlib.h>

#include "psfile.h"
#include "settings.h"
#include "errormsg.h"
#include "array.h"

using std::ofstream;
using std::setw;
using vm::array;
using vm::read;

namespace camp {

void checkColorSpace(ColorSpace colorspace)
{
  switch(colorspace) {
  case DEFCOLOR:
  case INVISIBLE:
    reportError("Cannot shade with invisible pen");
  case PATTERN:
    reportError("Cannot shade with pattern");
    break;
  default:
    break;
  }
}
    
  psfile::psfile(const string& filename, bool pdfformat)
    : filename(filename), pdfformat(pdfformat), pdf(false),
      transparency(false), out(NULL)
{
  if(filename.empty()) out=&cout;
  else out=new ofstream(filename.c_str());
  if(!out || !*out) {
    cerr << "Cannot write to " << filename << endl;
    throw handled_error();
  }
}

void psfile::close()
{
  if(out) {
    out->flush();
    if(!filename.empty()) {
      if(!out->good()) {
	ostringstream msg;
	msg << "Cannot write to " << filename;
	reportError(msg);
      }
      delete out;
      out=NULL;
    }
  }
}

psfile::~psfile()
{
  close();
}
  
void psfile::header()
{
  Int level=settings::getSetting<Int>("level");
  *out << "%!PS-Adobe-" << level << ".0 EPSF-" << level << ".0" << newl;
}
  
void psfile::prologue(const bbox& box)
{
  header();
  BoundingBox(box);
  *out << "%%Creator: " << settings::PROGRAM << " " << settings::VERSION
       << SVN_REVISION <<  newl;

  time_t t; time(&t);
  struct tm *tt = localtime(&t);
  char prev = out->fill('0');
  *out << "%%CreationDate: " << tt->tm_year + 1900 << "."
       << setw(2) << tt->tm_mon+1 << "." << setw(2) << tt->tm_mday << " "
       << setw(2) << tt->tm_hour << ":" << setw(2) << tt->tm_min << ":"
       << setw(2) << tt->tm_sec << newl;
  out->fill(prev);

  *out << "%%Pages: 1" << newl;
  *out << "%%Page: 1 1" << newl;
  
  if(!pdfformat)
    *out << "/Setlinewidth {0 exch dtransform dup abs 1 lt {pop 0}{round} ifelse idtransform setlinewidth pop} bind def" << newl;
}

void psfile::epilogue()
{
  *out << "showpage" << newl;
  *out << "%%EOF" << newl;
}

void psfile::setcolor(const pen& p, const string& begin="",
		      const string& end="")
{
  if(p.cmyk() && (!lastpen.cmyk() ||
		  (p.cyan() != lastpen.cyan() || 
		   p.magenta() != lastpen.magenta() || 
		   p.yellow() != lastpen.yellow() ||
		   p.black() != lastpen.black()))) {
    *out << begin << p.cyan() << " " << p.magenta() << " " << p.yellow() << " " 
	 << p.black() << (pdf ? " k" : " setcmykcolor") << end << newl;
  } else if(p.rgb() && (!lastpen.rgb() || 
			(p.red() != lastpen.red() || 
			 p.green() != lastpen.green() || 
			 p.blue() != lastpen.blue()))) {
    *out << begin << p.red() << " " << p.green() << " " << p.blue()
	 << (pdf ? " rg" : " setrgbcolor") << end << newl;
  } else if(p.grayscale() && (!lastpen.grayscale() ||
			      p.gray() != lastpen.gray())) {
    *out << begin << p.gray() << (pdf ? " g" : " setgray") << end << newl;
  }
}
  
void psfile::setpen(pen p)
{
  p.convert();
    
  if(p.blend() != lastpen.blend()) {
    *out << "/" << p.blend() << " .setblendmode" << newl;
    transparency=true;
  }
  
  if(p.opacity() != lastpen.opacity()) {
    *out << p.opacity() << " .setopacityalpha" << newl;
    transparency=true;
  }
  
  if(!p.fillpattern().empty() && p.fillpattern() != lastpen.fillpattern()) 
    *out << p.fillpattern() << " setpattern" << newl;
  else setcolor(p);
  
  // Defer dynamic linewidth until stroke time in case currentmatrix changes.
  if(p.width() != lastpen.width()) {
    *out << p.width() << (pdfformat ? " setlinewidth" : " Setlinewidth") 
	 << newl;
  }
    
  if(p.cap() != lastpen.cap()) {
    *out << p.cap() << " setlinecap" << newl;
  }
    
  if(p.join() != lastpen.join()) {
    *out << p.join() << " setlinejoin" << newl;
  }
    
  if(p.stroke() != lastpen.stroke() || 
     p.linetype().offset != lastpen.linetype().offset) {
    out->setf(std::ios::fixed);
    *out << "[" << p.stroke() << "] " << std::setprecision(6) 
	 << p.linetype().offset << " setdash" << newl;
    out->unsetf(std::ios::fixed);
  }
    
  lastpen=p;
}

void psfile::write(pen p)
{
  if(p.cmyk())
    *out << p.cyan() << " " << p.magenta() << " " << p.yellow() << " " 
	 << p.black();
  else if(p.rgb())
    *out << p.red() << " " << p.green() << " " << p.blue();
  else if(p.grayscale())
    *out << p.gray();
}
  
void psfile::write(path p, bool newPath)
{
  Int n = p.size();
  assert(n != 0);

  if(newPath) newpath();

  if (n == 1) {
    moveto(p.point((Int) 0));
    lineto(p.point((Int) 0));
    return;
  }

  // Draw points
  moveto(p.point((Int) 0));
  for (Int i = 1; i < n; i++) {
    if(p.straight(i-1)) lineto(p.point(i));
    else curveto(p.postcontrol(i-1), p.precontrol(i), p.point(i));
  }

  if (p.cyclic()) {
    if(p.straight(n-1)) lineto(p.point((Int) 0));
    else curveto(p.postcontrol(n-1), p.precontrol((Int) 0), p.point((Int) 0));
    closepath();
  }    
}

static const char *inconsistent="inconsistent colorspaces";
  
void psfile::latticeshade(const vm::array& a, const bbox& b)
{
  checkLevel();
  size_t n=a.size();
  if(n == 0) return;
  
  array *a0=read<array *>(a,0);
  size_t m=a0->size();
  setfirstpen(*a0);
  
  ColorSpace colorspace=maxcolorspace2(a);
  checkColorSpace(colorspace);
  
  unsigned ncomponents=ColorComponents[colorspace];
  
  *out << "<< /ShadingType 1" << newl
       << "/Matrix ";

  write(matrix(b.Min(),b.Max()));
  *out << newl;
  *out << "/ColorSpace /Device" << ColorDeviceSuffix[colorspace] << newl
       << "/Function" << newl
       << "<< /FunctionType 0" << newl
       << "/Order 1" << newl
       << "/Domain [0 1 0 1]" << newl
       << "/Range [" << newl;
  for(unsigned i=0; i < ncomponents; ++i)
    *out << "0 1 ";
  *out << "]" << newl
       << "/Decode [";
  for(unsigned i=0; i < ncomponents; ++i)
    *out << "0 1 ";
  *out << "]" << newl;
  *out << "/BitsPerSample 8" << newl;
  *out << "/Size [" << m << " " << n << "]" << newl
       << "/DataSource <" << newl;
  
  beginHex();
  for(size_t i=n; i > 0;) {
    array *ai=read<array *>(a,--i);
    checkArray(ai);
    size_t aisize=ai->size();
    if(aisize != m) reportError("shading matrix must be rectangular");
    for(size_t j=0; j < m; j++) {
      pen *p=read<pen *>(ai,j);
      p->convert();
      if(!p->promote(colorspace))
	reportError(inconsistent);
      writeHex(p,ncomponents);
    }
  }
  endHex();

  *out << ">" << newl
       << ">>" << newl
       << ">>" << newl
       << "shfill" << newl;
}

// Axial and radial shading
void psfile::gradientshade(bool axial, const ColorSpace &colorspace,
			   const pen& pena, const pair& a, double ra,
			   const pen& penb, const pair& b, double rb)
{
  checkLevel();
  setpen(pena);
  checkColorSpace(colorspace);
  
  *out << "<< /ShadingType " << (axial ? "2" : "3") << newl
       << "/ColorSpace /Device" << ColorDeviceSuffix[colorspace] << newl
       << "/Coords [";
  write(a);
  if(!axial) write(ra);
  write(b);
  if(!axial) write(rb);
  *out << "]" << newl
       << "/Extend [true true]" << newl
       << "/Function" << newl
       << "<< /FunctionType 2" << newl
       << "/Domain [0 1]" << newl
       << "/C0 [";
  write(pena);
  *out << "]" << newl
       << "/C1 [";
  write(penb);
  *out << "]" << newl
       << "/N 1" << newl
       << ">>" << newl
       << ">>" << newl
       << "shfill" << newl;
}
  
void psfile::gouraudshade(const array& pens, const array& vertices,
			  const array& edges)
{
  checkLevel();
  size_t size=pens.size();
  if(size == 0) return;
  
  setfirstpen(pens);
  ColorSpace colorspace=maxcolorspace(pens);

  *out << "<< /ShadingType 4" << newl
       << "/ColorSpace /Device" << ColorDeviceSuffix[colorspace] << newl
       << "/DataSource [" << newl;
  for(size_t i=0; i < size; i++) {
    write(read<Int>(edges,i));
    write(read<pair>(vertices,i));
    pen *p=read<pen *>(pens,i);
    p->convert();
    if(!p->promote(colorspace))
      reportError(inconsistent);
    *out << " ";
    write(*p);
    *out << newl;
  }
  *out << "]" << newl
       << ">>" << newl
       << "shfill" << newl;
}
 
// Tensor-product patch shading
void psfile::tensorshade(const array& pens, const array& boundaries,
			 const array& z)
{
  checkLevel();
  size_t size=pens.size();
  if(size == 0) return;
  size_t nz=z.size();
  
  array *p0=read<array *>(pens,0);
  if(checkArray(p0) != 4)
    reportError("4 pens required");
  setfirstpen(*p0);
  
  ColorSpace colorspace=maxcolorspace2(pens);
  checkColorSpace(colorspace);

  *out << "<< /ShadingType 7" << newl
       << "/ColorSpace /Device" << ColorDeviceSuffix[colorspace] << newl
       << "/DataSource [" << newl;
  
  for(size_t i=0; i < size; i++) {
    // Only edge flag 0 (new patch) is implemented since the 32% data
    // compression (for RGB) afforded by other edge flags really isn't worth
    // the trouble or confusion for the user. 
    write(0);
    path g=read<path>(boundaries,i);
    if(!(g.cyclic() && g.size() == 4))
      reportError("specify cyclic path of length 4");
    for(Int j=0; j < 4; ++j) {
      write(g.point(j));
      write(g.postcontrol(j));
      write(g.precontrol(j+1));
    }
    if(nz == 0) { // Coons patch
      static double nineth=1.0/9.0;
      for(Int j=0; j < 4; ++j) {
	write(nineth*(-4.0*g.point(j)+6.0*(g.precontrol(j)+g.postcontrol(j))
		      -2.0*(g.point(j-1)+g.point(j+1))
		      +3.0*(g.precontrol(j-1)+g.postcontrol(j+1))
		      -g.point(j+2)));
      }
    } else {
      array *zi=read<array *>(z,i);
      if(checkArray(zi) != 4)
	reportError("specify 4 internal control points for each path");
      for(Int j=0; j < 4; ++j)
	write(read<pair>(zi,j));
    }
    
    array *pi=read<array *>(pens,i);
    if(checkArray(pi) != 4)
      reportError("specify 4 pens for each path");
    for(Int j=0; j < 4; ++j) {
      pen *p=read<pen *>(pi,j);
      p->convert();
      if(!p->promote(colorspace))
	reportError(inconsistent);
      *out << " ";
      write(*p);
    }
    *out << newl;
  }
  
  *out << "]" << newl
       << ">>" << newl
       << "shfill" << newl;
}
 
inline unsigned byte(double r) // Map [0,1] to [0,255]
{
  if(r < 0.0) r=0.0;
  else if(r > 1.0) r=1.0;
  int a=(int)(256.0*r);
  if(a == 256) a=255;
  return a;
}

void psfile::writeHex(pen *p, Int ncomponents) 
{
  switch(ncomponents) {
  case 0:
    break;
  case 1: 
    write2(byte(p->gray())); 
    *out << newl;
    break;
  case 3:
    write2(byte(p->red())); 
    write2(byte(p->green())); 
    write2(byte(p->blue())); 
    *out << newl;
    break;
  case 4:
    write2(byte(p->cyan())); 
    write2(byte(p->magenta())); 
    write2(byte(p->yellow())); 
    write2(byte(p->black())); 
    *out << newl;
  default:
    break;
  }
}

void psfile::imageheader(size_t width, size_t height, ColorSpace colorspace,
			 const string& filter)
{
  unsigned ncomponents=ColorComponents[colorspace];
  *out << "/Device" << ColorDeviceSuffix[colorspace] << " setcolorspace" 
       << newl
       << "<<" << newl
       << "/ImageType 1" << newl
       << "/Width " << width << newl
       << "/Height " << height << newl
       << "/BitsPerComponent 8" << newl
       << "/Decode [";
  
  for(unsigned i=0; i < ncomponents; ++i)
    *out << "0 1 ";
  
  *out << "]" << newl
       << "/ImageMatrix [" << width << " 0 0 " << height << " 0 0]" << newl
       << "/DataSource currentfile " << filter << " filter" << newl
       << ">>" << newl
       << "image" << newl;
}

void psfile::image(const array& a, const array& P)
{
  size_t asize=a.size();
  size_t Psize=P.size();
  if(asize == 0 || Psize == 0) return;
  
  array *a0=read<array *>(a,0);
  size_t a0size=a0->size();
  if(a0size == 0) return;
  
  setfirstpen(P);
  
  ColorSpace colorspace=maxcolorspace(P);
  checkColorSpace(colorspace);
  
  unsigned ncomponents=ColorComponents[colorspace];
  
  imageheader(a0size,asize,colorspace);
    
  double min=read<double>(a0,0);
  double max=min;
  for(size_t i=0; i < asize; i++) {
    array *ai=read<array *>(a,i);
    for(size_t j=0; j < a0size; j++) {
	double val=read<double>(ai,j);
	if(val > max) max=val;
	else if(val < min) min=val;
    }
  }
  
  double step=(max == min) ? 0.0 : (Psize-1)/(max-min);
  
  beginHex();
  for(size_t i=0; i < asize; i++) {
    array *ai=read<array *>(a,i);
    for(size_t j=0; j < a0size; j++) {
      double val=read<double>(ai,j);
      size_t index=(size_t) ((val-min)*step+0.5);
      pen *p=read<pen *>(P,index < Psize ? index : Psize-1);
      p->convert();
      if(!p->promote(colorspace))
	reportError(inconsistent);
      writeHex(p,ncomponents);
    }
  }
  endHex();
  
  *out << ">" << endl;
}

void psfile::image(const array& a)
{
  size_t asize=a.size();
  if(asize == 0) return;
  
  array *a0=read<array *>(a,0);
  size_t a0size=a0->size();
  if(a0size == 0) return;
  
  setfirstpen(*a0);
  
  ColorSpace colorspace=maxcolorspace2(a);
  checkColorSpace(colorspace);
  
  unsigned ncomponents=ColorComponents[colorspace];
  
  imageheader(a0size,asize,colorspace);
    
  beginHex();
  for(size_t i=0; i < asize; i++) {
    array *ai=read<array *>(a,i);
    for(size_t j=0; j < a0size; j++) {
      pen *p=read<pen *>(ai,j);
      p->convert();
      if(!p->promote(colorspace))
	reportError(inconsistent);
      writeHex(p,ncomponents);
    }
  }
  endHex();
  
  *out << ">" << endl;
}
  
void psfile::rgbimage(const unsigned char *a, size_t width, size_t height)
{
  pen p(0.0,0.0,0.0);
  p.convert();
  ColorSpace colorspace=p.colorspace();
  checkColorSpace(colorspace);
  
  unsigned ncomponents=ColorComponents[colorspace];
  
  if(colorspace != RGB || settings::getSetting<Int>("level") < 3) {
    imageheader(width,height,colorspace);
    beginHex();
    for(size_t i=0; i < width; ++i) {
      for(size_t j=0; j < height; ++j) {
	size_t index=3*(height*i+j);
	if(colorspace == RGB) {
	  for(size_t k=0; k < 3; ++k)
	    write2(a[index+k]);
	  *out << newl;
	} else {
	  pen p(a[index]/255.0,a[index+1]/255.0,a[index+2]/255.0);
	  p.convert();
	  if(!p.promote(colorspace))
	    reportError(inconsistent);
	  writeHex(&p,ncomponents);
	}
      }	
    }
    endHex();
    *out << ">" << endl;
  } else {
    imageheader(width,height,colorspace,"/FlateDecode");
    
    unsigned long size= 3*width*height;
    /* Bound calculation taken from zlib. */
    unsigned long compressedSize=size+(size >> 12)+(size >> 14)+11;
    Bytef *compressed=new Bytef[compressedSize];

    compress(compressed,&compressedSize,a,size);
    
    for(size_t i=0; i < compressedSize; ++i)
      *out << compressed[i];
    *out << endl;
    
    delete[] compressed;
  }
}

} //namespace camp
