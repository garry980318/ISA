/**
 * @file isabot.hpp
 *
 * @brief Header file for isabot.cpp, which contains all the necessary includes and procedure/function definitions
 *
 * @author Radoslav Grenčík
 * Contact: xgrenc00@stud.fit.vutbr.cz
 */

#ifndef ISABOT_HPP_
#define ISABOT_HPP_

#include <arpa/inet.h>
#include <getopt.h>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <openssl/evp.h>
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
#define SECOND 1000000 // 1s = 1000000us

using namespace std;

/**
 * This procedure is called when SIGINT is received. It sets the "keep_running" variable to 0 and the program will end correctly.
 */
void SIGINTHandler(int);

/**
 * This procedure prints the given "err" to STDERR and exits the program with given "errnum".
 *
 * @param errnum the error number
 * @param err the error string
 */
void ErrExit(int errnum, string err);

/**
 * This procedure parse the command line arguments (options).
 * If -v|--verbose option was used it sets the "flag_verbose" to "true".
 * If bad combination or number of options were used, ErrExit() procedure is called with BAD_OPTIONS errno.
 * If no option was used, PrintHelp() procedure is called.
 *
 * @param argc the number of command line arguments (options)
 * @param argv the array of command line arguments (options)
 * @param access_token contains the access token (to authorize the bot), which was passed by the user through the -t <access_token> option
 */
void ParseOpt(int argc, char** argv, char* access_token);

/**
 * This procedure prints the help to STDOUT and exits the program with EXIT_SUCCESS.
 */
void PrintHelp();

/**
 * This procedure creates the client socket, sets the socket send and receive timeouts and
 * performs connection to the server at the specified hostname and on the specified port.
 * This procedure also translate hostname (domain name) of server to the corresponding IP address, which is used in connection.
 *
 * @param sock pointer to the socket descriptor - socket descriptor is written to the memory where it points
 * @param hostname the server hostname
 * @param port the port number
 * @param return_str  pointer to return string - if error occurs in the procedure, error message is written to the memory where it points
 */
void OpenConnection(int* sock, const char* hostname, int port, string* return_str);

/**
 *
 *
 * @param ctx
 * @param sock
 * @param ssl
 */
void Startup(SSL_CTX** ctx, int* sock, SSL** ssl);

/**
 *
 *
 * @param ctx
 * @param sock
 * @param ssl
 */
void Cleanup(SSL_CTX** ctx, int* sock, SSL** ssl);

/**
 *
 *
 * @param ssl
 * @param received
 * @param return_str
 */
void SSLReadAnswer(SSL* ssl, string* received, string* return_str);

/**
 * This function converts a string to the lowercase.
 *
 * @param str the string to be converted to lowercase
 * @returns string in lowercase or empty string if string "str" is empty
 */
string ToLower(string str);

/**
 * This function checkes if the passed string is empty or contains only whitespace characters.
 *
 * @param str the string to be checked
 * @returns true if the string is empty or contains only whitespace characters or false otherwise
 */
bool IsWhiteSpaceOrEmpty(string str);

/**
 *
 *
 * @param str
 * @param delimiter
 * @param list
 */
void SplitString(string str, string delimiter, vector<string>* list);

/**
 *
 *
 * @param array
 * @param list
 */
void SplitArrayOfJSON(string array, vector<string>* list);

#endif
/*** End of file isabot.hpp ***/
