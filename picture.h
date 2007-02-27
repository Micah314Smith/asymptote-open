/*****
 * picture.h
 * Andy Hamerlindl 2002/06/06
 *
 * Stores a picture as a list of drawElements and handles its output to
 * PostScript.
 *****/

#ifndef PICTURE_H
#define PICTURE_H

#include <list>
#include <sstream>
#include <iostream>

#include "drawelement.h"

namespace camp {

class texstream : public iopipestream {
public:
  void pipeclose();
};
extern texstream tex; // Bi-directional pipe to latex (to find label bbox)

typedef mem::list<drawElement*> nodelist;
  
class picture : public gc {
private:
  bool labels;
  size_t lastnumber;
  transform *T; // Keep track of accumlative picture transform
  bbox b;
  boxvector labelbounds;
  bboxlist bboxstack;
  
  static bool epsformat,pdfformat,tgifformat,pdf,Labels;
  static double paperWidth,paperHeight;

public:
  nodelist nodes;
  
  picture() : labels(false), lastnumber(0), T(0) {}
  
  // Destroy all of the owned picture objects.
  ~picture();

  // Prepend an object to the picture.
  void prepend(drawElement *p);
  
  // Append an object to the picture.
  void append(drawElement *p);

  // Enclose each layer with begin and end.
  void enclose(drawElement *begin, drawElement *end);
  
  // Add the content of another picture.
  void add(picture &pic);
  void prepend(picture &pic);
  
  bool havelabels();
  bbox bounds();

  void texinit();

  int epstopdf(const mem::string& epsname, const mem::string& pdfname);
  
  bool texprocess(const mem::string& texname, const mem::string& tempname,
		  const mem::string& prefix, const pair& bboxshift); 
    
  bool postprocess(const mem::string& prename, const mem::string& outname, 
		   const mem::string& outputformat, bool wait, bool view);
    
  // Ship the picture out to PostScript & TeX files.
  bool shipout(picture* preamble, const mem::string& prefix,
	       const mem::string& format, bool wait, bool view=true,
	       bool Delete=false);
 
  picture *transformed(const transform& t);
  
  bool null() {
    return nodes.empty();
  }
  
};

inline picture *transformed(const transform& t, picture *p)
{
  return p->transformed(t);
}

} //namespace camp

#endif
