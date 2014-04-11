#include <stdio.h>
#include <stdlib.h>

#include "mpc.h"
#include <editline/readline.h>

#ifndef __APPLE__
/* not needed for osx */
#include <editline/history.h>
#endif


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
    puts("Lispy Version 0.0.0.0.1");
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
            /* On Success Print the AST */
            mpc_ast_print(r.output);
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
