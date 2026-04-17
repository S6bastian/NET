 /* Client code in C++ */
 
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <string>
#include <thread>
#include <iomanip>
#include <sstream>
#include <vector>
#include <map>
 
using namespace std;

// thread (threadReadSocket,SocketFD).detach()


//cd -l 45000   to try



class ClientTCP{
public:
  ClientTCP(){
    logged = false;

    ClientFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    

    if (-1 == ClientFD)
    {
      perror("cannot create socket");
      exit(EXIT_FAILURE);
    }

    memset(&stSockAddr, 0, sizeof(struct sockaddr_in));

    stSockAddr.sin_family = AF_INET;
    stSockAddr.sin_port = htons(45000);
    Res = inet_pton(AF_INET, "10.0.2.15", &stSockAddr.sin_addr);

    if (0 > Res)
    {
      perror("error: first parameter is not a valid address family");
      close(ClientFD);
      exit(EXIT_FAILURE);
    }
    else if (0 == Res)
    {
      perror("char string (second parameter does not contain valid ipaddress");
      close(ClientFD);
      exit(EXIT_FAILURE);
    }

    if (-1 == connect(ClientFD, (const struct sockaddr *)&stSockAddr, sizeof(struct sockaddr_in)))
    {
      perror("connect failed");
      close(ClientFD);
      exit(EXIT_FAILURE);
    }

    login();

    thread t(&ClientTCP::threadReadSocket,this,ClientFD);
    t.detach();
  }
  
  ~ClientTCP(){
    shutdown(ClientFD, SHUT_RDWR);
    close(ClientFD);
    cout  << "*********************Disconnected**********************\n";
  }

  int display_interface(){
    cout  << "*******************************************************\n"
          << "**************ClientTCP Display Interface**************\n"
          << "*******************************************************\n"
      << "Choose an option:\n"
      << "1. Logout\n"
      << "2. Broadcast\n"
      << "3. Unicast\n"
      << "4. List\n"
      << "\n\n";

    int opt;
    while(opt > 3 && opt < 1){
      cout << "Select an allowed option: ";
      cin >> opt;
      cin.ignore();
      cout << "\n";
    }

    vector<int> headBytes;
    vector<string> content;
    switch(opt){
      case 1:   //Logout
        headBytes = {1};   //key
        content = {"O"};

        write_TCP(ClientFD,headBytes,content);
        return 0;
        break;

      case 2:   //Broadcast
        headBytes = {1,7};    //key,msg;
        content.assign(headBytes.size(),"");
        
        content[0] = "B";
        getline(cin,content[1]);

        write_TCP(ClientFD,headBytes,content);
        break;
      
      case 3:   //Unicast
        headBytes = {1,5,7};    //key,msg,nickname;
        content.assign(headBytes.size(),"");
        
        content[0] = "U";

        for(size_t i = 1; i < content.size(); i++){
          getline(cin,content[i]);
        }

        write_TCP(ClientFD,headBytes,content);
        break;

      case 4:   //List
        headBytes = {1};   //key
        content = {"T"};

        write_TCP(ClientFD,headBytes,content);
        break;
    }

    return 1;
  }


private:
  bool logged;

  struct sockaddr_in stSockAddr;
  int Res;
  int ClientFD;
  char buffer[256];
  int n;
  string nickname;


  void login(){
    cout  << "*******************************************************\n"
          << "********************ClientTCP Login********************\n"
          << "*******************************************************\n";
    while(true){
      cout << "Enter nickname: ";
      cin >> nickname;
      vector<int> headBytes = {1,4}; 
      vector<string> content = {"L",nickname};
      write_TCP(ClientFD,headBytes,content);
      char ans[2];
      read_TCP(ClientFD,ans);
      if(ans[0] == 'O'){
        cout << "Logged in succesfully\n";
        logged = true;
        break;
      }
      else cout << "There is already a " << nickname << "logged. Try another nickname\n";
    }
    
  }

  int write_TCP(const int &FD, const vector<int>& headBytes, const vector<string>& content){
      ostringstream oss;

      oss << setw(headBytes[0]) << content[0];
      
      for(int i = 1; i < (int)headBytes.size(); i++){
        oss << setfill('0') << setw(headBytes[i]) << content[i].length()
          << content[i];
      }
      
      string packet = oss.str();

      int total_sent = write(FD, packet.data(), (int)packet.size());

      if (total_sent == -1){
          return -1;
      }

      return total_sent;
  }

  int read_TCP(const int &FD, char* buffer){
    vector<int> headBytes;
    vector<string> content;
    int received = read(FD,buffer,1);
    if(received == -1){
      cout << "ERROR\n";
      return -1;
    }
    
    buffer[received] = '\0';
    string opt = buffer;
    
    if(opt == "K") cout << "-->OK\n";
    else if(opt == "E") cout << "-->ERROR\n";
    else if(opt == "u"){
      headBytes = {1,7,5};    //key,nickname,msg
      content = {"u","[unicast]:","\n-->"};
    }
    else if(opt == "b"){
      headBytes = {1,3,7};
      content = {"b","[broadcast]:","\n-->"};
    }

    
    //cout << buffer ;

    for(int i = 1; i < (int)headBytes.size(); i++){
      received = read(FD,buffer,headBytes[i]);
      buffer[received] = '\0';
      int msgSz = atoi(buffer);
      received = read(FD,buffer,msgSz);
      buffer[received] = '\0';
      cout << content[i] << " " << buffer << " ";
    }
    cout << "\n";
    return received;
  }
  
  void threadReadSocket(const int& ClientFD){
    char local_buffer[1000];
    while(read_TCP(ClientFD,local_buffer) > 0);
  }

};




int main(void)
{
  ClientTCP client;
  while(client.display_interface());


  return 0;
}