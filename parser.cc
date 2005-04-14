/*****
 * parser.cc
 * Tom Prince 2004/01/10
 *
 *****/

#include <string>
#include <fstream>
#include <sstream>

#include "interact.h"
#include "locate.h"
#include "parser.h"

using std::string;

// The lexical analysis and parsing functions used by parseFile.
void setlexer(size_t (*input) (char* bif, size_t max_size),
              string filename);
extern bool yyparse(void);
extern int yydebug;
extern int yy_flex_debug;
static const int YY_NULL = 0;

namespace parser {

namespace yy { // Lexers

std::streambuf *sbuf = NULL;

size_t stream_input(char *buf, size_t max_size)
{
  size_t count= sbuf ? sbuf->sgetn(buf,max_size) : 0;
  return count ? count : YY_NULL;
}

} // namespace yy

void debug(bool state)
{
  // For debugging the lexer and parser that were machine generated.
  yy_flex_debug = yydebug = state;
}

absyntax::file *parseFile(string filename)
{
  string file = settings::locateFile(filename);

  if (file.empty())
    return 0;

  debug(false); 

  std::filebuf filebuf;
  if (!filebuf.open(file.c_str(),std::ios::in)) {
    return 0;
  }
  yy::sbuf = &filebuf;

  setlexer(yy::stream_input,filename);
  absyntax::file *root = yyparse() == 0 ? absyntax::root : 0;
  yy::sbuf = 0;
  return root;
}

absyntax::file *parseString(string code)
{
  std::stringbuf buf(code);
  yy::sbuf = &buf;
  setlexer(yy::stream_input,"<eval>");
  absyntax::file *root = yyparse() == 0 ? absyntax::root : 0;
  yy::sbuf = 0;
  return root;
}

absyntax::file *parseInteractive()
{
  debug(false);
  
#if defined(HAVE_LIBREADLINE) && defined(HAVE_LIBCURSES)
  setlexer(interact::interactive_input,"<stdin>");
#else
  yy::sbuf = std::cin.rdbuf();
  setlexer(yy::stream_input,"<stdin>");
#endif
  absyntax::file *root = yyparse() == 0 ? absyntax::root : 0;
  yy::sbuf = 0;
  return root;
}

}
