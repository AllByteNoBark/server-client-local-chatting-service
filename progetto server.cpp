#include <iostream>
#include <string.h>
#include <winsock2.h>
#include <thread>
#include <vector>
#include "color.hpp"

using namespace std;

// Controlla se il server è offline o no
bool offline = false;

#define MAX_BUFFER_SIZE 4096

// Classe client - memorizza gli attributi dei client connessi
class Client {
    public:
    char name[MAX_BUFFER_SIZE];
    SOCKET socket;
    unsigned long address;
    string color = "grey";
    bool muted = false;

    Client(SOCKET s, char n[MAX_BUFFER_SIZE], unsigned long a) {
        this->socket = s;
        strcpy(this->name, n);
        this->address = a;
    }
    ~Client() {}
};

// Vettore che memorizza i colori supportati dallo server
vector<string> colors = {"red", "blue", "green", "aqua", "purple", "yellow", "white", "grey"};

// Variabili e vettori usati per gestire i commandi
char sep = ' ';
vector<string> args;
vector<string> argsSrv;

// Salva i thread creati
vector<thread> cThreads;

// Salva gli utenti connessi e gli utenti bannati dallo server
vector<Client> clientList;
vector<unsigned long> banList;

// funzione che divide una stringa in 1+ parti dato un delimitatore
vector<string> customSplit(string str, char separator) {
    vector<string> arg;
    int startIndex = 0, endIndex = 0;
    for (int i = 0; i <= str.size(); i++) {

        if (str[i] == separator || i == str.size()) {
            endIndex = i;
            string temp;
            temp.append(str, startIndex, endIndex - startIndex);
            arg.push_back(temp);
            startIndex = endIndex + 1;
        }
    }
    return arg;
}

// Manda un pacchetto solo a un client
void sendPack(Client c, char msg[MAX_BUFFER_SIZE]) {
    send(c.socket, msg, strlen(msg), 0);
}

// Funzione per inviare un pacchetto a tutti i client tranne il mittente
void sendNotSelf(Client c, char msg[MAX_BUFFER_SIZE]) {
    char temp[MAX_BUFFER_SIZE];
    strcpy(temp, "%");
    strcat(temp, c.color.c_str());
    strcat(strcat(temp, "%&sep$%"), msg);

    for(auto& s: clientList) {
        if(c.socket != s.socket) {
            sendPack(s, temp);
        }
    }
}

// Funzione per inviare un messaggio a tutti i client
void broadcast(char msg[MAX_BUFFER_SIZE]) {
    for(auto& s: clientList) {
        sendPack(s, msg);
    }
}

// Funzione per ottenere un puntatore al client in base al nome
Client *getClient(string name) {
    for(auto& c: clientList) {
        if(strcmp(c.name, name.c_str()) == 0) {
            return &c;
        }
    }
    return &clientList[-1];
}

// Funzione per cercare un client in base al nome
bool searchClient(string name) {
    for(auto& c: clientList) {
        if(strcmp(c.name, name.c_str()) == 0) {
            return true;
        }
    }
    return false;
}

// Funzione per rimuovere un client dalla lista dei client
void removeClient(SOCKET s) {
    vector<Client>::iterator i;
    i = clientList.begin();
    for(auto& c: clientList) {
        if(c.socket == s) {
            clientList.erase(i);
            c.~Client();
            break;
        }
        i++;
    }
}

// Questa funzione gestisce i dati ricevuti dai client
void clientMsg(Client c) {
    char buffer[MAX_BUFFER_SIZE];
    int msg;

    while (true) {

        char temp[MAX_BUFFER_SIZE];
        strcpy(temp, c.name);

        // Riceve messaggio dal cliente
        memset(buffer, 0, sizeof(buffer));
        msg = recv(c.socket, buffer, MAX_BUFFER_SIZE, 0);

        // Controlla se il client non è più connesso
        if (msg <= 0) {
            removeClient(c.socket);
            string t = c.name;
            t = "[" + t + " disconnected]";
            cout << ">>> [" << c.name << " disconnected]" << endl;
            broadcast((char *) t.c_str());
            break;
        }

        // Se (if == true) capisce che il messaggio mandato è un commando e gestisce quello in modo giusto, altrimenti manda il messaggio normale (non commando) a tutti i client tranne il mittente
        if(buffer[0] == 33) {
            char t1[MAX_BUFFER_SIZE];
            strcpy(t1, buffer);
            string t = t1;
            args = customSplit(t, sep);

            if(args[0] == "!help") {
                char helpMsg[] = "[!clear -> Clear screen]\n> [!exit -> Disconnect from server]\n> [!changename (name) -> Changes your name for the current connection]\n> [!userlist -> Shows a list of all the users connected to the server]\n> [!color (color name) -> Changes what color your name appears to others. Supported colors: {red, blue, green, aqua, purple, yellow, white, grey}]";
                sendPack(c, helpMsg);
            } else if(args[0] == "!clear") {
                char clear[] = "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n";
                sendPack(c, clear);
                cout << ">>> [" <<  c.name << " requested to clear their chat]" << endl;
            } else if(args[0] == "!exit") {
                sendPack(c, (char *) "[You have successfully been disconnected from the server.]");
                removeClient(c.socket);
                closesocket(c.socket);
                c.~Client();
            } else if(args[0] == "!changename") {
                if(args.size() > 1) {
                    if(!searchClient(args[1])) {
                        for(int i = 2; i < args.size(); i++) {
                            args[1] = args[1] + " " + args[i];
                        }

                        cout << ">>> [" <<  c.name << " requested to change their name to '" << args[1].c_str() << "']" << endl;
                        string t = temp;
                        t = "['" + t + "' requested to change their name to '" + args[1] + "']";
                        sendNotSelf(c, (char *) t.c_str());
                        t = args[1];
                        t = "[Your name changed to '" + t + "']";
                        sendPack(c, (char *) t.c_str());

                        Client *cT = getClient(c.name);
                        strcpy(cT->name, args[1].c_str());
                        c = *cT;
                    } else {
                        sendPack(c, (char *) "[Username chosen has been taken, try another.]");
                    }
                } else {
                    sendPack(c, (char *) "[Invalid arguments! Try again]");
                }
            } else if(args[0] == "!userlist") {
                string list = c.name;
                list += " ->";
                for(auto& s: clientList) {
                    if(c.socket != s.socket) {
                        if(s.socket == clientList.begin()->socket) {
                            list = list + " " + s.name;
                        } else {
                            list = list + " | " + s.name;
                        }
                    }
                }
                sendPack(c, (char *) list.c_str());
            } else if(args[0] == "!color") {
                bool confirm = false;
                for(string a: colors) {
                    if(a == args[1]) {
                        confirm = true;
                    }
                }
                if(confirm) {

                    cout << ">>> " << c.name << " changed their chat color from ['" << c.color << "'] to ['" << args[1] << "]'" << endl;
                    sendPack(c, (char *) "[Color changed!]");

                    Client *cT = getClient(c.name);
                    cT->color = args[1];
                    c = *cT;
                } else {
                    sendPack(c, (char *) "[The requested color is not supported.]");
                }
            }
            args.clear();
        } else {
            Client *cT = getClient(c.name);
            if(cT->muted) {
                sendPack(c, (char *) "[You are muted!]");
            } else {
                cout << ">>> " << c.name << ": " << buffer << endl;
                string t = c.name;
                string t1 = buffer;
                t += "%&sep$%: " + t1;

                sendNotSelf(c, (char *) (t.c_str()));
            }
        }
    }
}

// Questa funzione gestisce i commandi che può seguire lo server, diversi a quelli dei client
void serverCommands(SOCKET s) {
    while(true){
        char command[MAX_BUFFER_SIZE];
        cin.getline(command, MAX_BUFFER_SIZE);

        if(command[0] == 33) {
            char t1[MAX_BUFFER_SIZE];
            strcpy(t1, command);
            string t = t1;
            argsSrv = customSplit(t, sep);

            if (argsSrv[0] == "!kick") {
                for (int i = 2; i < argsSrv.size(); i++) {
                    argsSrv[1] = argsSrv[1] + " " + argsSrv[i];
                }

                if (searchClient(argsSrv[1])) {
                    Client *c = getClient(argsSrv[1]);
                    cout << ">>> [Kicking " << c->name << "]" << endl;
                    sendPack(*c, (char *) "%&endKick$%");
                    closesocket(c->socket);
                } else {
                    cout << ">>> [Client not found]\n";
                }
            } else if (argsSrv[0] == "!restart") {
                cout << ">>> [Restarting server]\n";
                if (clientList.size() > 0) {
                    for (auto &c: clientList) {
                        closesocket(c.socket);
                        c.~Client();
                    }
                    clientList.clear();
                }
                if(banList.size() > 0) banList.clear();
            } else if (argsSrv[0] == "!stop") {
                closesocket(s);
                offline = true;
                break;
            } else if (argsSrv[0] == "!mute") {
                for (int i = 2; i < argsSrv.size(); i++) {
                    argsSrv[1] = argsSrv[1] + " " + argsSrv[i];
                }

                if (searchClient(argsSrv[1])) {
                    Client *c = getClient(argsSrv[1]);
                    cout << ">>> [Muting " << c->name << "]" << endl;
                    sendPack(*c, (char *) "[You are now muted in this server.]");
                    c->muted = true;
                } else {
                    cout << ">>> [Client not found]\n";
                }
            } else if (argsSrv[0] == "!userlist") {
                string list;
                for (auto &s: clientList) {
                    string t = s.name;
                    if (s.socket == clientList.begin()->socket) {
                        list += t;
                    } else {
                        list += ", " + t;
                    }
                }
                cout << ">>> " << list << endl;
            } else if(argsSrv[0] == "!ban") {
                for (int i = 2; i < argsSrv.size(); i++) {
                    argsSrv[1] = argsSrv[1] + " " + argsSrv[i];
                }

                if (searchClient(argsSrv[1])) {
                    Client *c = getClient(argsSrv[1]);
                    cout << ">>> [Banning " << c->name << "]" << endl;
                    sendPack(*c, (char *) "%&endBan$%");
                    banList.push_back(c->address);
                    closesocket(c->socket);
                } else {
                    cout << ">>> [Client not found]\n";
                }
            } else if(argsSrv[0] == "!getinfo") {
                cout << ">>> Threads: " << cThreads.size() << endl;
                cout << ">>> Clients: " << clientList.size() << endl;
                for(Client c: clientList) {
                    cout << ">>> Client socket: " << c.socket;
                    cout << " -> Client Address: " << c.address;
                    cout << " -> Client name: " << c.name;
                    cout << " -> Client color: " << c.color;
                    cout << " -> Client muted status: " << c.muted << endl;
                }
            }
            argsSrv.clear();
        }
    }
}

// Funzione main
int main() {
    int port;
    cout << "Starting server on port: "; cin >> port;
    // Parte Winsock
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    // Crea un socket
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    // Prepara i dati dello server (tipo, indirizzo e porta)
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(port);

    // Bind
    bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

    // Listen
    listen(serverSocket, 3);

    cout << ">>> Server is running on port " << port << endl;
    thread serverThread(serverCommands, serverSocket);

    // Questo while gestisce tutti i client che vogliono connettere allo server
    while(true) {
        SOCKET clientSocket;
        sockaddr_in clientAddress{};
        int clientAddressLength = sizeof(clientAddress);


        // Accetta la connessione
        if ((clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddressLength)) != INVALID_SOCKET) {
            // Get client name
            char clientName[MAX_BUFFER_SIZE];
            memset(clientName, 0, sizeof(clientName));
            recv(clientSocket, clientName, MAX_BUFFER_SIZE, 0);

            Client c(clientSocket, clientName, clientAddress.sin_addr.s_addr);

            bool banControl = false;
            for (unsigned long a: banList) {
                if (a == clientAddress.sin_addr.s_addr) {
                    banControl = true;
                }
            }
            // Diversi controlli che possono dire che la connessione è fallita
            if (searchClient(c.name)) {
                sendPack(c, (char *) "%&connFailureNameTaken$%");
                closesocket(c.socket);
                c.~Client();
                continue;
            } else if (banControl) {
                sendPack(c, (char *) "%&connFailureBanned$%");
                closesocket(c.socket);
                c.~Client();
            } else {
                sendPack(c, (char *) "%&connSuccess$%");
            }

            cout << ">>> [" << clientName << " connected]" << endl;

            // Aggiunge il nuovo client al vettore e crea un nuovo thread
            clientList.push_back(move(c));
            cThreads.push_back(thread(clientMsg, c));

            string t = clientName;
            t = "['" + t + "' connected]";
            broadcast((char *) t.c_str());
        }

        // Esce da while se offline
        if(offline) {
            break;
        }
    }
}