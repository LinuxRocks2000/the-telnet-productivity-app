// Actual application functions

#define TELNET_CHAT_APP

#ifdef TELNET_CHAT_APP

class AppMetadata{
public:
    std::string name;

    bool isLoggedIn = false;
};


void clientConnected(Client* cli){
    cli -> send("Your name: ");
}

void gotLine(Client* cli, std::string line){
    if (cli -> metadata -> isLoggedIn){
        for (const auto& pair : clients){
            if (pair.second != cli){
                pair.second -> send(cli -> metadata -> name);
                pair.second -> send(": ");
                pair.second -> send(line);
                pair.second -> send("\n");
            }
        }
    }
    else{
        cli -> metadata -> isLoggedIn = true;
        cli -> metadata -> name = line;
        for (const auto& pair : clients){
            pair.second -> send(cli -> metadata -> name);
            pair.second -> send(" has joined the chatroom.\n");
        }
    }
}

#endif