/**
 * @author Radoslav Grenčík, xgrenc00@stud.fit.vutbr.cz
 */

#include "isabot.hpp"

void errExit(int errnum, const char* err)
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

int main(int argc, char** argv)
{
    /***** PARSING OF THE ARGUMENTS *****/
    bool flag_verbose = false;
    bool flag_access_token = false;
    char access_token[100];

    int opt;

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
                errExit(BAD_OPTIONS, "bad option - option '-v|--verbose' declared more than once");
            break;
        case 't':
            if (flag_access_token == false) {
                flag_access_token = true;
                strcpy(access_token, optarg);
            } else
                errExit(BAD_OPTIONS, "bad option - option '-t <access_token>' declared more than once");
            break;
        case '?':
            errExit(BAD_OPTIONS, "");
            break;
        }
    }

    if (optind < argc) {
        fprintf(stderr, "Non-option arguments detected: ");
        while (optind < argc)
            fprintf(stderr, "%s ", argv[optind++]);
        fprintf(stderr, "\n");
        errExit(BAD_OPTIONS, "");
    }

    if (flag_access_token == false)
        errExit(BAD_OPTIONS, "bad option - option '-t <access_token>' is missing");

#ifdef DEBUG
    printf("Verbose: %s\n", flag_verbose ? "true" : "false");
    printf("Access token: %s\n", access_token);
#endif

    /***** CREATING THE SOCKET *****/

    int sock;
    // int msg_size;
    int i;
    socklen_t len;
    struct sockaddr_in local, server;
    char buffer[BUFFER];
    // uid_t uid;
    // struct passwd* uname;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) //create a client socket
        errExit(EXIT_FAILURE, "socket() failed");

    printf("Socket successfully created\n");
    // uid = getuid();
    // uname = getpwuid(uid);

    memset(&server, 0, sizeof(server)); // erase the server structure
    memset(&local, 0, sizeof(local)); // erase the local address structure

    struct hostent* server_ip;
    server_ip = gethostbyname("discord.com");
    if (server_ip == NULL)
        errExit(EXIT_FAILURE, "IP not found");
    printf("IP ADDRESS: %s\n", inet_ntoa(*((struct in_addr*)server_ip->h_addr_list[0])));

    server.sin_addr.s_addr = inet_addr(inet_ntoa(*((struct in_addr*)server_ip->h_addr_list[0]))); // set the server address
    server.sin_family = AF_INET;
    server.sin_port = htons(443); // set the server port (network byte order)

    // connect to the remote server
    // client port and IP address are assigned automatically by the operating system
    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) == -1)
        errExit(EXIT_FAILURE, "connect() failed");

    // obtain the local IP address and port using getsockname()
    len = sizeof(local);
    if (getsockname(sock, (struct sockaddr*)&local, &len) == -1)
        errExit(EXIT_FAILURE, "getsockname() failed");

    printf("Client successfully connected from %s, port %d (%d) to %s, port %d (%d)\n", inet_ntoa(local.sin_addr), ntohs(local.sin_port), local.sin_port, inet_ntoa(server.sin_addr), ntohs(server.sin_port), server.sin_port);

    strcpy(buffer, "GET /api/v6/channels/760873107513933864/messages HTTP/1.1\r\nHost: discord.com\r\nAuthorization: Bot NzYwNjE5OTA0NDE5NzU4MDgy.X3OsfA.6XCY4sabX63d8hmm5s0vgElHA8o\r\n\r\n");

    i = write(sock, buffer, strlen(buffer));
    if (i == -1)
        errExit(EXIT_FAILURE, "initial write() failed");

    if ((i = read(sock, buffer, BUFFER)) == -1) // read an initial string
        errExit(EXIT_FAILURE, "initial read() failed");
    else
        printf("%.*s\n", i, buffer);

    close(sock);
    printf("Closing client socket...\n");
    return EXIT_SUCCESS;
}
/*** End of file isabot.cpp ***/
