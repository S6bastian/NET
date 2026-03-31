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
  int Res;
  int ClientFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  char buffer[256];
  int n;

  if (-1 == ClientFD)
  {
    perror("cannot create socket");
    exit(EXIT_FAILURE);
  }

  memset(&stSockAddr, 0, sizeof(struct sockaddr_in));

  stSockAddr.sin_family = AF_INET;
  stSockAddr.sin_port = htons(8888);
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





  write(ClientFD,"Client connected.",17); // n era la cantidad de bits leidos os escritos
  
  string msg;
  bzero(buffer,256);
  n = read(ClientFD,buffer,256);
  if (n < 0) perror("ERROR reading from socket");
  buffer[n] = '\0';
  msg = buffer;
  //printf("Server: [%s]\n",buffer);
  cout << msg << "\n";

  string name;
  while(true){
    cin >> name;
    cin.ignore();
    getline(cin,msg);
    
    if (msg == "exit") {
      printf("Servidor cerró la conexión\n");
      break;
    }

    writeTCP(ClientFD,buffer,name,msg);
    


    readTCP(ClientFD,buffer,name,msg);
    cout << name << ": " << msg << "\n";
    
    /*
    cout << "IMPRIMIENDO BUFFER TEST\n";
    for(int i = 0; i < n; i++){
      cout << buffer[i];
    }
    cout << "\n";
    */
    
    /*    
    //scanf("%s",newMessage); usamos fgets para mensajes con espacios 
    fgets(newMessage,256,stdin);
    n = write(ClientFD,newMessage,256);
    bzero(buffer,256);
    n = read(ClientFD,buffer,256);
    buffer[strcspn(buffer, "\n")] = '\0';
    printf("Server: [%s]\n",buffer);
    if (strncmp(buffer, "exit", 4) == 0) {
      printf("Servidor cerró la conexión\n");
      break;
    }
    */
  }



  shutdown(ClientFD, SHUT_RDWR);

  close(ClientFD);
  return 0;
}
