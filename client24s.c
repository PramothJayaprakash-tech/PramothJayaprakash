#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

//above i added header

#define PORT 15000  // Port for connecting to Smain server here
#define BUFFER_SIZE 1024

//this function i had used for implentation purpose

void send_command(int sock, const char *command);
void upload_file(int sock, const char *filename, const char *destination);
void delete_file(int sock, const char *filename);
void download_file(int sock, const char *filename);
void create_tar(int sock, const char *filetype);
void display_menu();

//main function

int main() {
    int client_sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    // Creating socket
    client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sock < 0) {
        perror("Socket creation not successfull");
        exit(EXIT_FAILURE);
    }

    // Server address seting up
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connecting to the server
    if (connect(client_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection not successful");
        close(client_sock);
        exit(EXIT_FAILURE);
    }

    printf("Connected to Smain server\n");

    while (1) {
        display_menu();
        printf("client24s$ ");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = 0;  // Remove trailing newline

        if (strncmp(buffer, "ufile", 5) == 0) {
            char filename[BUFFER_SIZE], destination[BUFFER_SIZE];
            if (sscanf(buffer, "ufile %s %s", filename, destination) == 2) {
                upload_file(client_sock, filename, destination);
            } else {
                printf("invalid input format for ufile. Usage: ufile <filename> <destination>\n");
            }
        } else if (strncmp(buffer, "rmfile", 6) == 0) {
            char filename[BUFFER_SIZE];
            if (sscanf(buffer, "rmfile %s", filename) == 1) {
                delete_file(client_sock, filename);
            } else {
                printf("Invalid input format for rmfile. Usage: rmfile <filename>\n");
            }
        } else if (strncmp(buffer, "dfile", 5) == 0) {
            char filename[BUFFER_SIZE];
            if (sscanf(buffer, "dfile %s", filename) == 1) {
                download_file(client_sock, filename);
            } else {
                printf("Invalid command input for dfile. Usage: dfile <filename>\n");
            }
        } else if (strncmp(buffer, "dtar", 4) == 0) {
            char filetype[BUFFER_SIZE];
            if (sscanf(buffer, "dtar %s", filetype) == 1) {
                create_tar(client_sock, filetype);
            } else {
                printf("Invalid command input for dtar. Usage: dtar <filetype>\n");
            }
        } else if (strncmp(buffer, "exit", 4) == 0) {
            printf("Exiting client...\n");
            break;
        } else {
            printf("Invalid input. Please try again.\n");
        }
    }

    close(client_sock);
    return 0;
}

//sending command

void send_command(int sock, const char *command) {
    char buffer[BUFFER_SIZE];

    printf("Client sending command: %s\n", command);
    write(sock, command, strlen(command));
    bzero(buffer, BUFFER_SIZE);
    ssize_t bytes_read = read(sock, buffer, BUFFER_SIZE);
    if (bytes_read > 0) {
        printf("%s\n", buffer);
        if (strstr(buffer, "Error:") != NULL) {
            printf("Server got an error: %s\n", buffer);
        }
    } else {
        printf("No response from server or may be connection might closed.\n");
    }
}

//upload file

void upload_file(int sock, const char *filename, const char *destination) {
    char command[BUFFER_SIZE];
    snprintf(command, sizeof(command), "ufile %s %s", filename, destination);
    send_command(sock, command);
}

//delete file

void delete_file(int sock, const char *filename) {
    char command[BUFFER_SIZE];
    snprintf(command, sizeof(command), "rmfile %s", filename);
    send_command(sock, command);
}

//download file

void download_file(int sock, const char *filename) {
    char command[BUFFER_SIZE];
    snprintf(command, sizeof(command), "dfile %s", filename);
    send_command(sock, command);

    // Creating a local file to save the downloaded content
    FILE *file = fopen(filename, "wb");  // Opening in binary mode for downloading
    if (!file) {
        perror("not able to opening file for download");
        return;
    }

    char buffer[BUFFER_SIZE];
    bzero(buffer, BUFFER_SIZE);

    // Reading from the socket and writing to the file
    ssize_t bytes_read;
    while ((bytes_read = read(sock, buffer, BUFFER_SIZE)) > 0) {
        if (strstr(buffer, "EOF") != NULL) {
            printf("EOF signal got, download complete.\n");
            break;
        }
        size_t bytes_written = fwrite(buffer, 1, bytes_read, file);
        if (bytes_written < bytes_read) {
            perror("not able to write to file");
            break;
        }
        printf("Bytes read: %zd, Bytes written: %zu\n", bytes_read, bytes_written);
        bzero(buffer, BUFFER_SIZE);
    }

    fclose(file);
    printf("File %s downloaded successfully.\n", filename);
}

//create tar

void create_tar(int sock, const char *filetype) {
    char command[BUFFER_SIZE];
    snprintf(command, sizeof(command), "dtar %s", filetype);
    send_command(sock, command);

    char tar_filename[BUFFER_SIZE];
    snprintf(tar_filename, sizeof(tar_filename), "%sfiles.tar", filetype + 1);  // Remove the leading dot from the filetype

    FILE *file = fopen(tar_filename, "wb");
    if (!file) {
        perror("not  opening tar file for download");
        return;
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    size_t total_bytes_written = 0;

    while ((bytes_read = read(sock, buffer, BUFFER_SIZE)) > 0) {
        if (strstr(buffer, "EOF") != NULL) {
            printf("EOF signal received, download complete.\n");
            break;
        }
        size_t bytes_written = fwrite(buffer, 1, bytes_read, file);
        total_bytes_written += bytes_written;

        printf("Bytes read: %zd, Bytes written: %zu, Total written: %zu\n", bytes_read, bytes_written, total_bytes_written);
        bzero(buffer, BUFFER_SIZE);
    }

    fclose(file);
    printf("Tar file %s downloaded successfully. Total bytes: %zu\n", tar_filename, total_bytes_written);
}

//display menu

void display_menu() {
    printf("Available commands:\n");
    printf("1. ufile <filename> <destination> - Upload a file\n");
    printf("2. rmfile <filename> - Delete a file\n");
    printf("3. dfile <filename> - Download a file\n");
    printf("4. dtar <filetype> - Download a tar archive of a filetype\n");
    printf("5. exit - Exit the client\n");
}









