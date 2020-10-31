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

/* -------------------------------- libraries ------------------------------- */
#include <arpa/inet.h>
#include <getopt.h>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <openssl/evp.h>
#include <openssl/ssl.h>
#include <poll.h>
#include <regex>
#include <signal.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

/* ------------------------------- ERROR codes ------------------------------ */
#define BAD_OPTIONS 420
#define EXIT_HELP 421
#define EXIT_RESTART 422

/* ------------------------------ other macros ------------------------------ */
#define BUFFER 8192
#define HTTPS 443

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
 * This function parses the command line arguments (options). If "-v|--verbose" option has been used, it sets the bool where "flag_verbose" points to "true". If no option has been used, PrintHelp() function is called and it's return value is returned.
 *
 * @param argc the number of command line arguments (options)
 * @param argv the array of command line arguments/strings (options)
 * @param access_token pointer to array of chars where the access token (to authorize the bot), which was passed by the user through the "-t <access_token>" option, will be written
 * @param flag_verbose pointer to "flag_verbose"
 * @returns EXIT_SUCCESS/return code of function PrintHelp() on success or BAD_OPTIONS if bad combination or number of options has been used
 */
int ParseOpt(int argc, char** argv, char* access_token, bool* flag_verbose);

/**
 * This function prints the help to STDOUT.
 *
 * @returns EXIT_HELP
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
 * This function tries to read data from a TLS/SSL connection. It actually calls the function SSL_read(), which reads the data into a buffer and these data are continuously copied into the string where "received" points. The function SSL_read() is called until some of these events happen...Connection was closed or fatal error has occured => restart of SSL connection is performed; no more data can be read right now => poll() is used on the underlying socket. The content of the string where the "received" points is always errased and then filled with new content. After the procedure finishes, the content of the string where "received" points can contain received data, but it depends and it can be determined by return code => EXIT_RESTART means that the string where the "received" points does NOT contain valid received data and other return codes mean otherwise.
 *
 * @param ctx
 * @param sock pointer to the socket descriptor
 * @param ssl
 * @param received pointer to received string - data which has been read are copied from buffer to the memory where "received" points
 * @returns EXIT_SUCCESS on success, EXIT_FAILURE on failure, EXIT_RESTART if internal server error has occurred => string where the "received" points does NOT contain valid received data
 */
int SSLReadAnswer(SSL_CTX** ctx, int* sock, SSL** ssl, string* received);

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
 * @returns "true" if the string is empty/contains only whitespace characters or "false" otherwise
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
