/**
 * @author Radoslav Grenčík, xgrenc00@stud.fit.vutbr.cz
 */

#define _POSIX_C_SOURCE 2
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

void errExit (int errno, char *err)
{
    fprintf(stderr, "ERROR: %s.\n", err);
    exit(errno);
}

int main(int argc, char **argv)
{
    bool flagRecursive = false;
    bool flagReverse = false;
    bool flagAAAA = false;
    bool flagServer = false;
    char *server = NULL;
    int port = 53;
    bool flagAddress = false;
    char *address = NULL;
    int opt;

    while (optind != argc)
    {
        opt = getopt(argc, argv, ":rx6s:p:");
        switch (opt)
        {
            case 'r':
                if (flagRecursive == false)
                    flagRecursive = true;
                else
                    errExit(1, "bad option - option -r declared more than once");
                break;
            case 'x':
                if (flagReverse == false)
                    flagReverse = true;
                else
                    errExit(1, "bad option - option -x declared more than once");
                break;
            case '6':
                if (flagAAAA == false)
                    flagAAAA = true;
                else
                    errExit(1, "bad option - option -6 declared more than once");
                break;
            case 's':
                if (flagServer == false)
                {
                    flagServer = true;
                    server = optarg;
                }
                else
                    errExit(1, "bad option - option -s declared more than once");
                break;
            case 'p':
                if (port == 53)
                {
                    if (sscanf(optarg, "%d", &port) != 1)
                        errExit(1, "bad option - port is not an integer");
                }
                else
                    errExit(1, "bad option - option -p declared more than once");
                break;
            case -1:
                if (flagAddress == false)
                {
                    flagAddress = true;
                    address = argv[optind];
                    optind++;
                }
                else
                    errExit(1, "bad option - address declared more than once");
                break;
            case ':':
                errExit(1, "bad option - option needs a value or already declared");
                break;
            case '?':
                    errExit(1, "bad option - unknown option");
                break;
        }
    }

    if (flagServer == false || flagAddress == false)
        errExit(1, "bad option - server or address is missing");

    printf("Recursive: %s\n", flagRecursive ? "true" : "false");
    printf("Reverse: %s\n", flagReverse ? "true" : "false");
    printf("AAAA: %s\n", flagAAAA ? "true" : "false");
    printf("Server: %s\n", server);
    printf("Port: %d\n", port);
    printf("Address: %s\n", address);

    return 0;
}
/*** End of file dns.c ***/
