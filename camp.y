%{
/*****
 * camp.y
 * Andy Hammerlindl 08/12/2002
 *
 * The grammar of the camp language.
 *****/

#include "errormsg.h"
#include "exp.h"
#include "newexp.h"
#include "stm.h"


// Used when a position needs to be determined and no token is
// available.  Defined in camp.l.
position lexerPos();

int yylex(void); /* function prototype */

void yyerror(const char *s)
{
  em->error(lexerPos());
  *em << s;
  em->sync();
}

namespace as { file *root; }

using namespace as;
using sym::symbol;
%}

%union {
  position pos;
  //sym::symbol *sym;
  struct {
    position pos;
    sym::symbol *sym;
  } ps;
  as::name *n;
  as::varinit *vi;
  as::arrayinit *ai;
  as::exp *e;
  as::join *j;
  as::dir *dr;
  as::explist *elist;
  as::dimensions *dim;
  as::ty  *t;
  as::decid *di;
  as::decidlist *dil;
  as::decidstart *dis;
  as::runnable *run;
  as::modifierList *ml;
  struct {
    position pos;
    int keyword;
  } mod;
  //as::program *prog;
  as::vardec *vd;
  //as::vardecs *vds;
  as::dec *d;
  as::stm *s;
  as::blockStm *bs;
  as::stmExpList *sel;
  //as::funheader *fh;
  as::formal *fl;
  as::formals *fls;
  as::file *fil;
}  

%token <ps> PRIM ID OP ADD SUBTRACT TIMES DIVIDE MOD EXPONENT
            COR CAND EQ NEQ LT LE GT GE
            '+' '-' '*' '/' '%' '^' LOGNOT POW
            STRING
%token <pos> LOOSE ASSIGN '?' ':'
             DIRTAG JOIN DOTS DASHES INCR CONTROLS TENSION ATLEAST AND CURL
             '{' '}' '(' ')' '.' ','  '[' ']' ';'
             IMPORT STRUCT TYPEDEF NEW
             IF ELSE WHILE DO FOR BREAK CONTINUE RETURN_ CYCLE
             STATIC DYNAMIC PUBLIC_TOK PRIVATE_TOK THIS
%token <e>   LIT

%left  LOOSE
%right ASSIGN ADD SUBTRACT TIMES DIVIDE MOD EXPONENT
%right '?' ':'
%left  COR
%left  CAND
%left  EQ NEQ
%left  LT LE GT GE

%left  DIRTAG
%left  JOIN DOTS DASHES
%left  CONTROLS TENSION ATLEAST AND
%left  CURL '{' '}'

%left  '+' '-' 
%left  '*' '/' '%'
%left  '^'
%left  '(' ')'
%left  UNARY LOGNOT INCR
%left  LIT
%right POW

%type  <fil> file fileblock
%type  <bs>  block bareblock
%type  <n>   name
%type  <run> runnable
%type  <ml>  modifiers
%type  <mod> modifier
%type  <d>   dec fundec typedec
%type  <vd>  vardec barevardec 
%type  <t>   type celltype
%type  <dim> dims
%type  <dil> decidlist
%type  <di>  decid
%type  <dis> decidstart
%type  <vi>  varinit
%type  <ai>  arrayinit varinits
%type  <fl>  formal
%type  <fls> formals
%type  <e>   value exp
%type  <j> join basicjoin tension controls
%type  <dr> dir
%type  <elist> explist dimexps
%type  <s>   stm stmexp
%type  <run> forinit
%type  <sel> forupdate stmexplist

/* There are two shift/reduce conflicts:
 *   the dangling ELSE in IF (exp) IF (exp) stm ELSE stm
 *   new ID
 */
%expect 2

/* Enable grammar debugging. */
/*%debug*/

%%

file:
  fileblock        { as::root = $1; }
;

fileblock:
  /* empty */      { $$ = new file(lexerPos()); }
| fileblock runnable
                   { $$ = $1; $$->add($2); }
;

bareblock:
  /* empty */      { $$ = new blockStm(lexerPos()); }
| bareblock runnable
                   { $$ = $1; $$->add($2); }
;

/*imports:
  IMPORT name ';'
| imports IMPORT name ';'
;*/

name:
  ID               { $$ = new simpleName($1.pos, $1.sym); }
| name '.' ID      { $$ = new qualifiedName($2, $1, $3.sym); }
;

/*runnables:
  runnable
| runnables runnable
;*/

runnable:
  dec              { $$ = $1; }
| stm              { $$ = $1; }
| modifiers dec
                   { $$ = new modifiedRunnable($1->getPos(), $1, $2); }
| modifiers block
                   { $$ = new modifiedRunnable($1->getPos(), $1, $2); }
;

modifiers:
  modifier         { $$ = new modifierList($1.pos); $$->add($1.keyword); }
| modifiers modifier
                   { $$ = $1; $$->add($2.keyword); }
;

modifier:
  STATIC           { $$.pos = $1; $$.keyword = STATIC; }
| DYNAMIC          { $$.pos = $1; $$.keyword = DYNAMIC; }
| PUBLIC_TOK       { $$.pos = $1; $$.keyword = PUBLIC_TOK; }
| PRIVATE_TOK      { $$.pos = $1; $$.keyword = PRIVATE_TOK; }
;

dec:
  vardec           { $$ = $1; }
| fundec           { $$ = $1; }
| typedec          { $$ = $1; }
| IMPORT ID ';'    { $$ = new importdec($1, $2.sym); }
| IMPORT STRING ';' { $$ = new importdec($1, $2.sym); }
;

vardec:
  barevardec ';'   { $$ = $1; }
;

barevardec:
  type decidlist   { $$ = new vardec($1->getPos(), $1, $2); }
;

type:
  celltype         { $$ = $1; }
| PRIM dims        { $$ = new arrayTy($1.pos, 
                            new nameTy($1.pos,
                              new simpleName($1.pos, $1.sym)),
                            $2); }
| name dims        { $$ = new arrayTy($1->getPos(),
                                      new nameTy($1->getPos(), $1), $2); }
;

celltype:
  name             { $$ = new nameTy($1->getPos(), $1); }
| PRIM             { $$ = new nameTy($1.pos,
                                     new simpleName($1.pos, $1.sym)); }
;

dims:
 '[' ']'           { $$ = new dimensions($1); }
| dims '[' ']'     { $$ = $1; $$->increase(); }
;

dimexps:
  '[' exp ']'      { $$ = new explist($1); $$->add($2); }
| dimexps '[' exp ']'
                   { $$ = $1; $$->add($3); }
;

decidlist:
  decid            { $$ = new decidlist($1->getPos()); $$->add($1); }
| decidlist ',' decid
                   { $$ = $1; $$->add($3); }
;

decid:
  decidstart       { $$ = new decid($1->getPos(), $1); }
| decidstart ASSIGN varinit
                   { $$ = new decid($1->getPos(), $1, $3); }
;

decidstart:
  ID               { $$ = new decidstart($1.pos, $1.sym); }
| ID dims          { $$ = new decidstart($1.pos, $1.sym, $2); }
| ID '(' ')'       { $$ = new fundecidstart($1.pos, $1.sym, 0,
                                            new formals($2)); }
| ID '(' formals ')'
                   { $$ = new fundecidstart($1.pos, $1.sym, 0, $3); }
;

varinit:
  exp              { $$ = $1; }
| arrayinit        { $$ = $1; }
;

block:
  '{' bareblock '}'
                   { $$ = $2; }
;

arrayinit:
  '{' '}'          { $$ = new arrayinit($1); }
| '{' ',' '}'      { $$ = new arrayinit($1); }
| '{' varinits '}' { $$ = $2; }
| '{' varinits ',' '}'
                   { $$ = $2; }
;

varinits:
  varinit
                   { $$ = new arrayinit($1->getPos());
		     $$->add($1);}
| varinits ',' varinit
                   { $$ = $1; $$->add($3); }
;

formals:
  formal           { $$ = new formals($1->getPos()); $$->add($1); }
| formals ',' formal
                   { $$ = $1; $$->add($3); }
;

formal:
  type             { $$ = new formal($1->getPos(), $1); }
| type decidstart  { $$ = new formal($2->getPos(), $1, $2); }
| type decidstart ASSIGN varinit
                   { $$ = new formal($2->getPos(), $1, $2, $4); }
;

fundec:
  type ID '(' ')' block
                   { $$ = new fundec($3, $1, $2.sym, new formals($3), $5); }
| type ID '(' formals ')' block
                   { $$ = new fundec($3, $1, $2.sym, $4, $6); }
| type OP '(' formals ')' block
                   { $$ = new fundec($3, $1, $2.sym, $4, $6); }
;

typedec:
  STRUCT ID block  { $$ = new recorddec($1, $2.sym, $3); }
| TYPEDEF vardec   { $$ = new typedec($1, $2); }
;


value:
  value '.' ID     { $$ = new fieldExp($2, $1, $3.sym); } 
| name '[' exp ']' { $$ = new subscriptExp($2,
                              new nameExp($1->getPos(), $1), $3); }
| value '[' exp ']'{ $$ = new subscriptExp($2, $1, $3); }
| name '(' ')'     { $$ = new callExp($2,
                                      new nameExp($1->getPos(), $1),
                                      new explist($2)); } 
| name '(' explist ')'
                   { $$ = new callExp($2, 
                                      new nameExp($1->getPos(), $1),
                                      $3); }
| value '(' ')'    { $$ = new callExp($2, $1, new explist($2)); }
| value '(' explist ')'
                   { $$ = new callExp($2, $1, $3); }
//| '(' name ')'     { $$ = new nameExp($2->getPos(), $2); }
| '(' exp ')' %prec LOOSE
                   { $$ = $2; }
| THIS             { $$ = new thisExp($1); }
;

explist:
  exp              { $$ = new explist($1->getPos()); $$->add($1); }
| explist ',' exp  { $$ = $1; $$->add($3); }
;

exp:
  name             { $$ = new nameExp($1->getPos(), $1); }
| value            { $$ = $1; }
| LIT              { $$ = $1; }
| STRING           { $$ = new stringExp($1.pos, *$1.sym);; }
/* This is for scaling expressions such as 105cm */
| LIT exp          { $$ = new scaleExp($1->getPos(), $1, $2); }
| '(' PRIM ')' exp { $$ = new castExp($2.pos,
                                      new simpleName($2.pos, $2.sym),
                                      $4); }
// NOTE: This casting is useful - try to bring it back in.
//| '(' name ')' exp
//                   { $$ = new castExp($2->getPos(), $2, $4); }
| '+' exp %prec UNARY
                   { $$ = new unaryExp($1.pos, $2, $1.sym); }
| '-' exp %prec UNARY
                   { $$ = new unaryExp($1.pos, $2, $1.sym); }
| LOGNOT exp       { $$ = new unaryExp($1.pos, $2, $1.sym); }
| exp '+' exp      { $$ = new binaryExp($2.pos, $1, $2.sym, $3); }
| exp '-' exp      { $$ = new binaryExp($2.pos, $1, $2.sym, $3); }
| exp '*' exp      { $$ = new binaryExp($2.pos, $1, $2.sym, $3); }
| exp '/' exp      { $$ = new binaryExp($2.pos, $1, $2.sym, $3); }
| exp '%' exp      { $$ = new binaryExp($2.pos, $1, $2.sym, $3); }
| exp '^' exp      { $$ = new binaryExp($2.pos, $1, $2.sym, $3); }
| exp LT exp       { $$ = new binaryExp($2.pos, $1, $2.sym, $3); }
| exp LE exp       { $$ = new binaryExp($2.pos, $1, $2.sym, $3); }
| exp GT exp       { $$ = new binaryExp($2.pos, $1, $2.sym, $3); }
| exp GE exp       { $$ = new binaryExp($2.pos, $1, $2.sym, $3); }
| exp EQ exp       { $$ = new binaryExp($2.pos, $1, $2.sym, $3); }
| exp NEQ exp      { $$ = new binaryExp($2.pos, $1, $2.sym, $3); }
| exp CAND exp     { $$ = new binaryExp($2.pos, $1, $2.sym, $3); }
| exp COR exp      { $$ = new binaryExp($2.pos, $1, $2.sym, $3); }
| NEW celltype
                   { $$ = new newRecordExp($1, $2); }
//| NEW celltype dims
//                   { $$ = new newRecordExp($1,
//                                           new arrayTy($2->getPos(), $2, $3)); }
//| NEW name block
| NEW celltype dimexps
                   { $$ = new newArrayExp($1, $2, $3, 0, 0); }
| NEW celltype dimexps dims
                   { $$ = new newArrayExp($1, $2, $3, $4, 0); }
| NEW celltype dims
                   { $$ = new newArrayExp($1, $2, 0, $3, 0); }
| NEW celltype dims arrayinit
                   { $$ = new newArrayExp($1, $2, 0, $3, $4); }
| NEW celltype '(' ')' block
                   { $$ = new newFunctionExp($1, $2, new formals($3), $5); }
| NEW celltype dims '(' ')' block
                   { $$ = new newFunctionExp($1,
                                             new arrayTy($2->getPos(), $2, $3),
                                             new formals($4),
                                             $6); }
| NEW celltype '(' formals ')' block
                   { $$ = new newFunctionExp($1, $2, $4, $6); }
| NEW celltype dims '(' formals ')' block
                   { $$ = new newFunctionExp($1,
                                             new arrayTy($2->getPos(), $2, $3),
                                             $5,
                                             $7); }
| exp '?' exp ':' exp
                   { $$ = new conditionalExp($2, $1, $3, $5); }
| exp ASSIGN exp   { $$ = new assignExp($2, $1, $3); }
// Camp stuff
| '(' exp ',' exp ')'
                   { $$ = new pairExp($1, $2, $4); }
| exp join exp %prec JOIN 
                   { $$ = new joinExp($1->getPos(), $1, $2, $3); }
| exp join CYCLE %prec JOIN
                   { $$ = new joinExp($1->getPos(),
                                      $1, $2, new cycleExp($3)); }
| exp dir %prec DIRTAG
                   { $$ = new dirguideExp($2->getPos(), $1, $2); }
| INCR exp %prec UNARY
                   { $$ = new prefixExp($1, $2, symbol::trans("+")); }
| DASHES exp %prec UNARY
                   { $$ = new prefixExp($1, $2, symbol::trans("-")); }
/* Illegal - will be caught during translation. */
| exp INCR %prec UNARY 
                   { $$ = new postfixExp($2, $1, symbol::trans("+")); }
| exp ADD exp      { $$ = new selfExp($2.pos, $1, $2.sym, $3); }
| exp SUBTRACT exp
                   { $$ = new selfExp($2.pos, $1, $2.sym, $3); }
| exp TIMES exp    { $$ = new selfExp($2.pos, $1, $2.sym, $3); }
| exp DIVIDE exp   { $$ = new selfExp($2.pos, $1, $2.sym, $3); }
| exp MOD exp      { $$ = new selfExp($2.pos, $1, $2.sym, $3); }
| exp EXPONENT exp
                   { $$ = new selfExp($2.pos, $1, $2.sym, $3); }
;

// This verbose definition is because leaving empty as an expansion for dir
// made a whack of reduce/reduce errors.
join:
  DASHES           { $$ = new join($1); // treat as {curl 1}..{curl 1}
                     $$->setLeftDir(new curlDir($1, new realExp($1, 1.0))); 
                     $$->setRightDir(new curlDir($1, new realExp($1, 1.0))); }
| basicjoin %prec JOIN 
                   { $$ = $1; }
| dir basicjoin %prec JOIN
                   { $$ = $2; $$->setLeftDir($1); }
| basicjoin dir %prec JOIN 
                   { $$ = $1; $$->setRightDir($2); }
| dir basicjoin dir %prec JOIN
                   { $$ = $2; $$->setLeftDir($1); $$->setRightDir($3); }
;

dir:
  '{' CURL exp '}' { $$ = new curlDir($2, $3); }
| '{' exp '}'      { $$ = new givenDir($1, $2); }
| '{' exp ',' exp '}'
                   { $$ = new givenDir($1, new pairExp($3, $2, $4)); }
;

basicjoin:
  DOTS             { $$ = new join($1); }
| DOTS tension DOTS
                   { $$ = $2; }
| DOTS controls DOTS
                   { $$ = $2; }
;

tension:
  TENSION exp      { $$ = new join($1, $2, true); }
| TENSION exp AND exp
                   { $$ = new join($1, $2, $4, true); }
| TENSION ATLEAST exp
                   { $$ = new join($1, $3, true, true); }
| TENSION ATLEAST exp AND exp
                   { $$ = new join($1, $3, $5, true, true); }
;

controls:
  CONTROLS exp     { $$ = new join($1, $2, false); }
| CONTROLS exp AND exp
                   { $$ = new join($1, $2, $4, false); }
;

stm:
  ';'              { $$ = new emptyStm($1); }
| block            { $$ = $1; }
| stmexp ';'       { $$ = $1; }
| IF '(' exp ')' stm
                   { $$ = new ifStm($1, $3, $5); }
| IF '(' exp ')' stm ELSE stm
                   { $$ = new ifStm($1, $3, $5, $7); }
| WHILE '(' exp ')' stm
                   { $$ = new whileStm($1, $3, $5); }
| DO stm WHILE '(' exp ')' ';'
                   { $$ = new doStm($1, $2, $5); }
| FOR '(' forinit ';' exp ';' forupdate ')' stm
                   { $$ = new forStm($1, $3, $5, $7, $9); }
| BREAK ';'        { $$ = new breakStm($1); }
| CONTINUE ';'     { $$ = new continueStm($1); }
| RETURN_ ';'       { $$ = new returnStm($1); }
| RETURN_ exp ';'   { $$ = new returnStm($1, $2); }
;

stmexp:
  exp              { $$ = new expStm($1->getPos(), $1); }
;

forinit:
  /* empty */      { $$ = 0; }
| stmexplist       { $$ = $1; }
| barevardec       { $$ = $1; }
;

forupdate:
  /* empty */      { $$ = 0; }
| stmexplist       { $$ = $1; }
;

stmexplist:
  stmexp           { $$ = new stmExpList($1->getPos()); $$->add($1); }
| stmexplist ',' stmexp
                   { $$ = $1; $$->add($3); }
;


