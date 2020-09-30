/**
 * @author Radoslav Grenčík, xgrenc00@stud.fit.vutbr.cz
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <getopt.h>

void errExit(int errnum, const char *err)
{
    fprintf(stderr, "ERROR: %s\nTry 'isabot -h|--help' for more information.\n", err);
    exit(errnum);
}

int main(int argc, char **argv)
{
    bool flag_verbose = false;
    bool flag_access_token = false;
    char access_token[100];

    int opt;

    while (true)
    {
        int option_index = 0;
        static struct option long_options[] = {
            {"help", no_argument, NULL, 'h'},
            {"verbose", no_argument, NULL, 'v'},
            {0, 0, 0, 0}};

        opt = getopt_long(argc, argv, "hvt:", long_options, &option_index);
        if (opt == -1)
            break;

        switch (opt)
        {
        case 'h':
            printf("HELP WILL BE DISPLAYED HERE...\n");
            exit(EXIT_SUCCESS);
            break;
        case 'v':
            if (flag_verbose == false)
                flag_verbose = true;
            else
                errExit(EXIT_FAILURE, "bad option - option '-v|--verbose' declared more than once.");
            break;
        case 't':
            if (flag_access_token == false)
            {
                flag_access_token = true;
                strcpy(access_token, optarg);
            }
            else
                errExit(EXIT_FAILURE, "bad option - option '-t <access_token>' declared more than once.");
            break;
        case '?':
            errExit(EXIT_FAILURE, "...");
            break;
        }
    }

    if (optind < argc)
    {
        fprintf(stderr, "Non-option arguments detected: ");
        while (optind < argc)
            fprintf(stderr, "%s ", argv[optind++]);
        fprintf(stderr, "\n");
        errExit(EXIT_FAILURE, "...");
    }

    if (flag_access_token == false)
        errExit(EXIT_FAILURE, "bad option - option '-t <access_token>' is missing.");

    printf("Verbose: %s\n", flag_verbose ? "true" : "false");
    printf("Access token: %s\n", access_token);

    return EXIT_SUCCESS;
}
/*** End of file isabot.cpp ***/
