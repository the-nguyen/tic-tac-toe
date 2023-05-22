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

const char *greetings[] = {
    "Succesfully connected to server. You are player 1, who places symbol X on the board",
    "Succesfully connected to server. You are player 2, who places symbol O on the board"
};
const char mym_msg[] = { MYM };
const char connection_failed_msg[] = { END, 0xff };
const char invalid_move_msg[] = { TXT, 'I', 'n', 'v', 'a', 'l', 'i', 'd', ' ', 'm', 'o', 'v', 'e', '.', ' ', 'P', 'l', 'e', 'a', 's', 'e', ' ', 't', 'r', 'y', ' ', 'a', 'g', 'a', 'i', 'n', '!', '\0' };

void listen_to_port(char **buf, struct sockaddr_in *from, socklen_t *from_len) {
    int buf_size = INCREMENT_SIZE;
    *buf = malloc(buf_size);
    memset(from, 0, sizeof(*from));
    *from_len = sizeof(*from);
    while (recvfrom(sock_fd, *buf, buf_size, MSG_PEEK, (struct sockaddr *)from, from_len) == buf_size) {
        buf_size += INCREMENT_SIZE;
        *buf = realloc(*buf, buf_size);
    }
    recvfrom(sock_fd, *buf, buf_size, 0, (struct sockaddr *)from, from_len);
}

void print_board(char board[3][3]) {
    int i, j;
    printf("+-+-+-+\n");
    for (i = 0; i < 3; i++) {
	    printf("|");
        for (j = 0; j < 3; j++) {
            char player = board[i][j];
            char symbol;
            if (player == 1) symbol = 'X';
            else if (player == 2) symbol = 'O';
            else symbol = ' ';
            printf("%c|", symbol);
        }
        printf("\n+-+-+-+\n");
    }
}

/**
 * @brief Validate a move and update @param board accordingly.
 * @param buf The received move message.
 * @param board The board of the game.
 * @param player The person who made this move.
 * @return 0: valid move.
 *         1: invalid move.
*/
int execute_move(const char *const buf, char board[3][3], unsigned char player) {
    char column = buf[1], row = buf[2];
    if (row < 0 || row > 2 || column < 0 || column > 2 || board[(unsigned char)row][(unsigned char)column]) {
        printf("Received invalid move from player %d!\n", (int)player);
        printf("Waiting for player %d to make a move again ...\n", (int)player);
        return 1;
    } else {
        board[(unsigned char)row][(unsigned char)column] = (char)player;
        printf("Player %d made a move at column %d and row %d\n", (int)player, (int)column, (int)row);
        return 0;
    }
}

/**
 * @brief Check the status of a game.
 * @param board The board of the game.
 * @return 0: draw.
 *         1: player 1 won.
 *         2: player 2 won.
 *         3: the game is not over.
*/
char check_game_status(char board[3][3]) {
    int i, j;
    for (i = 0; i < 3; i++) {
        if (board[i][0] && board[i][0] == board[i][1] && board[i][1] == board[i][2]) return board[i][0];
        if (board[0][i] && board[0][i] == board[1][i] && board[1][i] == board[2][i]) return board[0][i];
    }
    if (board[0][0] && board[0][0] == board[1][1] && board[1][1] == board[2][2]) return board[1][1];
    if (board[0][2] && board[0][2] == board[1][1] && board[1][1] == board[2][0]) return board[1][1];
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            if (board[i][j] == 0) return 3;
        }
    }
    return 0;
}

void create_fyi_msg(char **buf, size_t *len, char board[3][3]) {
    char *fyi_msg = malloc(2 + 3 * 9);
    fyi_msg[0] = FYI;
    int i, j;
    int n = 0;
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            if (board[i][j] != 0) {
                fyi_msg[2 + 3 * n] = board[i][j];
                fyi_msg[3 + 3 * n] = j;
                fyi_msg[4 + 3 * n] = i;
                n++;
            }
        }
    }
    fyi_msg[1] = n;
    *len = 2 + 3 * n;
    *buf = realloc(fyi_msg, *len);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Missing argument. Please enter port number\n");
        return 1;
    }

    struct sockaddr_in serv;
    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = htonl(INADDR_ANY);
    if (sscanf(argv[1], "%hu", &serv.sin_port) == 0 || serv.sin_port > MAX_PORT) {
        fprintf(stderr, "Invalid port number\n");
        return 2;
    }
    serv.sin_port = htons(serv.sin_port);

    sock_fd = socket(serv.sin_family, SOCK_DGRAM, 0);
    printf("Socket created\n");

    if (bind(sock_fd, (struct sockaddr *)&serv, sizeof(serv))) {
        fprintf(stderr, "Could not bind socket: %s\n", strerror(errno));
        return 3;
    } else {
        printf("Bind to port %hu\n", ntohs(serv.sin_port));
    }

    printf("Waiting for connections ...\n");

    struct sockaddr_in players[2];
    socklen_t players_len[2];
    unsigned char n_connected_players = 0;
    while (1) {
        char *buf;
        struct sockaddr_in from;
        socklen_t from_len;
        listen_to_port(&buf, &from, &from_len);

        if (buf[0] == TXT && strcmp(buf + 1, "Hello") == 0) {
            players[n_connected_players] = from;
            players_len[n_connected_players] = from_len;
            ++n_connected_players;
            
            char *greeting_msg = malloc(1 + strlen(greetings[n_connected_players - 1]) + 1);
            greeting_msg[0] = TXT;
            strcpy(greeting_msg + 1, greetings[n_connected_players - 1]);
            sendto(sock_fd, greeting_msg, 1 + strlen(greetings[n_connected_players - 1]) + 1, 0, (struct sockaddr *)&from, from_len);
            free(greeting_msg);

            printf("%d player(s) connected\n", (int)n_connected_players);
            if (n_connected_players == 2) {
                break;
            }
        } else {
            fprintf(stderr, "Received invalid message from unidentified client. Ignoring it ...\n");
        }
    }

    printf("Starting game ...\n");
    char board[3][3];
    memset(board, 0, 3 * 3);
    print_board(board);
    unsigned char current_player_id = 0;
    while (1) {
        char *fyi_msg;
        size_t fyi_msg_len;
        create_fyi_msg(&fyi_msg, &fyi_msg_len, board);
        sendto(sock_fd, fyi_msg, fyi_msg_len, 0, (struct sockaddr *)(players + current_player_id), players_len[current_player_id]);

        sendto(sock_fd, mym_msg, 1, 0, (struct sockaddr *)(players + current_player_id), players_len[current_player_id]);

	    printf("Waiting for player %d to make a move ...\n", (int)(current_player_id + 1));
        while (1) {
            char *buf;
            struct sockaddr_in from;
            socklen_t from_len;
            listen_to_port(&buf, &from, &from_len);
            
            if (buf[0] == MOV && from.sin_addr.s_addr == players[current_player_id].sin_addr.s_addr && from.sin_port == players[current_player_id].sin_port) {
                int invalid = execute_move(buf, board, current_player_id + 1);
                
                if (invalid) {
                    sendto(sock_fd, invalid_move_msg, strlen(invalid_move_msg) + 1, 0, (struct sockaddr *)(players + current_player_id), players_len[current_player_id]);
                    sendto(sock_fd, mym_msg, 1, 0, (struct sockaddr *)(players + current_player_id), players_len[current_player_id]);
                } else {
        	        print_board(board);
                    char game_status = check_game_status(board);
                    if (game_status < 3) {
                        char end_msg[2];
                        end_msg[0] = END;
                        end_msg[1] = game_status;
                        sendto(sock_fd, end_msg, 2, 0, (struct sockaddr *)players, players_len[0]);
                        sendto(sock_fd, end_msg, 2, 0, (struct sockaddr *)(players + 1), players_len[1]);

			            printf("Game over. ");
                        if (game_status) {
                            printf("Player %d won\n", (int)game_status);
                        } else {
                            printf("Draw\n");
                        }

                        close(sock_fd);
                        return 0;
                    } else {
                        current_player_id ^= 1;
                        break;
                    }
                }
            } else if (buf[0] == TXT && strcmp(buf + 1, "Hello") == 0) {
                sendto(sock_fd, connection_failed_msg, 2, 0, (struct sockaddr *)&from, from_len);
                fprintf(stderr, "Received late connection. Sending connection failed ...\n");
            } else {
                fprintf(stderr, "Received invalid message from unidentified client. Ignoring it ...\n");
            }
        }
    }

    return 0;
}
