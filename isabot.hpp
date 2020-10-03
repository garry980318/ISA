/**
 * @author Radoslav Grenčík, xgrenc00@stud.fit.vutbr.cz
 */

#ifndef LIST_H_
#define LIST_H_

#include <arpa/inet.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <openssl/ssl.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BAD_OPTIONS 2

#define BUFFER 1024

/**
 * @brief
 */
void errExit(int errnum, const char* err);

#endif
/*** End of file isabot.hpp ***/
