#include "common.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#define BUFSZ 4000

char bancoArquivos[100][100];
int numArquivos = 0;

void usage(int argc, char **argv) {
    printf("usage: %s <v4|v6> <server port>\n", argv[0]);
    printf("example: %s v4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        usage(argc, argv);
    }

    struct sockaddr_storage storage;
    if (0 != server_sockaddr_init(argv[1], argv[2], &storage)) {
        usage(argc, argv);
    }

    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1) {
        logexit("socket");
    }

    int enable = 1;
    if (0 != setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
        logexit("setsockopt");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != bind(s, addr, sizeof(storage))) {
        logexit("bind");
    }

    if (0 != listen(s, 10)) {
        logexit("listen");
    }

    struct sockaddr_storage cstorage;
    struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
    socklen_t caddrlen = sizeof(cstorage);

    char buf[BUFSZ];
    char bufCpy[BUFSZ];
    char response[2*BUFSZ];

    char *filenameReceived = NULL;
    char extensionReceived[5];
    char filenameToSave[100];
    char originalFileName[100];

    while (1) {
        int csock = accept(s, caddr, &caddrlen);
        if (csock == -1) {
            logexit("accept");
        }

        while (1) {
            memset(buf, 0, BUFSZ);
            memset(response, 0, 2*BUFSZ);
            memset(bufCpy, 0, BUFSZ);
            memset(originalFileName, 0, 100);

            size_t count = recv(csock, buf, BUFSZ - 1, 0);

            if (count == -1) {
                logexit("recv");
            } else if (count == 0) {
                break;
            }

            if (strncmp("exit", buf, 4) == 0){
                sprintf(response, "connection closed");
                count = send(csock, response, strlen(response) + 1, 0);
                if (count != strlen(response) + 1) {
                    logexit("send conn closed");
                }
                close(csock);
                break;
            } else if(strncmp("send file", buf, 9) == 0) {
                memset(buf, 0, BUFSZ);

                size_t count = recv(csock, buf, BUFSZ - 1, 0);
                
                strcpy(bufCpy, buf);
                const char *ponto = strchr(buf, '.');
                char extC[5];

                if(ponto != NULL){
                    extC[0] = ponto[1];
                    extC[1] = ponto[2];
                    extC[2] = ponto[3];
                    extC[3] = ponto[4];
                    extC[4] = '\0';
                } 

                filenameReceived = strtok(buf, ".");

                if (strncmp("java", extC, 4) == 0){
                    strcpy(extensionReceived, "java");
                } else if (strncmp("txt", extC, 3) == 0){
                    strcpy(extensionReceived, "txt");
                } else if (strncmp("cpp", extC, 3) == 0){
                    strcpy(extensionReceived, "cpp");
                } else if (strncmp("tex", extC, 3) == 0){
                    strcpy(extensionReceived, "tex");
                } else if (strncmp("py", extC, 2) == 0){
                    strcpy(extensionReceived, "py");
                } else if (strncmp("c", extC, 1) == 0){
                    strcpy(extensionReceived, "c");
                } else {
                    strcpy(extensionReceived, "erro");
                }

                sprintf(filenameToSave, "%sRECEIV.%s", filenameReceived, extensionReceived);
                sprintf(originalFileName, "%s.%s", filenameReceived, extensionReceived);

                sprintf(response, "file %s received", originalFileName);

                int cond = 1;

                for (int i = 0; i < numArquivos; i++){
                    if(strcmp(bancoArquivos[i], originalFileName) == 0){
                        sprintf(response, "file %s overwritten", originalFileName);
                        cond = 0;
                        break;
                    } 
                }
                
                if(cond){
                    strcpy(bancoArquivos[numArquivos], originalFileName);
                    numArquivos++;
                }

                char *content = bufCpy + strlen(originalFileName);

                FILE *file = fopen(filenameToSave, "w");
                if (file != NULL) {
                    fprintf(file, "%s", content);
                    fclose(file);

                } else {
                    printf("Erro para salvar arquivo '%s' no server\n", filenameToSave);
                }

                count = send(csock, response, strlen(response) + 1, 0);
                if (count != strlen(response) + 1) {
                    logexit("send");
                }
            }
        }
    }
    exit(EXIT_SUCCESS);
}