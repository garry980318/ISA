/**
 * @file isabot.hpp
 *
 * @brief Header file for isabot.cpp, which contains all the necessary library includes and declarations of macros and procedures/functions
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

#define BAD_OPTIONS 99 // bad options error number

#define BUFFER 8192 // 8KB
#define HTTPS 443
#define ONE_M_USEC 1000000 // one million microseconds

using namespace std;

/**
 * This function prints the given "err" to STDERR and returns "errnum".
 *
 * @param errnum the error number
 * @param err the error string
 * @returns "errnum"
 */
int Error(int errnum, string err);

/**
 * This function parses the command line arguments (options). If "-v|--verbose" option has been used, it sets the "flag_verbose" to "true". If no option has been used, PrintHelp() function is called and it's return value is returned.
 *
 * @param argc the number of command line arguments (options)
 * @param argv the array of command line arguments (options)
 * @param access_token pointer to array of chars where the access token (to authorize the bot),
 * which was passed by the user through the -t <access_token> option, will be written
 * @returns EXIT_SUCCESS on success or BAD_OPTIONS if bad combination or number of options has been used
 */
int ParseOpt(int argc, char** argv, char* access_token);

/**
 * This function prints the help to STDOUT.
 *
 * @returns EXIT_SUCCESS
 */
int PrintHelp();

/**
 * This function creates the client socket, sets socket's send and receive timeouts and performs connection to the server with the specified hostname on the specified port. This function also translates hostname (domain name) of server to the corresponding IP address, which is used in connection.
 *
 * @param sock pointer to the socket descriptor - after the socket is created, it's descriptor is written to the memory where "sock" points
 * @param hostname the server hostname
 * @param port the port number
 * @param sec send and receive timeout in seconds
 * @param usec send and receive timeout in microseconds
 * @returns EXIT_SUCCESS on success or EXIT_FAILURE if an error occured
 */
int OpenConnection(int* sock, const char* hostname, int port, time_t sec, time_t usec);

/**
 *
 *
 * @param ctx
 * @param sock pointer to the socket decriptor
 * @param ssl
 */
int Startup(SSL_CTX** ctx, int* sock, SSL** ssl);

/**
 *
 *
 * @param ctx
 * @param sock pointer to the socket descriptor
 * @param ssl
 */
void Cleanup(SSL_CTX** ctx, int* sock, SSL** ssl);

/**
 *
 *
 * If the content of the string where the "return_str" points is "HTTP/1.1 200 OK", NO error has occured and data has been read successfully. If the content of the string where the "return_str" points is "HTTP/1.1 500 Internal Server Error", internal server error has occured and the restart of the SSL connection is necessary (data has NOT been read successfully). If the content of the string where the "return_str" points is something else, an error has occured.
 *
 * @param ssl
 * @param received pointer to received string - data which has been read are copied from buffer to the memory where "received" points
 * @param return_str pointer to return string - error message or HTTP return code is written to the memory where "return_str" points
 */
void SSLReadAnswer(SSL* ssl, string* received, string* return_str);

/**
 * This function converts a string to lowercase.
 *
 * @param str the string to be converted to lowercase
 * @returns string in lowercase or empty string if string "str" is empty
 */
string ToLower(string str);

/**
 * This function checkes if the passed string is empty or contains only whitespace characters.
 *
 * @param str the string to be checked
 * @returns "true" if the string is empty or contains only whitespace characters or "false" otherwise
 */
bool IsWhiteSpaceOrEmpty(string str);

/**
 * This procedure splits a string "str" by the "delimiter" and fills the splitted strings into the vector where "list" points. Splitted string is inserted into vector only if function IsWhiteSpaceOrEmpty() with the splitted string passed as parameter returns "false". The content of the vector where the "list" points is always errased and then filled with new content.
 *
 * @param str the string to be splitted
 * @param delimiter the delimiter by which the string is splitted
 * @param list pointer to the vector of strings - splitted strings are written to the vector where "list" points
 */
void SplitString(string str, string delimiter, vector<string>* list);

/**
 * This procedure splits the array of JSON objects and fills them into the vector where "list" points. Content of JSON object is inserted into vector only if function IsWhiteSpaceOrEmpty() with the content of JSON object passed as parameter returns "false". The inner JSON objects which are inside of the main JSON objects are not inserted into the vector separately. The content of the vector where the "list" points is always errased and then filled with new content. If the array contains bad structured JSON objects (some of the curly brackets are missing), procedure ends and nothing is inserted into the vector.
 *
 * @param array the array of JSON objects to be splitted
 * @param list pointer to the vector of strings - JSON objects are written to the vector where "list" points
 */
void SplitArrayOfJSON(string array, vector<string>* list);

#endif
/*** End of file isabot.hpp ***/
