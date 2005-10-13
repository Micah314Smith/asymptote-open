/*****
 * record.h
 * Andy Hammerlindl 2003/07/09
 *
 * The type for records and modules in the language.
 *****/

#ifndef RECORD_H
#define RECORD_H

#include "types.h"
#include "env.h"
#include "frame.h"
#include "access.h"

namespace vm {
struct lambda;
}

using trans::frame;
using trans::protoenv;
using trans::varEntry;
using trans::tyEntry;

namespace types {

struct record : public ty {
  // The base name of this type.
  symbol *name;
  
  // The frame.  Like a frame for a function, it allocates the accesses
  // for fields and specifies the size of the record.
  frame *level;
  
  // The runtime representation of the record used by the virtual machine.
  vm::lambda *init;

public:
  // The name bindings for fields of the record.
  protoenv e;

  record(symbol *name, frame *level);
  ~record();

#if 0 //{{{
  void addType(symbol *name, tyEntry *desc)
  {
    te.enter(name, desc);
  }

  void addVar(symbol *name, varEntry *desc)
  {
    ve.enter(name, desc);
  }

  void list()
  {
    ve.list();
  }

  tyEntry *lookupTypeEntry(symbol *s)
  {
    return te.look(s);
  }

  ty *lookupType(symbol *s)
  {
    tyEntry *ent=lookupTypeEntry(s);
    return ent ? ent->t : 0;
  }

  varEntry *lookupVarByType(symbol *name, ty *t)
  {
    return ve.lookByType(name, t);
  }

  ty *varGetType(symbol *name)
  {
    return ve.getType(name);
  }
#endif //}}}

  symbol *getName()
  {
    return name;
  }

  bool isReference() {
    return true;
  }

  size_t hash() {
    // Use the pointer, as two records are equivalent only if they are the
    // same object.
    return (size_t)this;
  }

  // Initialize to null by default.
  trans::access *initializer();

  frame *getLevel(bool statically = false)
  {
    if (statically) {
      frame *f=level->getParent();
      return f ? f : level;
    }
    else
      return level;
  }

  vm::lambda *getInit()
  {
    return init;
  }

  // Allocates a new dynamic field in the record.
  trans::access *allocField(bool statically)
  {
    frame *underlevel = getLevel(statically);
    assert(underlevel);
    return underlevel->allocLocal();
  }

  // Create a statically enclosed record from this record.
  record *newRecord(symbol *id, bool statically);

  void print(ostream& out) const
  {
    out << *name;
  }

  void debug(ostream& out) const
  {
    out << "struct " << *name << std::endl;
    out << "types:" << endl;
    out << "re-implement" << endl;
    //out << te;
    out << "fields: " << endl;
    out << "re-implement" << endl;
    //out << ve;
  }
};

} //namespace types

#endif  
