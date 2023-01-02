#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <cstring>
#include <unistd.h>
#include <map>
#include <iostream>
#include <fcntl.h>

#define PORT 8080
#define BUFFER_SIZE 1024


struct Client{
    int sock;
};


std::map <int, Client*> clients; // Map sockets to clients

fd_set fds; // This is a list of raw file descriptors.
int clientsCount = 0;

int server;


void deblock(int socket){
    if (fcntl(socket, F_SETFL, fcntl(socket, F_GETFL, 0) | O_NONBLOCK) < 0){ // Add the Non Blocking Flag to the previously extant flags
        perror("Deblocking");
    }
}


void regenFDs(){
    FD_ZERO(&fds);
    FD_SET(server, &fds);
    for (const auto& pair : clients){
        FD_SET(pair.first, &fds);
    }
}


void gracefullyClose(Client* cli){
    clients.erase(cli -> sock);
    delete cli;
    regenFDs();
}


void handleMessage(Client* cli){
    char buf[BUFFER_SIZE];
    std::string message;
    while (true){
        memset(buf, 0, BUFFER_SIZE);
        int ret = recv(cli -> sock, buf, BUFFER_SIZE, 0); // Socket to read from, buffer (a pointer to load data into), amount to read, flags (flags of 0 = nothin')
        if (ret == 0){
            gracefullyClose(cli);
            return;
        }
        message += buf;
        if (ret < BUFFER_SIZE){
            break; // Message fully received - exit the pull loop
        }
    }
    std::cout << "Got message: " << message << std::endl;
}


int main(){
    server = socket(AF_INET, SOCK_STREAM, 0);
    const int reuseaddr = 1;
    if (setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr)) < 0){
        perror("Failed to set to reuse address");
    }
    if (setsockopt(server, SOL_SOCKET, SO_REUSEPORT, &reuseaddr, sizeof(reuseaddr)) < 0){
        perror("Failed to set to reuse port");
    }
    deblock(server); // A well-known race condition is when a socket is disconnected between select returning and accept being called. That will cause the program to block. So we want to use nonblocking sockets for all things.
    sockaddr_in serverAddress, clientInfo;
    unsigned int sizeOfClientInfo = sizeof(clientInfo);
    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(PORT);
    if (bind(server, (sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
        perror("Failed to bind");
    }
    listen(server, 32);

    regenFDs();

    while (true){
        select(
            1024, // Highest possible socket with select, I think; will need to implement some actual checks later
            &fds, // Select needs the list of file descriptors to check
            NULL, // This is a file descriptor set for writing - dunno why; it'll gracefully handle the NULL
            NULL, // Absolutely no clue what "except fds" are. Read the manpage - `man select`
            NULL // This is usually the timeout. I don't want to have a timeout, so this will run forever. We'll add a timeout later if necessary.
        ); // Select literally just waits until a file descriptor (socket) is ready for something
        if (FD_ISSET(server, &fds)){
            int client = accept(server, (sockaddr*)&clientInfo, &sizeOfClientInfo); // ClientInfo is pretty much ignored for now
            if (client < 0){ // failed
                perror("Accept appears to have failed");
            }
            else{
                clients[client] = new Client {client};
                deblock(client);
                regenFDs();
            }
        }
        for (const auto& pair : clients){
            if (FD_ISSET(pair.first, &fds)){
                handleMessage(pair.second);
            }
        }
    }
    close(server);
    return 0;
}