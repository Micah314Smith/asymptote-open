/*****
 * qname.h
 * Andy Hammerlindl2002/07/14
 *
 * Qualified names (such as x, f, builtin.sin, a.b.c.d, etc.) can be used
 * either as varibles or a type names.  This class stores qualified
 * names used in nameExp and nameTy in the abstract syntax, and
 * implements the exp and type functions.
 *****/

#ifndef QNAME_H
#define QNAME_H

#include "absyn.h"
#include "types.h"
#include "cast.h"
#include "env.h"

namespace as {

using trans::env;
using trans::env;
using types::record;
using std::ostream;

class name : public absyn {
public:
  name(position pos)
    : absyn(pos) {}

  // Used for determining the type when the context does not establish
  // the name as a variable or a type.
  // First, the function looks for a non-function variable fitting the
  // description.  If one fits, the type of the variable is returned.
  // Failing that, the function looks for a fitting type and returns 
  // that.  Finally, if no type matches, it looks to the environment to
  // get a module from the name.  If nothing is found, an appropriate
  // error is reported and ty_error is returned.
  // Because this is used only on qualifiers, it does not look at
  // function variables.
  // Tacit means that no error messages will be reported to the user.
  virtual types::ty *getType(env &e, bool tacit = false) = 0;

  // As a variable:
  virtual void varTrans(env &e, types::ty *target) = 0;
  virtual void varTransWrite(env &e, types::ty *target) = 0;
  virtual void varTransCall(env &e, types::ty *target) = 0;
  virtual types::ty *varGetType(env &e) = 0;

  // As a type:
  virtual types::ty *typeTrans(env &e, bool tacit = false) = 0;
  virtual trans::import *typeGetImport(env &e) = 0;

  // Pushes the highest level frame possible onto the stack.  Returning
  // the frame pushed.  If no frame can be pushed, returns 0.
  virtual frame *frameTrans(env &e) = 0;

  virtual void prettyprint(ostream &out, int indent) = 0;
  virtual void print(ostream& out) const {
    out << "<base name>";
  }

  virtual symbol *getName() = 0;
};

inline ostream& operator<< (ostream& out, const name& n) {
  n.print(out);
  return out;
}

class simpleName : public name {
  symbol *id;

public:
  simpleName(position pos, symbol *id)
    : name(pos), id(id) {}

  types::ty *getType(env &e, bool tacit = false);

  // As a variable:
  void varTrans(env &, types::ty *target);
  void varTransWrite(env &, types::ty *target);
  void varTransCall(env &, types::ty *target);
  types::ty *varGetType(env &);

  // As a type:
  types::ty *typeTrans(env &e, bool tacit = false);
  trans::import *typeGetImport(env &e);

  frame *frameTrans(env &e);
  
  void prettyprint(ostream &out, int indent);
  void print(ostream& out) const {
    out << *id;
  }
  symbol *getName() {
    return id;
  }
};


class qualifiedName : public name {
  name *qualifier;
  symbol *id;

  // Gets the record type associated with the container, and reports an
  // error and returns null if the qualifier is a variable or type of a
  // record.
  record *getRecord(types::ty *t, bool tacit = false);

public:
  qualifiedName(position pos, name *qualifier, symbol *id)
    : name(pos), qualifier(qualifier), id(id) {}

  types::ty *getType(env &e, bool tacit = false);

  // As a variable:
  void varTrans(env &, types::ty *target);
  void varTransWrite(env &, types::ty *target);
  void varTransCall(env &, types::ty *target);
  types::ty *varGetType(env &);

  // As a type:
  types::ty *typeTrans(env &e, bool tacit = false);
  trans::import *typeGetImport(env &e);

  frame *frameTrans(env &e);
  
  void prettyprint(ostream &out, int indent);
  void print(ostream& out) const {
    out << *qualifier << "." << *id;
  }
  symbol *getName() {
    return id;
  }
};

} // namespace as

#endif
