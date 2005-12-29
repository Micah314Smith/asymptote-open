/*****
 * settings.h
 * Andy Hammerlindl 2004/05/10
 *
 * Declares a list of global variables that act as settings in the system.
 *****/

#ifndef SETTINGS_H
#define SETTINGS_H

#include <string>
#include <list>
#include <fstream>

#include "pair.h"
#include "item.h"

using std::string;

namespace types {
  class record;
}

namespace settings {
extern const char PROGRAM[];
extern const char VERSION[];
extern const char BUGREPORT[];

extern const string docdir;
  
extern int safe;
enum origin {CENTER,BOTTOM,TOP,ZERO};
//extern int origin;
  
extern int ShipoutNumber;
  
extern const string suffix;
extern const string guisuffix;
  
extern bool TeXinitialized; // Is LaTeX process initialized?
extern string initdir;

extern string paperType;
extern double pageWidth;
extern double pageHeight;
  
extern int scrollLines;
  
types::record *getSettingsModule();

vm::item &getSetting(string name);
  
template <typename T>
inline T getSetting(string name)
{
  return vm::get<T>(getSetting(name));
}

extern int verbose;

bool view();
bool trap();

void setOptions(int argc, char *argv[]);

// Access the arguments once options have been parsed.
int numArgs();
char *getArg(int n);
}
#endif
