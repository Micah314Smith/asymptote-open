/*****
 * symbol.h
 * Andy Hammerlindl 2002/06/18
 *
 * Creates symbols from strings so that multiple calls for a symbol of
 * the same string will return a pointer to the same object.
 *****/

#ifndef SYMBOL_H
#define SYMBOL_H

#include <iostream>
#include <cassert>
#include <string>
#include <map>

#include "memory.h"

//using std::assert;
using std::ostream;
using std::string;

namespace sym {

struct symbol {
private:
  string name;

  static std::map<mem::string,symbol> dict;

  static symbol *specialTrans(string s) {
    assert(dict.find(s) == dict.end());
    return &(dict[s]=symbol(s,true));
  }

  symbol() : special(false) {}
  symbol(string name, bool special=false)
    : name(name), special(special) {}

  friend class std::map<mem::string,symbol>;
public:
  bool special; // NOTE: make this const (later).
  
  static symbol *initsym;
  static symbol *castsym;
  static symbol *ecastsym;
  
  static symbol *literalTrans(string s) {
    if (dict.find(s) != dict.end())
      return &dict[s];
    else
      return &(dict[s]=symbol(s));
  }

  static symbol *opTrans(string s) {
    return literalTrans("operator "+s);
  }

  static symbol *trans(string s) {
    // Figure out whether it's an operator or an identifier by looking at the
    // first character.
    char c=s[0];
    return isalpha(c) || c == '_' ? literalTrans(s) : opTrans(s);
  }

  operator string () { return string(name); }

  friend ostream& operator<< (ostream& out, const symbol& sym)
  { return out << sym.name; }
};

} // namespace sym

#endif
