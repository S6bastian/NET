 /* Client code in C */
 
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
    n = write(ClientFD,"Hi, this is Sebastian.",22);
    /* perform read write operations ... */

    bzero(buffer,256);
    n = read(ClientFD,buffer,256);

    if (n < 0) perror("ERROR reading from socket");

    printf("Server: [%s]\n",buffer);


    for(;;){
      char newMessage[256];

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
    }



    shutdown(ClientFD, SHUT_RDWR);
 
    close(ClientFD);
    return 0;
  }
