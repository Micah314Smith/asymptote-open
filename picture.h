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

#include "pool.h"
#include "drawelement.h"

namespace camp {

extern iopipestream tex; // Bi-directional pipe to latex (to find label bbox)

class picture : public mempool::pooled<picture> {
private:
  bbox b;
  pair bboxshift;
  std::list<drawElement*> nodes;
  bool labels;
  bool epsformat,pdfformat,tgifformat;

public:
  picture() : labels(false) {}
  
  // Destroys all of the owned picture objects.
  ~picture();

  // Prepend an object to the picture.
  void prepend(drawElement *p);
  
  // Append an object to the picture.
  void append(drawElement *p);

  // Adds the content of another picture.
  void add(picture &pic);

  bbox bounds();

  void texinit();

  bool texprocess(const std::string& texname, const std::string& tempname,
		  const std::string& prefix, const bbox& bpos); 
    
  bool postprocess(const std::string& epsname, const std::string& outname, 
		   const std::string& outputformat, bool wait,
		   const bbox& bpos);
    
  // Ships the picture out to PostScript & TeX files.
  bool shipout(const picture& preamble, const std::string& prefix,
	       const std::string& format, bool wait);
 
  picture *transformed(const transform& t);
  
  // Returns the number of element in picture.
  int number() {
    return nodes.size();
  }
};

inline picture *transformed(const transform& t, picture *p)
{
  return p->transformed(t);
}

} //namespace camp

#endif
