#include <sys/socket.h> // socket() and some other commands
#include <netinet/in.h> // inet (ipv4) stuff, I think
#include <stdio.h>      // standard IO
#include <cstring>      // C string stuff (memset)
#include <unistd.h>     // more standard commands - close() in particular
#include <map>          // std::map
#include <iostream>     // std::cout, std::cin
#include <fcntl.h>      // fcntl()
#include <poll.h>       // poll is more powerful, more developer-friendly (in my opinion), and cleaner than select

#define PORT 8080        // The port to bind to - a number that the client has to specify. Default port for websites is 80 or 443 and the web browser hides it from you.
#define BUFFER_SIZE 1024 // Number of bytes in the message receive buffer; 1 kilobyte for conventionalness (kibibyte? I dunno)


struct Client{ // Add more things to this later; for now it just holds the socket
    int sock; // All sockets are numbers, because they're file descriptors. Descriptor 0 is stdout, descriptor 1 is stdin, sockets are added after that. The server is probably descriptor 2.
};


std::map <int, Client*> clients; // Map socket numbers to clients

pollfd* fds = 0; // pollfd is a structure. This is a list of pollfds.
// initialize it to 0

int server; // The server socket allocation; it's actually set by the socket() command.


void deblock(int socket){ // Set a socket to be non-blocking, meaning commands like recv won't wait until they get some data to return
    if (fcntl(socket, F_SETFL, fcntl(socket, F_GETFL, 0) | O_NONBLOCK) < 0){ // Add the Non Blocking Flag to the previously extant flags
        perror("Deblocking"); // perror prints out an error.
    }
}


void regenFDs(){ // Regenerate the FD set based on the map
    if (fds){
        free(fds); // Free the old pollFDs memory
    }
    fds = (pollfd*)malloc(sizeof(pollfd) * (clients.size() + 1)); // Allocate new memory for a pollFD set
    memset(fds, 0, sizeof(pollfd) * (clients.size() + 1)); // Set every byte to 0 - not technically necessary, but a Good Practice™.
    // Add one to the size for the server file descriptor, which we'll slap in as the first element
    int i = 1; // Iterating over a map like a vector is nontrivial, so adding my own iterator is easier
    fds[0].fd = server;
    fds[0].events = POLLIN; // Data to read I guess
    // it's 1 because the first element in the pollfd list is the server
    for (const auto& pair : clients){ // Iterate over a map. pair has two properties of note - first, which is an int (a file descriptor), and last, which is a Client*.
        fds[i].fd = pair.first; // Set the watched file descriptor to the key in the map - a file descriptor
        fds[i].events = POLLIN;
        i ++;
    }
}


void gracefullyClose(Client* cli){ // Close a socket and delete the client
    close (cli -> sock); // Properly close the socket
    clients.erase(cli -> sock); // Erase it from the map
    delete cli; // Free the memory
    regenFDs(); // Regenerate the FD set based on the updated map
}


void gotMessage(Client* cli, std::string message){ // Successfully read a proper message from the attached client
    std::cout << "Client " << cli -> sock << " got a message: " << message << std::endl;
}


void handleMessage(Client* cli){ // Handle a message received from a client
    char buf[BUFFER_SIZE + 1]; // A statically allocated buffer of BUFFER_SIZE + 1 bytes: this is to preserve the null terminating character at the end of the string
    std::string message; // Proper std::string of the message
    while (true){ // Infinite loop. You'll see.
        memset(buf, 0, BUFFER_SIZE + 1); // Set every byte in the buffer to 0
        int ret = recv(cli -> sock, buf, BUFFER_SIZE, 0); // Socket to read from, buffer (a pointer to load data into), amount to read, flags (flags of 0 = nothin')
        if (ret == 0){ // If the socket was closed
            std::cout << "Client disconnected" << std::endl;
            gracefullyClose(cli); // Call that function from earlier
            return; // End the function - we don't want to waste time processing data from a closed client. THIS MAY LEAD TO BUGS!
        }
        message += buf; // Append the buffer to the proper string
        if (ret < BUFFER_SIZE){ // recv returns either -1 (error; usually just means "no data" if you're using nonblocking sockets, which we are)
            break; // Message fully received - exit the pull loop
        }
    }
    gotMessage(cli, message); // Fully got a message from a client. Call the nice happy function.
}


int main(){
    server = socket(AF_INET, SOCK_STREAM, 0); // Create the server socket: AF_INET means ipv4, SOCK_STREAM means TCP, 0 means no flags.
    const int reuseaddr = 1; // Boolean 1 to reuse the address on bind; this means that even if the socket wasn't properly closed it'll bind to it
    if (setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr)) < 0){ // Make the socket reuse the address it's bound to. setsockopt, like most Unix library functions, returns -1 if it fails.
        perror("Failed to set to reuse address");
    }
    if (setsockopt(server, SOL_SOCKET, SO_REUSEPORT, &reuseaddr, sizeof(reuseaddr)) < 0){ // Same, but for reusing the port.
        perror("Failed to set to reuse port");
    }
    deblock(server); // A well-known race condition is when a socket is disconnected between select returning and accept being called. That will cause the program to block. So we want to use nonblocking sockets for all things.
    sockaddr_in serverAddress, clientInfo; // sockaddr_in is an ipv4 address
    unsigned int sizeOfClientInfo = sizeof(clientInfo); // Stupidly written function below wants a pointer to the size of the client ipv4 address
    memset(&serverAddress, 0, sizeof(serverAddress)); // Zero it - every byte is now 0. Just a Good Practice ™.
    serverAddress.sin_family = AF_INET; // Configure it to use ipv4. This is because of some casting magic below.
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY); // HTONL flips the byte order of a long (I think it's 32 bits) int from host to network. Not sure of the distinction, but it's important.
    // INADDR_ANY is useful in that it accepts traffic from any address
    serverAddress.sin_port = htons(PORT); // Same but for 16 bit ints.
    if (bind(server, (sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){ // Attach a socket to an address and port
        perror("Failed to bind");
    }
    listen(server, 32); // Set the queue size to 32; there can be 32 clients in the pool ready to accept before it starts rejecting 'em

    regenFDs(); // (re)generate the file descriptor set

    while (true){
        poll( // Waits until an event happens on a socket. Notably, all of the sockets use only the POLLIN event, so we don't actually got to check what event has happened.
            fds, // List of file descriptors
            clients.size() + 1, // Number of clients to check, + 1 for the server socket
            -1 // Don't want a timeout, yet, mr bond
        );
        if (fds[0].revents & POLLIN){ // If the POLLIN event has been returned on the server socket
            int client = accept(server, (sockaddr*)&clientInfo, &sizeOfClientInfo); // ClientInfo is pretty much ignored for now
            // Accept pulls a client socket from the queue
            if (client < 0){ // failed
                perror("Accept appears to have failed");
            }
            else{
                clients[client] = new Client {client}; // Create a new client and slap it in the map
                deblock(client); // Configure the socket to be nonblocking; nonblocking is always a good idea even if you're using poll or select
                regenFDs(); // Regenerate the FD set from the modified map
            }
        }
        for (int i = 0; i < clients.size(); i ++){
            if (fds[i + 1].revents & POLLIN){ // if POLLIN has been returned on the socket
                handleMessage(clients[fds[i + 1].fd]); // Get the client referenced by the pollfd's file descriptor, and call handleMessage on it
            }
        }
    }
    close(server); // IF the while loop exits naturally, nicely close the server. Need to sprinkle a few more calls to this around at critical points. Just a good practice.
    return *(uint64_t*)"hi"; // Hehehehehehe
}