/*****
 * psfile.cc
 * Andy Hammerlindl 2002/06/10
 *
 * Encapsulates the writing of commands to a PostScript file.
 * Allows identification and removal of redundant commands.
 *****/

#include <ctime>
#include <iomanip>

#include "psfile.h"
#include "settings.h"
#include "errormsg.h"

namespace camp {

using std::ofstream;
using std::setw;
using std::setprecision;
  
psfile::psfile(const string& filename, const bbox& box, const pair& shift)
  : filename(filename), box(box), shift(shift)
{
  if(filename != "") out=new ofstream(filename.c_str());
  else out=&std::cout;
  if(!out || !*out) {
    std::cerr << "Can't write to " << filename << std::endl;
    throw handled_error();
  }
}

psfile::~psfile()
{
  if(filename != "" && out) delete out;
}

void psfile::prologue()
{
  //*out << "%!PS" << newl;
  *out << "%!PS-Adobe-3.0 EPSF-3.0" << newl;

//  assert(box.nonempty());
  
  *out << "%%BoundingBox: " << box.LowRes() << newl;
  *out << "%%HiResBoundingBox: " << setprecision(9) << box << newl;
  *out << "%%Creator: " << settings::PROGRAM << " " << settings::VERSION
       <<  newl;

  time_t t; time(&t);
  struct tm *tt = localtime(&t);
  char prev = out->fill('0');
  *out << "%%CreationDate: " << tt->tm_year + 1900 << "."
       << setw(2) << tt->tm_mon+1 << "." << setw(2) << tt->tm_mday << " "
       << setw(2) << tt->tm_hour << ":" << setw(2) << tt->tm_min << ":"
       << setw(2) << tt->tm_sec << newl;
  out->fill(prev);

  *out << "%%Pages: 1" << newl;
  *out << "%%EndProlog" << newl;
  *out << "%%Page: 1 1" << newl;

  pen defaultpen;
  defaultpen.defaultwidth();
  setpen(defaultpen);
  
  *out << " 1 setlinecap 1 setlinejoin 10 setmiterlimit" << newl;
  
}

void psfile::epilogue()
{
  *out << "showpage" << newl;
  *out << "%%EOF" << newl;
  out->flush();
}

} //namespace camp
