// Alexa Hoover, Computer Networks 10/23/24

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BOARD_SIZE 3

// Function prototypes
void initializeBoard(char board[BOARD_SIZE][BOARD_SIZE]);
void printBoard(char board[BOARD_SIZE][BOARD_SIZE]);
int isValidMove(char board[BOARD_SIZE][BOARD_SIZE], int row, int col);
void makeMove(char board[BOARD_SIZE][BOARD_SIZE], int row, int col, char player);
int checkWin(char board[BOARD_SIZE][BOARD_SIZE], char player);
int isBoardFull(char board[BOARD_SIZE][BOARD_SIZE]);

// Log to store the messages
char messageLog[1024 * 10] = "";

void logMessage(const char *message) {
    strcat(messageLog, message);
    strcat(messageLog, "\n");
}

// Function to initialize the board with empty spaces
void initializeBoard(char board[BOARD_SIZE][BOARD_SIZE]) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            board[i][j] = ' ';
        }
    }
}

// Function to print the board
void printBoard(char board[BOARD_SIZE][BOARD_SIZE]) {
    printf("-------\n");
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            printf("|%c", board[i][j]);
        }
        printf("|\n-------\n");
    }
}

// Function to check if a move is valid (i.e., the spot is empty)
int isValidMove(char board[BOARD_SIZE][BOARD_SIZE], int row, int col) {
    return (row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE && board[row][col] == ' ');
}

// Function to place a player's mark on the board
void makeMove(char board[BOARD_SIZE][BOARD_SIZE], int row, int col, char player) {
    if (isValidMove(board, row, col)) {
        board[row][col] = player;
    }
}

// Function to check if a player has won
int checkWin(char board[BOARD_SIZE][BOARD_SIZE], char player) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        if ((board[i][0] == player && board[i][1] == player && board[i][2] == player) || 
            (board[0][i] == player && board[1][i] == player && board[2][i] == player)) {
            return 1;
        }
    }

    if ((board[0][0] == player && board[1][1] == player && board[2][2] == player) || 
        (board[0][2] == player && board[1][1] == player && board[2][0] == player)) {
        return 1;
    }
    return 0;
}

int isBoardFull(char board[BOARD_SIZE][BOARD_SIZE]) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (board[i][j] == ' ') {
                return 0;
            }
        }
    }
    return 1;
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    char board[BOARD_SIZE][BOARD_SIZE];
    int row, col;
    char currentPlayer;
    char replay_choice[10];
    int replay = 1;

    // Create server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Define server address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    // Bind the socket to the IP/Port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Waiting for connections...\n");

    // Accept a connection
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("Accept failed");
        exit(EXIT_FAILURE);
    }

    while (replay) {
        initializeBoard(board);
        currentPlayer = 'X';
        replay = 0;

        while (1) {
            printf("Current board state:\n");
            printBoard(board);

            // Receive move from the client
            memset(buffer, 0, sizeof(buffer));
            read(new_socket, buffer, 1024);
            sscanf(buffer, "%d %d", &row, &col);  
            printf("Move received from client: %d %d\n", row, col);
            logMessage(buffer);

            if (isValidMove(board, row, col)) {
                makeMove(board, row, col, currentPlayer);

                // Send updated board
                char boardStr[1024];
                snprintf(boardStr, sizeof(boardStr),
                         "-------\n|%c|%c|%c|\n-------\n|%c|%c|%c|\n-------\n|%c|%c|%c|\n-------\n",
                         board[0][0], board[0][1], board[0][2],
                         board[1][0], board[1][1], board[1][2],
                         board[2][0], board[2][1], board[2][2]);
                send(new_socket, boardStr, strlen(boardStr), 0);
                logMessage(boardStr);

                // Check for win or tie
                if (checkWin(board, currentPlayer)) {
                    printf("Player %c wins!\n", currentPlayer);
                    send(new_socket, "WIN", strlen("WIN"), 0);
                    logMessage("WIN");
                    break;
                } else if (isBoardFull(board)) {
                    printf("The game is a tie!\n");
                    send(new_socket, "TIE", strlen("TIE"), 0);
                    logMessage("TIE");
                    break;
                }

                currentPlayer = (currentPlayer == 'X') ? 'O' : 'X';
            } else {
                send(new_socket, "INVALID", strlen("INVALID"), 0);
                logMessage("INVALID");
            }
        }

        printf("Ask client for replay...\n");
        send(new_socket, "Replay? (yes/no)", strlen("Replay? (yes/no)"), 0);
        logMessage("Replay? (yes/no)");

        memset(replay_choice, 0, sizeof(replay_choice));
        read(new_socket, replay_choice, 10);
        logMessage(replay_choice);

        if (strncmp(replay_choice, "yes", 3) == 0) {
            replay = 1;
            printf("Client chose to replay\n");
        } else {
            printf("Client chose to end the game\n");
            break;
        }
    }

    // After game ends, print message log
    printf("\nGame Over. Message Log:\n%s\n", messageLog);

    // Close the socket
    close(new_socket);
    close(server_fd);
    return 0;
}
