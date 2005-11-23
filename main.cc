#include <iostream>
#include <cfloat>
#include <csignal>
#include <cstdlib>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_LIBSIGSEGV
#include <sigsegv.h>
#endif

#include "types.h"
#include "errormsg.h"
#include "fpu.h"
#include "genv.h"
#include "stm.h"
#include "settings.h"
#include "vm.h"
#include "program.h"
#include "interact.h"
#include "parser.h"
#include "fileio.h"

#include "stack.h"
#include "runtime.h"

using namespace settings;
using std::list;

using absyntax::file;
using trans::genv;
using trans::coenv;
using trans::env;
using trans::coder;
using types::record;

errorstream *em;
using interact::interactive;
using interact::virtualEOF;
using interact::resetenv;
using interact::uptodate;

#ifdef HAVE_LIBSIGSEGV
void stackoverflow_handler (int, stackoverflow_context_t)
{
  em->runtime(vm::getPos());
  cout << "Stack overflow" << endl;
  abort();
}

int sigsegv_handler (void *, int emergency)
{
  if(!emergency) return 0; // Really a stack overflow
  em->runtime(vm::getPos());
  cout << "Segmentation fault" << endl;
  cout << "Please report this programming error to" << endl 
       << BUGREPORT << endl;
  abort();
}
#endif 

void setsignal(RETSIGTYPE (*handler)(int))
{
#ifdef HAVE_LIBSIGSEGV
  char mystack[16384];
  if(stackoverflow_install_handler(&stackoverflow_handler,
				   mystack,sizeof (mystack)) < 0) exit(1);
  if(sigsegv_install_handler (&sigsegv_handler) < 0) exit (1);
#endif
  signal(SIGBUS,handler);
  signal(SIGFPE,handler);
}

void signalHandler(int)
{
  if(em) em->runtime(vm::getPos());
  signal(SIGBUS,SIG_DFL);
  signal(SIGFPE,SIG_DFL);
}

void interruptHandler(int)
{
  if(em) em->Interrupt(true);
}

bool status=true;

namespace loop {

void init()
{
  setPath(startPath());
  ShipoutNumber=0;
  if(!em)
    em = new errorstream();
}

void purge()
{
#ifdef USEGC
  GC_gcollect();
#endif
}

void doPrint(genv&, record *m)
{
  // NOTE: Should make it possible to show more code.
  print(cout, m->getInit()->code);
}

// Run (an already translated) module of the given filename.
void doRun(genv& ge, std::string filename)
{
  vm::stack s;
  s.setInitMap(ge.getInitMap());
  s.load(filename);
  run::exitFunction(&s);
}

typedef vm::interactiveStack istack;
using absyntax::runnable;
using absyntax::block;

mem::vector<coenv*> estack;
mem::vector<vm::interactiveStack*> sstack;

// Abstract base class for the core object being run in line-at-a-time mode, it
// may be a runnable, block, file, or interactive prompt.
struct icore {
  virtual ~icore() {}
  
  virtual void run(coenv &e, istack &s) = 0;
  
  void embedded() {
    assert(estack.size() && sstack.size());
    run(*(estack.back()),*(sstack.back()));
  };
};

struct irunnable : public icore {
  runnable *r;

  irunnable(runnable *r)
    : r(r) {}

  void run(coenv &e, istack &s) {
    e.e.beginScope();
    lambda *codelet=r->transAsCodelet(e);
    em->sync();
    if(!em->errors()) {
      s.run(codelet);
    } else {
      e.e.endScope(); // Remove any changes to the environment.
      status=false;
    }
  }
};

struct itree : public icore {
  absyntax::block *ast;

  itree(absyntax::block *ast)
    : ast(ast) {}

  void run(coenv &e, istack &s) {
    for(list<runnable *>::iterator r=ast->stms.begin();
	r != ast->stms.end(); ++r)
      irunnable(*r).run(e,s);
  }
};

struct iprompt : public icore {
  void run(coenv &e, istack &s) {
    while (virtualEOF && !resetenv) {
      virtualEOF=false;
      try {
        file *ast = parser::parseInteractive();
        assert(ast);
        itree(ast).run(e,s);
	if(!uptodate)
	  run::updateFunction(&s);
      } catch (interrupted&) {
        if(em) em->Interrupt(false);
        cout << endl;
      } catch (handled_error) {
        status=false;
      }
    }
    run::cleanup();
  }
};

void doICore(icore &i, bool embedded=false) {
  assert(em);
  em->sync();
  if(em->errors()) return;
  
  try {
    if(embedded)
      i.embedded();
    else {
      purge();
      
      genv ge;
      env base_env(ge);
      coder base_coder;
      coenv e(base_coder,base_env);
      
      vm::interactiveStack s;
      s.setInitMap(ge.getInitMap());

      estack.push_back(&e);
      sstack.push_back(&s);

      if(settings::autoplain) {
	absyntax::runnable *r=absyntax::autoplainRunnable();
	irunnable(r).run(e,s);
      }

      // Now that everything is set up, run the core.
      i.run(e,s);
      
      run::exitFunction(&s);

      estack.pop_back();
      sstack.pop_back();
      
      if(settings::listvariables)
	base_env.list();
    
    }
  } catch (std::bad_alloc&) {
    cerr << "error: out of memory" << endl;
    status=false;
  } catch(handled_error) {
    status=false;
    run::cleanup();
  }

  em->clear();
}
      
void doIRunnable(runnable *r, bool embedded=false) {
  assert(r);
  irunnable i(r);
  doICore(i,embedded);
}

void doITree(block *tree, bool embedded=false) {
  assert(tree);
  itree i(tree);
  doICore(i,embedded);
}

void doIFile(const string& filename) {
  init();

  string basename = stripext(filename,suffix);
  if(verbose) cout << "Processing " << basename << endl;
  
  if(parseonly) {
    absyntax::file *tree = parser::parseFile(filename);
    assert(tree);
    em->sync();
    if(!em->errors())
      tree->prettyprint(cout, 0);
    else status=false;
  } else if(translate) {
    genv ge;
    record *m = ge.getModule(symbol::trans(basename),filename);
    if(!em->errors())
      doPrint(ge,m);
  } else {
    if(filename == "")
      doITree(parser::parseString(""));
    else {
      if(outname.empty())
	outname=(filename == "-") ? "out" : stripDir(basename);
      doITree(parser::parseFile(filename));
      outname="";
    }
  }
}

void doIPrompt() {
  init();
  outname="out";
  
  iprompt i;
  do {
    resetenv=false;
    doICore(i);
  } while(resetenv);
  outname="";
}

} // namespace loop

#ifdef USEGC
void no_GCwarn(char *, GC_word) {}
#endif

int main(int argc, char *argv[])
{
  setOptions(argc,argv);

#ifdef USEGC
  GC_free_space_divisor = 2;
  GC_dont_expand = 0;
  GC_INIT();
  if(!debug) GC_set_warn_proc(no_GCwarn);
#endif  

  fpu_trap(trap);
  setsignal(signalHandler);
  if(interactive) signal(SIGINT,interruptHandler);

  cout.precision(DBL_DIG);

  try {
    if(interactive)
      loop::doIPrompt();
    else
      if(numArgs() == 0) {
	loop::doIFile("");
      } else for(int ind=0; ind < numArgs() ; ind++)
	loop::doIFile(string(getArg(ind)));
  }
  catch (handled_error) {
    status=false;
  }
  loop::purge();
  return status ? 0 : 1;
}
