  /* Server code in C */
 
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <unistd.h>
 
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
    
    for(;;)
    {

      if(0 > ClientFD)
      {
        perror("error accept failed");
        close(ServerFD);
        exit(EXIT_FAILURE);
      }


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
      

      if (n < 0) perror("ERROR writing to socket");

      //shutdown(ClientFD, SHUT_RDWR);

      //close(ClientFD); // 
    }
 
    close(ServerFD);
    return 0;
  }


// para matar el puerto     sudo fuser -k 8888/tcp