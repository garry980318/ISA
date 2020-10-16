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
    cout << "HELP WILL BE WRITTEN HERE..." << endl;
    exit(EXIT_SUCCESS);
}

SSL_CTX* InitCTX()
{
    SSL_load_error_strings(); /* Bring in and register error messages */
    SSL_library_init();
    OpenSSL_add_all_algorithms(); /* Load cryptos, et.al. */

    const SSL_METHOD* method;
// if OpenSSL is prior version 1.1.x, older client method is needed
#if (OPENSSL_VERSION_NUMBER < 0x010100000)
    method = TLSv1_client_method();
#else
    method = TLS_client_method();
#endif

    SSL_CTX* ctx;
    ctx = SSL_CTX_new(method); /* Create new context */
    if (ctx == NULL) {
        ERR_print_errors_fp(stderr);
        EVP_cleanup();
        ERR_free_strings();
        ErrExit(EXIT_FAILURE, "creating of new context failed");
    }
    return ctx;
}

string OpenConnection(int* sock, const char* hostname, int port)
{
    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));

    if ((*sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) //create a client socket
        return "socket() failed";

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    if (setsockopt(*sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) < 0) {
        close(*sock);
        return "setsockopt() failed";
    }

    if (setsockopt(*sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout)) < 0) {
        close(*sock);
        return "setsockopt() failed";
    }

    struct hostent* he;
    he = gethostbyname(hostname);
    if (he == NULL) {
        close(*sock);
        return "server IP not found";
    }

    server.sin_addr.s_addr = inet_addr(inet_ntoa(*((struct in_addr*)he->h_addr_list[0])));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    if (connect(*sock, (struct sockaddr*)&server, sizeof(server)) == -1) {
        close(*sock);
        return "connect() failed";
    }

    return "";
}

string SSLReadAnswer(SSL* ssl, string* received)
{
    int bytes = 0;
    char buffer_answer[BUFFER];
    memset(buffer_answer, 0, sizeof(buffer_answer));

    received->clear();
    while (bytes != -1) {
        bytes = SSL_read(ssl, buffer_answer, sizeof(buffer_answer) - 1); /* get reply & decrypt */
        *received += buffer_answer;
        memset(buffer_answer, 0, sizeof(buffer_answer));
    }

    vector<string> HTTP_code = SplitString(*received, "\r\n");
    if (HTTP_code.size() < 1)
        return "bad answer from server";

    return HTTP_code.at(0);
}

void Cleanup(SSL_CTX* ctx, int* sock, SSL* ssl)
{
    SSL_free(ssl); // release connection state
    close(*sock); // close socket
    SSL_CTX_free(ctx); // release context
    EVP_cleanup();
    ERR_free_strings();
}

bool IsWhiteSpaceOrEmpty(const string& s)
{
    return all_of(s.begin(), s.end(), [](unsigned char c) { return isspace(c); });
}

vector<string> SplitString(string str, string delimiter)
{
    size_t position = 0;
    string token;
    vector<string> list;
    while ((position = str.find(delimiter)) != string::npos) {
        token = str.substr(0, position);
        if (IsWhiteSpaceOrEmpty(token) == false)
            list.push_back(token);
        str.erase(0, position + delimiter.size());
    }
    if (IsWhiteSpaceOrEmpty(str) == false)
        list.push_back(str);
    return list;
}

vector<string> SplitArrayOfJSON(string array)
{
    unsigned int num_of_brackets = 0;
    unsigned int l_counter = 0;
    unsigned int r_counter = 0;
    size_t position = 0;
    string token;
    vector<string> list;

    for (size_t i = 0; i < array.size(); i++)
        if (array[i] == '{' || array[i] == '}')
            num_of_brackets++;

    while (true) {
        while (array[position] != '{' && array[position] != '}') {
            if (l_counter == r_counter) {
                array.erase(array.begin());
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
            token = array.substr(0, position);
            if (IsWhiteSpaceOrEmpty(token) == false)
                list.push_back(token);
            if (l_counter + r_counter == num_of_brackets)
                break;
            array.erase(0, position);
            position = 0;
            continue;
        }
        position++;
    }

    return list;
}

int main(int argc, char** argv)
{
    signal(SIGINT, SIGINTHandler);

    /***** PARSING OF THE ARGUMENTS *****/
    char access_token[512];
    memset(access_token, 0, sizeof(access_token));
    ParseOpt(argc, argv, access_token);

    SSL_CTX* ctx;
    int sock;
    SSL* ssl;

    ctx = InitCTX();
    string socket_error = OpenConnection(&sock, "discord.com", HTTPS);
    if (socket_error.size()) {
        SSL_CTX_free(ctx);
        EVP_cleanup();
        ERR_free_strings();
        ErrExit(EXIT_FAILURE, socket_error);
    }
    ssl = SSL_new(ctx); /* create new SSL connection state */
    if (SSL_set_fd(ssl, sock) == 0) { /* attach the socket descriptor */
        ERR_print_errors_fp(stderr);
        Cleanup(ctx, &sock, ssl);
        ErrExit(EXIT_FAILURE, "SSL_set_fd() failed");
    }
    if (SSL_connect(ssl) == -1) { /* perform the connection */
        ERR_print_errors_fp(stderr);
        Cleanup(ctx, &sock, ssl);
        ErrExit(EXIT_FAILURE, "SSL_connect() failed");
    }

#ifdef DEBUG
    cout << "* Successfully connected with " << SSL_get_cipher(ssl) << " encryption..." << endl;
#endif

    char buffer_request[BUFFER];
    string received;
    string guild_id;
    string channel;
    string channel_id;
    string last_message_id;
    string HTTP_code;
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

        SSL_write(ssl, buffer_request, strlen(buffer_request)); /* encrypt & send message */
        HTTP_code = SSLReadAnswer(ssl, &received);
        if (HTTP_code.compare("HTTP/1.1 200 OK") != 0) {
            Cleanup(ctx, &sock, ssl);
            ErrExit(EXIT_FAILURE, HTTP_code);
        }

#ifdef DEBUG
        cout << endl
             << "* Bot guilds received..." << endl;
#endif

        if (regex_search(received, match, r_id_whole)) {
            if (match.size() > 2) {
                Cleanup(ctx, &sock, ssl);
                ErrExit(EXIT_FAILURE, "BOT is member of more than one Discord guild");
            }
            guild_id += match[0];
            if (regex_search(guild_id, match, r_id_num)) {
                guild_id.clear();
                guild_id += match[0];
            }
        } else {
            Cleanup(ctx, &sock, ssl);
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

        SSL_write(ssl, buffer_request, strlen(buffer_request)); /* encrypt & send message */
        HTTP_code = SSLReadAnswer(ssl, &received);
        if (HTTP_code.compare("HTTP/1.1 200 OK") != 0) {
            Cleanup(ctx, &sock, ssl);
            ErrExit(EXIT_FAILURE, HTTP_code);
        }

#ifdef DEBUG
        cout << endl
             << "* Guild channels received..." << endl;
#endif

        if (regex_search(received, match, r_isa_bot_channel)) {
            if (match.size() > 3) {
                Cleanup(ctx, &sock, ssl);
                ErrExit(EXIT_FAILURE, "there is more than 1 \"isa-bot\" channel in this Discord guild");
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
            Cleanup(ctx, &sock, ssl);
            ErrExit(EXIT_FAILURE, "there is no \"isa-bot\" channel in this Discord guild");
        }

#ifdef DEBUG
        cout << "   - \"isa-bot\" channel ID: " << channel_id << endl;
#endif
    }

    while (true) {
        if (!keep_running)
            break;

        memset(buffer_request, 0, sizeof(buffer_request));

        strcpy(buffer_request, "GET /api/v6/channels/");
        strcat(buffer_request, channel_id.c_str());
        strcat(buffer_request, "/messages?after=");
        strcat(buffer_request, last_message_id.c_str());
        strcat(buffer_request, " HTTP/1.1\r\nHost: discord.com\r\nAuthorization: Bot ");
        strcat(buffer_request, access_token);
        strcat(buffer_request, "\r\n\r\n");

        SSL_write(ssl, buffer_request, strlen(buffer_request)); /* encrypt & send message */
        HTTP_code = SSLReadAnswer(ssl, &received);
        if (HTTP_code.compare("HTTP/1.1 200 OK") != 0) {
            Cleanup(ctx, &sock, ssl);
            ErrExit(EXIT_FAILURE, HTTP_code);
        }

#ifdef DEBUG
        cout << endl
             << "* Trying to receive new messages from \"isa-bot\" channel..." << endl;
#endif

        string all_messages;
        string username;
        string content;
        string attachment;

        vector<string> received_body = SplitString(received, "\r\n\r\n");
        if (received_body.size() != 2) {
            Cleanup(ctx, &sock, ssl);
            ErrExit(EXIT_FAILURE, "bad answer from server");
        }
        all_messages += received_body.at(1);

        if (all_messages.compare("[]") == 0) { // no new messages
            if (!keep_running)
                break;
            usleep(SLEEP);
            continue;
        }

        vector<string> splitted_messages = SplitArrayOfJSON(all_messages);
        if (splitted_messages.size() < 1) {
            Cleanup(ctx, &sock, ssl);
            ErrExit(EXIT_FAILURE, "bad answer from server");
        }

#ifdef DEBUG
        cout << "# " << splitted_messages.size() << " new " << (splitted_messages.size() == 1 ? "message" : "messages")
             << " received..." << endl;
#endif

        if (regex_search(splitted_messages.at(0), match, r_id_whole)) {
            last_message_id.clear();
            last_message_id += match[0];
            if (regex_search(last_message_id, match, r_id_num)) {
                last_message_id.clear();
                last_message_id += match[0];
            }
        }

        for (int i = splitted_messages.size() - 1; i >= 0; i--) {
            if (!keep_running)
                break;
            username.clear();
            content.clear();
            attachment.clear();

            if (regex_search(splitted_messages.at(i), match, r_message_username)) {
                username += match[0];
                vector<string> splitted_username = SplitString(username, "\", \"avatar\":");
                if (splitted_username.size() < 1) {
                    Cleanup(ctx, &sock, ssl);
                    ErrExit(EXIT_FAILURE, "bad answer from server");
                }
                username.clear();
                username += splitted_username.at(0);

                splitted_username = SplitString(username, "\"username\": \"");
                if (splitted_username.size() != 1) {
                    Cleanup(ctx, &sock, ssl);
                    ErrExit(EXIT_FAILURE, "bad answer from server");
                }
                username.clear();
                username += splitted_username.at(0);
            }

            if (string::npos != username.find("bot") || string::npos != username.find("BOT")) { //if bot is a substring in username username => continue
#ifdef DEBUG
                cout << "isa-bot - Message from bot, skipping..." << endl;
#endif
                continue;
            }

            if (regex_search(splitted_messages.at(i), match, r_message_content)) {
                content += match[0];
                vector<string> splitted_content = SplitString(content, "\"content\": \"");
                if (splitted_content.size() != 1) {
                    Cleanup(ctx, &sock, ssl);
                    ErrExit(EXIT_FAILURE, "bad answer from server");
                }
                content.clear();
                content += splitted_content.at(0);

                splitted_content = SplitString(content, "\", \"channel_id\":");
                content.clear();
                switch (splitted_content.size()) {
                case 0:
                    break;
                case 1:
                    content += splitted_content.at(0);
                    break;
                default:
                    Cleanup(ctx, &sock, ssl);
                    ErrExit(EXIT_FAILURE, "bad answer from server");
                    break;
                }
            }

            if (flag_verbose)
                cout << "isa-bot - " << username << ": " << content << endl;

            if (regex_search(splitted_messages.at(i), match, r_message_url)) {
                attachment += match[0];
                vector<string> splitted_url = SplitString(attachment, "\"url\": \"");
                if (splitted_url.size() != 1) {
                    Cleanup(ctx, &sock, ssl);
                    ErrExit(EXIT_FAILURE, "bad answer from server");
                }
                attachment.clear();
                attachment += splitted_url.at(0);

                splitted_url = SplitString(attachment, "\", \"proxy_url\":");
                if (splitted_url.size() != 1) {
                    Cleanup(ctx, &sock, ssl);
                    ErrExit(EXIT_FAILURE, "bad answer from server");
                }
                attachment.clear();
                attachment += splitted_url.at(0);

                if (IsWhiteSpaceOrEmpty(content) == false)
                    content += " ";
                content += attachment;
            }

            //send echo to received message
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

            SSL_write(ssl, buffer_request, strlen(buffer_request)); /* encrypt & send message */

#ifdef DEBUG
            cout << "   - Echo to message sent..." << endl;
#endif

            HTTP_code = SSLReadAnswer(ssl, &received);
            if (HTTP_code.compare("HTTP/1.1 200 OK") != 0) {
                Cleanup(ctx, &sock, ssl);
                ErrExit(EXIT_FAILURE, HTTP_code);
            }

#ifdef DEBUG
            cout << "   - Answer: " << HTTP_code << endl;
#endif
        }
        if (!keep_running)
            break;
        usleep(SLEEP);
    }
    Cleanup(ctx, &sock, ssl);

#ifdef DEBUG
    cout << endl
         << "* CLEANUP..." << endl
         << "* EXIT SUCCESS..." << endl;
#endif

    return EXIT_SUCCESS;
}
/*** End of file isabot.cpp ***/
