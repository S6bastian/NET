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
 
using namespace std;

// thread (threadReadSocket,SocketFD).detach()


//cd -l 45000   to try


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
  total += msg.size()+ nickname.size();
  return total;
}

void threadReadSocket(int ServerFD){
  char local_buffer[1000];
  string nickname,msg;
  int n;
  while(true){
    //n = read(ServerFD,local_buffer,255);
    readTCP(ServerFD,local_buffer,nickname,msg);
    //if(n <= 0) break;
    //local_buffer[n] = '\0';
    //msg = local_buffer;
    cout << nickname << ": " << msg << "\n";
  }
}


int main(void)
{
  struct sockaddr_in stSockAddr;
  int Res;
  int ClientFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  char buffer[256];
  int n;
  string nickname, msg;

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

  


  cin >> nickname;
  cin.ignore();

  write(ClientFD,nickname.c_str(),(int)nickname.size()); // n era la cantidad de bits leidos os escritos
  
  n = read(ClientFD,buffer,255);
  if (n < 0) perror("ERROR reading from socket");
  buffer[n] = '\0';
  msg = buffer;
  cout << msg << "\n";

  thread t(threadReadSocket,ClientFD);
  t.detach();


  while(true){
    cin >> nickname;
    cin.ignore();
    getline(cin,msg);
    //write(ClientFD,msg.c_str(),(int)msg.size());
    writeTCP(ClientFD,buffer,nickname,msg);
  
    if (msg == "exit") {
      cout << "Servidor cerró la conexión\n";
      break;
    }
  }



  shutdown(ClientFD, SHUT_RDWR);

  close(ClientFD);
  return 0;
}