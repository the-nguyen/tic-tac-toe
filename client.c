#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>

#define MAX_PORT 65535

#define FYI 0x01
#define MYM 0x02
#define END 0x03
#define TXT 0x04
#define MOV 0x05
#define LFT 0x06

#define INCREMENT_SIZE 100

int sock_fd;
struct sockaddr_in to;
const char init_msg[] = { TXT, 'H', 'e', 'l', 'l', 'o', '\0' };
char mov_msg[] = { MOV, 0, 0 };

/**
 * @brief Receive message from server.
 * Check if a message is sent from the same server over time by looking through the from.sin.s_addr. 
 * @param buf pointer to the buffer will be used to store the message.
 * @return Nothing (void).

*/
void receive_from_server(char **buf) {
    int buf_size = INCREMENT_SIZE;
    *buf = malloc(buf_size);
    struct sockaddr_in from;
    memset(&from, 0, sizeof(from));
    socklen_t from_len = sizeof(from);
    while (1) {
        recvfrom(sock_fd, *buf, buf_size, MSG_PEEK, (struct sockaddr *)&from, &from_len);
        if (from.sin_addr.s_addr == to.sin_addr.s_addr && from.sin_port == to.sin_port) {
            break;
        } else {
            fprintf(stderr, "Received message not from server. Ignoring it ...\n");
            recvfrom(sock_fd, *buf, buf_size, 0, (struct sockaddr *)&from, &from_len);
        }
    }
    while (recvfrom(sock_fd, *buf, buf_size, MSG_PEEK, (struct sockaddr *)&from, &from_len) == buf_size) {
        buf_size += INCREMENT_SIZE;
        *buf = realloc(*buf, buf_size);
    }
    recvfrom(sock_fd, *buf, buf_size, 0, (struct sockaddr *)&from, &from_len);
}

/**
 * @brief Take input from user and send MYM message to server.
 * @return Nothing (void).
*/
void make_your_move() {
    printf("Now is your turn\n");
    while (1) {
        printf("Enter your selection for column: ");
        char *input = NULL;
        size_t input_size;
        getline(&input, &input_size, stdin);
        if (strlen(input) == 2 && (input[0] == '0' || input[0] == '1' || input[0] == '2')) {
            mov_msg[1] = input[0] - '0';
            break;
        } else {
            printf("Invalid input. Please enter only 0, 1 or 2\n");
        }
        free(input);
    }
    while (1) {
        printf("Enter your selection for row: ");
        char *input = NULL;
        size_t input_size;
        getline(&input, &input_size, stdin);
        if (strlen(input) == 2 && (input[0] == '0' || input[0] == '1' || input[0] == '2')) {
            mov_msg[2] = input[0] - '0';
            break;
        } else {
            printf("Invalid input. Please enter only 0, 1 or 2\n");
        }
        free(input);
    }
    sendto(sock_fd, mov_msg, 3, 0, (struct sockaddr *)&to, sizeof(to));
    printf("Wait for server to validate your move/for your opponent to make a move ...\n");
}

/**
* @brief Process the FYI message from server then draw and print a 3x3 board for the user.
* @param buf Message FYI received from server.
* @return Nothing (void).
*/
void for_your_info(const char *const buf) {
    char board[3][3];
    memset(board, 0, 3 * 3);
    char n = buf[1];
    const char *const start = buf + 2;
    printf("%d filled position(s)\n", (int)n);
    int i, j;
    for (i = 0; i < n; i++) {
        char player = start[3 * i];
        char column = start[3 * i + 1];
        char row = start[3 * i + 2];
        board[(unsigned char)row][(unsigned char)column] = player;
    }
    printf("  0 1 2 \n");
    printf(" +-+-+-+\n");
    for (i = 0; i < 3; i++) {
        printf("%d|", i);
        for (j = 0; j < 3; j++) {
            char player = board[i][j];
            char symbol;
            if (player == 1) symbol = 'X';
            else if (player == 2) symbol = 'O';
            else symbol = ' ';
            printf("%c|", symbol);
        }
        printf("\n +-+-+-+\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Missing argument(s). Please enter IP address and port number\n");
        return 1;
    }
    // fill the server's information to the struct sockaddr_in.
    memset(&to, 0, sizeof(to));
    to.sin_family = AF_INET;
    if (inet_pton(AF_INET, argv[1], &to.sin_addr) == 0) {
        fprintf(stderr, "%s is not a valid IPv4 address. Please use IPv4 address only\n", argv[1]);
        return 2;
    }
    if (sscanf(argv[2], "%hu", &to.sin_port) == 0 || to.sin_port > MAX_PORT) {
        fprintf(stderr, "Invalid port number\n");
        return 3;
    }
    to.sin_port = htons(to.sin_port);
    
    sock_fd = socket(to.sin_family, SOCK_DGRAM, 0);
    printf("Socket created\n");
    // send the first hello message to connect to the server.
    sendto(sock_fd, init_msg, strlen(init_msg) + 1, 0, (struct sockaddr *)&to, sizeof(to));
    printf("Connecting to server %s:%hu ...\n", argv[1], ntohs(to.sin_port));

    while (1) {
        char *buf;
        receive_from_server(&buf);
        
        char msg_type = buf[0];
        if (msg_type == TXT) {
            printf("%s\n", buf + 1);
        } else if (msg_type == MYM) {
            make_your_move();
        } else if (msg_type == END) {
            unsigned char winner = (unsigned char)buf[1];
            if (winner == 255) {
                fprintf(stderr, "Could not connect to server\n");
                return 4;
            } else {
                printf("Game over. ");
                if (winner > 2) {
                    printf("Could not identify winner\n");
                } else if (winner == 0) {
                    printf("Draw\n");
                } else {
                    printf("Player %d won\n", (int)winner);
                }
            }
            break;
        } else if (msg_type == FYI) {
            for_your_info(buf);
        } else {
	        fprintf(stderr, "Received invalid message from server\n");
        }
        
        free(buf);
    }

    close(sock_fd);
    return 0;
}
