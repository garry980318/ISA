/**
 * @file isabot.cpp
 *
 * @brief
 *
 * @author Radoslav Grenčík
 * Contact: xgrenc00@stud.fit.vutbr.cz
 */

#include "isabot.hpp"

static volatile sig_atomic_t keep_running = 1;
static bool restart_needed = false;
static bool flag_verbose = false;

void SIGINTHandler(int)
{
    keep_running = 0;
}

void ErrExit(int errnum, string err)
{
    if (err.size())
        cerr << "ERROR: " << err << "." << endl;
    switch (errnum) {
    case BAD_OPTIONS:
        cerr << "Try 'isabot -h|--help' for more information." << endl;
        break;
    }
    exit(errnum);
}

void ParseOpt(int argc, char** argv, char* access_token)
{
    int opt;
    bool flag_access_token = false;

    while (true) {
        int option_index = 0;
        static struct option long_options[] = {
            { "help", no_argument, NULL, 'h' },
            { "verbose", no_argument, NULL, 'v' },
            { 0, 0, 0, 0 }
        };

        opt = getopt_long(argc, argv, "hvt:", long_options, &option_index);
        if (opt == -1)
            break;

        switch (opt) {
        case 'h':
            PrintHelp();
            break;
        case 'v':
            if (flag_verbose == false)
                flag_verbose = true;
            else
                ErrExit(BAD_OPTIONS, "bad option - option '-v|--verbose' declared more than once");
            break;
        case 't':
            if (flag_access_token == false) {
                flag_access_token = true;
                strcpy(access_token, optarg);
            } else
                ErrExit(BAD_OPTIONS, "bad option - option '-t <access_token>' declared more than once");
            break;
        case '?':
            ErrExit(BAD_OPTIONS, "");
            break;
        }
    }

    if (optind < argc) {
        cerr << "Non-option arguments detected: ";
        while (optind < argc)
            cerr << argv[optind++] << " ";
        cerr << endl;
        ErrExit(BAD_OPTIONS, "");
    }

    if (flag_access_token)
        return;

    PrintHelp();
}

void PrintHelp()
{
    cout << "USAGE: ./isabot [-h|--help] [-v|--verbose] [-t <access_token>]" << endl
         << endl
         << "This program serves as discord bot, which sends echoes to all messages in text channel named \"isa-bot\"," << endl
         << "only if these have been sent by users that don't have substring \"bot\" in their username." << endl
         << endl
         << "Bot must be previously authorized in only one discord server with only one \"isa-bot\" text channel!" << endl
         << "For more info about creation and authorization of bot visit:" << endl
         << "https://discordpy.readthedocs.io/en/latest/discord.html" << endl
         << endl
         << "OPTIONS:" << endl
         << "-h|--help            Show this help." << endl
         << "-v|--verbose         Print messages (to the STDOUT), to which the bot will send echoes, in format:" << endl
         << "                     <channel> - <username>: <message>" << endl
         << "-t <access_token>    Token that the bot needs for authorization." << endl
         << endl
         << "If no options were provided, this help will be displayed." << endl;
    exit(EXIT_SUCCESS);
}

void OpenConnection(int* sock, const char* hostname, int port, string* return_str)
{
    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    return_str->clear();

    if ((*sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) { // create a client socket
        *return_str += "socket() failed";
        return;
    }

    struct timeval timeout;
    timeout.tv_sec = 2; // timeot for receiving and sending is 2s
    timeout.tv_usec = 0;

    if (setsockopt(*sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) < 0) {
        close(*sock);
        *return_str += "setsockopt() SO_RCVTIMEO failed";
        return;
    }

    if (setsockopt(*sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout)) < 0) {
        close(*sock);
        *return_str += "setsockopt() SO_SNDTIMEO failed";
        return;
    }

    struct hostent* he;
    he = gethostbyname(hostname);
    if (he == NULL) {
        close(*sock);
        *return_str += "server IP not found";
        return;
    }

    server.sin_addr.s_addr = inet_addr(inet_ntoa(*((struct in_addr*)he->h_addr_list[0])));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    if (connect(*sock, (struct sockaddr*)&server, sizeof(server)) == -1) {
        close(*sock);
        *return_str += "connect() failed";
    }
}

void Startup(SSL_CTX** ctx, int* sock, SSL** ssl)
{
    SSL_library_init();
    OpenSSL_add_all_algorithms();

    *ctx = SSL_CTX_new(SSLv23_client_method()); // create new context
    if (*ctx == NULL) {
        EVP_cleanup();
        ErrExit(EXIT_FAILURE, "SSL_CTX_new() failed");
    }

    string socket_error;
    OpenConnection(sock, "discord.com", HTTPS, &socket_error);
    if (socket_error.size()) {
        SSL_CTX_free(*ctx);
        EVP_cleanup();
        ErrExit(EXIT_FAILURE, socket_error);
    }

    *ssl = SSL_new(*ctx); // create new SSL connection state
    SSL_set_connect_state(*ssl);
    if (SSL_set_fd(*ssl, *sock) == 0) { // attach the socket descriptor
        Cleanup(ctx, sock, ssl);
        ErrExit(EXIT_FAILURE, "SSL_set_fd() failed");
    }

    if (SSL_connect(*ssl) == -1) { // perform the connection
        Cleanup(ctx, sock, ssl);
        ErrExit(EXIT_FAILURE, "SSL_connect() failed");
    }

#ifdef DEBUG
    cout << "* Successfully connected with " << SSL_get_cipher(*ssl) << " encryption..." << endl;
#endif
}

void Cleanup(SSL_CTX** ctx, int* sock, SSL** ssl)
{
    SSL_free(*ssl); // release connection state
    *ssl = NULL;

    close(*sock); // close socket
    *sock = -1;

    SSL_CTX_free(*ctx); // release context
    *ctx = NULL;

    EVP_cleanup();

#ifdef DEBUG
    cout << endl
         << "* CLEANUP..." << endl;
#endif
}

void SSLReadAnswer(SSL* ssl, string* received, string* return_str)
{
    int bytes;
    char buffer_answer[BUFFER];
    vector<string> answer;
    received->clear();
    return_str->clear();

    unsigned int attempt = 0;
    bool loop;
    while (true) {
        loop = true;
        while (loop) { // the read loop
            memset(buffer_answer, 0, sizeof(buffer_answer));
            bytes = SSL_read(ssl, buffer_answer, sizeof(buffer_answer) - 1); // get reply & decrypt
            *received += buffer_answer;

            if (bytes <= 0) { // some error occurred
                switch (SSL_get_error(ssl, bytes)) {
                case SSL_ERROR_ZERO_RETURN: // this is recoverable error
                case SSL_ERROR_WANT_WRITE: // this one also
                case SSL_ERROR_WANT_READ: // this one indicates, that nothing more can be read right now...
                    loop = false; // the read loop can be interrupted
                    break;
                case SSL_ERROR_SYSCALL: // this is not recoverable fatal error
                case SSL_ERROR_SSL: // this also
                    restart_needed = true; // we need to restart the SSL connection
                    loop = false; // the read loop must be interrupted
                    break;
                default:
                    *return_str += "UNKNOWN ERROR";
                    return;
                }
            }
        }
        if (received->size() == 0) { // sometimes can happen => nothing has been received...try 10 more times, than return error message
            attempt++;
            if (attempt < 11) {
#ifdef DEBUG
                cout << "# No answer from server..trying to read again (" << attempt << "/10)..." << endl;
#endif
                usleep(1 * SECOND); // wait 1s then try again...
                continue;
            }
            *return_str += "no answer from server";
            return;
        }
        break; // pretty much always => something has been received => break
    }

    SplitString(*received, "\r\n", &answer);
    if (answer.size() < 1) // something unexpected, with only one line has been received
        *return_str += "bad answer from server";
    else
        *return_str += answer.at(0); // return first line of the answer => the HTTP return code
}

string ToLower(string str)
{
    if (str.size() == 0)
        return "";
    string lower_str;
    for (unsigned int i = 0; i < str.size(); i++)
        lower_str += tolower(str[i]);
    return lower_str;
}

bool IsWhiteSpaceOrEmpty(string str)
{
    if (str.size() == 0) // string is empty => true
        return true;
    for (unsigned int i = 0; i < str.size(); i++) {
        if (isspace(str[i]) == 0) // string now contains one NON-whitespace => false
            return false;
    }
    return true; // string contains only whitespaces => true
}

void SplitString(string str, string delimiter, vector<string>* list)
{
    size_t position = 0;
    string token;
    list->clear();
    vector<string>(*list).swap(*list);

    while ((position = str.find(delimiter)) != string::npos) {
        token = str.substr(0, position);
        if (IsWhiteSpaceOrEmpty(token) == false)
            list->push_back(token);
        str.erase(0, position + delimiter.size());
    }
    if (IsWhiteSpaceOrEmpty(str) == false)
        list->push_back(str);
}

void SplitArrayOfJSON(string array, vector<string>* list)
{
    unsigned int num_of_brackets = 0;
    unsigned int l_counter = 0;
    unsigned int r_counter = 0;
    size_t position = 0;
    string token;
    list->clear();
    vector<string>(*list).swap(*list);

    for (unsigned int i = 0; i < array.size(); i++)
        if (array[i] == '{' || array[i] == '}')
            num_of_brackets++; // count all curly brackets in the array

    if (num_of_brackets == 0 || num_of_brackets % 2 != 0) // bad JSON
        return;

    while (true) {
        while (array[position] != '{' && array[position] != '}') {
            if (l_counter == r_counter) {
                array.erase(array.begin()); // erase everything that is not between '{' and '}'
                continue;
            }
            position++;
        }

        if (array[position] == '{')
            l_counter++;

        if (array[position] == '}')
            r_counter++;

        if (l_counter == r_counter) {
            position++;
            token = array.substr(0, position); // string between '{' and '}'
            if (IsWhiteSpaceOrEmpty(token) == false)
                list->push_back(token);
            if (l_counter + r_counter == num_of_brackets) // nothing more is between '{' and '}'
                break;
            array.erase(0, position);
            position = 0;
            continue;
        }
        position++;
    }
}

int main(int argc, char** argv)
{
    signal(SIGINT, SIGINTHandler); // SIGINT handling with SIGINTHandler procedure

    char access_token[512];
    memset(access_token, 0, sizeof(access_token));
    ParseOpt(argc, argv, access_token);

    SSL_CTX* ctx = NULL;
    int sock = -1;
    SSL* ssl = NULL;
    Startup(&ctx, &sock, &ssl);

    char buffer_request[BUFFER];
    string received;
    string guild_id;
    string channel;
    string channel_id;
    string last_message_id;
    string SSL_read_return;
    const regex r_id_whole("(\"id\": \"[0-9]+\")");
    const regex r_id_num("([0-9]+)");
    const regex r_whole_last_message_id("(\"last_message_id\": (null|\"[0-9]+\"))");
    const regex r_last_message_id("(null|[0-9]+)");
    const regex r_message_content("(\"content\": \".*\", \"channel_id\":)");
    const regex r_message_username("(\"username\": \".*\", \"avatar\":)");
    const regex r_message_url("(\"url\": \"https://cdn\\.discordapp\\.com/attachments/[0-9]+/[0-9]+/.*\", \"proxy_url\":)");
    const regex r_isa_bot_channel("(\"id\": \"[0-9]+\", \"last_message_id\": (null|\"[0-9]+\"), \"type\": [0-9]+, \"name\": \"isa-bot\")");
    smatch match;

    if (keep_running) {
        memset(buffer_request, 0, sizeof(buffer_request));

        strcpy(buffer_request, "GET /api/v6/users/@me/guilds HTTP/1.1\r\nHost: discord.com\r\nAuthorization: Bot ");
        strcat(buffer_request, access_token);
        strcat(buffer_request, "\r\n\r\n");

        if (SSL_write(ssl, buffer_request, strlen(buffer_request)) <= 0) {
            Cleanup(&ctx, &sock, &ssl);
            ErrExit(EXIT_FAILURE, "SSL_write() failed");
        } // encrypt & send message
        SSLReadAnswer(ssl, &received, &SSL_read_return);
        if (SSL_read_return.compare("HTTP/1.1 200 OK") != 0) {
            Cleanup(&ctx, &sock, &ssl);
            ErrExit(EXIT_FAILURE, SSL_read_return);
        }

#ifdef DEBUG
        cout << endl
             << "* Bot guilds received..." << endl;
#endif

        if (regex_search(received, match, r_id_whole)) {
            if (match.size() > 2) {
                Cleanup(&ctx, &sock, &ssl);
                ErrExit(EXIT_FAILURE, "BOT is member of more than one Discord guild");
            }
            guild_id += match[0];
            if (regex_search(guild_id, match, r_id_num)) {
                guild_id.clear();
                guild_id += match[0];
            }
        } else {
            Cleanup(&ctx, &sock, &ssl);
            ErrExit(EXIT_FAILURE, "BOT is not member of any Discord guild");
        }

#ifdef DEBUG
        cout << "   - Guild ID: " << guild_id << endl;
#endif
    }

    if (keep_running) {
        memset(buffer_request, 0, sizeof(buffer_request));

        strcpy(buffer_request, "GET /api/v6/guilds/");
        strcat(buffer_request, guild_id.c_str());
        strcat(buffer_request, "/channels HTTP/1.1\r\nHost: discord.com\r\nAuthorization: Bot ");
        strcat(buffer_request, access_token);
        strcat(buffer_request, "\r\n\r\n");

        if (SSL_write(ssl, buffer_request, strlen(buffer_request)) <= 0) {
            Cleanup(&ctx, &sock, &ssl);
            ErrExit(EXIT_FAILURE, "SSL_write() failed");
        } // encrypt & send message
        SSLReadAnswer(ssl, &received, &SSL_read_return);
        if (SSL_read_return.compare("HTTP/1.1 200 OK") != 0) {
            Cleanup(&ctx, &sock, &ssl);
            ErrExit(EXIT_FAILURE, SSL_read_return);
        }

#ifdef DEBUG
        cout << endl
             << "* Guild channels received..." << endl;
#endif

        if (regex_search(received, match, r_isa_bot_channel)) {
            if (match.size() > 3) {
                Cleanup(&ctx, &sock, &ssl);
                ErrExit(EXIT_FAILURE, "there is more than 1 \"isa-bot\" channel in this Discord guild");
            }
            channel += match[0];
            if (regex_search(channel, match, r_id_whole))
                channel_id += match[0];
            if (regex_search(channel_id, match, r_id_num)) {
                channel_id.clear();
                channel_id += match[0];
            }

            if (regex_search(channel, match, r_whole_last_message_id)) // obtaining the ID of last message from answer
                last_message_id += match[0];
            if (regex_search(last_message_id, match, r_last_message_id)) {
                last_message_id.clear();
                last_message_id += match[0];
            }
        } else {
            Cleanup(&ctx, &sock, &ssl);
            ErrExit(EXIT_FAILURE, "there is no \"isa-bot\" channel in this Discord guild");
        }

#ifdef DEBUG
        cout << "   - \"isa-bot\" channel ID: " << channel_id << endl;
#endif
    }

#ifdef DEBUG
    unsigned long long int request_counter = 0;
#endif

    string all_messages;
    string username;
    string content;
    string attachment;
    vector<string> received_body;
    vector<string> splitted_messages;
    vector<string> splitted_username;
    vector<string> splitted_content;
    vector<string> splitted_url;
    char json_message[2048];
    char json_message_size[5]; // lenght of json_message can be max 2047 => 4 chars + '\0'

    while (true) {
        if (!keep_running) // SIGINT received
            break;

        all_messages.clear();
        memset(buffer_request, 0, sizeof(buffer_request));

        strcpy(buffer_request, "GET /api/v6/channels/");
        strcat(buffer_request, channel_id.c_str());
        strcat(buffer_request, "/messages?after=");
        strcat(buffer_request, last_message_id.c_str()); // we want to get all new messages which were posted => all messages after the message with the last_message_id
        strcat(buffer_request, " HTTP/1.1\r\nHost: discord.com\r\nAuthorization: Bot ");
        strcat(buffer_request, access_token);
        strcat(buffer_request, "\r\n\r\n");

#ifdef DEBUG
        cout << endl
             << "* Trying to receive new messages from \"isa-bot\" channel...Request No." << ++request_counter << endl;
#endif

        if (SSL_write(ssl, buffer_request, strlen(buffer_request)) <= 0) {
            Cleanup(&ctx, &sock, &ssl);
            ErrExit(EXIT_FAILURE, "SSL_write() failed");
        } // encrypt & send message
        SSLReadAnswer(ssl, &received, &SSL_read_return);
        if (SSL_read_return.compare("HTTP/1.1 200 OK") != 0) {
            Cleanup(&ctx, &sock, &ssl);
            if (SSL_read_return.compare("HTTP/1.1 500 Internal Server Error") == 0) { // sometimes the server returns this, and we need to restart the SSL connection
#ifdef DEBUG
                cout << "***** Internal Server Error - RESTARTING *****" << endl;
#endif
                Startup(&ctx, &sock, &ssl); // RESTART
                continue;
            }
            ErrExit(EXIT_FAILURE, SSL_read_return);
        }

        SplitString(received, "\r\n\r\n", &received_body);
        if (received_body.size() != 2) {
            Cleanup(&ctx, &sock, &ssl);
            ErrExit(EXIT_FAILURE, "bad answer from server");
        }
        all_messages += received_body.at(1);

        if (all_messages.compare("[]") == 0) { // no new messages received
            if (!keep_running) // SIGINT received
                break;
#ifdef DEBUG
            cout << "# No new messages received..." << endl;
#endif
            if (restart_needed) { // FATAL error in the SSL_read() has occurred => restart of the SSL connection is necessary
                restart_needed = false;
                Cleanup(&ctx, &sock, &ssl);
#ifdef DEBUG
                cout << "***** SSL fatal error - RESTARTING *****" << endl;
#endif
                Startup(&ctx, &sock, &ssl);
            }
            continue;
        }

        SplitArrayOfJSON(all_messages, &splitted_messages);
        if (splitted_messages.size() < 1) {
            Cleanup(&ctx, &sock, &ssl);
            ErrExit(EXIT_FAILURE, "bad answer from server");
        }

#ifdef DEBUG
        cout << "# " << splitted_messages.size() << " new " << (splitted_messages.size() == 1 ? "message" : "messages")
             << " received..." << endl;
#endif

        if (regex_search(splitted_messages.at(0), match, r_id_whole)) { // actualizing the ID of last message => last received message is the first element in splitted_messages
            last_message_id.clear();
            last_message_id += match[0];
            if (regex_search(last_message_id, match, r_id_num)) {
                last_message_id.clear();
                last_message_id += match[0];
            }
        }

        for (int i = splitted_messages.size() - 1; i >= 0; i--) { // first received message is the last element in splitted messages
            // this loop is not affected by SIGINT => bot always send echoes to all received messages
            username.clear();
            content.clear();
            attachment.clear();

            if (regex_search(splitted_messages.at(i), match, r_message_username)) {
                username += match[0];
                SplitString(username, "\", \"avatar\":", &splitted_username);
                if (splitted_username.size() < 1) {
                    Cleanup(&ctx, &sock, &ssl);
                    ErrExit(EXIT_FAILURE, "bad answer from server");
                }
                username.clear();
                username += splitted_username.at(0);

                SplitString(username, "\"username\": \"", &splitted_username);
                if (splitted_username.size() != 1) {
                    Cleanup(&ctx, &sock, &ssl);
                    ErrExit(EXIT_FAILURE, "bad answer from server");
                }
                username.clear();
                username += splitted_username.at(0);
            }

            if (ToLower(username).find("bot") != string::npos) { // if bot is a substring in username => continue (CASE INSENSITIVE)
#ifdef DEBUG
                cout << "isa-bot - Message from bot, skipping..." << endl;
#endif
                continue;
            }

            if (regex_search(splitted_messages.at(i), match, r_message_content)) {
                content += match[0];
                SplitString(content, "\"content\": \"", &splitted_content);
                if (splitted_content.size() != 1) {
                    Cleanup(&ctx, &sock, &ssl);
                    ErrExit(EXIT_FAILURE, "bad answer from server");
                }
                content.clear();
                content += splitted_content.at(0);

                SplitString(content, "\", \"channel_id\":", &splitted_content);
                content.clear();
                switch (splitted_content.size()) {
                case 0:
                    break;
                case 1:
                    content += splitted_content.at(0);
                    break;
                default:
                    Cleanup(&ctx, &sock, &ssl);
                    ErrExit(EXIT_FAILURE, "bad answer from server");
                    break;
                }
            }

            if (flag_verbose) // -v|--verbose option has been used
                cout << "isa-bot - " << username << ": " << content << endl;

            if (regex_search(splitted_messages.at(i), match, r_message_url)) { // this is bonus...bot can send proper echo (URL of attachment) to messages containing attachments
                attachment += match[0];
                SplitString(attachment, "\"url\": \"", &splitted_url);
                if (splitted_url.size() != 1) {
                    Cleanup(&ctx, &sock, &ssl);
                    ErrExit(EXIT_FAILURE, "bad answer from server");
                }
                attachment.clear();
                attachment += splitted_url.at(0);

                SplitString(attachment, "\", \"proxy_url\":", &splitted_url);
                if (splitted_url.size() != 1) {
                    Cleanup(&ctx, &sock, &ssl);
                    ErrExit(EXIT_FAILURE, "bad answer from server");
                }
                attachment.clear();
                attachment += splitted_url.at(0);

                if (IsWhiteSpaceOrEmpty(content) == false)
                    content += " ";
                content += attachment; // append the URL of attachment
            }

            // send echo to received message
            memset(json_message, 0, sizeof(json_message));
            strcpy(json_message, "{\"content\": \"echo: ");
            strcat(json_message, username.c_str());
            strcat(json_message, " - ");
            strcat(json_message, content.c_str());
            strcat(json_message, "\"}");

            memset(json_message_size, 0, sizeof(json_message_size));
            sprintf(json_message_size, "%lu", strlen(json_message));

            memset(buffer_request, 0, sizeof(buffer_request));

            strcpy(buffer_request, "POST /api/v6/channels/");
            strcat(buffer_request, channel_id.c_str());
            strcat(buffer_request, "/messages HTTP/1.1\r\nHost: discord.com\r\nAuthorization: Bot ");
            strcat(buffer_request, access_token);
            strcat(buffer_request, "\r\nContent-Type: application/json");
            strcat(buffer_request, "\r\nContent-Length: ");
            strcat(buffer_request, json_message_size);
            strcat(buffer_request, "\r\n\r\n");
            strcat(buffer_request, json_message);
            strcat(buffer_request, "\r\n\r\n");

            if (SSL_write(ssl, buffer_request, strlen(buffer_request)) <= 0) {
                Cleanup(&ctx, &sock, &ssl);
                ErrExit(EXIT_FAILURE, "SSL_write() failed");
            } // encrypt & send message

#ifdef DEBUG
            cout << "   - Echo to message sent..." << endl;
#endif

            SSLReadAnswer(ssl, &received, &SSL_read_return);
            if (SSL_read_return.compare("HTTP/1.1 200 OK") != 0) {
                Cleanup(&ctx, &sock, &ssl);
                if (SSL_read_return.compare("HTTP/1.1 500 Internal Server Error") == 0) { // sometimes the server returns this, and we need to restart the SSL connection
#ifdef DEBUG
                    cout << "***** Internal Server Error - RESTARTING *****" << endl;
#endif
                    Startup(&ctx, &sock, &ssl); // RESTART
                    continue;
                }
                ErrExit(EXIT_FAILURE, SSL_read_return);
            }

#ifdef DEBUG
            cout << "   - Answer: " << SSL_read_return << endl;
#endif
        }
        if (!keep_running) // SIGINT received
            break;
        if (restart_needed) { // FATAL error in the SSL_read() has occurred => restart of the SSL connection is necessary
            restart_needed = false;
            Cleanup(&ctx, &sock, &ssl);
#ifdef DEBUG
            cout << "***** SSL fatal error - RESTARTING *****" << endl;
#endif
            Startup(&ctx, &sock, &ssl);
        }
    }

    Cleanup(&ctx, &sock, &ssl);
#ifdef DEBUG
    cout << "* EXIT SUCCESS..." << endl;
#endif
    return EXIT_SUCCESS;
}
/*** End of file isabot.cpp ***/
