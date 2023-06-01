#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "common.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFSZ 4000

void usage(int argc, char **argv) {
	printf("usage: %s <server IP> <server port>\n", argv[0]);
	printf("example: %s 127.0.0.1 51511\n", argv[0]);
	exit(EXIT_FAILURE);
}

int isValidExtension(const char *filename) {
    const char *validExtensions[] = {".txt", ".c", ".cpp", ".py", ".tex", ".java"};
    const char *extension = strrchr(filename, '.');

    if (extension != NULL) {
        for (int i = 0; i < (sizeof(validExtensions) / sizeof(validExtensions[0])); i++) {
            if (strcmp(extension, validExtensions[i]) == 0) {
                return 1;
            }
        }
    }
    return 0;
}

void sendFile(int socket, const char *filename, FILE *f){
	char buffer[BUFSZ];
	char buffer2[BUFSZ];

	memset(buffer, 0, BUFSZ);
	memset(buffer2, 0, BUFSZ);

    fseek(f, 0, SEEK_SET); 

    size_t bytesRead = fread(buffer, 1, BUFSZ, f);
    snprintf(buffer2, BUFSZ - bytesRead, "%s%s", filename, buffer);

    send(socket, buffer2, strlen(buffer2) + 1, 0);
}

int main(int argc, char **argv) {
	if (argc < 3) {
		usage(argc, argv);
	}

	struct sockaddr_storage storage;
	if (0 != addrparse(argv[1], argv[2], &storage)) {
		usage(argc, argv);
	}

	int s;
	s = socket(storage.ss_family, SOCK_STREAM, 0);
	if (s == -1) {
		logexit("socket");
	}
	struct sockaddr *addr = (struct sockaddr *)(&storage);
	if (0 != connect(s, addr, sizeof(storage))) {
		logexit("connect");
	}

	char addrstr[BUFSZ];
	addrtostr(addr, addrstr, BUFSZ);

	char buf[BUFSZ];
	memset(buf, 0, BUFSZ);

	char serverResponse[BUFSZ];
	memset(serverResponse, 0, BUFSZ);

	int haveFileSelected = 0;
	char *selectedFilename = NULL;

    while(1) {		
	    fgets(buf, BUFSZ-1, stdin);

    	if(strncmp("exit", buf, 4) == 0) {
			size_t count = send(s, buf, strlen(buf) + 1, 0);
			if (count != strlen(buf) + 1) {
				logexit("send");
			}
			memset(buf, 0, BUFSZ);

			count = recv(s, serverResponse, BUFSZ - 1, 0);
			printf("%s\n", serverResponse);

			if (count == -1) {
				logexit("recv");
			} else {
				close(s);
				break;
			}

    	} else if (strncmp("select file ", buf, 12) == 0) {
            char *filename = buf + 12;
            filename[strcspn(filename, "\n")] = '\0'; 

            if (access(filename, F_OK) != -1) {
                if (isValidExtension(filename)) {
                    printf("%s selected\n", filename);
					haveFileSelected = 1;
					selectedFilename = strdup(filename);
                } else {
                    printf("%s not valid!\n", filename);
                }
            } else {
                printf("%s does not exist\n", filename);
            }

        } else if (strncmp("send file", buf, 9) == 0) {
			if(haveFileSelected){
				send(s, "send file", strlen("send file") + 1, 0);

				FILE *file = fopen(selectedFilename, "rb");

				if(file != NULL){
					sendFile(s, selectedFilename, file);
					haveFileSelected = 0;
					free(selectedFilename);
					selectedFilename = NULL;
					memset(buf, 0, BUFSZ);

					size_t count = send(s, serverResponse, strlen(serverResponse) + 1, 0);
					count = recv(s, serverResponse, BUFSZ - 1, 0);

					if (count == -1) {
						logexit("recv");
					}

					printf("%s\n", serverResponse);

				} else {
					printf("Erro abrindo %s\n", selectedFilename);
				}

			} else {
				printf("no file selected\n");
			}

    	} else {
			close(s);
			break;
		}	
    }

    close(s);
	exit(EXIT_SUCCESS);
}