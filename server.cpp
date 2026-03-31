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

using namespace std;

map<string,int> clients;
bool working = true;


int writeTCP(const int &FD, char* buffer, const string &nickname, const string &msg){
  snprintf(buffer, 3+1, "%03d", (int)nickname.size());
  int n = 3;
  snprintf(buffer+n, (int)nickname.size()+1, "%s", nickname.c_str());
  n += nickname.size();
  snprintf(buffer+n, 3+1, "%03d", (int)msg.size());
  n += 3;
  snprintf(buffer+n, (int)msg.size()+1, "%s",msg.c_str());
  n += msg.size();
  write(FD,buffer,n);
  return n;
}

int readTCP(const int &FD, char* buffer, string &nickname, string &msg){
  int n, total = 6;
  n = read(FD,buffer,3);
  buffer[n] = '\0';
  int l = atoi(buffer);
  n = read(FD,buffer,l);
  buffer[n] = '\0';
  nickname = buffer; //strcpy(nickname,buffer);
  n = read(FD,buffer,3);
  buffer[n] = '\0';
  int ll = atoi(buffer);
  n = read(FD,buffer,ll);
  buffer[n] = '\0';
  msg = buffer;
  total += msg.size() + nickname.size();
  return total;
}

void threadReadSocket(int ClientFD,string local_nickname){
  char local_buffer[1000];
  string nickname,msg;
  int n;
  while(true){
    //n = read(ClientFD,local_buffer,255);
    n = readTCP(ClientFD,local_buffer,nickname,msg);
    //local_buffer[n] = '\0';
    //msg = local_buffer;
    if(n <= 0 || msg == "exit" || nickname == "exit"){
        /*  
        for(auto it = clients.begin(); it != clients.end(); ){
          if(it->second == ClientFD) it = clients.erase(it);
          else ++it;
        }
        */
        clients.erase(local_nickname);
        if(clients.empty()) working = false;
        close(ClientFD);
        break;
    }
    if(nickname == "all"){
      for(auto &u : clients){
        //write(u.second,msg.c_str(),(int)msg.size());
        writeTCP(u.second,local_buffer,nickname,msg);
      }
    }
    else{
      if(clients.find(nickname) != clients.end()){
          writeTCP(clients[nickname],local_buffer,local_nickname,msg);
      }
      else{
          writeTCP(ClientFD,local_buffer,"server","user not found");
      }
    }
  }
}

int main(void)
{
  struct sockaddr_in stSockAddr;
  int ServerFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  char buffer[256];
  int n;

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
  /*
  int ClientFD = accept(ServerFD, NULL, NULL); // con esto acepta solo un cliente
  if(0 > ClientFD){
    perror("error accept failed");
    close(ServerFD);
    exit(EXIT_FAILURE);
  }
  */
  

  

  string nickname, msg;

  while(true){
    
    int ClientFD = accept(ServerFD, NULL, NULL);
    if(ClientFD > 0){
      n = read(ClientFD,buffer,255);
      buffer[n] = '\0';
      nickname = buffer;
      clients[nickname] = ClientFD; 
      cout << nickname << " connected\n";
      write(ClientFD, "Connected to server.", 20);
      
      thread t(threadReadSocket,ClientFD,nickname);
      t.detach();
    }

    
    /*
    if (nickname == "exit" || msg == "exit") {
      printf("Cerrando servidor...\n");
      shutdown(ClientFD, SHUT_RDWR);
      close(ClientFD);
      break;
    }
    */

    if(!working) break;
  }

  close(ServerFD);
  return 0;
}


// para matar el puerto     sudo fuser -k 45000/tcp