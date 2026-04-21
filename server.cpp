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
#include <fstream>
#include <cstdio>

#define LOGIN_BYTES 1,4                   //key,nick
#define LOGOUT_BYTES 1                    //key
#define BROADCAST_BYTES 1,7               //key,msg
#define BROADCAST_RESPONSE_BYTES 1,3,7    //key,nick,msg
#define UNICAST_BYTES 1,5,7               //key,msg,nick
#define UNICAST_RESPONSE_BYTES 1,7,5      //key,nick,msg
#define LIST_BYTES 1                      //key
#define LIST_RESPONSE_BYTES 1,5           //key,size
#define FILE_BYTES 1,5,5,5                //key,file,filename,destnick
#define FILE_RESPONSE_BYTES 1,5,5,5       //key,file,filename,sourcenick         
#define OK_BYTES 1                        //key
#define ERROR_BYTES 1,5                   //key,msg

#define LOGIN_KEY "L"
#define LOGOUT_KEY "O"
#define BROADCAST_KEY "B"
#define BROADCAST_RESPONSE_KEY "b"
#define UNICAST_KEY "U"
#define UNICAST_RESPONSE_KEY "u"
#define LIST_KEY "T"
#define LIST_RESPONSE_KEY "t"
#define FILE_KEY "F"
#define FILE_RESPONSE_KEY "f"
#define OK_KEY "K"
#define ERROR_KEY "E"

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
          << "*******************************************************\n\n";
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
      

      if(content[0] == LOGIN_KEY && clients.find(content[1]) == clients.end()){
        cout << "---> " << content[1] << " joined\n"; 
        clients[content[1]] = ClientFD;
        write_TCP(ClientFD,{OK_BYTES},{OK_KEY});
        thread t(&ServerTCP::threadReadSocket,this,ClientFD,content[1]);
        t.detach();
      }
      else{
        write_TCP(ClientFD,{ERROR_BYTES},{ERROR_KEY,"Rejected"});
        close(ClientFD);
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
      
      for(size_t i = 1; i < headBytes.size(); i++){
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
    char buffer[99999];
    int received = read(FD,buffer,1);
    if(received == -1){
      headBytes = {ERROR_BYTES};
      content = {ERROR_KEY,"server could not read the message"};
      return -1;
    }
    
    buffer[received] = '\0';
    string opt = buffer;
    
    if(opt == LOGIN_KEY){
      headBytes = {LOGIN_BYTES};    //key,nickname
      content = {LOGIN_KEY,""};
    }
    else if(opt == LOGOUT_KEY){
      headBytes = {LOGOUT_BYTES};      //key
      content = {LOGOUT_KEY};
    }
    else if(opt == BROADCAST_KEY){
      headBytes = {BROADCAST_BYTES};    //key,msg
      content = {BROADCAST_KEY,""};
    }
    else if(opt == UNICAST_KEY){
      headBytes = {UNICAST_BYTES};    //key,msg,nickname
      content = {UNICAST_KEY,"",""};
    }
    else if(opt == LIST_KEY){
      headBytes = {LIST_BYTES};    //key
      content = {LIST_KEY};
    }
    else if(opt == FILE_KEY){    //
      headBytes = {FILE_BYTES};    //key,file,msg,nickname
      content = {FILE_KEY,"","",""};
      return 1;
    }

    for(size_t i = 1; i < headBytes.size(); i++){
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
      if(opt == LOGOUT_KEY){
        cout << local_nickname << " has left the chat\n";
        //write_TCP(ClientFD,{LOGOUT_BYTES},{LOGOUT_KEY});
        write_TCP(ClientFD,{OK_BYTES},{OK_KEY});
        clients.erase(local_nickname);
        if(clients.empty()){
          working = false;
          cout << "Everyboyd is gone\n";
        }
        close(ClientFD);
        break;
      }
      else if(opt == BROADCAST_KEY){
        for(const auto& client : clients){
          write_TCP(client.second,
            {BROADCAST_RESPONSE_BYTES},
            {BROADCAST_RESPONSE_KEY,local_nickname,content[1]});
        }
      }
      else if(opt == UNICAST_KEY){
        if(clients.find(content[2]) == clients.end())
          write_TCP(clients[local_nickname],
          {ERROR_BYTES},
          {ERROR_KEY,"User not found"});
        else 
          write_TCP(clients[content[2]],
          {UNICAST_RESPONSE_BYTES},
          {UNICAST_RESPONSE_KEY,local_nickname,content[1]});
      }
      else if(opt == LIST_KEY){
        string fileName = "list.json";
        ofstream outFile(fileName);
        if (outFile.is_open()) {
            outFile << "{\n  \"users\": [\n";
            for (auto it = clients.begin(); it != clients.end(); ++it) {
                outFile << "    \"" << it->first << "\"";
                if (std::next(it) != clients.end()) outFile << ",";
                outFile << "\n";
            }
            outFile << "  ]\n}";
            outFile.close();
        }

        ifstream inFile(fileName);
        stringstream buffer;
        buffer << inFile.rdbuf();
        string jsonContent = buffer.str();
        inFile.close();

        write_TCP(ClientFD, {LIST_RESPONSE_BYTES}, {LIST_RESPONSE_KEY, jsonContent});
      }
      else if(opt == FILE_KEY){
        size_t n = read(ClientFD,buffer,5);
        buffer[n] = '\0';
        int fileSize = atoi(buffer);

        for(int i = 0; i < fileSize;){
          n = read(ClientFD,buffer+i,fileSize-i);
          i += n; 
        }
        buffer[fileSize] = '\0';

        
        char auxBuffer[256];

        n = read(ClientFD,auxBuffer,5);
        auxBuffer[n] = '\0';
        int auxSize = atoi(auxBuffer);
        n = read(ClientFD,auxBuffer,auxSize);
        auxBuffer[n] = '\0';
        content[2] = auxBuffer;

        n = read(ClientFD,auxBuffer,5);
        auxBuffer[n] = '\0';
        auxSize = atoi(auxBuffer);
        n = read(ClientFD,auxBuffer,auxSize);
        auxBuffer[n] = '\0';
        content[3] = auxBuffer;
        
        if(clients.find(content[3]) == clients.end()){
          write_TCP(ClientFD, {ERROR_BYTES}, {ERROR_KEY, "User not found"});
          return;
        }


        int TargetFD = clients[content[3]];
        write_TCP(TargetFD, {1}, 
                  {FILE_RESPONSE_KEY});
        
        ostringstream oss;
        oss << setfill('0') << setw(5) << content[1].length();
        string packet = oss.str();
        write(TargetFD,packet.c_str(),headBytes[1]);
        
        for(size_t i = 0; i < fileSize;){
          i += write(TargetFD,buffer+i,fileSize-i);
        }

        write_TCP(TargetFD,
                  {headBytes[2],headBytes[3]},
                  {content[2],local_nickname});

        
      }
      else{
        write_TCP(clients[local_nickname],{ERROR_BYTES},{ERROR_KEY,"My name is Giovanni Giorgio, but everybody calls me Giorgio\n"});
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