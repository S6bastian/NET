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
#include <fstream>
#include <set>

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

    cout  << "*******************************************************\n"
          << "**************ClientTCP Display Interface**************\n"
          << "*******************************************************\n"
      << "Choose an option:\n"
      << "1. Logout\n"
      << "2. Broadcast\n"
      << "3. Unicast\n"
      << "4. List\n"
      << "5. File\n"
      << "\n\n";
  }
  
  ~ClientTCP(){
    shutdown(ClientFD, SHUT_RDWR);
    close(ClientFD);
    cout  << "*********************Disconnected**********************\n";
  }

  int display_interface(){
    int opt = -1;
    while (true) {
      cout << "Select an allowed option (1-5):\n";
      
      if (cin >> opt) {
          if (opt >= 1 && opt <= 5) {
              break;
          } else {
              cout << "ClientError: Number out of range.\n";
          }
      } else {
          cout << "ClientError: Invalid input. Please enter a number.\n";
          
          cin.clear(); 
          cin.ignore(10000, '\n');
      }
    }
    cin.ignore();

    vector<int> headBytes;
    vector<string> content;
    switch(opt){
      case 1:   //Logout
        headBytes = {LOGOUT_BYTES};   //key
        content = {LOGOUT_KEY};

        write_TCP(ClientFD,headBytes,content);
        return 0;
        break;

      case 2:   //Broadcast        
        headBytes = {BROADCAST_BYTES};    //key,msg;
        content.assign(headBytes.size(),"");
  
        content[0] = BROADCAST_KEY;
        cout << "message: ";
        getline(cin,content[1]);

        write_TCP(ClientFD,headBytes,content);
        break;
      
      case 3:   //Unicast
        headBytes = {UNICAST_BYTES};    //key,msg,nickname;
        content.assign(headBytes.size(),"");
        
        content[0] = UNICAST_KEY;

        cout << "message: ";
        getline(cin,content[1]);
        cout << "nickname: ";
        getline(cin,content[2]);

        write_TCP(ClientFD,headBytes,content);
        break;

      case 4:   //List
        headBytes = {LIST_BYTES};   //key
        content = {LIST_KEY};

        write_TCP(ClientFD,headBytes,content);
        break;

      case 5://File
        headBytes = {FILE_BYTES};   //key,file,filename,destnick
        content.assign(headBytes.size(),"");
        content[0] = FILE_KEY;

        string fileName;
        cout << "file name(.txt): ";
        cin >> fileName;
        cin.ignore();
        content[2] = fileName;
        cout << "nickname: ";
        cin >> content[3];

        //const size_t CHUNK_SIZE = 4096;
           

        ifstream file(fileName, ios::binary);
        if (!file) {
            cout << "ClientError: Couldn't open.\n";
            return 1;  
        }
        file.seekg(0,ios::end);
        size_t fileSize = file.tellg();
        file.seekg(0,ios::beg);
        content[1] = to_string(fileSize);

        int maxByteSize = 1;
        for(int i = 0; i < 5; i++) maxByteSize *= 10;
        maxByteSize--;

        if(fileSize > maxByteSize){
          cout << "ClientError: File size is bigger than TCP parameters\n";
          return 1;
        }

        //const int CHUNK_SIZE = 4096;
        char* buffer = new char[fileSize];

        write_TCP(ClientFD,{headBytes[0]},{content[0]});

        ostringstream oss;
        oss << setfill('0') << setw(5) << content[1].length();
        string packet = oss.str();
        write(ClientFD,packet.c_str(),headBytes[1]);
        
        for(size_t i = 0; i < fileSize;){
          i += write(ClientFD,buffer+i,fileSize-i);
        }

        file.close();

        write_TCP(ClientFD,{headBytes[2],headBytes[3]},{content[2],content[3]});

        break;
    }

    return 1;
  }


private:
  bool logged;
  set<string> receivedFiles;

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
      vector<int> headBytes = {LOGIN_BYTES}; 
      vector<string> content = {LOGIN_KEY,nickname};
      write_TCP(ClientFD,headBytes,content);

      read_TCP(ClientFD,headBytes,content);
      if(content[0] == OK_KEY){
        cout << "Logged in succesfully\n";
        logged = true;
        break;
      }
      else cout << "--->Error: "<< content[1] << "\n"; 
      //cout << "There is already a " << nickname << " logged. Try another nickname\n";
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

  int read_TCP(const int &FD, vector<int>& headBytes, vector<string>& content){
    char buffer[99999];
    int received = read(FD,buffer,1);
    if(received == -1){
      headBytes = {ERROR_BYTES};
      content = {ERROR_KEY,"client could not read the message"};
      return -1;
    }
    
    buffer[received] = '\0';
    string opt = buffer;
    
    if(opt == OK_KEY){
      headBytes = {OK_BYTES};
      content = {OK_KEY};
    }
    else if(opt == ERROR_KEY){
      headBytes = {ERROR_BYTES};
      content = {ERROR_KEY};
    }
    else if(opt == BROADCAST_RESPONSE_KEY){
      headBytes = {BROADCAST_RESPONSE_BYTES};
      content = {BROADCAST_RESPONSE_KEY,"",""};
    }
    else if(opt == UNICAST_RESPONSE_KEY){
      headBytes = {UNICAST_RESPONSE_BYTES};    
      content = {UNICAST_RESPONSE_KEY,"",""};
    }
    else if(opt == LIST_RESPONSE_KEY){
      headBytes = {LIST_RESPONSE_BYTES}; 
      content = {LIST_RESPONSE_KEY, ""};
    }
    else if(opt == FILE_RESPONSE_KEY){
      headBytes = {FILE_RESPONSE_BYTES}; // {1, 5, 5, 5}
      content = {FILE_RESPONSE_KEY, "", "", ""};
      return 1;
    }
    

    for(size_t i = 1; i < headBytes.size(); i++){
      received = read(FD,buffer,headBytes[i]);
      buffer[received] = '\0';
      int msgSize = atoi(buffer);
      received = read(FD,buffer,msgSize);
      buffer[received] = '\0';
      content[i] = buffer;
      //cout << content[i] << " " << buffer << " ";
    }
    //cout << "\n";
    return received;
  }
  
  void threadReadSocket(const int& ClientFD){
    //char local_buffer[1000];
    vector<int> headBytes;
    vector<string> content;
    while(read_TCP(ClientFD,headBytes,content) > 0){
      string opt = content[0];

      if(opt == OK_KEY){
        cout << "--> OK\n";
      }
      else if(opt == ERROR_KEY){
        cout << "Error: " << content[1];
      }
      else if(opt == BROADCAST_RESPONSE_KEY){
        cout << "[broadcast] " << content[1] << ": " << content[2] << "\n";
      }
      else if(opt == UNICAST_RESPONSE_KEY){
        cout << "[unicast] " << content[1] << ": " << content[2] << "\n";
      }
      else if(opt == LIST_RESPONSE_KEY){
        cout << "\n[list] " << content[1] << "\n"; 
      }
      else if(opt == FILE_RESPONSE_KEY) {

        char buffer[256];
        int n = read(ClientFD,buffer,5);
        buffer[n] = '\0';
        int fileSize = atoi(buffer);

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

        string newFileName = "rec_" + content[2] + "_" + content[3];
    
        ofstream file(newFileName, ios::binary );
        if (file.is_open()) {
            file.write(buffer,fileSize);
            file.close();
            cout << "\n[File] " << content[3] << ": " << newFileName << "\n";
        }
        rename(content[2].c_str(),newFileName.c_str());
      }
    }
  }

};




int main(void)
{
  ClientTCP client;
  while(client.display_interface());


  return 0;
}