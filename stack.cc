/*****
 * stack.cc
 * Andy Hammerlindl 2002/06/27
 *
 * The general stack machine that will be used to run compiled camp
 * code.
 *****/

#include "stack.h"
#include "errormsg.h"
#include "util.h"

//#define DEBUG_STACK

#ifdef DEBUG_STACK
#include <iostream>
using std::cout;
using std::cerr;
using std::endl;
#endif

namespace vm {

namespace {
const program::label nulllabel;

}

inline stack::vars_t stack::make_frame(size_t size)
{
  return frame(new item[size]);
}

stack::stack(int numGlobals)
  : numGlobals(numGlobals), vars()
{
  ip = nulllabel;
  globals = make_frame(numGlobals);
}

stack::~stack()
{}

void stack::run(lambda *l)
{
  func f;
  f.body = l;
    
  run(&f);
}

void stack::run(record *r)
{
  func f;
  f.body = r->init;
  f.closure = make_frame(r->size);

  run(&f);
}

#define UNALIAS                                 \
  {                                             \
    this->ip = ip;                              \
    this->vars = vars;                          \
    this->body = body;                          \
  }

void stack::run(func *f)
{
  lambda *body = f->body;

#ifdef DEBUG_STACK
  cout << "running lambda: \n";
  print(cout, body->code);
  cout << endl;
#endif
  
  /* alias the variables */
  
  /* start the new function */
  program::label ip = body->code.begin();
  /* make new activation record */
  vars_t vars = vars = make_frame(body->vars);

  vars[0] = f->closure;
  for (int i = body->params; i > 0; --i)
    vars[i] = pop();

 /* for binops */
  int a, b;
  bool m, n;
  double x, y;
  vars_t u, v;

  try {
    for (;;) {
#ifdef DEBUG_STACK
      UNALIAS;
      printInst(cerr, ip, body->code.begin());
      cerr << "\n";
#endif
   
      switch ((ip++)->op)
        {
          case inst::pop:
            pop();
            break;

          case inst::intpush:
            push((ip++)->val);
            break;
        
          case inst::constpush:
            push((ip++)->ref);
            break;

          case inst::varpush:
            push(vars[(ip++)->val]);
            break;

          case inst::varsave:
            vars[(ip++)->val] = top();
            break;

          case inst::globalpush:
            push(globals[(ip++)->val]);
            break;

          case inst::globalsave:
            globals[(ip++)->val] = top();
            break;

          case inst::fieldpush: {
            vars_t frame = pop<vars_t>();
            if (!frame) {
              UNALIAS;
              em->runtime(getPos());
              *em << "dereference of null pointer";
              em->sync();
              throw handled_error();
            }
            push(frame[(ip++)->val]);
            break;
          }

          case inst::fieldsave: {
            vars_t frame = pop<vars_t>();
            if (!frame) {
              UNALIAS;
              em->runtime(getPos());
              *em << "dereference of null pointer";
              em->sync();
              throw handled_error();
            }
            frame[(ip++)->val] = top();
            break;
          }
	
          case inst::mem_eq:
            v = pop<vars_t>();
            u = pop<vars_t>();
            push(u == v);
            break;

          case inst::mem_neq:
            v = pop<vars_t>();
            u = pop<vars_t>();
            push(u != v);
            break;

          case inst::func_eq: {
            callable *l = pop<callable*>();
            callable *r = pop<callable*>();
            push(l->compare(r));
            break;
          }

          case inst::func_neq: {
            callable *l = pop<callable*>();
            callable *r = pop<callable*>();
            push(!(l->compare(r)));
            break;
          }
		
          case inst::i_plus:
            b = pop<int>();
            a = pop<int>();
            push(a + b);
            break;

          case inst::i_minus:
            b = pop<int>();
            a = pop<int>();
            push(a - b);
            break;


          case inst::i_times:
            b = pop<int>();
            a = pop<int>();
            push(a * b);
            break;

          case inst::i_divide:
            b = pop<int>();
            a = pop<int>();
            if (b == 0) {
              UNALIAS;
              em->runtime(getPos());
              (*em) << "Divide by 0.";
              em->sync();
              throw handled_error();
            }
            push(a / b);
            break;

          case inst::i_negate:
            a = pop<int>();
            push(-a);
            break;

          case inst::log_not:
            m = pop<bool>();
            push(!m);
            break;
            
          case inst::log_eq:
            m = pop<bool>();
            n = pop<bool>();
            push(m==n);
            break;

          case inst::log_neq:
            m = pop<bool>();
            n = pop<bool>();
            push(m!=n);
            break;

          case inst::i_incr:
            a = pop<int>();
            push(++a);
            break;

          case inst::i_decr:
            a = pop<int>();
            push(--a);
            break;

          case inst::i_eq:
            b = pop<int>();
            a = pop<int>();
            push(a == b);
            break;

          case inst::i_neq:
            b = pop<int>();
            a = pop<int>();
            push(a != b);
            break;

          case inst::i_gt:
            b = pop<int>();
            a = pop<int>();
            push(a > b);
            break;

          case inst::i_ge:
            b = pop<int>();
            a = pop<int>();
            push(a >= b);
            break;

          case inst::i_lt:
            b = pop<int>();
            a = pop<int>();
            push(a < b);
            break;

          case inst::i_le:
            b = pop<int>();
            a = pop<int>();
            push(a <= b);
            break;

			
          case inst::f_plus:
            y = pop<double>();
            x = pop<double>();
            push(x + y);
            break;

          case inst::f_minus:
            y = pop<double>();
            x = pop<double>();
            push(x - y);
            break;

          case inst::f_times:
            y = pop<double>();
            x = pop<double>();
            push(x * y);
            break;

          case inst::f_divide:
            y = pop<double>();
            x = pop<double>();
            if (y == 0) {
              UNALIAS;
              em->runtime(getPos());
              (*em) << "Divide by 0.";
              em->sync();
              throw handled_error();
            }
            push(x / y);
            break;

          case inst::f_negate:
            x = pop<double>();
            push(-x);
            break;

          case inst::f_eq:
            y = pop<double>();
            x = pop<double>();
            push(x == y);
            break;

          case inst::f_neq:
            y = pop<double>();
            x = pop<double>();
            push(x != y);
            break;

          case inst::f_gt:
            y = pop<double>();
            x = pop<double>();
            push(x > y);
            break;

          case inst::f_ge:
            y = pop<double>();
            x = pop<double>();
            push(x >= y);
            break;

          case inst::f_lt:
            y = pop<double>();
            x = pop<double>();
            push(x < y);
            break;

          case inst::f_le:
            y = pop<double>();
            x = pop<double>();
            push(x <= y);
            break;

	
          case inst::builtin: {
            this->ip = ip;
            this->body = body; // For debugging information in case of errors.
            bltin func = (ip++)->bfunc;
            func(this);
            em->checkCamp(getPos());
            break;
          }

          case inst::jmp:
            ip = ip->label;
            break;

          case inst::cjmp:
            if (pop<bool>()) ip = ip->label;
            else ip++;
            break;

          case inst::njmp:
            if (!pop<bool>()) ip = ip->label;
            else ip++;
            break;

          case inst::popcall: {
            /* get the function reference off of the stack */
            callable* f = pop<callable*>();
            UNALIAS;
        
            f->call(this);
            
            UNALIAS;
            em->checkCamp(getPos());
            
            break;
          }

          case inst::pushclosure:
            push(vars);
            break; 

          case inst::makefunc: {
            func *f = new func;
            f->closure = pop<vars_t>();
            f->body = (ip++)->lfunc;

            push((callable*)f);
            break;
          }
        
          case inst::ret: {
            return;
          }
		      
          case inst::alloc: {
            // Get the record's enclosing frame off the stack.
            vars_t frame = pop<vars_t>();
	
            record *r = (ip++)->r;
            vars_t fields(make_frame(r->size));
            fields[0] = frame;

            push(fields);

            // Call the initializer.
            func f;
            f.body = r->init; f.closure = fields;

            run(&f);
            break;
          }
	
          default:
            UNALIAS;
            em->runtime(getPos());
            *em << "Internal VM error: Bad stack operand";
            em->sync();
            throw handled_error();
        }

#ifdef DEBUG_STACK
      UNALIAS;
      draw(cerr);
      cerr << "\n";
#endif
    }
  } catch (boost::bad_any_cast&) {
    em->runtime(getPos());
    *em << "Trying to use uninitialized value.";
    em->sync();
    throw handled_error();
  }
}

#undef UNALIAS

#ifdef DEBUG_STACK
#if __GNUC__
#include <cxxabi.h>
std::string demangle(const char *s)
{
  int status;
  char *demangled = abi::__cxa_demangle(s,NULL,NULL,&status);
  if (status == 0 && demangled) {
    std::string str(demangled);
    free(demangled);
    return str;
  } else if (status == -2) {
    free(demangled);
    return s;
  } else {
    free(demangled);
    return std::string("Unknown(") + s + ")";
  }
};
#else
std::string demangle(const char* s)
{
  return s;
}
#endif 

void stack::draw(ostream& out)
{
//  out.setf(out.hex);

  out << "operands:";
  stack_t::const_iterator left = theStack.begin();
  if (theStack.size() > 10) {
    left = theStack.end()-10;
    out << " ...";
  }
  
  while (left != theStack.end())
    {
      out << " " << demangle(left->type().name());
      left++;
    }
  out << "\n";

  out << "vars:    ";
  vars_t v = vars;
  
  if (!!v) {
    out << (!get<vars_t>(v[0]) ? " 0" : " link");
    for (int i = 1; i < 10 && i < body->vars; i++)
      out << " " << demangle(v[i].type().name());
    if (body->vars > 10)
      out << "...";
    out << "\n";
  }
  else
    out << "\n";

  out << "globals: ";
  vars_t g = globals;
  for (int i = 0; i < 10 && i < numGlobals; i++)
    out << " " << demangle(g[i].type().name());
  if (numGlobals > 10)
    out << " ...\n";
  else
    out << " \n";
}
#endif // DEBUG_STACK

position stack::getPos()
{
  return body ? body->pl.getPos(ip) : position::nullPos();
}

} // namespace vm
