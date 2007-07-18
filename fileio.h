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
#include "process.h"

namespace vm {
  extern bool indebugger;  
}

namespace camp {

extern string tab;
extern string newline;
  
class file : public gc {
protected:  
  string name;
  Int nx,ny,nz;    // Array dimensions
  bool linemode;   // Array reads will stop at eol instead of eof.
  bool csvmode;    // Read comma-separated values.
  bool wordmode;   // Delimit strings by white space instead of eol.
  bool singlereal; // Read/write single-precision XDR/binary reals.
  bool singleint;  // Read/write single-precision XDR/binary ints.
  bool closed;     // File has been closed.
  bool checkerase; // Check input for errors/erase output.
  bool standard;   // Standard input/output
  bool binary;     // Read in binary mode.
  Int lines;       // Number of scrolled lines
  
  bool nullfield;  // Used to detect a final null field in csv+line mode.
  string whitespace;
  size_t index;	   // Terminator index.
public: 

  void resetlines() {lines=0;}
  
  bool Standard() {return standard;}
  
  void standardEOF() {
#if defined(HAVE_LIBREADLINE) && defined(HAVE_LIBCURSES)
    cout << endl;
#endif	
  }
  
  template<class T>
  void purgeStandard(T&) {
    if(standard) {
      int c;
      if(cin.eof())
	standardEOF();
      else {
	cin.clear();
	while((c=cin.peek()) != EOF) {
	  cin.ignore();
	  if(c == '\n') break;
	}
      }
    }
  }
  
  void purgeStandard(string&) {
    if(cin.eof())
      standardEOF();
  }
  
  void dimension(Int Nx=-1, Int Ny=-1, Int Nz=-1) {nx=Nx; ny=Ny; nz=Nz;}
  
  file(const string& name, bool checkerase=true, bool binary=false,
       bool closed=false) : 
    name(name), linemode(false), csvmode(false),
    singlereal(false), singleint(true),
    closed(closed), checkerase(checkerase), standard(name.empty()),
    binary(binary), lines(0), nullfield(false), whitespace("") {dimension();}
  
  virtual void open() {}
  
  void Check() {
    if(error()) {
      ostringstream buf;
      buf << "Cannot open file \"" << name << "\".";
      reportError(buf);
    }
  }
  
  virtual ~file() {}

  virtual const char* Mode() {return "";}

  bool isOpen() {
    if(closed) {
      ostringstream buf;
      buf << "I/O operation attempted on ";
      if(name != "") buf << "closed file \'" << name << "\'.";
      else buf << "null file.";
      reportError(buf);
    }
    return true;
  }
		
  string filename() {return name;}
  virtual bool eol() {return false;}
  virtual bool nexteol() {return false;}
  virtual bool text() {return false;}
  virtual bool eof() {return true;}
  virtual bool error() {return true;}
  virtual void close() {}
  virtual void clear() {}
  virtual void precision(Int) {}
  virtual void flush() {}
  virtual size_t tell() {return 0;}
  virtual void seek(Int, bool=true) {}
  
  void unsupported(const char *rw, const char *type) {
    ostringstream buf;
    buf << rw << " of type " << type << " not supported in " << Mode()
	<< " mode.";
    reportError(buf);
  }
  
  void noread(const char *type) {unsupported("Read",type);}
  void nowrite(const char *type) {unsupported("Write",type);}
  
  virtual void Read(bool&) {noread("bool");}
  virtual void Read(Int&) {noread("Int");}
  virtual void Read(double&) {noread("real");}
  virtual void Read(float&) {noread("real");}
  virtual void Read(pair&) {noread("pair");}
  virtual void Read(triple&) {noread("triple");}
  virtual void Read(char&) {noread("char");}
  virtual void Read(string&) {noread("string");}
  virtual void readwhite(string&) {noread("string");}
  
  virtual void write(bool) {nowrite("bool");}
  virtual void write(Int) {nowrite("Int");}
  virtual void write(double) {nowrite("real");}
  virtual void write(const pair&) {nowrite("pair");}
  virtual void write(const triple&) {nowrite("triple");}
  virtual void write(const string&) {nowrite("string");}
  virtual void write(const pen&) {nowrite("pen");}
  virtual void write(guide *) {nowrite("guide");}
  virtual void write(const transform&) {nowrite("transform");}
  virtual void writeline() {nowrite("string");}
  
  virtual void ignoreComment(bool=false) {};
  virtual void csv() {};
  
  template<class T>
  void ignoreComment(T&) {
    ignoreComment();
  }
  
  void ignoreComment(string&) {}
  void ignoreComment(char&) {}
  
  template<class T>
  void read(T& val) {
    if(binary) Read(val);
    else {
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
  }
  
  Int Nx() {return nx;}
  Int Ny() {return ny;}
  Int Nz() {return nz;}
  
  void LineMode(bool b) {linemode=b;}
  bool LineMode() {return linemode;}
  
  void CSVMode(bool b) {csvmode=b; if(b) wordmode=false;}
  bool CSVMode() {return csvmode;}
  
  void WordMode(bool b) {wordmode=b; if(b) csvmode=false;}
  bool WordMode() {return wordmode;}
  
  void SingleReal(bool b) {singlereal=b;}
  bool SingleReal() {return singlereal;}
  
  void SingleInt(bool b) {singleint=b;}
  bool SingleInt() {return singleint;}
};

class ifile : public file {
protected:  
  istream *stream;
  std::fstream *fstream;
  char comment;
  bool comma;
  
public:
  ifile(const string& name, char comment, bool check=true) :
    file(name,check), comment(comment), comma(false) {stream=&cin;}
  
  // Binary file
  ifile(const string& name, bool check=true) : file(name,check,true) {}
  
  ~ifile() {close();}
  
  void open() {
    if(standard) {
      stream=&cin;
    } else {
      stream=fstream=new std::fstream(name.c_str());
      index=global.back()->ifile.add(fstream);
      if(checkerase) Check();
    }
  }
  
  const char* Mode() {return "input";}
  
  bool eol();
  bool nexteol();
  
  bool text() {return true;}
  bool eof() {return stream->eof();}
  bool error() {return stream->fail();}
  
  void close() {
    if(!standard && fstream) {
      fstream->close();
      closed=true;
      delete fstream;
      fstream=NULL;
      global.back()->ifile.remove(index);
    }
  }
  
  void clear() {stream->clear();}
  
  void seek(Int pos, bool begin=true) {
    if(!standard && fstream) {
      clear();
      fstream->seekg(pos,begin ? std::ios::beg : std::ios::end);
    }
  }
  
  size_t tell() {return fstream->tellg();}
  
  void csv();
  
  virtual void ignoreComment(bool readstring=false);
  
  // Skip over white space
  void readwhite(string& val) {val=string(); *stream >> val;}
  
  void Read(bool &val) {string t; readwhite(t); val=(t == "true");}
  void Read(Int& val) {*stream >> val;}
  void Read(double& val) {*stream >> val;}
  void Read(pair& val) {*stream >> val;}
  void Read(triple& val) {*stream >> val;}
  void Read(char& val) {stream->get(val);}
  void Read(string& val);
};
  
class iofile : public ifile {
public:
  iofile(const string& name, char comment=0) : ifile(name,true,comment) {}

  void precision(Int p) {stream->precision(p);}
  void flush() {fstream->flush();}
  
  void write(bool val) {*fstream << (val ? "true " : "false ");}
  void write(Int val) {*fstream << val;}
  void write(double val) {*fstream << val;}
  void write(const pair& val) {*fstream << val;}
  void write(const triple& val) {*fstream << val;}
  void write(const string& val) {*fstream << val;}
  void write(const pen& val) {*fstream << val;}
  void write(guide *val) {*fstream << *val;}
  void write(const transform& val) {*fstream << val;}
  
  void writeline() {
    *fstream << newline;
    if(errorstream::interrupt) throw interrupted();
  }
};
  
class ofile : public file {
protected:
  ostream *stream;
  std::ofstream *fstream;
public:
  ofile(const string& name) : file(name), fstream(NULL) {stream=&cout;}
  
  ~ofile() {close();}
  
  void open() {
    checkLocal(name);
    if(standard) {
      stream=&cout;
    } else {
      stream=fstream=new std::ofstream(name.c_str(),std::ios::trunc);
      index=global.back()->ofile.add(fstream);
      Check();
    }
  }
  
  const char* Mode() {return "output";}

  bool text() {return true;}
  bool eof() {return stream->eof();}
  bool error() {return stream->fail();}
  
  void close() {
    if(!standard && fstream) {
      fstream->close();
      closed=true;
      delete fstream;
      fstream=NULL;
      global.back()->ofile.remove(index);
    }
  }
  void clear() {stream->clear();}
  void precision(Int p) {stream->precision(p);}
  void flush() {stream->flush();}
  
  void seek(Int pos, bool begin=true) {
    if(!standard && fstream) {
      clear();
      fstream->seekp(pos,begin ? std::ios::beg : std::ios::end);
    }
  }
  
  size_t tell() {return fstream->tellp();}
  
  void write(bool val) {*stream << (val ? "true " : "false ");}
  void write(Int val) {*stream << val;}
  void write(double val) {*stream << val;}
  void write(const pair& val) {*stream << val;}
  void write(const triple& val) {*stream << val;}
  void write(const string& val) {*stream << val;}
  void write(const pen& val) {*stream << val;}
  void write(guide *val) {*stream << *val;}
  void write(const transform& val) {*stream << val;}
  
  void writeline();
};

class ibfile : public ifile {
public:
  ibfile(const string& name, bool check=true) : ifile(name,check) {}

  void open() {
    if(standard) {
      reportError("Cannot open standard input in binary mode");
    } else {
      stream=fstream=new std::fstream(name.c_str(),std::ios::binary |
				      std::ios::in | std::ios::out);
      if(checkerase) Check();
    }
  }
  
  template<class T>
  void iread(T& val) {
    val=T();
    fstream->read((char *) &val,sizeof(T));
  }
  
  void Read(bool& val) {iread(val);}
  void Read(Int& val) {
    if(singleint) {int ival; iread(ival); val=ival;}
    else iread(val);
  }
  void Read(char& val) {iread(val);}
  void Read(string& val) {iread(val);}
  
  void Read(double& val) {
    if(singlereal) {float fval; iread(fval); val=fval;}
    else iread(val);
  }
};
  
class iobfile : public ibfile {
public:
  iobfile(const string& name) : ibfile(name,true) {}

  void flush() {fstream->flush();}
  
  template<class T>
  void iwrite(T val) {
    fstream->write((char *) &val,sizeof(T));
  }
  
  void write(bool val) {iwrite(val);}
  void write(Int val) {
    if(singleint) iwrite(intcast(val));
    else iwrite(val);
  }
  void write(const string& val) {iwrite(val);}
  void write(const pen& val) {iwrite(val);}
  void write(guide *val) {iwrite(val);}
  void write(const transform& val) {iwrite(val);}
  void write(double val) {
    if(singlereal) iwrite((float) val);
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
  void writeline() {}
};
  
class obfile : public ofile {
public:
  obfile(const string& name) : ofile(name) {}

  void open() {
    checkLocal(name);
    if(standard) {
      reportError("Cannot open standard output in binary mode");
    } else {
      stream=fstream=new std::ofstream(name.c_str(),
				       std::ios::binary | std::ios::trunc);
      Check();
    }
  }
  
  template<class T>
  void iwrite(T val) {
    fstream->write((char *) &val,sizeof(T));
  }
  
  void write(bool val) {iwrite(val);}
  void write(Int val) {
    if(singleint) iwrite(intcast(val));
    else iwrite(val);
  }
  void write(const string& val) {iwrite(val);}
  void write(const pen& val) {iwrite(val);}
  void write(guide *val) {iwrite(val);}
  void write(const transform& val) {iwrite(val);}
  void write(double val) {
    if(singlereal) iwrite((float) val);
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
  
  void writeline() {}
};
  
#ifdef HAVE_RPC_RPC_H

class ixfile : public file {
protected:  
  xdr::ioxstream *fstream;
  xdr::xios::open_mode mode;
public:
  ixfile(const string& name, bool check=true,
	 xdr::xios::open_mode mode=xdr::xios::in) :
    file(name,check,true), mode(mode) {}

  void open() {
    fstream=new xdr::ioxstream(name.c_str(),mode);
    index=global.back()->ixfile.add(fstream);
    if(checkerase) Check();
  }
    
  void close() {
    if(fstream) {
      fstream->close();
      closed=true;
      delete fstream;
      fstream=NULL;
      global.back()->ixfile.remove(index);
    }
  }
  
  ~ixfile() {close();}
  
  const char* Mode() {return "xinput";}
  
  bool eof() {return fstream->eof();}
  bool error() {return fstream->fail();}

  void clear() {fstream->clear();}
  
  void seek(Int pos, bool begin=true) {
    if(!standard && fstream) {
      clear();
      fstream->seek(pos,begin ? xdr::xios::beg : xdr::xios::end);
    }
  }
  
  size_t tell() {return fstream->tell();}
  
  void Read(Int& val) {
    if(singleint) {int ival=0; *fstream >> ival; val=ival;}
    else {
      val=0;
      *fstream >> val;
    }
  }
  void Read(double& val) {
    if(singlereal) {float fval=0.0; *fstream >> fval; val=fval;}
    else {
      val=0.0;
      *fstream >> val;
    }
  }
  void Read(pair& val) {
    double x,y;
    Read(x);
    Read(y);
    val=pair(x,y);
  }
  void Read(triple& val) {
    double x,y,z;
    Read(x);
    Read(y);
    Read(z);
    val=triple(x,y,z);
  }
};

class ioxfile : public ixfile {
public:
  ioxfile(const string& name) : ixfile(name,true,xdr::xios::out) {}

  void flush() {fstream->flush();}
  
  void write(Int val) {
    if(singleint) *fstream << intcast(val);
    else *fstream << val;
  }
  void write(double val) {
    if(singlereal) *fstream << (float) val;
    else *fstream << val;
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
  
class oxfile : public file {
  xdr::oxstream *fstream;
public:
  oxfile(const string& name) : file(name) {}

  void open() {
    fstream=new xdr::oxstream((checkLocal(name),name.c_str()),xdr::xios::trunc);
    index=global.back()->oxfile.add(fstream);
    Check();
  }
  
  void close() {
    if(fstream) {
      fstream->close();
      closed=true;
      delete fstream;
      fstream=NULL;
      global.back()->oxfile.remove(index);
    }
  }
  
  ~oxfile() {close();}
  
  const char* Mode() {return "xoutput";}
  
  bool eof() {return fstream->eof();}
  bool error() {return fstream->fail();}
  void clear() {fstream->clear();}
  void flush() {fstream->flush();}
  
  void seek(Int pos, bool begin=true) {
    if(!standard && fstream) {
      clear();
      fstream->seek(pos,begin ? xdr::xios::beg : xdr::xios::end);
    }
  }
  
  size_t tell() {return fstream->tell();}
  
  void write(Int val) {
    if(singleint) *fstream << intcast(val);
    else *fstream << val;
  }
  void write(double val) {
    if(singlereal) *fstream << (float) val;
    else *fstream << val;
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
extern file nullfile;

} // namespace camp

#endif // FILEIO_H
