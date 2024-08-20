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

//header is added above

#define PORT 15001  // Port for Spdf server
#define BUFFER_SIZE 1024

//these are the functions i used for implemetation

void handle_request(int client_sock);
void save_pdf_file(int client_sock, char *filename, char *destination_path);
void delete_pdf_file(int client_sock, char *filename);
void send_pdf_file(int client_sock, char *filename);
void send_message(int client_sock, const char *message);
void create_tar(int client_sock, char *filetype);
int mkdir_recursive(const char *path, mode_t mode);

//main function

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len;

    // Creating socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Socket creation not completed");
        exit(EXIT_FAILURE);
    }

    // Binding socket 
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Binding is not success");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // Listeninh to connections
    if (listen(server_sock, 5) < 0) {
        perror("Listening not successfull");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("Spdf server is listening to port %d\n", PORT);

    while (1) {
        addr_len = sizeof(client_addr);
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_len);
        if (client_sock < 0) {
            perror("Accept failed");
            continue;
        }

        // Debug statement after accepting the client connection sucessfull
        printf("Spdf server got a request\n");

        // Handling the request from Smain server
        handle_request(client_sock);

        close(client_sock);
    }

    close(server_sock);
    return 0;
}

//handling request

void handle_request(int client_sock) {
    char buffer[BUFFER_SIZE];
    char command[BUFFER_SIZE];
    char filename[BUFFER_SIZE];

    bzero(buffer, BUFFER_SIZE);
    read(client_sock, buffer, BUFFER_SIZE);

    // Debug statement to see the raw input got or not
    printf("Raw input got in Spdf: %s\n", buffer);

    // Extract the input and filename
    sscanf(buffer, "%s %s", command, filename);

    // Handle based on the input
    if (strcmp(command, "dfile") == 0) {
        printf("Download request got for: %s\n", filename);
        send_pdf_file(client_sock, filename);
    } else if (strcmp(command, "ufile") == 0) {
        char destination_path[BUFFER_SIZE];
        sscanf(buffer + strlen(command), "%s %s", filename, destination_path);
        printf("Save request got for: %s\n", filename);
        save_pdf_file(client_sock, filename, destination_path);
    } else if (strcmp(command, "rmfile") == 0) {
        printf("Delete request got for: %s\n", filename);
        delete_pdf_file(client_sock, filename);
    } else if (strcmp(command, "dtar") == 0) {
        printf("Tar request received\n");
        create_tar(client_sock, ".pdf");
    } else {
        printf("Unexpected request rechead.\n");
        send_message(client_sock, "Unexpected request received.");
    }
}
// pdf file transfer

void send_pdf_file(int client_sock, char *filename) {
    const char *home_path = getenv("HOME");
    char full_path[BUFFER_SIZE];

    // Assuming the filename is already a full path as received from Smain
    snprintf(full_path, sizeof(full_path), "%s", filename);

    // Debug statement
    printf("Preparing to send file: %s\n", full_path);

    FILE *file = fopen(full_path, "r");
    if (file == NULL) {
        send_message(client_sock, "not open the PDF file for download.");
        perror("fopen");
        return;
    }

    char buffer[BUFFER_SIZE];
    // Debug statement
    printf("Opened file: %s for reading\n", full_path);

    while (fgets(buffer, BUFFER_SIZE, file) != NULL) {
        write(client_sock, buffer, strlen(buffer));
        printf("Sending data: %s\n", buffer);  // Debug statement
    }

    fclose(file);
    printf("Finished sending file: %s\n", full_path);  // Debug statement
}

//upload pdf file

void save_pdf_file(int client_sock, char *filename, char *destination_path) {
    const char *home_path = getenv("HOME");
    if (home_path == NULL) {
        send_message(client_sock, "Not determine home directory.");
        return;
    }

    // Constructing the full directory path for saving the file
    char full_path[BUFFER_SIZE];
    snprintf(full_path, sizeof(full_path), "%s/spdf/%s", home_path, destination_path + 7); // +7 to skip "~smain/"

    if (mkdir_recursive(full_path, 0755) == -1) {
        send_message(client_sock, "Not create directory.");
        perror("mkdir_recursive");
        return;
    }

    // Constructing the full file path
    char file_path[BUFFER_SIZE];
    snprintf(file_path, sizeof(file_path), "%s/%s", full_path, filename);

    FILE *file = fopen(file_path, "w");
    if (file == NULL) {
        send_message(client_sock, " Not saving the file.");
        perror("fopen");
        return;
    }

    //  writing to the file 
    fprintf(file, "This is a test PDF file content for %s\n", filename);
    fclose(file);

    send_message(client_sock, "PDF file uploaded successfully");
    printf("PDF file uploaded successfully to: %s\n", file_path);
}

//pdf delete

void delete_pdf_file(int client_sock, char *filename) {
    const char *home_path = getenv("HOME");
    if (home_path == NULL) {
        send_message(client_sock, " Not determine home directory.");
        return;
    }

    // Using the received filename directly since it's already a full path
    char full_path[BUFFER_SIZE];
    snprintf(full_path, sizeof(full_path), "%s", filename);

    if (remove(full_path) == 0) {
        send_message(client_sock, "PDF file deleted successfully");
        printf("PDF file deleted successfully: %s\n", full_path);
    } else {
        perror("Error deleting PDF file");
        send_message(client_sock, "Not delete the PDF file");
    }
}
//creating tar

void create_tar(int client_sock, char *filetype) {
    const char *home_path = getenv("HOME");
    char tar_file[BUFFER_SIZE];
    char command[BUFFER_SIZE];

    if (strcmp(filetype, ".pdf") == 0) {
        snprintf(tar_file, sizeof(tar_file), "/tmp/pdffiles.tar");
        // Adjusted tar command to exclude pdffiles.tar more effectively
        snprintf(command, sizeof(command), "tar --exclude='spdf/pdffiles.tar' -cvf %s -C %s spdf", tar_file, home_path);
        printf("Creating tar file with command: %s\n", command);
        int result = system(command);
        if (result != 0) {
            perror("Not able to creating tar file");
            send_message(client_sock, " not create tar file.");
            return;
        }
        printf("Tar file created: %s\n", tar_file);
    } else {
        send_message(client_sock, "Invalid file type for tar creation.");
        return;
    }

    FILE *file = fopen(tar_file, "rb");
    if (file == NULL) {
        perror("Error opening tar file");
        send_message(client_sock, "not open tar file.");
        return;
    }

    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        if (write(client_sock, buffer, bytes_read) < 0) {
            perror("Error sending tar file");
            fclose(file);
            return;
        }
        printf("Sent %zu bytes to Smain\n", bytes_read);
    }

    fclose(file);
    printf("Tar file sent: %s\n", tar_file);

    // Ensure the EOF signal is clear
    send(client_sock, "EOF", 3, 0);
    printf("EOF signal sent.\n");
}


void send_message(int client_sock, const char *message) {
    write(client_sock, message, strlen(message));
}

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










