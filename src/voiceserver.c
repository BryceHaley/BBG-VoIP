#include <sys/socket.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "include/voiceserver.h"
#include "include/connection.h"
#include "include/addressbook.h"

static pthread_t voiceServerThread;

static void* acceptConnections(void*);

int listenSocket;
int stop = 0;
IncomingCallEventHandler handleIncomingCall;

void VoiceServer_start(IncomingCallEventHandler callback) {
    stop = 0;

    handleIncomingCall = callback;

    listenSocket = Connection_listen();

    int res = pthread_create(&voiceServerThread, NULL, acceptConnections, NULL);

    if (res != 0) {
        printf("VoiceServer: pthread_create failed.\n");
        exit(1);
    }

    printf("VoiceServer started.\n");
}

void VoiceServer_stop() {
    stop = 1;

    int res = pthread_cancel(voiceServerThread);

    if (res != 0) {
        fprintf(stderr, "pthread_cancel failed (thread doesn't exist wtf?)\n");
    }

    void* exitResult;

    pthread_join(voiceServerThread, &exitResult);

    close(listenSocket);

    if (exitResult == PTHREAD_CANCELED) {
        printf("VoiceServer stopped.\n");
    } else {
        fprintf(stderr, "Unable to stop VoiceServer.\n");
    }
}

void* acceptConnections(void* ptr) {
    int tcpSocket;
    struct sockaddr_in peerAddr;

    // This must be set to the size of the input struct.
    int peerAddressSize = sizeof(peerAddr);
    char callerIp[INET_ADDRSTRLEN];
    Connection connection;
    Address callerAddr;

    while (!stop) {
        memset(&peerAddr, 0, sizeof(struct sockaddr_in));
        tcpSocket = accept(listenSocket, (struct sockaddr*) &peerAddr, (socklen_t*) &peerAddressSize);

        const char* ptr = inet_ntop(AF_INET, &peerAddr.sin_addr.s_addr, callerIp, INET_ADDRSTRLEN);
        // printf("ListenSocket=%d, TCPSocket=%d, s_addr=%d, CallerIP=%s\n", listenSocket, tcpSocket, peerAddr.sin_addr.s_addr, callerIp);
        if (ptr == NULL) {
            perror("This shouldn't happen");
            exit(1);
        }

        connection.socket = tcpSocket;
        memcpy(&connection.sourceHost, &peerAddr, peerAddressSize);
        int res = AddressBook_reverseLookup(callerIp, &callerAddr);

        if (res == ADDRESS_REVERSE_LOOKUP_FAILED) {
            printf("Unknown caller (possibly NSA). Closing connection...\n");
            close(tcpSocket);
            continue;
        }

        // printf("Handling incoming call...\n");
        handleIncomingCall(&callerAddr, &connection);
    }

    pthread_exit(NULL);
}
