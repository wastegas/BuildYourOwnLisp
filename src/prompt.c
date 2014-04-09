#include <stdio.h>
#include <stdlib.h>

#include <editline/readline.h>

#ifndef __APPLE__
/* not needed for osx */
#include <editline/history.h>
#endif

/* Declare a static buffer for user input of maximum size 2048 */
//static char input[2048]; --- using editline functions now

int main(int argc, char** argv)
{
    /* Print version and exit information */
    puts("Lispy Version 0.0.0.0.1");
    puts("Press Ctrl-c to Exit\n");

    /* In a never ending loop */
    while (1)
    {
        /* output our prompt */
        //fputs("lispy> ", stdout); -- using editline functions now
        char* input = readline("lispy> ");

        /* read a line of user input of a maximum size 2048 */
        //fgets(input, 2048, stdin); -- using editline functions now
        add_history(input);

        /* echo input back to user */
        printf("No you're a %s\n", input);

        /* Free retrieved input */
        free(input);    // housekeeping for editline
    }

    return 0;
}
