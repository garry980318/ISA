/**
 * @author Radoslav Grenčík, xgrenc00@stud.fit.vutbr.cz
 */

#ifndef ISABOT_HPP_
#define ISABOT_HPP_

#include <arpa/inet.h>
#include <ctype.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <pwd.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <regex>
#include <string>
#include <vector>

using namespace std;

#define BAD_OPTIONS 99

#define BUFFER 1024
#define HTTPS 443

/**
 * @brief
 */
void SIGINTHandler(int);

/**
 * @brief
 */
void ErrExit(int errnum, string err);

/**
 * @brief
 */
void ParseOpt(int argc, char** argv, char* access_token);

/**
 * @brief
 */
void PrintHelp();

/**
 * @brief
 */
SSL_CTX* InitCTX();

/**
 * @brief
 */
string OpenConnection(int* sock, const char* hostname, int port);

/**
 * @brief
 */
string SSLReadAnswer(SSL* ssl, string* received);

/**
 * @brief
 */
void Cleanup(SSL_CTX* ctx, int* sock, SSL* ssl);

/**
 * @brief
 */
string ToLower(string str);

/**
 * @brief
 */
bool IsWhiteSpaceOrEmpty(string str);

/**
 * @brief
 */
// vector<string> SplitString(string str, string delimiter);
void SplitString(string str, string delimiter, vector<string>* list);

/**
 * @brief
 */
// vector<string> SplitArrayOfJSON(string array);
void SplitArrayOfJSON(string array, vector<string>* list);

#endif
/*** End of file isabot.hpp ***/
