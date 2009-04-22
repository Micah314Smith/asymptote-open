/*****
 * builtin.cc
 * Tom Prince 2004/08/25
 *
 * Initialize builtins.
 *****/

#include <cmath>

#include "builtin.h"
#include "entry.h"
#include "runtime.h"
#include "types.h"

#include "castop.h"
#include "mathop.h"
#include "arrayop.h"
#include "vm.h"

#include "coder.h"
#include "exp.h"
#include "refaccess.h"
#include "settings.h"

#ifdef HAVE_LIBGSL  
#include <gsl/gsl_sf.h>
#include <gsl/gsl_errno.h>
#endif
  
using namespace types;
using namespace camp;
using namespace vm;  

namespace trans {
using camp::transform;
using camp::pair;
using vm::bltin;
using run::divide;
using run::less;
using run::greater;
using run::plus;
using run::minus;


using namespace run;  
  
void gen_base_venv(venv &ve);

void addType(tenv &te, const char *name, ty *t)
{
  te.enter(symbol::trans(name), new tyEntry(t,0,0,position()));
}

// The base environments for built-in types and functions
void base_tenv(tenv &te)
{
#define PRIMITIVE(name,Name,asyName)  addType(te, #asyName, prim##Name());
#include <primitives.h>
#undef PRIMITIVE
}

const formal noformal(0);  

void addFunc(venv &ve, access *a, ty *result, symbol *id,
             formal f1=noformal, formal f2=noformal, formal f3=noformal,
             formal f4=noformal, formal f5=noformal, formal f6=noformal,
             formal f7=noformal, formal f8=noformal, formal f9=noformal,
             formal fA=noformal, formal fB=noformal, formal fC=noformal,
             formal fD=noformal, formal fE=noformal, formal fF=noformal)
{
  function *fun = new function(result);

  if (f1.t) fun->add(f1);
  if (f2.t) fun->add(f2);
  if (f3.t) fun->add(f3);
  if (f4.t) fun->add(f4);
  if (f5.t) fun->add(f5);
  if (f6.t) fun->add(f6);
  if (f7.t) fun->add(f7);
  if (f8.t) fun->add(f8);
  if (f9.t) fun->add(f9);
  if (fA.t) fun->add(fA);
  if (fB.t) fun->add(fB);
  if (fC.t) fun->add(fC);
  if (fD.t) fun->add(fD);
  if (fE.t) fun->add(fE);
  if (fF.t) fun->add(fF);

  // NOTE: If the function is a field, we should encode the defining record in
  // the entry
  varEntry *ent = new varEntry(fun, a, 0, position());
  
  ve.enter(id, ent);
}

// Add a function with one or more default arguments.
void addFunc(venv &ve, bltin f, ty *result, const char *name, 
             formal f1, formal f2, formal f3, formal f4, formal f5, formal f6,
             formal f7, formal f8, formal f9, formal fA, formal fB,
             formal fC, formal fD, formal fE, formal fF)
{
  access *a = new bltinAccess(f);
  addFunc(ve,a,result,symbol::trans(name),f1,f2,f3,f4,f5,f6,f7,f8,f9,
          fA,fB,fC,fD,fE,fF);
}
  
void addFunc(venv &ve, access *a, ty *result, const char *name, formal f1)
{
  addFunc(ve,a,result,symbol::trans(name),f1);
}

void addOpenFunc(venv &ve, bltin f, ty *result, const char *name)
{
  function *fun = new function(result, signature::OPEN);

  access *a= new bltinAccess(f);

  varEntry *ent = new varEntry(fun, a, 0, position());
  
  ve.enter(symbol::trans(name), ent);
}


// Add a rest function with zero or more default/explicit arguments.
void addRestFunc(venv &ve, bltin f, ty *result, const char *name, formal frest,
                 formal f1=noformal, formal f2=noformal, formal f3=noformal,
                 formal f4=noformal, formal f5=noformal, formal f6=noformal,
                 formal f7=noformal, formal f8=noformal, formal f9=noformal)
{
  access *a = new bltinAccess(f);
  function *fun = new function(result);

  if (f1.t) fun->add(f1);
  if (f2.t) fun->add(f2);
  if (f3.t) fun->add(f3);
  if (f4.t) fun->add(f4);
  if (f5.t) fun->add(f5);
  if (f6.t) fun->add(f6);
  if (f7.t) fun->add(f7);
  if (f8.t) fun->add(f8);
  if (f9.t) fun->add(f9);

  if (frest.t) fun->addRest(frest);

  varEntry *ent = new varEntry(fun, a, 0, position());

  ve.enter(symbol::trans(name), ent);
}

void addRealFunc0(venv &ve, bltin fcn, const char *name)
{
  addFunc(ve, fcn, primReal(), name);
}

template<double (*fcn)(double)>
void addRealFunc(venv &ve, const char* name)
{
  addFunc(ve, realReal<fcn>, primReal(), name, formal(primReal(),"x"));
  addFunc(ve, arrayFunc<double,double,fcn>, realArray(), name,
          formal(realArray(),"a"));
}

#define addRealFunc(fcn) addRealFunc<fcn>(ve, #fcn);
  
void addRealFunc2(venv &ve, bltin fcn, const char *name)
{
  addFunc(ve,fcn,primReal(),name,formal(primReal(),"a"),
          formal(primReal(),"b"));
}

#ifdef HAVE_LIBGSL  
bool GSLerror=false;
  
types::dummyRecord *GSLModule;

types::record *getGSLModule()
{
  return GSLModule;
}

inline void checkGSLerror()
{
  if(GSLerror) {
    GSLerror=false;
    throw handled_error();
  }
}
  
template <double (*func)(double)>
void realRealGSL(vm::stack *s) 
{
  double x=pop<double>(s);
  s->push(func(x));
  checkGSLerror();
}

template <double (*func)(double, gsl_mode_t)>
void realRealDOUBLE(vm::stack *s) 
{
  double x=pop<double>(s);
  s->push(func(x,GSL_PREC_DOUBLE));
  checkGSLerror();
}

template <double (*func)(double, double, gsl_mode_t)>
void realRealRealDOUBLE(vm::stack *s) 
{
  double y=pop<double>(s);
  double x=pop<double>(s);
  s->push(func(x,y,GSL_PREC_DOUBLE));
  checkGSLerror();
}

template <double (*func)(unsigned)>
void realIntGSL(vm::stack *s) 
{
  s->push(func(unsignedcast(pop<Int>(s))));
  checkGSLerror();
}

template <double (*func)(int, double)>
void realIntRealGSL(vm::stack *s) 
{
  double x=pop<double>(s);
  
  s->push(func(intcast(pop<Int>(s)),x));
  checkGSLerror();
}

template <double (*func)(double, double)>
void realRealRealGSL(vm::stack *s) 
{
  double x=pop<double>(s);
  double n=pop<double>(s);
  s->push(func(n,x));
  checkGSLerror();
}

template <double (*func)(double, unsigned)>
void realRealIntGSL(vm::stack *s) 
{
  Int n=pop<Int>(s);
  double x=pop<double>(s);
  s->push(func(x,unsignedcast(n)));
  checkGSLerror();
}

// Add a GSL special function from the GNU GSL library
template<double (*fcn)(double)>
void addGSLRealFunc(const char* name)
{
  addFunc(GSLModule->e.ve, realRealGSL<fcn>, primReal(), name,
          formal(primReal(),"x"));
}

// Add a GSL_PREC_DOUBLE GSL special function.
template<double (*fcn)(double, gsl_mode_t)>
void addGSLDOUBLEFunc(const char* name)
{
  addFunc(GSLModule->e.ve, realRealDOUBLE<fcn>, primReal(), name,
          formal(primReal(),"x"));
}

template<double (*fcn)(double, double, gsl_mode_t)>
void addGSLDOUBLE2Func(const char* name)
{
  addFunc(GSLModule->e.ve, realRealRealDOUBLE<fcn>, primReal(), name, 
          formal(primReal(),"phi"), formal(primReal(),"k"));
}

template<double (*fcn)(unsigned)>
void addGSLIntFunc(const char* name)
{
  addFunc(GSLModule->e.ve, realIntGSL<fcn>, primReal(), name,
          formal(primInt(),"s"));
}

template<double (*fcn)(int, double)>
void addGSLIntRealFunc(const char* name, const char *arg1="n")
{
  addFunc(GSLModule->e.ve, realIntRealGSL<fcn>, primReal(), name,
          formal(primInt(),arg1), formal(primReal(),"x"));
}

template<double (*fcn)(double, double)>
void addGSLRealRealFunc(const char* name)
{
  addFunc(GSLModule->e.ve, realRealRealGSL<fcn>, primReal(), name,
          formal(primReal(),"nu"), formal(primReal(),"x"));
}

template<double (*fcn)(double, unsigned)>
void addGSLRealIntFunc(const char* name)
{
  addFunc(GSLModule->e.ve, realRealIntGSL<fcn>, primReal(), name, 
          formal(primReal(),"nu"), formal(primInt(),"s"));
}

// Handle GSL errors gracefully.
void GSLerrorhandler(const char *reason, const char *, int, int) 
{
  if(!GSLerror) {
    vm::errornothrow(reason);
    GSLerror=true;
  }
}
#endif
  
void addInitializer(venv &ve, ty *t, access *a)
{
  addFunc(ve, a, t, symbol::initsym);
}

void addInitializer(venv &ve, ty *t, bltin f)
{
  access *a = new bltinAccess(f);
  addInitializer(ve, t, a);
}

// Specifies that source may be cast to target, but only if an explicit
// cast expression is used.
void addExplicitCast(venv &ve, ty *target, ty *source, access *a) {
  addFunc(ve, a, target, symbol::ecastsym, source);
}

// Specifies that source may be implicitly cast to target by the
// function or instruction stores at a.
void addCast(venv &ve, ty *target, ty *source, access *a) {
  //addExplicitCast(target,source,a);
  addFunc(ve, a, target, symbol::castsym, source);
}

void addExplicitCast(venv &ve, ty *target, ty *source, bltin f) {
  addExplicitCast(ve, target, source, new bltinAccess(f));
}

void addCast(venv &ve, ty *target, ty *source, bltin f) {
  addCast(ve, target, source, new bltinAccess(f));
}

template<class T>
void addVariable(venv &ve, T *ref, ty *t, const char *name,
                 record *module=settings::getSettingsModule()) {
  access *a = new refAccess<T>(ref);
  varEntry *ent = new varEntry(t, a, PUBLIC, module, 0, position());
  ve.enter(symbol::trans(name), ent);
}

template<class T>
void addVariable(venv &ve, T value, ty *t, const char *name,
                 record *module=settings::getSettingsModule(),
                 permission perm=PUBLIC) {
  item* ref=new item;
  *ref=value;
  access *a = new itemRefAccess(ref);
  varEntry *ent = new varEntry(t, a, perm, module, 0, position());
  ve.enter(symbol::trans(name), ent);
}

template<class T>
void addConstant(venv &ve, T value, ty *t, const char *name,
                 record *module=settings::getSettingsModule()) {
  addVariable(ve,value,t,name,module,RESTRICTED);
}

// The identity access, i.e. no instructions are encoded for a cast or
// operation, and no functions are called.
identAccess id;

function *IntRealFunction()
{
  return new function(primInt(),primReal());
}

function *realPairFunction()
{
  return new function(primReal(),primPair());
}

function *voidFileFunction()
{
  return new function(primVoid(),primFile());
}

void addInitializers(venv &ve)
{
  addInitializer(ve, primBoolean(), boolFalse);
  addInitializer(ve, primInt(), IntZero);
  addInitializer(ve, primReal(), realZero);

  addInitializer(ve, primString(), emptyString);
  addInitializer(ve, primPair(), pairZero);
  addInitializer(ve, primTriple(), tripleZero);
  addInitializer(ve, primTransform(), transformIdentity);
  addInitializer(ve, primGuide(), nullGuide);
  addInitializer(ve, primPath(), nullPath);
  addInitializer(ve, primPath3(), nullPath3);
  addInitializer(ve, primPen(), newPen);
  addInitializer(ve, primPicture(), newPicture);
  addInitializer(ve, primFile(), nullFile);
}

void addCasts(venv &ve)
{
  addExplicitCast(ve, primString(), primInt(), stringCast<Int>);
  addExplicitCast(ve, primString(), primReal(), stringCast<double>);
  addExplicitCast(ve, primString(), primPair(), stringCast<pair>);
  addExplicitCast(ve, primString(), primTriple(), stringCast<triple>);
  addExplicitCast(ve, primInt(), primString(), castString<Int>);
  addExplicitCast(ve, primReal(), primString(), castString<double>);
  addExplicitCast(ve, primPair(), primString(), castString<pair>);
  addExplicitCast(ve, primTriple(), primString(), castString<triple>);

  addExplicitCast(ve, primInt(), primReal(), castDoubleInt);

  addCast(ve, primReal(), primInt(), cast<Int,double>);
  addCast(ve, primPair(), primInt(), cast<Int,pair>);
  addCast(ve, primPair(), primReal(), cast<double,pair>);
  
  addCast(ve, primPath(), primPair(), cast<pair,path>);
  addCast(ve, primGuide(), primPair(), pairToGuide);
  addCast(ve, primGuide(), primPath(), pathToGuide);
  addCast(ve, primPath(), primGuide(), guideToPath);

  addCast(ve, primFile(), primNull(), nullFile);
  
  // Vectorized casts.
  addExplicitCast(ve, IntArray(), realArray(), arrayToArray<double,Int>);
  
  addCast(ve, realArray(), IntArray(), arrayToArray<Int,double>);
  addCast(ve, pairArray(), IntArray(), arrayToArray<Int,pair>);
  addCast(ve, pairArray(), realArray(), arrayToArray<double,pair>);
}

void addGuideOperators(venv &ve)
{
  // The guide operators .. and -- take an array of guides, and turn them
  // into a single guide.
  addRestFunc(ve, dotsGuide, primGuide(), "..", guideArray());
  addRestFunc(ve, dashesGuide, primGuide(), "--", guideArray());
}

/* Avoid typing the same type three times. */
void addSimpleOperator(venv &ve, bltin f, ty *t, const char *name)
{
  addFunc(ve,f,t,name,formal(t,"a"),formal(t,"b"));
}
void addBooleanOperator(venv &ve, bltin f, ty *t, const char *name)
{
  addFunc(ve,f,primBoolean(),name,formal(t,"a"),formal(t,"b"));
}

template<class T, template <class S> class op>
void addOps(venv &ve, ty *t1, const char *name, ty *t2)
{
  addSimpleOperator(ve,binaryOp<T,op>,t1,name);
  addFunc(ve,opArray<T,T,op>,t2,name,formal(t1,"a"),formal(t2,"b"));
  addFunc(ve,arrayOp<T,T,op>,t2,name,formal(t2,"a"),formal(t1,"b"));
  addSimpleOperator(ve,arrayArrayOp<T,op>,t2,name);
}

template<class T, template <class S> class op>
void addBooleanOps(venv &ve, ty *t1, const char *name, ty *t2)
{
  addBooleanOperator(ve,binaryOp<T,op>,t1,name);
  addFunc(ve,opArray<T,T,op>,boolArray(),name,formal(t1,"a"),formal(t2,"b"));
  addFunc(ve,arrayOp<T,T,op>,boolArray(),name,formal(t2,"a"),formal(t1,"b"));
  addFunc(ve,arrayArrayOp<T,op>,boolArray(),name,formal(t2,"a"),
          formal(t2,"b"));
}

void addWrite(venv &ve, bltin f, ty *t1, ty *t2)
{
  addRestFunc(ve,f,primVoid(),"write",t2,
              formal(primFile(),"file",true),formal(primString(),"s",true),
              formal(t1,"x"),formal(voidFileFunction(),"suffix",true));
}

template<class T>
void addUnorderedOps(venv &ve, ty *t1, ty *t2, ty *t3, ty *t4)
{
  addBooleanOps<T,equals>(ve,t1,"==",t2);
  addBooleanOps<T,notequals>(ve,t1,"!=",t2);
  
  addCast(ve,t1,primFile(),read<T>);
  addCast(ve,t2,primFile(),readArray<T>);
  addCast(ve,t3,primFile(),readArray<T>);
  addCast(ve,t4,primFile(),readArray<T>);
  
  addWrite(ve,write<T>,t1,t2);
  addRestFunc(ve,writeArray<T>,primVoid(),"write",t3,
              formal(primFile(),"file",true),formal(primString(),"s",true),
              formal(t2,"a",false,true));
  addFunc(ve,writeArray2<T>,primVoid(),"write",
          formal(primFile(),"file",true),t3);
  addFunc(ve,writeArray3<T>,primVoid(),"write",
          formal(primFile(),"file",true),t4);
}

inline double abs(pair z) {
  return z.length();
}

inline double abs(triple v) {
  return v.length();
}

template<class T, template <class S> class op>
void addBinOps(venv &ve, ty *t1, ty *t2, ty *t3, ty *t4, const char *name)
{
  addFunc(ve,binopArray<T,op>,t1,name,formal(t2,"a"));
  addFunc(ve,binopArray2<T,op>,t1,name,formal(t3,"a"));
  addFunc(ve,binopArray3<T,op>,t1,name,formal(t4,"a"));
}

template<class T>
void addOrderedOps(venv &ve, ty *t1, ty *t2, ty *t3, ty *t4)
{
  addBooleanOps<T,less>(ve,t1,"<",t2);
  addBooleanOps<T,lessequals>(ve,t1,"<=",t2);
  addBooleanOps<T,greaterequals>(ve,t1,">=",t2);
  addBooleanOps<T,greater>(ve,t1,">",t2);
  
  addOps<T,run::min>(ve,t1,"min",t2);
  addOps<T,run::max>(ve,t1,"max",t2);
  addBinOps<T,run::min>(ve,t1,t2,t3,t4,"min");
  addBinOps<T,run::max>(ve,t1,t2,t3,t4,"max");
    
  addFunc(ve,sortArray<T>,t2,"sort",formal(t2,"a"));
  addFunc(ve,sortArray2<T>,t3,"sort",formal(t3,"a"));
  
  addFunc(ve,searchArray<T>,primInt(),"search",formal(t2,"a"),
          formal(t1,"key"));
}

template<class T>
void addBasicOps(venv &ve, ty *t1, ty *t2, ty *t3, ty *t4, bool integer=false,
                 bool Explicit=false)
{
  addOps<T,plus>(ve,t1,"+",t2);
  addOps<T,minus>(ve,t1,"-",t2);
  
  addFunc(ve,&id,t1,"+",formal(t1,"a"));
  addFunc(ve,&id,t2,"+",formal(t2,"a"));
  addFunc(ve,Negate<T>,t1,"-",formal(t1,"a"));
  addFunc(ve,arrayNegate<T>,t2,"-",formal(t2,"a"));
  if(!integer) addFunc(ve,interp<T>,t1,"interp",formal(t1,"a",false,Explicit),
                       formal(t1,"b",false,Explicit),
                       formal(primReal(),"t"));
  
  addFunc(ve,sumArray<T>,t1,"sum",formal(t2,"a"));
  addUnorderedOps<T>(ve,t1,t2,t3,t4);
}

template<class T>
void addOps(venv &ve, ty *t1, ty *t2, ty *t3, ty *t4, bool integer=false,
            bool Explicit=false)
{
  addBasicOps<T>(ve,t1,t2,t3,t4,integer,Explicit);
  addOps<T,times>(ve,t1,"*",t2);
  if(!integer) addOps<T,run::divide>(ve,t1,"/",t2);
  addOps<T,power>(ve,t1,"^",t2);
}


// Adds standard functions for a newly added array type.
void addArrayOps(venv &ve, types::array *t)
{
  ty *ct = t->celltype;
  
  addFunc(ve, run::arrayAlias,
          primBoolean(), "alias", formal(t, "a"), formal(t, "b"));

  addFunc(ve, run::newDuplicateArray,
          t, "array", formal(primInt(), "n"),
          formal(ct, "value"),
          formal(primInt(), "depth", /*optional=*/ true));

  switch (t->depth()) {
    case 1:
      addFunc(ve, run::arrayCopy, t, "copy", formal(t, "a"));
      addRestFunc(ve, run::arrayConcat, t, "concat", new types::array(t));
      addFunc(ve, run::arraySequence,
              t, "sequence", formal(new function(ct, primInt()), "f"),
              formal(primInt(), "n"));
      addFunc(ve, run::arrayFunction,
              t, "map", formal(new function(ct, ct), "f"), formal(t, "a"));
      addFunc(ve, run::arraySort,
              t, "sort", formal(t, "a"),
              formal(new function(primBoolean(), ct, ct), "f"));
      break;
    case 2:
      addFunc(ve, run::array2Copy, t, "copy", formal(t, "a"));
      addFunc(ve, run::array2Transpose, t, "transpose", formal(t, "a"));
      break;
    case 3:
      addFunc(ve, run::array3Copy, t, "copy", formal(t, "a"));
      addFunc(ve, run::array3Transpose, t, "transpose", formal(t, "a"),
              formal(IntArray(),"perm"));
      break;
    default:
      break;
  }
}

void addRecordOps(venv &ve, record *r)
{
  addFunc(ve, run::boolMemEq, primBoolean(), "alias", types::formal(r, "a"),
          types::formal(r, "b"));
  addFunc(ve, run::boolMemEq, primBoolean(), "==", types::formal(r, "a"),
          types::formal(r, "b"));
  addFunc(ve, run::boolMemNeq, primBoolean(), "!=", types::formal(r, "a"),
          types::formal(r, "b"));
}

void addFunctionOps(venv &ve, function *f)
{
  addFunc(ve, run::boolFuncEq, primBoolean(), "==", types::formal(f, "a"),
          types::formal(f, "b"));
  addFunc(ve, run::boolFuncNeq, primBoolean(), "!=", types::formal(f, "a"),
          types::formal(f, "b"));
}

void addOperators(venv &ve) 
{
  addSimpleOperator(ve,binaryOp<string,plus>,primString(),"+");
  
  addBooleanOps<bool,And>(ve,primBoolean(),"&",boolArray());
  addBooleanOps<bool,Or>(ve,primBoolean(),"|",boolArray());
  addBooleanOps<bool,Xor>(ve,primBoolean(),"^",boolArray());
  
  addUnorderedOps<bool>(ve,primBoolean(),boolArray(),boolArray2(),
                        boolArray3());
  addOps<Int>(ve,primInt(),IntArray(),IntArray2(),IntArray3(),true);
  addOps<double>(ve,primReal(),realArray(),realArray2(),realArray3());
  addOps<pair>(ve,primPair(),pairArray(),pairArray2(),pairArray3(),false,true);
  addBasicOps<triple>(ve,primTriple(),tripleArray(),tripleArray2(),
                      tripleArray3());
  addFunc(ve,opArray<double,triple,times>,tripleArray(),"*",
          formal(primReal(),"a"),formal(tripleArray(),"b"));
  addFunc(ve,arrayOp<triple,double,timesR>,tripleArray(),"*",
          formal(tripleArray(),"a"),formal(primReal(),"b"));
  addFunc(ve,arrayOp<triple,double,divide>,tripleArray(),"/",
          formal(tripleArray(),"a"),formal(primReal(),"b"));

  addUnorderedOps<string>(ve,primString(),stringArray(),stringArray2(),
                          stringArray3());
  
  addSimpleOperator(ve,binaryOp<pair,minbound>,primPair(),"minbound");
  addSimpleOperator(ve,binaryOp<pair,maxbound>,primPair(),"maxbound");
  addSimpleOperator(ve,binaryOp<triple,minbound>,primTriple(),"minbound");
  addSimpleOperator(ve,binaryOp<triple,maxbound>,primTriple(),"maxbound");
  addBinOps<pair,minbound>(ve,primPair(),pairArray(),pairArray2(),pairArray3(),
                           "minbound");
  addBinOps<pair,maxbound>(ve,primPair(),pairArray(),pairArray2(),pairArray3(),
                           "maxbound");
  addBinOps<triple,minbound>(ve,primTriple(),tripleArray(),tripleArray2(),
                             tripleArray3(),"minbound");
  addBinOps<triple,maxbound>(ve,primTriple(),tripleArray(),tripleArray2(),
                             tripleArray3(),"maxbound");
  
  addFunc(ve,arrayFunc<double,pair,abs>,realArray(),"abs",
          formal(pairArray(),"a"));
  addFunc(ve,arrayFunc<double,triple,abs>,realArray(),"abs",
          formal(tripleArray(),"a"));
  
  addFunc(ve,binaryOp<Int,divide>,primReal(),"/",
          formal(primInt(),"a"),formal(primInt(),"b"));
  addFunc(ve,arrayOp<Int,Int,divide>,realArray(),"/",
          formal(IntArray(),"a"),formal(primInt(),"b"));
  addFunc(ve,opArray<Int,Int,divide>,realArray(),"/",
          formal(primInt(),"a"),formal(IntArray(),"b"));
  addFunc(ve,arrayArrayOp<Int,divide>,realArray(),"/",
          formal(IntArray(),"a"),formal(IntArray(),"b"));
  
  addOrderedOps<Int>(ve,primInt(),IntArray(),IntArray2(),IntArray3());
  addOrderedOps<double>(ve,primReal(),realArray(),realArray2(),realArray3());
  addOrderedOps<string>(ve,primString(),stringArray(),stringArray2(),
                        stringArray3());
  
  addOps<Int,mod>(ve,primInt(),"%",IntArray());
  addOps<double,mod>(ve,primReal(),"%",realArray());
  
  addRestFunc(ve,run::diagonal,realArray2(),"diagonal",realArray());
}

dummyRecord *createDummyRecord(venv &ve, const char *name)
{
  dummyRecord *r=new dummyRecord(name);
  addConstant(ve, new vm::frame(0), r, name);
  addRecordOps(ve, r);
  return r;
}

double identity(double x) {return x;}
double pow10(double x) {return run::pow(10.0,x);}

// An example of an open function.
#ifdef OPENFUNCEXAMPLE
void openFunc(stack *Stack)
{
  vm::array *a=vm::pop<vm::array *>(Stack);
  size_t numArgs=checkArray(a);
  for (size_t k=0; k<numArgs; ++k)
    cout << k << ": " << (*a)[k];
  
  Stack->push<Int>((Int)numArgs);
}
#endif

// NOTE: We should move all of these into a "builtin" module.
void base_venv(venv &ve)
{
  addInitializers(ve);
  addCasts(ve);
  addOperators(ve);
  addGuideOperators(ve);
  
  addRealFunc(sin);
  addRealFunc(cos);
  addRealFunc(tan);
  addRealFunc(asin);
  addRealFunc(acos);
  addRealFunc(atan);
  addRealFunc(exp);
  addRealFunc(log);
  addRealFunc(log10);
  addRealFunc(sinh);
  addRealFunc(cosh);
  addRealFunc(tanh);
  addRealFunc(asinh);
  addRealFunc(acosh);
  addRealFunc(atanh);
  addRealFunc(sqrt);
  addRealFunc(cbrt);
  addRealFunc(fabs);
  addRealFunc<fabs>(ve,"abs");
  addRealFunc(expm1);

  addRealFunc(pow10);
  addRealFunc(identity);
  
#ifdef HAVE_LIBGSL  
  GSLModule=new types::dummyRecord(symbol::trans("gsl"));
  gsl_set_error_handler(GSLerrorhandler);
  
  addGSLDOUBLEFunc<gsl_sf_airy_Ai>("Ai");
  addGSLDOUBLEFunc<gsl_sf_airy_Bi>("Bi");
  addGSLDOUBLEFunc<gsl_sf_airy_Ai_deriv>("Ai_deriv");
  addGSLDOUBLEFunc<gsl_sf_airy_Bi_deriv>("Bi_deriv");
  addGSLIntFunc<gsl_sf_airy_zero_Ai>("zero_Ai");
  addGSLIntFunc<gsl_sf_airy_zero_Bi>("zero_Bi");
  addGSLIntFunc<gsl_sf_airy_zero_Ai_deriv>("zero_Ai_deriv");
  addGSLIntFunc<gsl_sf_airy_zero_Bi_deriv>("zero_Bi_deriv");
  
  addGSLIntRealFunc<gsl_sf_bessel_In>("I");
  addGSLIntRealFunc<gsl_sf_bessel_Kn>("K");
  addGSLIntRealFunc<gsl_sf_bessel_jl>("j","l");
  addGSLIntRealFunc<gsl_sf_bessel_yl>("y","l");
  addGSLIntRealFunc<gsl_sf_bessel_il_scaled>("i_scaled","l");
  addGSLIntRealFunc<gsl_sf_bessel_kl_scaled>("k_scaled","l");
  addGSLRealRealFunc<gsl_sf_bessel_Jnu>("J");
  addGSLRealRealFunc<gsl_sf_bessel_Ynu>("Y");
  addGSLRealRealFunc<gsl_sf_bessel_Inu>("I");
  addGSLRealRealFunc<gsl_sf_bessel_Knu>("K");
  addGSLRealIntFunc<gsl_sf_bessel_zero_Jnu>("zero_J");
  
  addGSLDOUBLE2Func<gsl_sf_ellint_E>("F");
  addGSLDOUBLE2Func<gsl_sf_ellint_E>("E");
  addGSLDOUBLE2Func<gsl_sf_ellint_E>("P");
  
  addGSLRealFunc<gsl_sf_expint_Ei>("Ei");
  addGSLRealFunc<gsl_sf_Si>("Si");
  addGSLRealFunc<gsl_sf_Ci>("Ci");
  
  addGSLIntRealFunc<gsl_sf_legendre_Pl>("Pl","l");
  
  addGSLRealFunc<gsl_sf_zeta>("zeta");
#endif
  
#ifdef STRUCTEXAMPLE
  dummyRecord *fun=createDummyRecord(ve, "test");
  addFunc(fun->e.ve,realReal<sin>,primReal(),"sin",formal(primReal(),"x"));
  addVariable<Int>(fun->e.ve,1,primInt(),"x");
#endif
  
  addFunc(ve,writestring,primVoid(),"write",
          formal(primFile(),"file",true),
          formal(primString(),"s"),
          formal(voidFileFunction(),"suffix",true));
  
  addWrite(ve,write<transform>,primTransform(),transformArray());
  addWrite(ve,write<guide *>,primGuide(),guideArray());
  addWrite(ve,write<pen>,primPen(),penArray());
  addFunc(ve,arrayArrayOp<pen,equals>,boolArray(),"==",
          formal(penArray(),"a"),formal(penArray(),"b"));
  addFunc(ve,arrayArrayOp<pen,notequals>,boolArray(),"!=",
          formal(penArray(),"a"),formal(penArray(),"b"));

  addFunc(ve,arrayFunction,realArray(),"map",
          formal(realPairFunction(),"f"),
          formal(pairArray(),"a"));
  addFunc(ve,arrayFunction,IntArray(),"map",
          formal(IntRealFunction(),"f"),
          formal(realArray(),"a"));
  
#ifdef HAVE_LIBFFTW3
  addFunc(ve,pairArrayFFT,pairArray(),"fft",formal(pairArray(),"a"),
          formal(primInt(),"sign",true));
#endif

  addConstant<Int>(ve, Int_MAX, primInt(), "intMax");
  addConstant<Int>(ve, Int_MIN, primInt(), "intMin");
  addConstant<double>(ve, HUGE_VAL, primReal(), "inf");
  addConstant<double>(ve, run::infinity, primReal(), "infinity");
  addConstant<double>(ve, DBL_MAX, primReal(), "realMax");
  addConstant<double>(ve, DBL_MIN, primReal(), "realMin");
  addConstant<double>(ve, DBL_EPSILON, primReal(), "realEpsilon");
  addConstant<Int>(ve, DBL_DIG, primInt(), "realDigits");
  addConstant<Int>(ve, RAND_MAX, primInt(), "randMax");
  addConstant<double>(ve, PI, primReal(), "pi");
  addConstant<string>(ve, string(settings::VERSION)+string(SVN_REVISION),
                      primString(),"VERSION");
  
  addVariable<pen>(ve, &processData().currentpen, primPen(), "currentpen");

#ifdef OPENFUNCEXAMPLE
  addOpenFunc(ve, openFunc, primInt(), "openFunc");
#endif

  gen_base_venv(ve);
}

void base_menv(menv&)
{
}

} //namespace trans

namespace run {

double infinity=cbrt(DBL_MAX); // Reduced for tension atleast infinity

void arrayDeleteHelper(vm::stack *Stack)
{
  array *a=pop<array *>(Stack);
  item itj=pop(Stack);
  item iti=pop(Stack);
  if(isdefault(iti)) {
    (*a).clear();
    return;
  }
  Int i=get<Int>(iti);
  Int j=isdefault(itj) ? i : get<Int>(itj);

  size_t asize=checkArray(a);
  if(a->cyclic() && asize > 0) {
    if(j-i+1 >= (Int) asize) {
      (*a).clear();
      return;
    }
    i=imod(i,asize);
    j=imod(j,asize);
    if(j >= i) 
      (*a).erase((*a).begin()+i,(*a).begin()+j+1);
    else {
      (*a).erase((*a).begin()+i,(*a).end());
      (*a).erase((*a).begin(),(*a).begin()+j+1);
    }
    return;
  }
  
  if(i < 0 || i >= (Int) asize || i > j || j >= (Int) asize) {
    ostringstream buf;
    buf << "delete called on array of length " << (Int) asize 
        << " with out-of-bounds index range [" << i << "," << j << "]";
    error(buf);
  }

  (*a).erase((*a).begin()+i,(*a).begin()+j+1);
}

}
