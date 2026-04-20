#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <iostream>       
#include <thread>         
#include <string>
#include <mutex>
#include <map>
#include <filesystem>
using namespace std;



struct Game{
public:
    char table[9];
    char letter;
    int winner;
    Game(){
        for(int i = 0; i < 9; i++){
            table[i] = ' ';
        }
        
        letter = 'X';
        winner = 0;
    }

};

struct Move{
    public:
    int position;
};


void printTable(Game& g){
    cout << "\n";
    cout << g.table[0] << "|" << g.table[1] << "|" << g.table[2] << "\n";
    cout << "-----\n";
    cout << g.table[3] << "|" << g.table[4] << "|" << g.table[5] << "\n";
    cout << "-----\n";
    cout << g.table[6] << "|" << g.table[7] << "|" << g.table[8] << "\n";
}


bool receiveStruct(int socket, void* buffer, size_t size){
    ssize_t total = 0;
    while(total < size){
        ssize_t n = read(socket, (char*)buffer+total, size-total);
        if(n<= 0) return false;
        total+=n;

    }
    return true;
}



int main(){
    int socket_client = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in d_client{};
    d_client.sin_family=AF_INET;
    d_client.sin_port=htons(8888);
    inet_pton(AF_INET, "10.0.2.15", &d_client.sin_addr);

    if(connect(socket_client, (sockaddr*)&d_client, sizeof(d_client)) < 0){
        perror("connect");
        return 1;
    }
    
    
    
    Game game;
    Move move;

    printTable(game);
 
    while(true){
        int ver;
        while(true){
            cout << "Choose a square from 1 to 9, turn:" << game.letter << "\n";
            if(!(cin >> ver)){
                cin.clear();
                cin.ignore(1000, '\n');
                continue;
            }
            if(ver >= 1 && ver <= 9 && game.table[ver-1] == ' ') break;
            cout << "Invalid move\n";
        }
        move.position = ver;
        write(socket_client, (char*)&move, sizeof(move));

        if(!receiveStruct(socket_client, &game, sizeof(game))){
            cout << "Error receiving table\n";
            return 1;
        }
        system("clear");
        printTable(game);

        if(game.winner != 0){
            if(game.winner == 3){
                cout << "\n----TIE----" << endl;
            }else{
              cout << "WINNER: " << (game.winner == 1 ? 'X' : 'O') << "\n";
            }
            break;
        }
    }







    close(socket_client);
}