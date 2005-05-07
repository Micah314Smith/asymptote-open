/*****
 * locate.h
 * Tom Prince 2005/03/24
 *
 * Locate files in search path.
 *****/

#ifndef LOCATE_H
#define LOCATE_H

#include <list>
#include <string>

using std::string;

namespace settings {

typedef std::list<string> file_list_t;
extern file_list_t searchPath;

// Find the appropriate file, first looking in the local directory, then the
// directory given in settings, and finally the global system directory.
string locateFile(string id);

} // namespace settings

#endif // LOCATE_H
