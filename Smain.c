#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/wait.h>

//headers are added above 

#define PORT 15000  // Port for Smain server which i kept
#define BUFFER_SIZE 1024

// function used in my implementation
void handleClientRequest(int client_sock);
void transferFile(int client_sock, char *filename, char *destination_path);
void processPdfFile(char *filename, char *destination_path);
void processPdfFile_deletion(char *filename);
void processTextFile(char *filename, char *destination_path);
void processTextFile_deletion(char *filename);
void downloadFile(int client_sock, char *filename);
void downloadCFile(int client_sock, char *filename);
void deleteFile(int client_sock, char *filename);
void createTarArchive(int client_sock, char *filetype);
void showFileList(int client_sock, char *pathname);
void sendResponse(int client_sock, const char *message);
int createDirectoriesRecursively(const char *path, mode_t mode);

void forwardToServer(int client_sock, char *buffer, const char *server_ip, int server_port);
void processLocalFile(int client_sock, char *filepath);

//main function

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len;
    pid_t child_pid;

    // Creating socket here
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("creation of socket is not complete");
        exit(EXIT_FAILURE);
    }

    // Binding socket to port here
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind is not success");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // Listening for connections 
    if (listen(server_sock, 5) < 0) {
        perror("listen not success");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("main server is listening  %d\n", PORT);

    while (1) {
        addr_len = sizeof(client_addr);
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_len);
        if (client_sock < 0) {
            perror("Accept is not done");
            continue;
        }

        printf("Client connected successfully Smain\n");

        // forking a child process to handle the client below
        child_pid = fork();
 if (child_pid == 0) {
            // Child process here
            close(server_sock);
            handleClientRequest(client_sock);
            close(client_sock);
            exit(0);
        } else if (child_pid > 0) {
            // Parent process takes place here
            close(client_sock);
            waitpid(child_pid, NULL, 0); // Waiting for the child process to complete
        } else {
            perror("Fork not success");
            close(client_sock);
        }
    }

    close(server_sock);
    return 0;
}

//handle client request

void handleClientRequest(int client_sock) {
    char buffer[BUFFER_SIZE];
    char command[BUFFER_SIZE], filename[BUFFER_SIZE], destination_path[BUFFER_SIZE];

    while (1) {
        bzero(buffer, BUFFER_SIZE);
        read(client_sock, buffer, BUFFER_SIZE);

        printf("Raw input received to main: %s\n", buffer);

        sscanf(buffer, "%s %s %s", command, filename, destination_path);

        if (strcmp(command, "dfile") == 0) {
            printf("dfile input is  identified\n");
            downloadFile(client_sock, filename);
        } else if (strcmp(command, "ufile") == 0) {
            printf("ufile input is identified\n");
            if (strstr(filename, ".pdf") != NULL) {
                processPdfFile(filename, destination_path);
                sendResponse(client_sock, "File uploaded completely to Spdf server");
            } else if (strstr(filename, ".c") != NULL) {
                transferFile(client_sock, filename, destination_path);
            } else if (strstr(filename, ".txt") != NULL) {
                processTextFile(filename, destination_path);
                sendResponse(client_sock, "file uploaded completely to Stext");
            } else {
                sendResponse(client_sock, "invalid file type to upload");
            }
        } else if (strcmp(command, "rmfile") == 0) {
            printf("rmfile input identified\n");
            deleteFile(client_sock, filename);
        } else if (strcmp(command, "dtar") == 0) {
            printf("dtar input identified\n");
            createTarArchive(client_sock, filename);
        } else if (strcmp(command, "display") == 0) {
            printf("display input identified\n");
            showFileList(client_sock, filename);
        } else {
            printf("Unknown command received: %s\n", command);
            sendResponse(client_sock, "Invalid input");
        }
    }
}

//download file function

void downloadFile(int client_sock, char *filename) {
    printf("downloadFile function called here for : %s\n", filename);

    if (strstr(filename, ".pdf") != NULL) {
        char buffer[BUFFER_SIZE];
        const char *home_path = getenv("HOME");

        if (home_path == NULL) {
            sendResponse(client_sock, "error not find home directory.");
            return;
        }
  snprintf(buffer, sizeof(buffer), "dfile %s/spdf/%s", home_path, filename + 7);  // Forwarding to Spdf server
        forwardToServer(client_sock, buffer, "127.0.0.1", 15001);  // Forwarding to Spdf server

    } else if (strstr(filename, ".txt") != NULL) {
        char buffer[BUFFER_SIZE];
        const char *home_path = getenv("HOME");

        if (home_path == NULL) {
            sendResponse(client_sock, "Not find home directory.");
            return;
        }

        snprintf(buffer, sizeof(buffer), "dfile %s/stext/%s", home_path, filename + 7);  // Forwarding to Stext server
        forwardToServer(client_sock, buffer, "127.0.0.1", 15002);  // Forwarding to Stext server

    } else if (strstr(filename, ".c") != NULL) {
        downloadCFile(client_sock, filename);  // Handling .c files with correct function

    } else {
        sendResponse(client_sock, "Not correct file type for download.");
    }
}

//download c file function

void downloadCFile(int client_sock, char *filename) {
    printf("downloadCFile function called here for: %s\n", filename);

    char buffer[BUFFER_SIZE];
    const char *home_path = getenv("HOME");

    if (home_path == NULL) {
        sendResponse(client_sock, " Nnot determine home directory.");
        return;
    }

    snprintf(buffer, sizeof(buffer), "%s/smain/%s", home_path, filename + 7);  // +7 to skip "~smain/"
    printf("Resolving .c file path: %s\n", buffer);

    processLocalFile(client_sock, buffer);  // Handling .c files locally
}

//downloaded forward to server this is used to communicate with pdf ans txt

void forwardToServer(int client_sock, char *buffer, const char *server_ip, int server_port) {
    int server_sock;
    struct sockaddr_in server_addr;

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Socket creation not complete");
        sendResponse(client_sock, "Not created socket for server.");
        return;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    if (connect(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection to server not complpete");
        close(server_sock);
        sendResponse(client_sock, "Not connected to server.");
        return;
    }

    printf("Sending request: %s to server\n", buffer);
    write(server_sock, buffer, strlen(buffer));
    bzero(buffer, BUFFER_SIZE);

    ssize_t bytes_read;
    while ((bytes_read = read(server_sock, buffer, BUFFER_SIZE)) > 0) {
        ssize_t bytes_written = write(client_sock, buffer, bytes_read);
        if (bytes_written < bytes_read) {
            perror("Not writing to client");
            break;
        }
        printf("Received %zu bytes from server, forwarded %zu bytes to client\n", bytes_read, bytes_written);

        if (bytes_read < BUFFER_SIZE) {
            printf("Final chunk received (%zu bytes), closing connection.\n", bytes_read);

  break;
        }
    }

    close(server_sock);
    printf("Connection to server terminated.\n");
}

//local file processing

void processLocalFile(int client_sock, char *filepath) {
    printf("trying to open file: %s\n", filepath);

    FILE *file = fopen(filepath, "r");
    if (file == NULL) {
        perror("fopen");
        sendResponse(client_sock, " Not open the C file for download.");
        return;
    }

    char buffer[BUFFER_SIZE];
    printf("Opened file: %s for reading\n", filepath);

    while (fgets(buffer, BUFFER_SIZE, file) != NULL) {
        write(client_sock, buffer, strlen(buffer));
        printf("Sending data: %s\n", buffer);
    }

    fclose(file);
    printf("completed sending file: %s\n", filepath);
}

//pdf file

void processPdfFile(char *filename, char *destination_path) {
    const char *home_path = getenv("HOME");
    if (home_path == NULL) {
        printf(" Not determine home directory.\n");
        return;
    }

    char expanded_path[BUFFER_SIZE];
    snprintf(expanded_path, sizeof(expanded_path), "%s/spdf/%s", home_path, destination_path + 7); // +7 to skip "~smain/"
    expanded_path[BUFFER_SIZE - 1] = '\0';

    if (createDirectoriesRecursively(expanded_path, 0755) == -1) {
        printf(" Not able to create directory.\n");
        perror("createDirectoriesRecursively");
        return;
    }

    char full_path[BUFFER_SIZE];
    snprintf(full_path, sizeof(full_path), "%s/%s", expanded_path, filename);
    full_path[BUFFER_SIZE - 1] = '\0';

    FILE *file = fopen(full_path, "w");
    if (file == NULL) {
        printf("Not saving the file.\n");
        perror("fopen");
        return;
    }

    fprintf(file, "This is a test PDF file content for %s\n", filename);
    fclose(file);

    printf("PDF file uploaded completely to: %s\n", full_path);
}

//text file

void processTextFile(char *filename, char *destination_path) {
    const char *home_path = getenv("HOME");
    if (home_path == NULL) {
        printf("Not determine home directory.\n");
        return;
    }

    char expanded_path[BUFFER_SIZE];
    snprintf(expanded_path, sizeof(expanded_path), "%s/stext/%s", home_path, destination_path + 7); // +7 to skip "~smain/"
    expanded_path[BUFFER_SIZE - 1] = '\0';

    if (createDirectoriesRecursively(expanded_path, 0755) == -1) {
        printf("Not create directory.\n");
  perror("createDirectoriesRecursively");
        return;
    }

    char full_path[BUFFER_SIZE];
    snprintf(full_path, sizeof(full_path), "%s/%s", expanded_path, filename);
    full_path[BUFFER_SIZE - 1] = '\0';

    FILE *file = fopen(full_path, "w");
    if (file == NULL) {
        printf("Not saving the file.\n");
        perror("fopen");
        return;
    }

    fprintf(file, "This is a test TXT file content for %s\n", filename);
    fclose(file);

    printf("TXT file uploaded successfully to: %s\n", full_path);
}
//delete function for pdf file

void processPdfFile_deletion(char *filename) {
    const char *home_path = getenv("HOME");
    if (home_path == NULL) {
        printf("Not determine home directory.\n");
        return;
    }

    char full_path[BUFFER_SIZE];
    snprintf(full_path, sizeof(full_path), "%s/spdf/%s", home_path, filename + 7); // +7 to skip "~smain/"

    if (remove(full_path) == 0) {
        printf("PDF file found successfully\n");
    } else {
        perror("error removing PDF file");
        printf(" not able to  delete the PDF file\n");
    }
}

//processtext file for deletion function

void processTextFile_deletion(char *filename) {
    const char *home_path = getenv("HOME");
    if (home_path == NULL) {
        printf("Not determine home directory.\n");
        return;
    }

    char full_path[BUFFER_SIZE];
    snprintf(full_path, sizeof(full_path), "%s/stext/%s", home_path, filename + 7); // +7 to skip "~smain/"

    if (remove(full_path) == 0) {
        printf("TXT file deleted successfully\n");
    } else {
        perror("not able to  deleting TXT file");
        printf("Not delete the TXT file\n");
    }
}

//transferfile function

void transferFile(int client_sock, char *filename, char *destination_path) {
    const char *home_path = getenv("HOME");
    if (home_path == NULL) {
        sendResponse(client_sock, "Not determine home directory.");
        return;
    }

    char expanded_path[BUFFER_SIZE];
    snprintf(expanded_path, sizeof(expanded_path), "%s/smain/%s", home_path, destination_path + 7); // +7 to skip "~smain/"
    expanded_path[BUFFER_SIZE - 1] = '\0';

    if (createDirectoriesRecursively(expanded_path, 0755) == -1) {
        sendResponse(client_sock, "not create directory.");
        perror("createDirectoriesRecursively");
        return;
    }

    char full_path[BUFFER_SIZE];
    snprintf(full_path, sizeof(full_path), "%s/%s", expanded_path, filename);
    full_path[BUFFER_SIZE - 1] = '\0';  FILE *file = fopen(full_path, "w");
    if (file == NULL) {
        sendResponse(client_sock, "not save the file.");
        perror("fopen");
        return;
    }

    fprintf(file, "This is a test C file content for %s\n", filename);
    fclose(file);

    sendResponse(client_sock, "C file uploaded successfully to Smain");
    printf("C file uploaded successfully to: %s\n", full_path);
}

void deleteFile(int client_sock, char *filename) {
    if (strstr(filename, ".pdf") != NULL) {
        processPdfFile_deletion(filename);
        sendResponse(client_sock, "PDF file deletion request sent to Spdf server");
    } else if (strstr(filename, ".txt") != NULL) {
        processTextFile_deletion(filename);
        sendResponse(client_sock, "TXT file deletion request sent to Stext server");
    } else if (strstr(filename, ".c") != NULL) {
        const char *home_path = getenv("HOME");
        if (home_path == NULL) {
            sendResponse(client_sock, "not determine home directory.");
            return;
        }

        char full_path[BUFFER_SIZE];
        snprintf(full_path, sizeof(full_path), "%s/smain/%s", home_path, filename + 7); // +7 to skip "~smain/"

        if (remove(full_path) == 0) {
            sendResponse(client_sock, "C file deleted successfully");
            printf("C file deleted successfully from: %s\n", full_path);
        } else {
            perror("Error deleting file");
            sendResponse(client_sock, "not delete the C file");
        }
    } else {
        sendResponse(client_sock, "Invalid file type for deletion");
    }
}

//tar creation

void createTarArchive(int client_sock, char *filetype) {
    char command[BUFFER_SIZE];
    char tar_file[BUFFER_SIZE];
    const char *home_path = getenv("HOME");

    if (strcmp(filetype, ".c") == 0) {
        snprintf(tar_file, sizeof(tar_file), "%s/smain/cfiles.tar", home_path);

        // Correcting the order and ensuring the tar file itself is excluded
        snprintf(command, sizeof(command), "tar --exclude='%s/smain/cfiles.tar' --exclude='*.pdf' --exclude='*.txt' -cvf %s -C %s smain", home_path, tar_file, home_path);

        printf("Creating tar file with input: %s\n", command);
        int result = system(command);
        if (result != 0) {
            perror("Error creating tar file");
            sendResponse(client_sock, "not create tar file.");
            return;
        }
        printf("Tar file created for .c files: %s\n", tar_file);
    } else if (strcmp(filetype, ".pdf") == 0) {
        snprintf(command, sizeof(command), "dtar .pdf");  // Forward to Spdf
        forwardToServer(client_sock, command, "127.0.0.1", 15001);  // Request Spdf server to create tar
        return;
    } else if (strcmp(filetype, ".txt") == 0) {
        snprintf(command, sizeof(command), "dtar .txt");  // Forward to Stext
        forwardToServer(client_sock, command, "127.0.0.1", 15002);  // Request Stext server to create tar
        return;
    } else {
        sendResponse(client_sock, "Invalid file type for tar creation.");
        return;
    }
 // Send the created tar file to the client
    FILE *file = fopen(tar_file, "rb");
    if (file == NULL) {
        perror("Error opening tar file");
        sendResponse(client_sock, " not open tar file.");
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
}

//i didnt do this 

void showFileList(int client_sock, char *pathname) {
    sendResponse(client_sock, " need to implememt\n");
}

// sending response
void sendResponse(int client_sock, const char *message) {
    write(client_sock, message, strlen(message));
}

// directory created recursively function for that
int createDirectoriesRecursively(const char *path, mode_t mode) {
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
