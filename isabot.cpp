/**
 * @author Radoslav Grenčík, xgrenc00@stud.fit.vutbr.cz
 */

#include "isabot.hpp"

static volatile sig_atomic_t keep_running = 1;

bool flag_verbose = false;

void SIGINTHandler(int)
{
    keep_running = 0;
}

void ErrExit(int errnum, const char* err)
{
    if (strlen(err))
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
    cout << "HELP WILL BE WRITTEN HERE..." << endl;
    exit(EXIT_SUCCESS);
}

int OpenConnection(const char* hostname, int port)
{
    int sock;
    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) //create a client socket
        ErrExit(EXIT_FAILURE, "socket() failed");

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    if (setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
        close(sock);
        ErrExit(EXIT_FAILURE, "setsockopt failed");
    }

    if (setsockopt (sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
        close(sock);
        ErrExit(EXIT_FAILURE, "setsockopt failed");
    }

    struct hostent* he;
    he = gethostbyname(hostname);
    if (he == NULL) {
        close(sock);
        ErrExit(EXIT_FAILURE, "IP not found");
    }

    server.sin_addr.s_addr = inet_addr(inet_ntoa(*((struct in_addr*)he->h_addr_list[0])));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) == -1) {
        close(sock);
        ErrExit(EXIT_FAILURE, "connect() failed");
    }

    return sock;
}

SSL_CTX* InitCTX()
{
    const SSL_METHOD *method;
    SSL_CTX *ctx;
    OpenSSL_add_all_algorithms();  /* Load cryptos, et.al. */
    SSL_load_error_strings();   /* Bring in and register error messages */

    // if OpenSSL is prior version 1.1.x older client method is needed
    #if (OPENSSL_VERSION_NUMBER < 0x010100000)
        method = TLSv1_client_method();
    #else
        method = TLS_client_method();
    #endif

    ctx = SSL_CTX_new(method);   /* Create new context */
    if (ctx == NULL) {
        ERR_print_errors_fp(stderr);
        ErrExit(EXIT_FAILURE, "creating of new context failed");
    }
    return ctx;
}

void SSL_read_answer(SSL *ssl, string *received)
{
    int bytes = 0;
    char buffer_answer[BUFFER];
    memset(buffer_answer, 0, sizeof(buffer_answer));

    received->clear();
    while(bytes != -1) {
        bytes = SSL_read(ssl, buffer_answer, sizeof(buffer_answer) - 1); /* get reply & decrypt */
        *received += buffer_answer;
        memset(buffer_answer, 0, sizeof(buffer_answer));
    }
}

vector<string> SplitString(string str, string delimiter)
{
    vector<string> list;
    size_t position = 0;
    string token;
    while ((position = str.find(delimiter)) != string::npos) {
        token = str.substr(0, position);
        list.push_back(token);
        str.erase(0, position + delimiter.length());
    }
    list.push_back(str);
    return list;
}

int main(int argc, char** argv)
{
    signal(SIGINT, SIGINTHandler);

    /***** PARSING OF THE ARGUMENTS *****/
    char access_token[512];
    ParseOpt(argc, argv, access_token);

    SSL_CTX *ctx;
    int sock;
    SSL *ssl;

    SSL_library_init();
    ctx = InitCTX();
    sock = OpenConnection("discord.com", HTTPS);
    ssl = SSL_new(ctx);      /* create new SSL connection state */
    SSL_set_fd(ssl, sock);    /* attach the socket descriptor */
    if (SSL_connect(ssl) == -1) {   /* perform the connection */
        SSL_free(ssl);
        close(sock);
        SSL_CTX_free(ctx);
        ERR_print_errors_fp(stderr);
        ErrExit(EXIT_FAILURE, "SSL connection failed");
    } else {

#ifdef DEBUG
    cout << "* Successfully connected with " << SSL_get_cipher(ssl) << " encryption..." << endl;
#endif

        string received;
        char buffer_request[BUFFER];
        memset(buffer_request, 0, sizeof(buffer_request));

        strcpy(buffer_request, "GET /api/v6/users/@me/guilds HTTP/1.1\r\nHost: discord.com\r\nAuthorization: Bot ");
        strcat(buffer_request, access_token);
        strcat(buffer_request, "\r\n\r\n");

        SSL_write(ssl, buffer_request, strlen(buffer_request));   /* encrypt & send message */
        SSL_read_answer(ssl, &received);

#ifdef DEBUG
    cout << endl << "* Bot guilds received..." << endl;
#endif

        const regex r_id_whole("(\"id\": \"[0-9]+\")");
        const regex r_id_num("([0-9]+)");
        const regex r_whole_last_message_id("(\"last_message_id\": (null|\"[0-9]+\"))");
        const regex r_last_message_id("(null|[0-9]+)");
        const regex r_messages("(\n\\[.*\\])");
        const regex r_message_content("(\"content\": \".*\", \"channel_id\":)");
        const regex r_message_username("(\"username\": \".*\", \"avatar\":)");
        const regex r_isa_bot_channel("(\"id\": \"[0-9]+\", \"last_message_id\": (null|\"[0-9]+\"), \"type\": [0-9]+, \"name\": \"isa-bot\")");
        smatch match;

        string guild_id;

        if (regex_search(received, match, r_id_whole)) {
            if (match.size() < 1 || match.size() > 2) {
                SSL_free(ssl);
                close(sock);
                SSL_CTX_free(ctx);
                ErrExit(EXIT_FAILURE, "BOT must be member of exactly one Discord guild");
            }
            guild_id += match[0];
            if (regex_search(guild_id, match, r_id_num)) {
                guild_id.clear();
                guild_id += match[0];
            }
        } else {
            SSL_free(ssl);
            close(sock);
            SSL_CTX_free(ctx);
            ErrExit(EXIT_FAILURE, "BOT unauthorized");
        }

#ifdef DEBUG
    cout << "   - Guild ID: " << guild_id << endl;
#endif

        memset(buffer_request, 0, sizeof(buffer_request));

        strcpy(buffer_request, "GET /api/v6/guilds/");
        strcat(buffer_request, guild_id.c_str());
        strcat(buffer_request, "/channels HTTP/1.1\r\nHost: discord.com\r\nAuthorization: Bot ");
        strcat(buffer_request, access_token);
        strcat(buffer_request, "\r\n\r\n");

        SSL_write(ssl, buffer_request, strlen(buffer_request));   /* encrypt & send message */
        SSL_read_answer(ssl, &received);

#ifdef DEBUG
    cout << endl << "* Guild channels received..." << endl;
#endif

        string channel;
        string channel_id;
        string last_message_id;

        if (regex_search(received, match, r_isa_bot_channel)) {
            if (match.size() < 1 || match.size() > 3) {
                SSL_free(ssl);
                close(sock);
                SSL_CTX_free(ctx);
                ErrExit(EXIT_FAILURE, "there is wrong number of \"isa-bot\" channels in specified Discord guild");
            }
            channel += match[0];
            if (regex_search(channel, match, r_id_whole))
                channel_id += match[0];
            if (regex_search(channel_id, match, r_id_num)) {
                channel_id.clear();
                channel_id += match[0];
            }

            if (regex_search(channel, match, r_whole_last_message_id))
                last_message_id += match[0];
            if (regex_search(last_message_id, match, r_last_message_id)) {
                last_message_id.clear();
                last_message_id += match[0];
            }
        } else {
            SSL_free(ssl);
            close(sock);
            SSL_CTX_free(ctx);
            ErrExit(EXIT_FAILURE, "there is wrong number of \"isa-bot\" channels in specified Discord guild");
        }

#ifdef DEBUG
    cout << "   - \"isa-bot\" channel ID: " << channel_id << endl;
#endif

        while (keep_running) {
            memset(buffer_request, 0, sizeof(buffer_request));

            strcpy(buffer_request, "GET /api/v6/channels/");
            strcat(buffer_request, channel_id.c_str());
            strcat(buffer_request, "/messages?after=");
            strcat(buffer_request, last_message_id.c_str());
            strcat(buffer_request, " HTTP/1.1\r\nHost: discord.com\r\nAuthorization: Bot ");
            strcat(buffer_request, access_token);
            strcat(buffer_request, "\r\n\r\n");

            SSL_write(ssl, buffer_request, strlen(buffer_request));   /* encrypt & send message */
            SSL_read_answer(ssl, &received);

#ifdef DEBUG
    cout << endl << "* Trying to receive new messages..." << endl;
#endif

            string all_messages;
            string username;
            string content;

            if (regex_search(received, match, r_messages)) {
                all_messages += match[0];
                if (all_messages.compare("\n[]") == 0) {
                    usleep(5000000);
                    continue;
                }

#ifdef DEBUG
    cout << "   - New messages received..." << endl;
#endif

                vector<string> splitted_messages = SplitString(all_messages, "}, {");

                if (regex_search(splitted_messages.at(0), match, r_id_whole)) {
                    last_message_id.clear();
                    last_message_id += match[0];
                    if (regex_search(last_message_id, match, r_id_num)) {
                        last_message_id.clear();
                        last_message_id += match[0];
                    }
                }

                for (int i = splitted_messages.size() - 1; i >= 0; i--) {
                    username.clear();
                    content.clear();
                    if (regex_search(splitted_messages.at(i), match, r_message_username)) {
                        username += match[0];
                        vector<string> splitted_username = SplitString(username, "\"username\": \"");
                        username.clear();
                        username += splitted_username.at(1);
                        splitted_username = SplitString(username, "\", \"avatar\":");
                        username.clear();
                        username += splitted_username.at(0);
                    }
                    if (string::npos != username.find("bot")) { //if bot is a substring in username username => continue

#ifdef DEBUG
    cout << endl << "* Message from bot, skipping..." << endl;
#endif

                        continue;
                    }
                    if (regex_search(splitted_messages.at(i), match, r_message_content)) {
                        content += match[0];
                        vector<string> splitted_content = SplitString(content, "\"content\": \"");
                        content.clear();
                        content += splitted_content.at(1);
                        splitted_content = SplitString(content, "\", \"channel_id\":");
                        content.clear();
                        content += splitted_content.at(0);
                    }

                    if (flag_verbose)
                        cout << "#isa-bot - " << username << ": " << content << endl;

                    char json_message[512];
                    memset(json_message, 0, sizeof(json_message));
                    strcpy(json_message, "{\"content\": \"echo: ");
                    strcat(json_message, username.c_str());
                    strcat(json_message, " - ");
                    strcat(json_message, content.c_str());
                    strcat(json_message, "\"}");

                    char json_message_size[32];
                    memset(json_message_size, 0, sizeof(json_message_size));
                    sprintf(json_message_size, "%lu", strlen(json_message));

                    //echo the message
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

                    SSL_write(ssl, buffer_request, strlen(buffer_request));   /* encrypt & send message */

#ifdef DEBUG
    cout << "   - Echo to message sent..." << endl;
#endif

                    SSL_read_answer(ssl, &received);

#ifdef DEBUG
    cout << endl << "* Answer:\n" << endl << received << endl;
#endif

                }
            }
            usleep(5000000); // sleep for 5 seconds
        }

        SSL_free(ssl);        /* release connection state */

#ifdef DEBUG
    cout << endl << "* Connection released...";
#endif

    }
    close(sock);         /* close socket */

#ifdef DEBUG
    cout << endl << "* Socket closed...";
#endif

    SSL_CTX_free(ctx);        /* release context */

#ifdef DEBUG
    cout << endl << "* Context released..." << endl << "* EXIT SUCCESS..." << endl;
#endif

    return EXIT_SUCCESS;
}
/*** End of file isabot.cpp ***/
