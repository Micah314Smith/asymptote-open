/*****
 * texfile.h
 * John Bowman 2003/03/14
 *
 * Encapsulates the writing of commands to a TeX file.
 *****/

#ifndef TEXFILE_H
#define TEXFILE_H

#include <fstream>
#include <string>
#include <iomanip>
#include <iostream>
#include <list>

#include "pair.h"
#include "bbox.h"
#include "pen.h"
#include "util.h"
#include "interact.h"

using std::string;

namespace camp {

typedef std::list<string> stringlist;
extern stringlist TeXpipepreamble, TeXpreamble;

const double tex2ps=72.0/72.27;
const double ps2tex=1.0/tex2ps;
  
template<class T>
void texdocumentclass(T& out, bool pipe=false)
{
  if(pipe || !settings::texmode)
    out << "\\documentclass[12pt]{article}" << newl;
}
  
template<class T>
void texpreamble(T& out, stringlist& preamble=TeXpreamble)
{
  stringlist::iterator p=preamble.begin();
  if(p != preamble.end()) {
    TeXcontaminated=true;
    for (; p != preamble.end(); ++p)
      out << stripblanklines(*p);
  }
}
  
template<class T>
void texdefines(T& out, stringlist& preamble=TeXpreamble, bool pipe=false) 
{
  texpreamble(out,preamble);
  out << "\\newbox\\ASYbox" << newl
      << "\\newbox\\ASYpsbox" << newl
      << "\\newdimen\\ASYdimen" << newl
      << "\\def\\ASYbase#1#2{\\setbox\\ASYbox=\\hbox{#1}"
      << "\\ASYdimen=\\ht\\ASYbox%"
      << newl
      << "\\setbox\\ASYbox=\\hbox{#2}\\lower\\ASYdimen\\box\\ASYbox}" << newl
      << "\\def\\ASYalign(#1,#2)(#3,#4)#5#6{\\setbox\\ASYbox=\\hbox{#6}%"
      << newl
      << "\\rput[lB]{#5}(#1,#2){\\ASYdimen=\\ht\\ASYbox%" << newl
      << "\\advance\\ASYdimen by\\dp\\ASYbox\\kern#3\\wd\\ASYbox"
      << "\\raise#4\\ASYdimen\\box\\ASYbox}}"
      << newl;
  
  if(pipe || !settings::texmode)
    out << "\\usepackage{graphicx}" << newl;
}
  
class texfile : public gc {
  ostream *out;
  pair offset;
  bbox box;
  pen lastpen;

public:
  texfile(const string& texname, const bbox& box);
  ~texfile();

  void prologue();

  void epilogue();

  void setpen(pen p);
  
  // Draws label rotated by angle (relative to the horizontal) at position z.
  void put(const string& label, double angle, pair z, pair Align);

  void beginlayer(const string& psname);
  void endlayer();
  
  void verbatim(const string& s) {
    *out << s << newl;
  }
  
};

} //namespace camp

#endif


