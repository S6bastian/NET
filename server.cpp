/* Server code in C++ */

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
#include <map>
#include <thread>
#include <vector>
#include <sstream>
#include <iomanip>

using namespace std;

class ServerTCP{
public:
  ServerTCP(){
    cout << "Inititalizing server\n\n\n";
    ServerFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    if(-1 == ServerFD)
    {
      perror("can not create socket");
      exit(EXIT_FAILURE);
    }

    memset(&stSockAddr, 0, sizeof(struct sockaddr_in));

    stSockAddr.sin_family = AF_INET;
    stSockAddr.sin_port = htons(45000);
    stSockAddr.sin_addr.s_addr = INADDR_ANY;

    if(-1 == bind(ServerFD,(const struct sockaddr *)&stSockAddr, sizeof(struct sockaddr_in)))
    {
      perror("error bind failed");
      close(ServerFD);
      exit(EXIT_FAILURE);
    }

    if(-1 == listen(ServerFD, 10))
    {
      perror("error listen failed");
      close(ServerFD);
      exit(EXIT_FAILURE);
    }
    cout  << "*******************************************************\n"
          << "*******************ServerTCP Listening*****************\n"
          << "*******************************************************\n";
  }
  
  ~ServerTCP(){
    shutdown(ServerFD, SHUT_RDWR);
    close(ServerFD);
    cout  << "*********************Disconnected**********************\n";
  }

  int listening(){
    int ClientFD = accept(ServerFD, NULL, NULL);
    int received;
    if(!working) return 0;
    
    if(ClientFD > 0){
      vector<int> headBytes;
      vector<string> content;
      received = read_TCP(ClientFD,headBytes,content);
      

      if(content[0] == "L" && clients.find(content[1]) == clients.end()){
        clients[content[1]] = ClientFD;
        write_TCP(ClientFD,{1},{"K"});
        thread t(&ServerTCP::threadReadSocket,this,ClientFD,content[1]);
        t.detach();
      }
      else{
        write_TCP(ClientFD,{1,5},{"E","Rejected"});
      }
      
    }
    return 1;
  }


private:
  map<string,int> clients;
  bool working = true;

  struct sockaddr_in stSockAddr;
  int ServerFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  char buffer[256];


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

  int read_TCP(const int &FD, vector<int>& headBytes, vector<string>& content){
    char buffer[256];
    int received = read(FD,buffer,1);
    if(received == -1){
      cout << "ERROR\n";
      return -1;
    }
    
    buffer[received] = '\0';
    string opt = buffer;
    
    if(opt == "L"){
      headBytes = {1,4};    //key,nickname
      content = {"L",""};
    }
    else if(opt == "O"){
      headBytes = {1};      //key
      content = {"O"};
    }
    else if(opt == "B"){
      headBytes = {1,7};    //key,msg
      content = {"b",""};
    }
    else if(opt == "U"){
      headBytes = {1,5,7};    //key,msg,nickname
      content = {"u","",""};
    }
    else if(opt == "T"){
      headBytes = {1};    //key
      content = {"T"};
    }
    else if(opt == "F"){    //REVISAR FALTA IMPLENTAR
      headBytes = {1,5};    //key,msg,nickname
      content = {"u",""};
    }

    for(int i = 1; i < (int)headBytes.size(); i++){
      received = read(FD,buffer,headBytes[i]);
      buffer[received] = '\0';
      int msgSz = atoi(buffer);
      received = read(FD,buffer,msgSz);
      buffer[received] = '\0';
      string data = buffer;
      content[i] = data;
    }
    return received;
  }
  
  void threadReadSocket(int ClientFD,string local_nickname){
  vector<int> headBytes;
  vector<string> content;
  int received;
  while(true){
    received = read_TCP(ClientFD,headBytes,content);
    
    string opt = content[0];
    if(opt == "O"){
      write_TCP(ClientFD,{1},{"K"});
      clients.erase(local_nickname);
      if(clients.empty()) working = false;
      close(ClientFD);
      break;
    }
    else if(opt == "B"){
      for(const auto& client : clients){
        write_TCP(client.second,{1,3,7},{"b",local_nickname,content[1]});
      }
    }
    else if(opt == "U"){
      if(clients.find(content[2]) == clients.end())
        write_TCP(clients[local_nickname],{1,5},{"E","User not found\n"});
      else 
        write_TCP(clients[content[2]],{1,7,5},{"u",local_nickname,content[1]});
    }
    else{
      write_TCP(clients[local_nickname],{1,5},{"E","My name is Giovanni Giorgio, but everybody calls me Giorgio\n"});
    }
  }
}

};






int main(void)
{
  ServerTCP server;

  while(server.listening());

  return 0;
}


// para matar el puerto     sudo fuser -k 45000/tcp