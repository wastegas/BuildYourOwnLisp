//#include <stdio.h>
//#include <stdlib.h>

#include "mpc.h"
#include <editline/readline.h>

#ifndef __APPLE__
/* not needed for osx */
#include <editline/history.h>
#endif

/* create enum for possible error types */
enum {
    LERR_DIV_ZERO,
    LERR_BAD_OP,
    LERR_BAD_NUM
};

/* create enum for possible lval types */
enum {
    LVAL_NUM,
    LVAL_ERR
};

/* declare new lval struct */
typedef struct lval {
    int type;
    long num;
    /* Error and symbol types have some string data */
    char* err;
    char* sym;

    /* Count and Pointer to a list of "lval" */
    int count;
    struct lval** cell;
} lval;

/* Construct a pointer to a new Number lval */
lval* lval_num(long x) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->num = x;
    return v;
}


/* construct a pointer to a new error type lval */
lval* lval_err(char* m) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_ERR;
    v->err = malloc(strnlen(m) + 1);
    strcpy(v->err, m);
    return v;
}

/* construct a pointer to a new symbol lval */
lval* lval_sym(char * s) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SYM;
    v->sym = malloc(strlen(s) + 1);
    strcpy(v->sym, s);
    return v;
}

/* a pointer to a new empty Sexpr lval */
lval* lval_sexpr(void) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

void lval_del(lval* v) {

    switch(v->type) {
        /* do nothing special for number type */
        case LVAL_NUM: break;

        /* for Err or Sym free the string data */
        case LVAL_ERR: free(v->err); break;
        case LVAL_SYM: free(v->sym); break;
        
        /*if Sexpr then delete all elements inside */
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

/* print an "lval" */
void lval_print(lval v) {
    switch(v.type) {
        /* if theres a number, print it and exit the switchcase */
        case LVAL_NUM: printf("%li", v.num);
        break;

        /* if type is an error */
        case LVAL_ERR:
        /* check what exact type of error it is and print it */
            if (v.err == LERR_DIV_ZERO) { printf("Error: Division by zero!"); }
            if (v.err == LERR_BAD_OP) { printf("Error: Invalid operator!"); }
            if (v.err == LERR_BAD_NUM) { printf("Error: Invalid number!"); }
            break;
    }
}

/* print an "lval" followed by a newline */
void lval_println(lval v) { 
    lval_print(v);
    putchar('\n');
}


/* Use operator string to see which operation to perform */
lval eval_op(lval x, char* op, lval y)
{
    /* if either value is an error return it */
    if (x.type == LVAL_ERR) { return x; }
    if (y.type == LVAL_ERR) { return y; }


    if (strcmp(op, "+") == 0) { return lval_num(x.num + y.num); }
    if (strcmp(op, "-") == 0) { return lval_num(x.num - y.num); }
    if (strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }
    if (strcmp(op, "/") == 0) { 
        return y.num == 0 ? lval_err(LERR_DIV_ZERO) : lval_num(x.num / y.num); 
    }
    return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t* t) {
   
    /* If tagged as number return it directly, otherwise expression*/
    if (strstr(t->tag, "number")) { 
        /* check if there is some error in conversion */
        errno = 0;
        long x = strtol(t->contents, NULL, 10);
        return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
    }

    /* The operator is always second child */
    char* op = t->children[1]->contents;

    lval x = eval(t->children[2]);

    /* Iterate the remaining children, combining using our operator */
    int i = 3;
    while (strstr(t->children[i]->tag, "expr"))
    {
        x = eval_op(x, op, eval(t->children[i]));
        i++;
    }

    return x;
}

int main(int argc, char** argv)
{
    /* Create some Parsers */
    mpc_parser_t* Number    = mpc_new("number");
    mpc_parser_t* Operator  = mpc_new("operator");
    mpc_parser_t* Expr      = mpc_new("expr");
    mpc_parser_t* Lispy     = mpc_new("lispy");

    /* Define them with the following Language */
    mpca_lang(MPC_LANG_DEFAULT,
        "                                                       \
          number    : /-?[0-9]+/ ;                              \
          operator  : '+' | '-' | '*' | '/' ;                   \
          expr      : <number> | '(' <operator> <expr>+ ')' ;   \
          lispy     : /^/ <operator> <expr>+ /$/ ;              \
        ",
    Number, Operator, Expr, Lispy);

    /* Print version and exit information */
    puts("Lispy Version 0.0.0.0.5");
    puts("Press Ctrl-c to Exit\n");

    /* In a never ending loop */
    while (1)
    {
        /* output our prompt */
        char* input = readline("lispy> ");
        add_history(input);

        /* Attempt to Parse the user Input */
        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lispy, &r)) {
 
            lval result = eval(r.output);
            lval_println(result);
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
    mpc_cleanup(4, Number, Operator, Expr, Lispy);

    return 0;
}
