/**
 * @author Radoslav Grenčík, xgrenc00@stud.fit.vutbr.cz
 */

#include "isabot.hpp"

void ErrExit(int errnum, const char* err)
{
    if (strlen(err))
        fprintf(stderr, "ERROR: %s.\n", err);
    switch (errnum) {
    case BAD_OPTIONS:
        fprintf(stderr, "Try 'isabot -h|--help' for more information.\n");
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
            printf("HELP WILL BE DISPLAYED HERE...\n");
            exit(EXIT_SUCCESS);
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
        fprintf(stderr, "Non-option arguments detected: ");
        while (optind < argc)
            fprintf(stderr, "%s ", argv[optind++]);
        fprintf(stderr, "\n");
        ErrExit(BAD_OPTIONS, "");
    }

    if (flag_access_token == false)
        ErrExit(BAD_OPTIONS, "bad option - option '-t <access_token>' is missing");
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

    if (setsockopt (sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
        ErrExit(EXIT_FAILURE, "setsockopt failed");

    if (setsockopt (sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
        ErrExit(EXIT_FAILURE, "setsockopt failed");

    struct hostent* he;
    he = gethostbyname(hostname);
    if (he == NULL)
        ErrExit(EXIT_FAILURE, "IP not found");

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
    method = TLS_client_method();  /* Create new client-method instance */
    ctx = SSL_CTX_new(method);   /* Create new context */
    if (ctx == NULL) {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}

int main(int argc, char** argv)
{
    /***** PARSING OF THE ARGUMENTS *****/
    char access_token[512];
    ParseOpt(argc, argv, access_token);

#ifdef DEBUG
    printf("Verbose: %s\n", flag_verbose ? "true" : "false");
    printf("Access token: %s\n", access_token);
#endif

    SSL_CTX *ctx;
    int sock;
    SSL *ssl;

    SSL_library_init();
    ctx = InitCTX();
    sock = OpenConnection("discord.com", HTTPS);
    ssl = SSL_new(ctx);      /* create new SSL connection state */
    SSL_set_fd(ssl, sock);    /* attach the socket descriptor */
    if (SSL_connect(ssl) == -1)   /* perform the connection */
        ERR_print_errors_fp(stderr);
    else {
        printf("* Connected with %s encryption\n", SSL_get_cipher(ssl));

        char buffer_request[BUFFER];
        char buffer_answer[BUFFER];
        memset(buffer_request, 0, sizeof(buffer_request));
        memset(buffer_answer, 0, sizeof(buffer_answer));

        strcpy(buffer_request, "GET /api/v6/users/@me/guilds HTTP/1.1\r\nHost: discord.com\r\nAuthorization: Bot ");
        strcat(buffer_request, access_token);
        strcat(buffer_request, "\r\n\r\n");

        SSL_write(ssl, buffer_request, strlen(buffer_request));   /* encrypt & send message */

        string received;
        printf("\n* Received:\n\n");
        int bytes = 0;
        while(bytes != -1) {
            bytes = SSL_read(ssl, buffer_answer, sizeof(buffer_answer) - 1); /* get reply & decrypt */
            received += buffer_answer;
            memset(buffer_answer, 0, sizeof(buffer_answer));
        }

        const regex r_id_whole("(\"id\": \"[0-9]+\")");
        const regex r_id_num("([0-9]+)");
        smatch sm_id_whole;
        smatch sm_id_num;
        string whole_id;
        string guild_id;

        if (regex_search(received, sm_id_whole, r_id_whole)) {
            if (sm_id_whole.size() < 1 || sm_id_whole.size() > 2)
                ErrExit(EXIT_FAILURE, "BOT must be member of exactly one Discord server");
            whole_id += sm_id_whole[0];
            if (regex_search(whole_id, sm_id_num, r_id_num))
                guild_id += sm_id_num[0];
        }

        memset(buffer_request, 0, sizeof(buffer_request));
        memset(buffer_answer, 0, sizeof(buffer_answer));

        strcpy(buffer_request, "GET /api/v6/guilds/");
        strcat(buffer_request, guild_id.c_str());
        strcat(buffer_request, "/channels HTTP/1.1\r\nHost: discord.com\r\nAuthorization: Bot ");
        strcat(buffer_request, access_token);
        strcat(buffer_request, "\r\n\r\n");

        SSL_write(ssl, buffer_request, strlen(buffer_request));   /* encrypt & send message */

        received.clear();
        printf("\n* Received:\n\n");
        bytes = 0;
        while(bytes != -1) {
            bytes = SSL_read(ssl, buffer_answer, sizeof(buffer_answer) - 1); /* get reply & decrypt */
            received += buffer_answer;
            memset(buffer_answer, 0, sizeof(buffer_answer));
        }

        const regex r_isa_bot_channel("(\"id\": \"[0-9]+\", \"last_message_id\": (null|\"[0-9]+\"), \"type\": [0-9]+, \"name\": \"isa-bot\")");
        smatch sm_isa_bot_channel;
        smatch sm_isa_bot_whole_channel_id;
        smatch sm_isa_bot_channel_id;
        string channel;
        string whole_channel_id;
        string channel_id;

        if (regex_search(received, sm_isa_bot_channel, r_isa_bot_channel)) {
            if (sm_isa_bot_channel.size() < 1 || sm_isa_bot_channel.size() > 3)
                ErrExit(EXIT_FAILURE, "there is wrong number of \"isa-bot\" channels in specified Discord server");
            channel += sm_isa_bot_channel[0];
            if (regex_search(channel, sm_isa_bot_whole_channel_id, r_id_whole))
                whole_channel_id += sm_isa_bot_whole_channel_id[0];
            if (regex_search(whole_channel_id, sm_isa_bot_channel_id, r_id_num))
                channel_id += sm_isa_bot_channel_id[0];
        }

        cout << channel_id << endl;

        SSL_free(ssl);        /* release connection state */
        printf("\n* Connection released...\n");
    }

    close(sock);         /* close socket */
    printf("\n* Socket closed...\n");
    SSL_CTX_free(ctx);        /* release context */
    printf("\n* Context released...\n");
    return EXIT_SUCCESS;
}
/*** End of file isabot.cpp ***/
