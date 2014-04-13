#include <stdio.h>
#include <stdlib.h>

#include "mpc.h"
#include <editline/readline.h>

#ifndef __APPLE__
/* not needed for osx */
#include <editline/history.h>
#endif

/* Use operator string to see which operation to perform */
long eval_op(long x, char* op, long y)
{
    if (strcmp(op, "+") == 0) { return x + y; }
    if (strcmp(op, "-") == 0) { return x - y; }
    if (strcmp(op, "*") == 0) { return x * y; }
    if (strcmp(op, "/") == 0) { return x / y; }
    return 0;
}

long eval(mpc_ast_t* t) {
    /* If tagged as number return it directly, otherwise expression*/
    if (strstr(t->tag, "number")) { return atoi(t->contents);}

    /* The operator is always second child */
    char* op = t->children[1]->contents;

    /* We store the third child in `x` */
    long x = eval(t->children[2]);

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
    puts("Lispy Version 0.0.0.0.3");
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
 
            long result = eval(r.output);
            /* On Success Print the AST */
            mpc_ast_print(r.output);
            printf("%li\n", result);
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
