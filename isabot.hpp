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
#include <openssl/err.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <regex>
#include <vector>
using namespace std;

#define BAD_OPTIONS 99

#define BUFFER 1024
#define HTTPS 443

bool flag_verbose = false;

/**
 * @brief
 */
void ErrExit(int errnum, const char* err);

/**
 * @brief
 */
void ParseOpt(int argc, char** argv, char* access_token);

/**
 * @brief
 */
int OpenConnection(const char* hostname, int port);

/**
 * @brief
 */
SSL_CTX* InitCTX();

/**
 * @brief
 */
void SSL_read_answer(SSL *ssl, string *received);

/**
 * @brief
 */
vector<string> SplitString(string str, string delimiter);

#endif
/*** End of file isabot.hpp ***/
