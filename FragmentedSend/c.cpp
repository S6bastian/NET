#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>

#include <string>

#define PORT 8080

using namespace std;

void fill(int size, string& msg){
    if(msg.size() > size) return;
    string tmp(size-msg.size(),'0');
    msg = tmp + msg;
}

void send_fragmented(int sock, const std::string& msg) {
    for (size_t i = 0; i < msg.size(); i += 3) {
        std::string chunk = msg.substr(i, 3);

        send(sock, chunk.c_str(), chunk.size(), 0);

        std::cout << "[Enviado fragmento]: " << chunk << std::endl;

        usleep(100000); // 100 ms (latencia artificial)
    }
}

void send_JSON(int sock, const string& msg){
    //size_t size = msg.size();
    string protocol_msg = to_string(msg.size());
    fill(5,protocol_msg);
    protocol_msg += "J" + msg;
    send_fragmented(sock,protocol_msg);
}

int main() {
    int sock = 0;
    sockaddr_in serv_addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    inet_pton(AF_INET, "10.0.2.15", &serv_addr.sin_addr);

    connect(sock, (sockaddr*)&serv_addr, sizeof(serv_addr));

    std::cout << "Conectado al servidor\n";

    // Enviar mensajes fragmentados
    //send_fragmented(sock, "Hola servidor\n");
    //send_fragmented(sock, "Este es otro mensaje largo\n");
    string jsonString = "{Este es otro mensaje largo 123456789}";
    send_JSON(sock,jsonString);

    sleep(1);

    // Desconexión inesperada
    std::cout << "Cerrando cliente abruptamente...\n";
    close(sock);

    return 0;
}