/**
 * @file isabot.cpp
 *
 * @brief This program functions as Discord bot, which sends echoes to all messages in text channel named "isa-bot", which have not been sent by this bot or by users that have substring "bot" in their username. This bot authorizes with a token of existing bot account, which must be authorized in only one Discord server, with only one text channel named "isa-bot".
 *
 * @author Radoslav Grenčík
 * Contact: xgrenc00@stud.fit.vutbr.cz
 */

#include "isabot.hpp"

static volatile sig_atomic_t keep_running = 1;
#ifdef DEBUG
unsigned long long int request_counter = 0;
#endif

void SIGINTHandler(int)
{
    keep_running = 0;
}

int Error(int errnum, string err)
{
    if (err.size())
        cerr << "ERROR: " << err << "." << endl;
    switch (errnum) {
    case BAD_OPTIONS:
        cerr << "Try 'isabot -h|--help' for more information." << endl;
        break;
    }
    return (errnum);
}

int ParseOpt(int argc, char** argv, char* access_token, bool* flag_verbose)
{
    int opt;
    bool flag_access_token = false;
    *flag_verbose = false;

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
            return PrintHelp();
        case 'v':
            if (*flag_verbose == false)
                *flag_verbose = true;
            else
                return Error(BAD_OPTIONS, "bad option - option '-v|--verbose' declared more than once");
            break;
        case 't':
            if (flag_access_token == false) {
                flag_access_token = true;
                strcpy(access_token, optarg);
            } else
                return Error(BAD_OPTIONS, "bad option - option '-t <access_token>' declared more than once");
            break;
        case '?':
            return Error(BAD_OPTIONS, "");
        }
    }

    if (optind < argc) {
        cerr << "Non-option arguments detected: ";
        while (optind < argc)
            cerr << argv[optind++] << " ";
        cerr << endl;
        return Error(BAD_OPTIONS, "");
    }

    /* ---------------------- GOOD COMBINATIONS OF OPTIONS ---------------------- */
    // (-t <access_token> && -v|--verbose) || -t <access_token>
    // -h|--help && everything else
    // -h|--help
    // no option
    /* ----------------------- BAD COMBINATIONS OF OPTIONS ---------------------- */
    // -v|--verbose
    else if ((flag_access_token && *flag_verbose) || flag_access_token)
        return EXIT_SUCCESS;
    else if (*flag_verbose)
        return Error(BAD_OPTIONS, "bad combination of options - option '-v|--verbose' can be used only with option '-t <access_token>'");
    else // no option
        return PrintHelp();
}

int PrintHelp()
{
    cout << "USAGE: ./isabot [-h|--help] [-v|--verbose] -t <access_token>" << endl
         << endl
         << "This program functions as Discord bot, which sends echoes to all messages in text channel named \"isa-bot\"," << endl
         << "which have not been sent by this bot or by users that have substring \"bot\" in their username." << endl
         << "This bot authorizes with a token of existing bot account, which must be authorized in only one Discord server," << endl
         << "with only one text channel named \"isa-bot\"." << endl
         << endl
         << "For more informations about creation and authorization of bot account visit:" << endl
         << "https://discordpy.readthedocs.io/en/latest/discord.html" << endl
         << endl
         << "OPTIONS:" << endl
         << "-h|--help            Show this help." << endl
         << "-v|--verbose         Print messages (to the STDOUT), to which the bot will send echoes, in format:" << endl
         << "                     <channel> - <username>: <message>" << endl
         << "-t <access_token>    Token of existing bot account, which is authorized in one Discord server." << endl
         << "                     This token is needed for authorization of HTTP requests." << endl
         << endl
         << "If no options have been used, this help is displayed." << endl;
    return EXIT_HELP;
}

int OpenConnection(int* sock, const char* hostname, int port, time_t sec, time_t usec)
{
    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));

    *sock = socket(AF_INET, SOCK_STREAM, 0);
    if (*sock == -1)
        return Error(EXIT_FAILURE, "socket() failed");

    struct timeval timeout;
    timeout.tv_sec = sec;
    timeout.tv_usec = usec;

    if (setsockopt(*sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) < 0)
        return Error(EXIT_FAILURE, "setsockopt() SO_RCVTIMEO failed");

    if (setsockopt(*sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout)) < 0)
        return Error(EXIT_FAILURE, "setsockopt() SO_SNDTIMEO failed");

    struct hostent* he;
    he = gethostbyname(hostname);
    if (he == NULL)
        return Error(EXIT_FAILURE, "server IP not found");

    server.sin_addr.s_addr = inet_addr(inet_ntoa(*((struct in_addr*)he->h_addr_list[0])));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    if (connect(*sock, (struct sockaddr*)&server, sizeof(server)) == -1)
        return Error(EXIT_FAILURE, "connect() failed");

    return EXIT_SUCCESS;
}

int Startup(SSL_CTX** ctx, int* sock, SSL** ssl)
{
    SSL_library_init();
    OpenSSL_add_all_algorithms();

    *ctx = SSL_CTX_new(SSLv23_client_method()); // create new context
    if (*ctx == NULL)
        return Error(EXIT_FAILURE, "SSL_CTX_new() failed");

    int return_code;
    return_code = OpenConnection(sock, "discord.com", HTTPS, 1, 0);
    if (return_code != EXIT_SUCCESS)
        return return_code;

    *ssl = SSL_new(*ctx); // create new SSL connection state
    if (*ssl == NULL)
        return Error(EXIT_FAILURE, "SSL_new() failed");

    SSL_set_connect_state(*ssl);
    if (SSL_set_fd(*ssl, *sock) == 0) // attach the socket descriptor
        return Error(EXIT_FAILURE, "SSL_set_fd() failed");

    if (SSL_connect(*ssl) == -1) // perform the connection
        return Error(EXIT_FAILURE, "SSL_connect() failed");

#ifdef DEBUG
    cout << "* Successfully connected with " << SSL_get_cipher(*ssl) << " encryption..." << endl;
#endif

    return EXIT_SUCCESS;
}

void Cleanup(SSL_CTX** ctx, int* sock, SSL** ssl)
{
    if (*ssl != NULL) {
        SSL_free(*ssl); // release connection state
        *ssl = NULL;
    }
    if (*sock != -1) {
        close(*sock); // close socket
        *sock = -1;
    }
    if (*ctx != NULL) {
        SSL_CTX_free(*ctx); // release context
        *ctx = NULL;
    }
    EVP_cleanup();

#ifdef DEBUG
    cout << endl
         << "* Bot sent " << request_counter << " requests until now." << endl
         << "* CLEANUP..." << endl;
#endif
}

int Restart(SSL_CTX** ctx, int* sock, SSL** ssl)
{
#ifdef DEBUG
    cout << "----- RESTARTING -----";
#endif

    Cleanup(ctx, sock, ssl);
    return Startup(ctx, sock, ssl);
}

int SSLReadAnswer(SSL_CTX** ctx, int* sock, SSL** ssl, string* received)
{
    int bytes;
    int return_code;
    char buffer_answer[BUFFER];
    vector<string> answer;
    received->clear();

    struct pollfd pfds[1];
    pfds[0].fd = *sock;
    pfds[0].events = POLLIN;
    int poll_return;
    int poll_timeout = 500;

    bool loop = true;
    while (true) {
        while (loop) { // the read loop
            memset(buffer_answer, 0, sizeof(buffer_answer));
            bytes = SSL_read(*ssl, buffer_answer, sizeof(buffer_answer) - 1);
            *received += buffer_answer;

            if (bytes <= 0) { // some error occurred
                switch (SSL_get_error(*ssl, bytes)) {
                case SSL_ERROR_WANT_READ: // this error indicates, that nothing more can be read right now...
                    loop = false;
                    break;
                case SSL_ERROR_ZERO_RETURN: // connection was closed
                case SSL_ERROR_SYSCALL: // this is not recoverable fatal error
                case SSL_ERROR_SSL: // this also...it's needed to restart the SSL connection
#ifdef DEBUG
                    cout << endl
                         << "!!! SSL fatal error/connection closed..." << endl;
#endif
                    return_code = Restart(ctx, sock, ssl);
                    if (return_code != EXIT_SUCCESS)
                        return return_code;

                    loop = false;
                    break;
                default:
                    return Error(EXIT_FAILURE, "UNKNOWN ERROR");
                }
            }
        }

        /* -------------------- USE poll() ON THE UNDERLYING SOCKET ------------------- */
        poll_return = poll(pfds, 1, poll_timeout);
        if (poll_return == 0) { // poll() timeout expired
            if (received->size() == 0) { // nothing has been read, try to double the timeout (maximum timeout is 16sec)
#ifdef DEBUG
                cout << endl
                     << "!!! Nothing has been read..." << endl;
#endif
                if (poll_timeout < 12000) { // the last time when timeout is going to be doubled is when timeout = 8000
                    poll_timeout *= 2;
#ifdef DEBUG
                    cout << "    - Doubling poll() timeout to " << poll_timeout << " miliseconds..." << endl;
#endif
                    continue; // read loop is skipped, because loop = false
                }
                return Error(EXIT_FAILURE, "nothing has been read from server");
            }
            break; // some data has been read => break
        } else if (poll_return == -1) { // poll() error
            if (keep_running)
                return Error(EXIT_FAILURE, "poll() failed");
            else
                break; // SIGINT received, poll() probably failed, but the program is about to end and it doesn't matter - it's not an error in this case
        } else {
            if (pfds[0].revents & POLLIN) { // more to read
                loop = true; // activate the read loop
#ifdef DEBUG
                cout << endl
                     << "* READING MORE DATA..." << endl;
#endif
                if (poll_timeout != 500) {
                    poll_timeout = 500; // reset the poll timeout
#ifdef DEBUG
                    cout << "    - resetting poll() timeout to " << poll_timeout << " miliseconds" << endl;
#endif
                }
                continue;
            }
            return Error(EXIT_FAILURE, "unexpected poll() return event");
        }
    }

    /* ----------------- SPLIT THE ANSWER - GET HTTP RETURN CODE ---------------- */
    SplitString(*received, "\r\n", &answer);
    if (answer.size() < 1) // something unexpected, with only one line has been received
        return Error(EXIT_FAILURE, "bad answer from server");

    /* --------------------------------- RETURN --------------------------------- */
    if (answer.at(0).compare("HTTP/1.1 500 Internal Server Error") == 0) {
#ifdef DEBUG
        cout << endl
             << "----- EXIT_SERVER_ERROR -----" << endl;
#endif
        return EXIT_SERVER_ERROR;
    } else if (answer.at(0).compare("HTTP/1.1 200 OK") == 0)
        return EXIT_SUCCESS;
    else
        return Error(EXIT_FAILURE, answer.at(0));
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
    signal(SIGINT, SIGINTHandler); // SIGINT handling with SIGINTHandler() procedure

    int return_code;
    char access_token[512];
    memset(access_token, 0, sizeof(access_token));
    bool flag_verbose;

    return_code = ParseOpt(argc, argv, access_token, &flag_verbose); // parse the command line arguments (options)
    switch (return_code) {
    case EXIT_SUCCESS:
        break;
    case EXIT_HELP: // "-h|--help" option has been used => return EXIT_SUCCESS
        return EXIT_SUCCESS;
    default:
        return return_code;
    }

    SSL_CTX* ctx = NULL;
    int sock = -1;
    SSL* ssl = NULL;
    return_code = Startup(&ctx, &sock, &ssl);
    if (return_code != EXIT_SUCCESS) {
        Cleanup(&ctx, &sock, &ssl);
        return return_code;
    }

    char buffer_request[BUFFER];
    string received;
    string guild_id;
    string channel;
    string channel_id;
    string last_message_id;
    string my_name; // name of this bot
    vector<string> temp_list;
    const regex r_id_whole("(\"id\": \"[0-9]+\")");
    const regex r_id_num("([0-9]+)");
    const regex r_whole_last_message_id("(\"last_message_id\": (null|\"[0-9]+\"))");
    const regex r_last_message_id("(null|[0-9]+)");
    const regex r_message_content("(\"content\": \".*\", \"channel_id\":)");
    const regex r_message_username("(\"username\": \".*\", \"avatar\":)");
    const regex r_message_url("(\"url\": \"https://cdn\\.discordapp\\.com/attachments/[0-9]+/[0-9]+/.*\", \"proxy_url\":)");
    const regex r_isa_bot_channel("(\"id\": \"[0-9]+\", \"last_message_id\": (null|\"[0-9]+\"), \"type\": [0-9]+, \"name\": \"isa-bot\")");
    smatch match;

    /* -------------------------------------------------------------------------- */
    /*            GET THE ID OF THE GUILD, OF WHICH THE BOT IS A MEMBER           */
    /* -------------------------------------------------------------------------- */
    if (keep_running) {
        memset(buffer_request, 0, sizeof(buffer_request));
        strcpy(buffer_request, "GET /api/v6/users/@me/guilds HTTP/1.1\r\nHost: discord.com\r\nAuthorization: Bot ");
        strcat(buffer_request, access_token);
        strcat(buffer_request, "\r\n\r\n");

        if (SSL_write(ssl, buffer_request, strlen(buffer_request)) <= 0) {
            Cleanup(&ctx, &sock, &ssl);
            return Error(EXIT_FAILURE, "SSL_write() failed");
        }
        return_code = SSLReadAnswer(&ctx, &sock, &ssl, &received);
        if (return_code != EXIT_SUCCESS) {
            Cleanup(&ctx, &sock, &ssl);
            return return_code;
        }

#ifdef DEBUG
        cout << endl
             << "* Bot guilds received..." << endl;
#endif

        SplitString(received, "\r\n\r\n", &temp_list);
        if (temp_list.size() != 2) {
            Cleanup(&ctx, &sock, &ssl);
            return Error(EXIT_FAILURE, "bad answer from server");
        }
        SplitArrayOfJSON(temp_list.at(1), &temp_list);

        switch (temp_list.size()) {
        case 0:
            Cleanup(&ctx, &sock, &ssl);
            return Error(EXIT_FAILURE, "BOT is not member of any Discord guild");
        case 1:
            if (regex_search(temp_list.at(0), match, r_id_whole)) {
                guild_id += match[0];
                if (regex_search(guild_id, match, r_id_num)) {
                    guild_id.clear();
                    guild_id += match[0];
                }
            }
            break;
        default:
            Cleanup(&ctx, &sock, &ssl);
            return Error(EXIT_FAILURE, "BOT is member of more than one Discord guild");
        }

#ifdef DEBUG
        cout << "    - Guild ID: " << guild_id << endl;
#endif
    }

    /* -------------------------------------------------------------------------- */
    /*                     GET THE ID OF THE "isa-bot" CHANNEL                    */
    /* -------------------------------------------------------------------------- */
    if (keep_running) {
        memset(buffer_request, 0, sizeof(buffer_request));
        strcpy(buffer_request, "GET /api/v6/guilds/");
        strcat(buffer_request, guild_id.c_str());
        strcat(buffer_request, "/channels HTTP/1.1\r\nHost: discord.com\r\nAuthorization: Bot ");
        strcat(buffer_request, access_token);
        strcat(buffer_request, "\r\n\r\n");

        if (SSL_write(ssl, buffer_request, strlen(buffer_request)) <= 0) {
            Cleanup(&ctx, &sock, &ssl);
            return Error(EXIT_FAILURE, "SSL_write() failed");
        }
        return_code = SSLReadAnswer(&ctx, &sock, &ssl, &received);
        if (return_code != EXIT_SUCCESS) {
            Cleanup(&ctx, &sock, &ssl);
            return return_code;
        }

#ifdef DEBUG
        cout << endl
             << "* Guild channels received..." << endl;
#endif

        SplitString(received, "\r\n\r\n", &temp_list);
        if (temp_list.size() != 2) {
            Cleanup(&ctx, &sock, &ssl);
            return Error(EXIT_FAILURE, "bad answer from server");
        }
        received.clear();
        received += temp_list.at(1);
        SplitArrayOfJSON(temp_list.at(1), &temp_list);

        int channel_counter = 0;
        for (unsigned int i = 0; i < temp_list.size(); i++)
            if (regex_search(temp_list.at(i), match, r_isa_bot_channel))
                channel_counter++;

        switch (channel_counter) {
        case 0:
            Cleanup(&ctx, &sock, &ssl);
            return Error(EXIT_FAILURE, "there is no \"isa-bot\" channel in this Discord guild");
        case 1:
            if (regex_search(received, match, r_isa_bot_channel)) {
                channel += match[0];
                if (regex_search(channel, match, r_id_whole))
                    channel_id += match[0];
                if (regex_search(channel_id, match, r_id_num)) {
                    channel_id.clear();
                    channel_id += match[0];
                }

                /* -------------- obtaining the ID of last message from answer -------------- */
                if (regex_search(channel, match, r_whole_last_message_id))
                    last_message_id += match[0];
                if (regex_search(last_message_id, match, r_last_message_id)) {
                    last_message_id.clear();
                    last_message_id += match[0];
                }
                /* -------------------------------------------------------------------------- */
            }
            break;
        default:
            Cleanup(&ctx, &sock, &ssl);
            return Error(EXIT_FAILURE, "there is more than 1 \"isa-bot\" channel in this Discord guild");
        }

#ifdef DEBUG
        cout << "    - \"isa-bot\" channel ID: " << channel_id << endl;
        cout << "    - last message ID: " << last_message_id << endl;
#endif
    }

    /* -------------------------------------------------------------------------- */
    /*                            GET NAME OF THIS BOT                            */
    /* -------------------------------------------------------------------------- */
    if (keep_running) {
        memset(buffer_request, 0, sizeof(buffer_request));
        strcpy(buffer_request, "GET /api/v6/users/@me HTTP/1.1\r\nHost: discord.com\r\nAuthorization: Bot ");
        strcat(buffer_request, access_token);
        strcat(buffer_request, "\r\n\r\n");

        if (SSL_write(ssl, buffer_request, strlen(buffer_request)) <= 0) {
            Cleanup(&ctx, &sock, &ssl);
            return Error(EXIT_FAILURE, "SSL_write() failed");
        }
        return_code = SSLReadAnswer(&ctx, &sock, &ssl, &received);
        if (return_code != EXIT_SUCCESS) {
            Cleanup(&ctx, &sock, &ssl);
            return return_code;
        }

        if (regex_search(received, match, r_message_username)) {
            my_name += match[0];
            SplitString(my_name, "\", \"avatar\":", &temp_list);
            if (temp_list.size() != 1) {
                Cleanup(&ctx, &sock, &ssl);
                return Error(EXIT_FAILURE, "bad answer from server");
            }
            my_name.clear();
            my_name += temp_list.at(0);

            SplitString(my_name, "\"username\": \"", &temp_list);
            if (temp_list.size() != 1) {
                Cleanup(&ctx, &sock, &ssl);
                return Error(EXIT_FAILURE, "bad answer from server");
            }
            my_name.clear();
            my_name += temp_list.at(0);
        }

#ifdef DEBUG
        cout << "    - name of this bot: " << my_name << endl;
#endif
    }

#ifdef DEBUG
    cout << endl
         << "* Trying to receive new messages from \"isa-bot\" channel..." << endl;
#endif

    string all_messages;
    vector<string> splitted_messages;
    string username;
    string content;
    string attachment;
    char json_message[BUFFER / 2];
    char json_message_size[5]; // lenght of "json_message" can be max 4095 => 4 chars + '\0'

    /* -------------------------------------------------------------------------- */
    /*                                THE MAIN LOOP                               */
    /* -------------------------------------------------------------------------- */
    while (true) {
        if (!keep_running) // SIGINT received
            break;

        all_messages.clear();
        memset(buffer_request, 0, sizeof(buffer_request));
        strcpy(buffer_request, "GET /api/v6/channels/");
        strcat(buffer_request, channel_id.c_str());
        strcat(buffer_request, "/messages");
        if (last_message_id.compare("null") != 0) { // if there is no message in channel "isa-bot", last_message_id = null => request without "?after=..."
            strcat(buffer_request, "?after=");
            strcat(buffer_request, last_message_id.c_str()); // new messages which were posted => all messages after the message with the last_message_id
        }
        strcat(buffer_request, " HTTP/1.1\r\nHost: discord.com\r\nAuthorization: Bot ");
        strcat(buffer_request, access_token);
        strcat(buffer_request, "\r\n\r\n");

#ifdef DEBUG
        request_counter++;
#endif

        if (SSL_write(ssl, buffer_request, strlen(buffer_request)) <= 0) {
            Cleanup(&ctx, &sock, &ssl);
            return Error(EXIT_FAILURE, "SSL_write() failed");
        }
        return_code = SSLReadAnswer(&ctx, &sock, &ssl, &received);
        if (return_code == EXIT_SERVER_ERROR) // no valid data has been received => continue in new iteration and receive valid data
            continue;
        if (return_code != EXIT_SUCCESS) {
            Cleanup(&ctx, &sock, &ssl);
            return return_code;
        }

        SplitString(received, "\r\n\r\n", &temp_list);
        if (temp_list.size() != 2) {
            Cleanup(&ctx, &sock, &ssl);
            return Error(EXIT_FAILURE, "bad answer from server");
        }
        all_messages += temp_list.at(1);

        /* ------------------------ no new messages received ------------------------ */
        if (all_messages.compare("[]") == 0)
            continue;
        /* -------------------------------------------------------------------------- */

        SplitArrayOfJSON(all_messages, &splitted_messages);
        if (splitted_messages.size() < 1) {
            Cleanup(&ctx, &sock, &ssl);
            return Error(EXIT_FAILURE, "bad answer from server");
        }

        /* --------------- actualizing the ID of last received message -------------- */
        // the last received message is the first element in vector "splitted_messages"
        if (regex_search(splitted_messages.at(0), match, r_id_whole)) {
            last_message_id.clear();
            last_message_id += match[0];
            if (regex_search(last_message_id, match, r_id_num)) {
                last_message_id.clear();
                last_message_id += match[0];
            }
        }
        /* -------------------------------------------------------------------------- */

#ifdef DEBUG
        cout << endl
             << "# Request no." << request_counter << ":" << endl
             << "> " << splitted_messages.size() << " new " << (splitted_messages.size() == 1 ? "message" : "messages") << " received" << endl
             << "> last message ID: " << last_message_id << endl;
#endif

        /* --------------------- PROCESSING OF RECEIVED MESSAGES -------------------- */
        /* ------------------- this loop is not affected by SIGINT ------------------ */
        /* ------------- bot always send echoes to all received messages ------------ */
        for (int i = splitted_messages.size() - 1; i >= 0; i--) { // oldest received message is the last element in splitted messages
            username.clear();
            content.clear();
            attachment.clear();

            if (regex_search(splitted_messages.at(i), match, r_message_username)) {
                username += match[0];
                SplitString(username, "\", \"avatar\":", &temp_list);
                if (temp_list.size() < 1) {
                    Cleanup(&ctx, &sock, &ssl);
                    return Error(EXIT_FAILURE, "bad answer from server");
                }
                username.clear();
                username += temp_list.at(0);

                SplitString(username, "\"username\": \"", &temp_list);
                if (temp_list.size() != 1) {
                    Cleanup(&ctx, &sock, &ssl);
                    return Error(EXIT_FAILURE, "bad answer from server");
                }
                username.clear();
                username += temp_list.at(0);
            }

            if (username.compare(my_name) == 0) { // message from myself
#ifdef DEBUG
                cout << "isa-bot - Message from myself, skipping..." << endl;
#endif
                continue;
            }
            if (ToLower(username).find("bot") != string::npos) { // message from bot
#ifdef DEBUG
                cout << "isa-bot - Message from bot, skipping..." << endl;
#endif
                continue;
            }

            if (regex_search(splitted_messages.at(i), match, r_message_content)) {
                content += match[0];
                SplitString(content, "\"content\": \"", &temp_list);
                if (temp_list.size() != 1) {
                    Cleanup(&ctx, &sock, &ssl);
                    return Error(EXIT_FAILURE, "bad answer from server");
                }
                content.clear();
                content += temp_list.at(0);

                SplitString(content, "\", \"channel_id\":", &temp_list);
                content.clear();
                switch (temp_list.size()) {
                case 0:
                    break;
                case 1:
                    content += temp_list.at(0);
                    break;
                default:
                    Cleanup(&ctx, &sock, &ssl);
                    return Error(EXIT_FAILURE, "bad answer from server");
                }
            }

            if (flag_verbose) // "-v|--verbose" option has been used
                cout << "isa-bot - " << username << ": " << content << endl;

            /* ----------- messages with attachments => get URL of attachment ----------- */
            if (regex_search(splitted_messages.at(i), match, r_message_url)) {
                attachment += match[0];
                SplitString(attachment, "\"url\": \"", &temp_list);
                if (temp_list.size() != 1) {
                    Cleanup(&ctx, &sock, &ssl);
                    return Error(EXIT_FAILURE, "bad answer from server");
                }
                attachment.clear();
                attachment += temp_list.at(0);

                SplitString(attachment, "\", \"proxy_url\":", &temp_list);
                if (temp_list.size() != 1) {
                    Cleanup(&ctx, &sock, &ssl);
                    return Error(EXIT_FAILURE, "bad answer from server");
                }
                attachment.clear();
                attachment += temp_list.at(0);

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
                return Error(EXIT_FAILURE, "SSL_write() failed");
            }

#ifdef DEBUG
            cout << "    - Echo to message sent" << endl;
#endif

            return_code = SSLReadAnswer(&ctx, &sock, &ssl, &received);
            if (return_code == EXIT_SERVER_ERROR)
                continue;
            if (return_code != EXIT_SUCCESS) {
                Cleanup(&ctx, &sock, &ssl);
                return return_code;
            }
        }
    }

    Cleanup(&ctx, &sock, &ssl);
#ifdef DEBUG
    cout << "* EXIT SUCCESS..." << endl;
#endif

    return EXIT_SUCCESS;
}
/*** End of file isabot.cpp ***/
