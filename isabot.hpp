/**
 * @author Radoslav Grenčík, xgrenc00@stud.fit.vutbr.cz
 */

#ifndef ISABOT_HPP_
#define ISABOT_HPP_

#include <arpa/inet.h>
#include <getopt.h>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <regex>
#include <signal.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#define BAD_OPTIONS 99 // bad options errno

#define BUFFER 8192 // 8KB
#define HTTPS 443

using namespace std;

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
void SSLReadAnswer(SSL* ssl, string* received, string* return_str);

/**
 * @brief
 */
void Startup(SSL_CTX** ctx, int* sock, SSL** ssl);

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
void SplitString(string str, string delimiter, vector<string>* list);

/**
 * @brief
 */
void SplitArrayOfJSON(string array, vector<string>* list);

#endif
/*** End of file isabot.hpp ***/
