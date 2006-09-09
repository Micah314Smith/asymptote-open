/*****
 * util.h
 * Andy Hammerlindl 2004/05/10
 *
 * A place for useful utility functions.
 *****/

#ifndef UTIL_H
#define UTIL_H

#include <sys/types.h>
#include <string>
#include <strings.h>
#include <iostream>

using std::cout;
using std::cerr;
using std::endl;
using std::ostringstream;
using std::string;

string stripext(const string& name, const string& suffix);
  
// Strip the directory from a filename.
string& stripDir(string& name);
  
// Construct a filename from the original, adding aux at the end, and
// changing the suffix.
string buildname(string filename, string suffix="", string aux="",
		 bool stripdir=true);

// Construct an alternate filename for a temporary file.
string auxname(string filename, string suffix="");

bool checkFormatString(const string& format);

// Similar to the standard system call except allows interrupts and does
// not invoke a shell.
int System(const char *command, int quiet=0, bool wait=true,
	   const char *hint=NULL, const char *application="",
	   int *pid=NULL);
int System(const ostringstream& command, int quiet=0, bool wait=true,
	   const char *hint=NULL, const char *application="",
	   int *pid=NULL); 
  
#if defined(__DECCXX_LIBCXX_RH70)
extern "C" int kill(pid_t pid, int sig) throw();
extern "C" char *strsignal(int sig);
#endif

#if defined(__DECCXX_LIBCXX_RH70) || defined(__CYGWIN__)
extern "C" int snprintf(char *str, size_t size, const char *format,...);
extern "C" double asinh(double x);
extern "C" double acosh(double x);
extern "C" double atanh(double x);
extern "C" double cbrt(double x);
extern "C" double erf(double x);
extern "C" double erfc(double x);
extern "C" double tgamma(double x);
extern "C" double remainder(double x, double y);
extern "C" double hypot(double x, double y) throw();
extern "C" double jn(int n, double x);
extern "C" double yn(int n, double x);
extern "C" int fileno(FILE *);
extern "C" char *strptime(const char *s, const char *format, struct tm *tm);
#endif

extern bool False;

// Strip blank lines (which would break the bidirectional TeX pipe)
string stripblanklines(string& s);

extern char *currentpath;

char *startPath();
char *getPath(char *p=currentpath);
int setPath(const char *s);

void backslashToSlash(string& s);
void spaceToUnderscore(string& s);
string Getenv(const char *name, bool quote=true);

void execError(const char *command, const char *hint, const char *application);
  
// This invokes a viewer to display the manual.  Subsequent calls will only
// pop-up a new viewer if the old one has been closed.
void popupHelp();
#endif
