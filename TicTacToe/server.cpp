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

char changeTurn(Game& g){
    char t = g.letter;
    t = (t=='X')? 'O': 'X';
    return t;
}


int checkWinner(Game& g){
    int win[8][3] = {
        {0,1,2}, {3,4,5}, {6,7,8},
        {0,3,6}, {1,4,7}, {2,5,8}, 
        {0,4,8}, {2,4,6}           
    };

    for(int i = 0; i < 8; i++){
        int a = win[i][0];
        int b = win[i][1];
        int c = win[i][2];

        if(g.table[a] != ' ' &&
           g.table[a] == g.table[b] &&
           g.table[b] == g.table[c]){

            if(g.table[a] == 'X') return 1;
            else return 2;
        }
    }

    return 0; 
}


bool validateMove(const Game& g, int pos){
    if(pos < 1 || pos >9) return false;
    if(g.table[pos-1] != ' ') return false;
    return true;
}
bool isFull(const Game& g){
    for(int i = 0;i<9;i++){
        if(g.table[i]== ' ') return false;
    }
    return true;
}





bool recibir_struct(int socket, void* buffer, size_t size){
    ssize_t total = 0;
    while(total < size){
        ssize_t n = read(socket, (char*)buffer+total, size-total);
        if(n<= 0) return false;
        total+=n;

    }
    return true;
}


int main(){
    int socket_server = socket(AF_INET, SOCK_STREAM,0);
    sockaddr_in d_server{};
    d_server.sin_family = AF_INET;
    d_server.sin_port = htons(8888);
    d_server.sin_addr.s_addr = INADDR_ANY;

    bind(socket_server, (sockaddr*)&d_server, sizeof(d_server));
    cout << "listening...\n";
    listen(socket_server, 1);

    int socket_client = accept(socket_server, NULL, NULL);
    

    Game game;
    Move move;


    while(true){

        ssize_t n = read(socket_client, (char*)&move, sizeof(move));
        if(n <= 0){
            cout << "Error" << endl;
            break;
        }
        if(validateMove(game, move.position)){
          
            game.table[move.position-1] = game.letter;
            game.winner = checkWinner(game);
            if(game.winner == 0 && isFull(game)) game.winner = 3;
            if(game.winner == 0) game.letter = changeTurn(game);
        } else {
            cout << "Invalid move" << endl;
        }
        write(socket_client, (char*)&game, sizeof(game));
        if(game.winner != 0){
            if(game.winner == 3){
              cout << "\n----TIE----\n";
            }else{
              cout << "WINNER: " << (game.winner == 1 ? 'X' : 'O') << "\n";
            }
            break;
    }
}

    close(socket_server);
    close(socket_client);
    return 0;
}