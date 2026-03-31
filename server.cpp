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

using namespace std;

int writeTCP(const int &FD, char* buffer, const string &name, const string &msg){
  snprintf(buffer, 3+1, "%03d", (int)name.size());
  int n = 3;
  snprintf(buffer+n, (int)name.size()+1, "%s", name.c_str());
  n += name.size();
  snprintf(buffer+n, 3+1, "%03d", (int)msg.size());
  n += 3;
  snprintf(buffer+n, (int)msg.size()+1, "%s",msg.c_str());
  n += msg.size();
  write(FD,buffer,n);
  return n;
}

void readTCP(const int &FD, char* buffer, string &name, string &msg){
  int n;
  n = read(FD,buffer,3);
  buffer[n] = '\0';
  int l = atoi(buffer);
  n = read(FD,buffer,l);
  buffer[n] = '\0';
  name = buffer; //strcpy(name,buffer);
  n = read(FD,buffer,3);
  buffer[n] = '\0';
  int ll = atoi(buffer);
  n = read(FD,buffer,ll);
  buffer[n] = '\0';
  msg = buffer;
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
  stSockAddr.sin_port = htons(8888);
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

  int ClientFD = accept(ServerFD, NULL, NULL); // con esto acepta solo un cliente
  if(0 > ClientFD){
    perror("error accept failed");
    close(ServerFD);
    exit(EXIT_FAILURE);
  }


  string name, msg;
  n = read(ClientFD,buffer,256);
  buffer[n] ='\0';
  msg = buffer;
  cout << msg << "\n";
  write(ClientFD, "Connected to server.", 20);


  while(true){
    
    readTCP(ClientFD,buffer,name,msg);
    cout << name << ": " << msg << "\n";



    cin >> name;
    cin.ignore();
    getline(cin,msg);
    
    if (name == "exit" || msg == "exit") {
      printf("Cerrando servidor...\n");
      shutdown(ClientFD, SHUT_RDWR);
      close(ClientFD);
      break;
    }

    writeTCP(ClientFD,buffer,name,msg);

    /*
    bzero(buffer,256);
    n = read(ClientFD,buffer,256);
    if (n < 0) perror("ERROR reading from socket");
    buffer[strcspn(buffer, "\n")] = '\0';
    printf("Cliente: [%s]\n",buffer);

    //n = write(ClientFD,"I got your message",256);
    char newMessage[256];
    fgets(newMessage,256,stdin);
    if (strncmp(newMessage, "exit", 4) == 0) {
      printf("Cerrando servidor...\n");
      shutdown(ClientFD, SHUT_RDWR);
      close(ClientFD);
      break;
    }
    n = write(ClientFD,newMessage,256);
    */

    if (n < 0) perror("ERROR writing to socket");

    //shutdown(ClientFD, SHUT_RDWR);

    //close(ClientFD); // 
  }

  close(ServerFD);
  return 0;
}


// para matar el puerto     sudo fuser -k 8888/tcp