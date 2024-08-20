#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <errno.h>

//header file added above

#define PORT 15002  // Port for Stext server given here
#define BUFFER_SIZE 1024

//these functions i will be implmenting

void handle_request(int client_sock);
void save_txt_file(int client_sock, char *filename, char *destination_path);
void delete_txt_file(int client_sock, char *filename);
void send_txt_file(int client_sock, char *filename);
void send_message(int client_sock, const char *message);
void create_tar(int client_sock, char *filetype);
int mkdir_recursive(const char *path, mode_t mode);

//main function

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len;

    // Creating socket below
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Socket creation not successfull");
        exit(EXIT_FAILURE);
    }

    // Binding socket to port below
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind not successfull");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // Listening for connections
    if (listen(server_sock, 5) < 0) {
        perror("Listening not successful");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("Stext server is listening on port %d\n", PORT);

    while (1) {
        addr_len = sizeof(client_addr);
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_len);
        if (client_sock < 0) {
            perror("Accept failed");
            continue;
        }

        printf("Stext server got a request\n");

        // Handling the request from Smain server
        handle_request(client_sock);

        close(client_sock);
    }

    close(server_sock);
    return 0;
}

//handling the request

void handle_request(int client_sock) {
    char buffer[BUFFER_SIZE];
    char command[BUFFER_SIZE];
    char filename[BUFFER_SIZE];

    bzero(buffer, BUFFER_SIZE);
    read(client_sock, buffer, BUFFER_SIZE);

    printf("Raw input got in Stext: %s\n", buffer);

    sscanf(buffer, "%s %s", command, filename);

    if (strcmp(command, "dfile") == 0) {
        printf("Download request got for: %s\n", filename);
        send_txt_file(client_sock, filename);
    } else if (strcmp(command, "ufile") == 0) {
        char destination_path[BUFFER_SIZE];
        sscanf(buffer + strlen(command), "%s %s", filename, destination_path);
        printf("Save request got for: %s\n", filename);
        save_txt_file(client_sock, filename, destination_path);
    } else if (strcmp(command, "rmfile") == 0) {
        printf("Delete request got for: %s\n", filename);
        delete_txt_file(client_sock, filename);
    } else if (strcmp(command, "dtar") == 0) {
        printf("Tar request got\n");
        create_tar(client_sock, ".txt");
    } else {
        printf("Unexpected request got.\n");
        send_message(client_sock, "Unexpected request received.");
    }
}

// transfering the text file function

void send_txt_file(int client_sock, char *filename) {
    const char *home_path = getenv("HOME");
    char full_path[BUFFER_SIZE];

    snprintf(full_path, sizeof(full_path), "%s", filename);

    printf("making Prepare to send file: %s\n", full_path);

    FILE *file = fopen(full_path, "r");
    if (file == NULL) {
        send_message(client_sock, " Not able to open the TXT file for download.");
        perror("fopen");
        return;
    }

    char buffer[BUFFER_SIZE];
    printf("Opened file: %s for reading\n", full_path);

    while (fgets(buffer, BUFFER_SIZE, file) != NULL) {
        write(client_sock, buffer, strlen(buffer));
        printf("Sending data: %s\n", buffer);
    }

    fclose(file);
    printf("completed sending file: %s\n", full_path);
}

// saving the txt file

void save_txt_file(int client_sock, char *filename, char *destination_path) {
    const char *home_path = getenv("HOME");
    if (home_path == NULL) {
        send_message(client_sock, "Not able to determine home directory.");
        return;
    }

    char full_path[BUFFER_SIZE];
    snprintf(full_path, sizeof(full_path), "%s/stext/%s", home_path, destination_path + 7);

    if (mkdir_recursive(full_path, 0755) == -1) {
        send_message(client_sock, "Not able create directory.");
        perror("mkdir_recursive");
        return;
    }

    char file_path[BUFFER_SIZE];
    snprintf(file_path, sizeof(file_path), "%s/%s", full_path, filename);

    FILE *file = fopen(file_path, "w");
    if (file == NULL) {
        send_message(client_sock, " not able to save the file.");
        perror("fopen");
        return;
    }

    fprintf(file, "This is a test TXT file content for %s\n", filename);
    fclose(file);

    send_message(client_sock, "TXT file uploaded completely");
    printf("TXT file uploaded completely to: %s\n", file_path);
}

//delete text file

void delete_txt_file(int client_sock, char *filename) {
    const char *home_path = getenv("HOME");
    if (home_path == NULL) {
        send_message(client_sock, "Not able to determine home directory.");
        return;
    }

    char full_path[BUFFER_SIZE];
    snprintf(full_path, sizeof(full_path), "%s", filename);

    if (remove(full_path) == 0) {
        send_message(client_sock, "TXT file deleted successfully");
        printf("TXT file deleted successfully: %s\n", full_path);
    } else {
        perror("Error deleting TXT file");
        send_message(client_sock, "Not able to delete the TXT file");
    }
}

//creating the tar

void create_tar(int client_sock, char *filetype) {
    const char *home_path = getenv("HOME");
    char tar_file[BUFFER_SIZE];
    char command[BUFFER_SIZE];

    if (strcmp(filetype, ".txt") == 0) {
        snprintf(tar_file, sizeof(tar_file), "/tmp/txtfiles.tar");
        snprintf(command, sizeof(command), "tar --exclude='stext/txtfiles.tar' -cvf %s -C %s stext", tar_file, home_path);
        printf("Creating tar file with input: %s\n", command);
        int result = system(command);
        if (result != 0) {
            perror("not able to create tar file");
            send_message(client_sock, " not able to create tar file.");
            return;
        }
        printf("Tar file created susseccfully: %s\n", tar_file);
    } else {
        send_message(client_sock, "wrong file type for tar creation.");
        return;
    }

    FILE *file = fopen(tar_file, "rb");
    if (file == NULL) {
        perror("unsuccessful opening tar file");
        send_message(client_sock, "Not open tar file.");
        return;
    }

    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        if (write(client_sock, buffer, bytes_read) < 0) {
            perror("not sending tar file");
            fclose(file);
            return;
        }
        printf("Sent %zu bytes to Smain\n", bytes_read);
    }

    fclose(file);
    printf("Tar file sent successfully : %s\n", tar_file);

    send(client_sock, "EOF", 3, 0);
    printf("EOF signal sent.\n");
}

//send message

void send_message(int client_sock, const char *message) {
    write(client_sock, message, strlen(message));
}

//creating directory recursively

int mkdir_recursive(const char *path, mode_t mode) {
    char tmp[BUFFER_SIZE];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    if (tmp[len - 1] == '/')
        tmp[len - 1] = 0;

    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;
            if (mkdir(tmp, mode) != 0) {
                if (errno != EEXIST)
                    return -1;
            }
            *p = '/';
        }
    }

    if (mkdir(tmp, mode) != 0) {
        if (errno != EEXIST)
            return -1;
    }

    return 0;
}















