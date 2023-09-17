#include <iostream>
#include <string.h>
#include <WinSock2.h>
#include <thread>
#include <vector>
#include <sstream>
#include "color.hpp"

using namespace std;

#define MAX_BUFFER_SIZE 4096

// Funzione che gestisce il colore che un nome deve avere quando stampato
dye::R<const char*> checkColor(string color, string name) {
    if(color == "red") {
        return dye::red(name);
    } else if(color == "blue") {
        return dye::blue(name);
    } else if(color == "green") {
        return dye::green(name);
    } else if(color == "aqua") {
        return dye::aqua(name);
    } else if(color == "purple") {
        return dye::purple(name);
    } else if(color == "yellow") {
        return dye::yellow(name);
    } else if(color == "white") {
        return dye::white(name);
    } else if(color == "grey") {
        return dye::grey(name);
    } else {
        return dye::black(name);
    }
}

bool endThread = false;

// Gestisce i messaggi ricevuti
vector<string> msg;

char sep[] = "%&sep$%";

// Converta un vettore di char a una stringa
string charvToStr(vector<char> a) {
    string s;
    for(char b: a) {
        s += b;
    }
    return s;
}

// Funzopme che divide un vettore di char in parti diversi dato un delimitatore
vector<string> customSplit(char s[], char del[]) {
    int len = strlen(s);
    vector<string> args;
    vector<char> temp;
    vector<char> sepSave;
    int j, m = 0, n = 0, k = 0;

    for(int i = 0; i < len; i++) {
        bool control = false;
        n = 0;
        for (j = i; j < len; j++) {
            if (s[j] == del[0]) break;
            temp.push_back(s[j]);
        }
        if (j < len) {
            for (k = j; k < j + strlen(del); k++) {
                if (s[k] != del[n]) {
                    control = true;
                    break;
                }
                n++;
                sepSave.push_back(s[k]);
            }
            if(control) {
                for(char a: sepSave) {
                    temp.push_back(a);
                }
            } else {
                string tString = charvToStr(temp);
                args.push_back(tString);
            }
            temp.clear();
            sepSave.clear();
        } else {
            string tString = charvToStr(temp);
            args.push_back(tString);
        }
        i = j;
    }
    return args;
}

// Funzione che gestisce i messaggi inviati dal client allo server
void sendMsg(SOCKET c1) {
    while(true) {
        char buffer[MAX_BUFFER_SIZE];
        cin.getline(buffer, MAX_BUFFER_SIZE);
        if(strcmp(buffer, "!exit") == 0) {
            endThread = true;
        }

        send(c1, buffer, strlen(buffer), 0);

        if(endThread) {
            break;
        }
    }
}

// Funzione che gestisce i messaggi ricevuti dallo server
void getMsg(SOCKET c1) {
    while(true) {
        char buffer[MAX_BUFFER_SIZE];
        int bytesRead;
        // Receive message from server
        memset(buffer, 0, sizeof(buffer));
        bytesRead = recv(c1, buffer, MAX_BUFFER_SIZE, 0);
        if (bytesRead == SOCKET_ERROR) {
            endThread = true;
            cout << "Server disconnected." << endl;
            break;
        }

        if(bytesRead <= 0) {
            endThread = true;
            break;
        }

        if(strcmp(buffer, "%&endKick$%") == 0) {
            endThread = true;
            cout << "You have been kicked from the server." << endl;
            break;
        }

        if(strcmp(buffer, "%&endBan$%") == 0) {
            endThread = true;
            cout << "You have been banned from the server." << endl;
            break;
        }

        // Se (if == true) vuol dire che il messaggio inviato è da un altro client, altrimenti è un messaggio "server";
        if(buffer[0] == 37){
            msg = customSplit(buffer, sep);

            if(msg.size() > 2) {
                auto nameColor = checkColor(msg[0], msg[1]);
                cout << "> " << nameColor << msg[2] << endl;
            } else {
                cout << checkColor(msg[0], msg[1]) << endl;
            }
            msg.clear();
        } else {
            cout << buffer << endl;
        }
    }
}

int main() {
    while(true) {
        endThread = false;
        char name[MAX_BUFFER_SIZE];
        char sAddress[15];
        int port;

        cout << "Connect to a server!" << endl;
        cout << "Your name: "; cin.getline(name, MAX_BUFFER_SIZE);
        cout << "Server IP: "; cin >> sAddress;
        cout << "Port: "; cin >> port;
        // Parte Winsock
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);

        // Crea un scokter
        SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket == INVALID_SOCKET) {
            cerr << "Failed to create socket" << endl;
            WSACleanup();
            return 1;
        }

        // Prepara i dati dello server (tipo, indirizzo e porta)
        sockaddr_in serverAddress{};
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_addr.s_addr = inet_addr(sAddress);
        serverAddress.sin_port = htons(port);

        // Connessione allo server
        if (connect(clientSocket, reinterpret_cast<const sockaddr *>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR) {
            cerr << "Failed to connect to server" << endl;
            closesocket(clientSocket);
            WSACleanup();
            return 1;
        }

        char buffer[MAX_BUFFER_SIZE];

        // Invia il nome del client allo server e gestisce la connessione (successo o fallito)
        send(clientSocket, name, strlen(name), 0);
        recv(clientSocket, buffer, MAX_BUFFER_SIZE, 0);

        if (strcmp(buffer, "%&connSuccess$%") == 0) {
            cout << "Connected to server - !help for commands\n" << endl;
        } else if(strcmp(buffer, "%&connFailureNameTaken$%") == 0) {
            cout << "This name is taken, try connecting with another name.\n" << endl;
            continue;
        } else if (strcmp(buffer, "%&connFailureBanned$%") == 0) {
            cout << "Your address is banned from this server." << endl;
            continue;
        }

        // Crea i thread per sendMsg() e getMsg()
        thread t2(getMsg, clientSocket);
        thread t1(sendMsg, clientSocket);
        t2.join();
        t1.join();

        closesocket(clientSocket);
        WSACleanup();
    }
    return 0;
}
