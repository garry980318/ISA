/**
 * @author Radoslav Grenčík, xgrenc00@stud.fit.vutbr.cz
 */

#ifndef ISABOT_HPP_
#define ISABOT_HPP_

#include <arpa/inet.h>
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
// sleep for 3 seconds
#define SLEEP 3000000

/**
 * @brief
 */
void SIGINTHandler(int);

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
void PrintHelp();

/**
* @brief
*/
SSL_CTX* InitCTX();

/**
 * @brief
 */
const char* OpenConnection(int* sock, const char* hostname, int port);

/**
 * @brief
 */
void SSLReadAnswer(SSL* ssl, string* received);

/**
* @brief
*/
void Cleanup(SSL_CTX* ctx, int* sock, SSL* ssl);

/**
 * @brief
 */
vector<string> SplitString(string str, string delimiter);

#endif
/*** End of file isabot.hpp ***/
