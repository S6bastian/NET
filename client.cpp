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
    ServerFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    

    if (-1 == ServerFD)
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
      close(ServerFD);
      exit(EXIT_FAILURE);
    }
    else if (0 == Res)
    {
      perror("char string (second parameter does not contain valid ipaddress");
      close(ServerFD);
      exit(EXIT_FAILURE);
    }

    if (-1 == connect(ServerFD, (const struct sockaddr *)&stSockAddr, sizeof(struct sockaddr_in)))
    {
      perror("connect failed");
      close(ServerFD);
      exit(EXIT_FAILURE);
    }

  }
  
  ~ClientTCP(){
    shutdown(ServerFD, SHUT_RDWR);
    close(ServerFD);
  }

  void display_interface(){
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
    while(opt > 5 && opt < 1){
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

        write_TCP(ServerFD,headBytes,content);
        break;

      case 2:   //Broadcast
        headBytes = {1,7};    //key,msg;
        content.assign(headBytes.size(),"");
        
        content[0] = "B";
        getline(cin,content[1]);

        write_TCP(ServerFD,headBytes,content);
        break;
      
      case 3:   //Unicast
        headBytes = {1,5,7};    //key,msg,nickname;
        content.assign(headBytes.size(),"");
        
        content[0] = "U";

        for(size_t i = 1; i < content.size(); i++){
          getline(cin,content[i]);
        }

        write_TCP(ServerFD,headBytes,content);
        break;
    }

    
  }


private:
  map<char,string> xd;
  bool logged;

  struct sockaddr_in stSockAddr;
  int Res;
  int ServerFD;
  char buffer[256];
  int n;
  string nickname;


  void login(){

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

  int read_TCP(const int &FD, char* buffer, const vector<int>& headBytes, const vector<string>& content){
    int received = -1;

    for(int i = 0; i < (int)headBytes.size(); i++){
      received = read(FD,buffer,headBytes[i]);
      buffer[received] = '\0';
      int msgSz = atoi(buffer);
      received = read(FD,buffer,msgSz);
      buffer[received] = '\0';
      cout << buffer;
    }
    cout << "\n";
    return received;
  }
  
  void threadReadSocket(int ServerFD){
    char local_buffer[1000];
    string nickname,msg;
    int n;
    while(true){
      //n = read(ServerFD,local_buffer,255);
      readTCP(ServerFD,local_buffer,msg);
      //if(n <= 0) break;
      //local_buffer[n] = '\0';
      //msg = local_buffer;
      cout << nickname << ": " << msg << "\n";
    }
  }

};


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