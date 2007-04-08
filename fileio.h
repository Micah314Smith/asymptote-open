/******
 * fileio.h
 * Tom Prince and John Bowman 2004/05/10
 *
 * Handle input/output
 ******/

#ifndef FILEIO_H
#define FILEIO_H

#include <fstream>
#include <iostream>
#include <sstream>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_RPC_RPC_H
#include "xstream.h"
#endif

#include "common.h"
#include "pair.h"
#include "triple.h"
#include "guide.h"
#include "pen.h"

#include "camperror.h"
#include "interact.h"
#include "errormsg.h"
#include "util.h"

namespace vm {
extern bool indebugger;  
}

namespace camp {

extern string tab;
extern string newline;
  
class file : public gc_cleanup {
protected:  
  string name;
  int nx,ny,nz;    // Array dimensions
  bool linemode;   // Array reads will stop at eol instead of eof.
  bool csvmode;    // Read comma-separated values.
  bool wordmode;   // Delimit strings by white space instead of eol.
  bool singlemode; // Read/write single-precision XDR/binary values.
  bool closed;     // File has been closed.
  bool checkappend;// Check input for errors/append to output.
  bool standard;   // Standard input/output
  int lines;       // Number of scrolled lines
public: 

  void resetlines() {lines=0;}
  
  bool Standard() {return standard;}
  
  void dimension(int Nx=-1, int Ny=-1, int Nz=-1) {nx=Nx; ny=Ny; nz=Nz;}
  
  file(const string& name, bool checkappend=true) : 
    name(name), linemode(false), csvmode(false), singlemode(false),
    closed(false), checkappend(checkappend), standard(name.empty()),
    lines(0) {dimension();}
  
  virtual void open() {}
  
  void Check() {
    if(error()) {
      ostringstream buf;
      buf << "Cannot open file \"" << name << "\".";
      reportError(buf);
    }
  }
  
  virtual ~file() {}

  virtual const char* Mode()=0;

  bool isOpen() {
    if(closed) {
      ostringstream buf;
      buf << "I/O operation attempted on closed file \'" << name << "\'.";
      reportError(buf);
    }
    return true;
  }
		
  string filename() {return name;}
  virtual bool eol() {return false;}
  virtual bool nexteol() {return false;}
  virtual bool text() {return false;}
  virtual bool eof()=0;
  virtual bool error()=0;
  virtual void close()=0;
  virtual void clear()=0;
  virtual void precision(int) {}
  virtual void flush() {}
  virtual size_t tell() {return 0;}
  virtual void seek(size_t) {}
  
  void unsupported(const char *rw, const char *type) {
    ostringstream buf;
    buf << rw << " of type " << type << " not supported in " << Mode()
	<< " mode.";
    reportError(buf);
  }
  
  void noread(const char *type) {unsupported("Read",type);}
  void nowrite(const char *type) {unsupported("Write",type);}
  
  virtual void read(bool&) {noread("bool");}
  virtual void read(int&) {noread("int");}
  virtual void read(double&) {noread("real");}
  virtual void read(float&) {noread("real");}
  virtual void read(pair&) {noread("pair");}
  virtual void read(triple&) {noread("triple");}
  virtual void read(char&) {noread("char");}
  virtual void readwhite(string&) {noread("string");}
  virtual void read(string&) {noread("string");}
  
  virtual void write(bool) {nowrite("bool");}
  virtual void write(int) {nowrite("int");}
  virtual void write(double) {nowrite("real");}
  virtual void write(const pair&) {nowrite("pair");}
  virtual void write(const triple&) {nowrite("triple");}
  virtual void write(const string&) {nowrite("string");}
  virtual void write(const pen&) {nowrite("pen");}
  virtual void write(guide *) {nowrite("guide");}
  virtual void write(const transform&) {nowrite("transform");}
  virtual void writeline() {nowrite("string");}
  
  int Nx() {return nx;}
  int Ny() {return ny;}
  int Nz() {return nz;}
  
  void LineMode(bool b) {linemode=b;}
  bool LineMode() {return linemode;}
  
  void CSVMode(bool b) {csvmode=b; if(b) wordmode=false;}
  bool CSVMode() {return csvmode;}
  
  void WordMode(bool b) {wordmode=b; if(b) csvmode=false;}
  bool WordMode() {return wordmode;}
  
  void SingleMode(bool b) {singlemode=b;}
  bool SingleMode() {return singlemode;}
};

class ifile : public file {
protected:  
  istream *stream;
  std::ifstream fstream;
  char comment;
  bool comma,nullfield; // Used to detect a final null field in csv+line mode.
  string whitespace;
  
public:
  ifile(const string& name, bool check=true, char comment=0)
    : file(name,check), comment(comment), comma(false), nullfield(false) {
    stream=&cin;
  }
  
  ~ifile() {close();}
  
  void open() {
    if(standard) {
      stream=&cin;
    } else {
      fstream.open(name.c_str());
      stream=&fstream;
      if(checkappend) Check();
    }
  }
  
  void seek(size_t pos) {
    if(!standard && !closed) fstream.seekg(pos);
  }
  
  size_t tell() {
    return fstream.tellg();
  }
  
  const char* Mode() {return "input";}
  
  void csv();
  
  void ignoreComment(bool readstring=false);
  
  template<class T>
  void ignoreComment(T&) {
    ignoreComment();
  }
  
  void ignoreComment(string&) {}
  void ignoreComment(char&) {}
  
  bool eol();
  bool nexteol();
  
  bool text() {return true;}
  bool eof() {return stream->eof();}
  bool error() {return stream->fail();}
  void close() {if(!standard && !closed) {fstream.close(); closed=true;}}
  void clear() {stream->clear();}
  
public:

  // Skip over white space
  void readwhite(string& val) {val=string(); *stream >> val;}
  
  void Read(bool &val) {string t; readwhite(t); val=(t == "true");}
  void Read(int& val) {*stream >> val;}
  void Read(double& val) {*stream >> val;}
  void Read(pair& val) {*stream >> val;}
  void Read(triple& val) {*stream >> val;}
  void Read(char& val) {stream->get(val);}
  void Read(string& val);
  
  template<class T>
  void iread(T&);
  
  void read(bool& val) {iread(val);}
  void read(int& val) {iread(val);}
  void read(double& val) {iread(val);}
  void read(pair& val) {iread(val);}
  void read(triple& val) {iread(val);}
  void read(char& val) {iread(val);}
  void read(string& val) {iread(val);}
};
  
class ibfile : public ifile {
public:
  ibfile(const string& name, bool check=true)
    : ifile(name,check) {}

  void open() {
    if(standard) {
      reportError("Cannot open standard input in binary mode");
    } else {
      fstream.open(name.c_str(),std::ios::binary);
      stream=&fstream;
      if(checkappend) Check();
    }
  }
  
  template<class T>
  void iread(T& val) {
    val=T();
    fstream.read((char *) &val,sizeof(T));
  }
  
  void read(bool& val) {iread(val);}
  void read(int& val) {iread(val);}
  void read(char& val) {iread(val);}
  void read(string& val) {iread(val);}
  
  void read(double& val) {
    if(singlemode) {float fval=0.0; iread(fval); val=fval;}
    else iread(val);
  }
};
  
class ofile : public file {
protected:
  std::ostream *stream;
  std::ofstream fstream;
public:
  ofile(const string& name, bool append=false) : file(name,append) {
      stream=&cout;
  }
  
  ~ofile() {close();}
  
  void open() {
    checkLocal(name);
    if(standard) {
      stream=&cout;
    } else {
      fstream.open(name.c_str(),checkappend ? std::ios::app : std::ios::trunc);
      stream=&fstream;
      Check();
    }
  }
  
  void seek(size_t pos) {
    if(!standard && !closed) fstream.seekp(pos);
  }
  
  const char* Mode() {return "output";}

  bool text() {return true;}
  bool eof() {return stream->eof();}
  bool error() {return stream->fail();}
  void close() {if(!standard && !closed) {fstream.close(); closed=true;}}
  void clear() {stream->clear();}
  void precision(int p) {stream->precision(p);}
  void flush() {stream->flush();}
  
  void write(bool val) {*stream << (val ? "true " : "false ");}
  void write(int val) {*stream << val;}
  void write(double val) {*stream << val;}
  void write(const pair& val) {*stream << val;}
  void write(const triple& val) {*stream << val;}
  void write(const string& val) {*stream << val;}
  void write(const pen& val) {*stream << val;}
  void write(guide *val) {*stream << *val;}
  void write(const transform& val) {*stream << val;}
  void writeline();
};

class obfile : public ofile {
public:
  obfile(const string& name, bool append=false)
    : ofile(name,append) {}

  void open() {
    checkLocal(name);
    if(standard) {
      reportError("Cannot open standard output in binary mode");
    } else {
      fstream.open(name.c_str(),std::ios::binary |
		   (checkappend ? std::ios::app : std::ios::trunc));
      stream=&fstream;
      Check();
    }
  }
  
  template<class T>
  void iwrite(T val) {
    fstream.write((char *) &val,sizeof(T));
  }
  
  void write(bool val) {iwrite(val);}
  void write(int val) {iwrite(val);}
  void write(const string& val) {iwrite(val);}
  void write(const pen& val) {iwrite(val);}
  void write(guide *val) {iwrite(val);}
  void write(const transform& val) {iwrite(val);}
  void writeline() {}
  
  void write(double val) {
    if(singlemode) {float fval=val; iwrite(fval);}
    else iwrite(val);
  }
  void write(const pair& val) {
    write(val.getx());
    write(val.gety());
  }
  void write(const triple& val) {
    write(val.getx());
    write(val.gety());
    write(val.getz());
  }
};
  
#ifdef HAVE_RPC_RPC_H

class ixfile : public file {
  xdr::ixstream stream;
public:
  ixfile(const string& name, bool check=true) : 
    file(name,check), stream(name.c_str()) {if(check) Check();}

  ~ixfile() {close();}
  
  const char* Mode() {return "xinput";}
  
  bool eof() {return stream.eof();}
  bool error() {return stream.fail();}
  void close() {if(!closed) {stream.close(); closed=true;}}
  void clear() {stream.clear();}
  
  void read(int& val) {val=0; stream >> val;}
  void read(double& val) {
    if(singlemode) {float fval=0.0; stream >> fval; val=fval;}
    else {
      val=0.0;
      stream >> val;
    }
  }
  void read(pair& val) {
    double x,y;
    read(x);
    read(y);
    val=pair(x,y);
  }
  void read(triple& val) {
    double x,y,z;
    read(x);
    read(y);
    read(z);
    val=triple(x,y,z);
  }
};

class oxfile : public file {
  xdr::oxstream stream;
public:
  oxfile(const string& name, bool append=false) : 
    file(name), stream((checkLocal(name), name.c_str()),
		       append ? xdr::xios::app : xdr::xios::trunc) {Check();}

  ~oxfile() {close();}
  
  const char* Mode() {return "xoutput";}
  
  bool eof() {return stream.eof();}
  bool error() {return stream.fail();}
  void close() {if(!closed) {stream.close(); closed=true;}}
  void clear() {stream.clear();}
  void flush() {stream.flush();}
  
  void write(int val) {stream << val;}
  void write(double val) {
    if(singlemode) {float fval=val; stream << fval;}
    else stream << val;
  }
  void write(const pair& val) {
    write(val.getx());
    write(val.gety());
  }
  void write(const triple& val) {
    write(val.getx());
    write(val.gety());
    write(val.getz());
  }
};

#endif

extern ofile Stdout;
extern ofile nullfile;

template<class T>
void ifile::iread(T& val)
{
  if(standard) clear();
  if(errorstream::interrupt) throw interrupted();
  else {
    ignoreComment(val);
    val=T();
    if(!nullfield)
      Read(val);
    csv();
    whitespace="";
  }
}

} // namespace camp

#endif // FILEIO_H
