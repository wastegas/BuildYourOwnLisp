//#include <stdio.h>
//#include <stdlib.h>

#include "mpc.h"
#include <editline/readline.h>

#ifndef __APPLE__
/* not needed for osx */
#include <editline/history.h>
#endif

struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;


/* create enum for possible lval types */
enum {
    LVAL_ERR,
    LVAL_NUM,
    LVAL_SYM,
    LVAL_FUN,
    LVAL_SEXPR,
    LVAL_QEXPR
};

typedef struct lval {
    int type;
    long num;
    char* err;
    char* sym;
    lbuiltin fun;

    int count;
    lval** cell;
} lval;

lval* lval_num(long x) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->num = x;
    return v;
}

lval* lval_err(char* m) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_ERR;
    
    va_list va;
    va_start(va, fmt);

    v->err = malloc(512);

    vsnprintf(v->err, 511, fmtm va);
    
    v->err = realloc(v->err, strlen(v->err) + 1);
    va_end(va);
    return v;
}

lval* lval_sym(char * s) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SYM;
    v->sym = malloc(strlen(s) + 1);
    strcpy(v->sym, s);
    return v;
}

lval* lval_sexpr(void) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

lval* lval_qexpr(void) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_QEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

void lval_del(lval* v) {

    switch(v->type) {
        case LVAL_NUM: break;
        case LVAL_ERR: free(v->err); break;
        case LVAL_SYM: free(v->sym); break;
        
        /*if Sexpr then delete all elements inside */
        case LVAL_QEXPR:
        case LVAL_SEXPR:
            for (int i = 0; i < v->count ; i++) {
                lval_del(v->cell[i]);
            }
            /* free memory allocated with pointers */
            free(v->cell);
            break;
    }
    free(v);
}

lval* lval_copy(lval* v) {

    lval* x = malloc(sizeof(lval));
    x->type = v->type;

    switch(v->type) {

        case LVAL_FUN: x->fun = v->fun; break;
        case LVAL_NUM: X->num = v->num; break;
        case LVAL_ERR: x->err = malloc(strlen(v->err) + 1); strcpy(x->err, v->err); break;
        case LVAL_SYM: x->sym = malloc(strlen(v->sym) + 1); strcpy(x->sym, v->sym); break;
        case LVAL_SEXPR:
        case LVAL_QEXPR:
            x->count = v->count;
            x->cell = malloc(sizeof(lval*) * x->count);
            for (int i = 0; i < x->count; i++) {
                x-<cell[i] = lval_copy(v->cell[i]);
            }
            break;
    }

    return x;
}

lval* lval_add(lval* v, lval* x) {
    v->count++;
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    v->cell[v->count-1] = x;
    return v;
}

lval* lval_pop(lval* v, int i) {
    lval* x = v->cell[i];
    memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*) * (v->count-i-1));
    v->count--;
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    return x;
}

lval* lval_join(lval* x, lval* y) {
    
    while (y->count) {
        x = lval_add(x, lval_pop(y, 0));
    }

    lval_del(y);
    return x;
}

lval* lval_take(lval* v, int i) {
    lval *x = lval_pop(v, i);
    lval_del(v);
    return x;
}

void lval_print(lval *v);

void lval_expr_print(lval* v, char open, char close) {
    putchar(open);
    for (int i = 0; i < v->count; i++) {

        lval_print(v->cell[i]);

        if (i != (v->count-1)) {
            putchar(' ');
        }
    }
    putchar(close);
}

void lval_print(lval* v) {
    switch(v->type) {
        case LVAL_FUN: printf("<function>"); break;
        case LVAL_NUM: printf("%li", v->num); break;
        case LVAL_ERR: printf("Error: %s", v->err); break;
        case LVAL_SYM: printf("%s", v->sym); break;
        case LVAL_SEXPR: lval_expr_print(v, '(',')'); break;
        case LVAL_QEXPR: lval_expr_print(v, '{','}'); break;
    }
}

void lval_println(lval* v) { 
    lval_print(v);
    putchar('\n');
}

#define LASSERT(args, cond, err) if (!(cond)) { lval_del(args); return lval_err(err); }

lval* lval_eval(lval* v);

lval* builtin_list(lval* a) {
    a->type = LVAL_QEXPR;
    return a;
}

char* ltype_name(int t) {
    switch(t) {
        case LVAL_FUN: return "Function";
        case LVAL_NUM: return "Number";
        case LVAL_ERR: return "Error";
        case LVAL_SYM: return "Symbol";
        case LVAL_SEXPR: return "S-Expression";
        case LVAL_QEXPR: return "Q-Expression";
        default: return "Unknown";
    }
}

/* Lisp Environment */

struct lenv {
    int count;
    char** syms;
    lval** vals;
}

lenv* lenv_new(void) {

    lenv* e = malloc(sizeof(lenv));
    e->count = 0;
    e->syms = NULL;
    e->vals = NULL;
    return e;
}

void lenv_del(lenv* e) {
    for (int i = 0; i < e->count; i++) {
        free(e->syms[i]);
        lval_del(e->vals[i]);
    }

    free(e->syms);
    free(e->vals);
    free(e);
}

lval* lenv_get(lenv* e lval* k) {
    for (int i = 0; i < e->count; i++) {
        if (strcmp(e->syms[i], k->sym) == 0) {
            return lval_copy(e->vals[i])
        }
    }
        return lval_err("Unbound Symbol '%s'", k->sym);
}

void lenv_put(lenv* e, lval* k, lval* v) {
    for (int i = 0; i < e->count; i++) {
        if (strcmp(e->syms[i], k->sym) == 0) {
            lval_del(e->vals[i]);
            e->vals[i] = lval_copy(v);
            return;
        }
    }

    e->count++;
    e->vals = realloc(e->vals, sizeof(lval*) * e->count);
    e->syms = realloc(e->syms, sizeof(char*) * e->count);

    e->vals[e->count-1] = lval_copy(v);
    e->syms[e->count-1] = malloc(strlen(k->sym)+1);
    strcpy(e->syms[e->count-1], k->sym);
}

#define LASSERT(args, cond, fmt, ...) \ 
    if (!(cond)) { \
        lval* err = lval_err(fmt, ##__VA_ARGS__); \
        lval_del(args); \
        return err; \
    }

#define LASSERT_TYPE(func, args, index, expect) \
    LASSERT(args, args->cell[index]->type == expect, \
      "Function '%s' passed incorrect type for argument %i. Got %s, \
       Expected %s", func, index, ltype_name(args->cell[index]->type), \
          ltype_name(expect))

#define LASSERT_NUM(func, args num) \
    LASSERT(args, args->count == num, \
            "Function '%s' passed incorrect number of arguments. Got %i, Expected %i.", \
            func, args->count, num)

#define LASSERT_NOT_EMPTY(func, args, index) \
    LASSERT(args, args->cell[index]->count != 0, \
            "Function '%s' passed {} for argument %i.", func, index);

lval* lval_eval(lenv* e, lval v);

lval* builtin_list(lenv* e, lval* a) {
    a->type = LVAL_QEXPR;
    return a;
}


lval* builtin_head(lenv* e, lval* a) {
    LASSERT_NUM("head", a, 1);
    LASSERT_TYPE("head", a, 0, LVAL_QEXPR);
    LASSERT_NOT_EMPTY("head", a, 0);

    lval* v = lval_take(a, 0);
    while (v->count > 1) { lval_del(lval_pop(v, 1)); }
    return v;
}

lval* builtin_tail(lenv* e, lval* a) {
    LASSERT_NUM("tail", a, 1);
    LASSERT_TYPE("tail", a, 0, LVAL_QEXPR);
    LASSERT_NOT_EMPTY("tail", a, 0);

    lval* v = lval_take(a, 0);
    lval_del(lval_pop(v, 0));
    return v;
}

lval* builtin_eval(lenv* e, lval* a) {
    LASSERT_NUM("eval", a, 1);
    LASSERT_TYPE("tail", a, 0, LVAL_QEXPR);

    lval* x = lval_take(a, 0);
    x->type = LVAL_SEXPR;
    return lval_eval(x);
}

lval* builtin_join(lenv* e, lval* a) {

    for (int i = 0; i < a->count; i++) {
        LASSERT_TYPE("join", a, i, LVAL_QEXPR);
    }

    lval* x = lval_pop(a, 0);

    while (a->count) {
        x = lval_join(x, lval_pop(a, 0));
    }

    lval_del(a);
    return x;
}

lval* builtin_op(lenv* e, lval* a, char* op) {

    for (int i = 0; i < a->count; i++) {
        LASSERT_TYPE(op, a, i, LVAL_NUM); 
    }

    lval* x = lval_pop(a, 0);

    if ((strcmp(op, "-") == 0) && a->count == 0) { x->num = -x->num; }

    while (a->count > 0) {

        lval* y = lval_pop(a, 0);

        if (strcmp(op, "+") == 0) { x->num += y->num; }
        if (strcmp(op, "-") == 0) { x->num -= y->num; }
        if (strcmp(op, "*") == 0) { x->num *= y->num; }
        if (strcmp(op, "/") == 0) { 
            if (y->num == 0) {
                lval_del(x); lval_del(y);
                x = lval_err("Division by zero.");
                break;
            } else {
                x->num /= y->num;
            }
        }

        /* delete element now finish with */
        lval_del(y);
    }

    /* delete input expression and return result */
    lval_del(a);
    return x;
}

lval *builtin_add(lenv* e, lval* a) { return builtin_op(e, a, "+"); }
lval *builtin_sub(lenv* e, lval* a) { return builtin_op(e, a, "-"); }
lval *builtin_mul(lenv* e, lval* a) { return builtin_op(e, a, "*"); }
lbal *builtin_div(lenv* e, lval* a) { return builtin_op(e, a, "/"); }

lval *builtin_def(lenv* e, lval* a) {
    LASSERT_TYPE("def", a, 0, LVAL_QEXPR);

    lval* syms = a->cell[0];

    for (int i = 0; i < syms->count; i++) {
        LASSERT(a, (syms->cell[i]->type == LVAL_SYM),
                "Function 'def' cannot define non-symbol. Got %s, Expected %s.",
                ltype_name(syms->cell[i]->type), ltype_name(LVAL_SYM));
    }

    LASSERT(a, (syms->count == a->count - 1),
            "Function 'def' passed too many arguments for symbols. Got %i, Expected %i.",
            syms->count, a->count-1);

    for (int i = 0; i < syms->count; i++) {
        lenv_put(e, syms->cell[i], a->cell[i+1]);
    }

    lval_del(a);
    return lval_sexpr();
}

lval* builtin(lval* a, char* func) {
    if (strcmp("list", func) == 0) { return builtin_list(a); }
    if (strcmp("head", func) == 0) { return builtin_head(a); }
    if (strcmp("tail", func) == 0) { return builtin_tail(a); }
    if (strcmp("join", func) == 0) { return builtin_join(a); }
    if (strcmp("eval", func) == 0) { return builtin_eval(a); }
    if (strstr("+-/*", func)) { return builtin_op(a, func); }
    lval_del(a);
    return lval_err("Unknown Function!");
}

lval* lval_eval_sexpr(lval* v) {

    for (int i = 0; i < v->count; i++) {
        v->cell[i] = lval_eval(v->cell[i]);
    }

    for (int i = 0; i < v->count; i++) {
        if (v->cell[i]->type == LVAL_ERR) { return lval_take(v, i); }
    }

    if (v->count == 0) { return v; }

    if (v->count == 1 ) { return lval_take(v, 0); }

    lval* f = lval_pop(v, 0);
    if (f->type != LVAL_SYM) {
        lval_del(f); lval_del(v);
        return lval_err("S-expression does not start with symbol.");
    }

    /* call builtin with operator */
    lval * result = builtin_op(v, f->sym);
    lval_del(f);
    return result;
}

lval* lval_eval(lval* v) {
    /* evaluate Sexpressions */
    if (v->type == LVAL_SEXPR) { return lval_eval_sexpr(v); }
    /* all other lval types remain the same */
    return v;
}

lval* lval_read_num(mpc_ast_t* t) {
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lval_num(x) : lval_err("invalid number");
}

lval* lval_read(mpc_ast_t* t) {
    /* if symbol or number return conversion to that type */
    if(strstr(t->tag, "number")) { return lval_read_num(t); }
    if(strstr(t->tag, "symbol")) { return lval_sym(t->contents); }

    /* if root (>) or sexpr then create empty list */
    lval* x = NULL;
    if(strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
    if(strstr(t->tag, "sexpr")) { x = lval_sexpr(); }
    if(strstr(t->tag, "qexpr")) { x = lval_qexpr(); }

    /* fill this list with any valid expression contained within */
    for (int i = 0; i < t->children_num; i++) {
        if(strcmp(t->children[i]->contents, "(") ==0) { continue; }
        if(strcmp(t->children[i]->contents, ")") ==0) { continue; }
        if(strcmp(t->children[i]->contents, "}") ==0) { continue; }
        if(strcmp(t->children[i]->contents, "{") ==0) { continue; }
        if(strcmp(t->children[i]->tag, "regex")==0) {continue;}
        x = lval_add(x, lval_read(t->children[i]));
    }

    return x;
}

int main(int argc, char** argv)
{
    /* Create some Parsers */
    mpc_parser_t* Number    = mpc_new("number");
    mpc_parser_t* Symbol    = mpc_new("symbol");
    mpc_parser_t* Sexpr     = mpc_new("sexpr");
    mpc_parser_t* Qexpr     = mpc_new("qexpr");
    mpc_parser_t* Expr      = mpc_new("expr");
    mpc_parser_t* Lispy     = mpc_new("lispy");

    /* Define them with the following Language */
    mpca_lang(MPC_LANG_DEFAULT,
        "                                                       \
          number    : /-?[0-9]+/ ;                              \
          symbol    : \"list\" | \"head\" | \"tail\" | \"eval\" | \"join\" | '+' | '-' | '*' | '/' ;                   \
          sexpr     : '(' <expr>* ')';                          \
          qexpr     : '{' <expr>* '}';                          \
          expr      : <number> | <symbol> | <sexpr> | <qexpr> ;           \
          lispy     : /^/ <expr>* /$/ ;                         \
        ",
    Number, Symbol, Sexpr, Qexpr, Expr, Lispy);

    puts("Lispy Version 0.0.0.0.7");
    puts("Press Ctrl-c to Exit\n");

    while (1)
    {
        char* input = readline("lispy> ");
        add_history(input);

        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lispy, &r)) {
 
            lval* x = lval_eval(lval_read(r.output));
            lval_println(x);
            lval_del(x);

            mpc_ast_delete(r.output);
        } else {
            /* otherwise Print the error */
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        /* Free retrieved input */
        free(input);    // housekeeping for editline
   }
    /* Undefine and delete our Parsers */
    mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Lispy);

    return 0;
}
