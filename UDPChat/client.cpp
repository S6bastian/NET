/* Client code in C++ */

#include <arpa/inet.h>
#include <cstddef>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdlib>
#include <ctime>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#define LOGIN_BYTES 1, 4                 // key,nick
#define LOGOUT_BYTES 1                   // key
#define BROADCAST_BYTES 1, 7             // key,msg
#define BROADCAST_RESPONSE_BYTES 1, 3, 7 // key,nick,msg
#define UNICAST_BYTES 1, 5, 7            // key,msg,nick
#define UNICAST_RESPONSE_BYTES 1, 7, 5   // key,nick,msg
#define LIST_BYTES 1                     // key
#define LIST_RESPONSE_BYTES 1, 5         // key,size
#define FILE_BYTES 1, 5, 5, 5            // key,file,filename,destnick
#define FILE_RESPONSE_BYTES 1, 5, 5, 5   // key,file,filename,sourcenick
#define OK_BYTES 1                       // key
#define ERROR_BYTES 1, 5                 // key,msg

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

// cd -l 45000   to try

class ClientUDP {
public:
    ClientUDP() {
        logged = false;

        ClientFD = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

        if (-1 == ClientFD) {
            perror("cannot create socket");
            exit(EXIT_FAILURE);
        }

        struct timeval timeout;
        timeout.tv_sec = 2;  // 2 seconds
        timeout.tv_usec = 0; // 0 microseconds

        if (setsockopt(ClientFD, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
            perror("Error configuring socket's timeout");
        }

        memset(&stSockAddr, 0, sizeof(struct sockaddr_in));

        stSockAddr.sin_family = AF_INET;
        stSockAddr.sin_port = htons(45000);
        Res = inet_pton(AF_INET, "10.0.2.15", &stSockAddr.sin_addr);

        if (0 > Res) {
            perror("error: first parameter is not a valid address family");
            close(ClientFD);
            exit(EXIT_FAILURE);
        } else if (0 == Res) {
            perror("char string (second parameter does not contain valid ipaddress");
            close(ClientFD);
            exit(EXIT_FAILURE);
        }

        if (-1 == connect(ClientFD, (const struct sockaddr *)&stSockAddr,
                        sizeof(struct sockaddr_in))) {
            perror("connect failed");
            close(ClientFD);
            exit(EXIT_FAILURE);
        }

        send_login();

        thread t(&ClientUDP::threadReadSocket, this, ClientFD);
        t.detach();

        cout << "*******************************************************\n"
            << "**************ClientUDP Display Interface**************\n"
            << "*******************************************************\n"
            << "Choose an option:\n"
            << "1. Logout\n"
            << "2. Broadcast\n"
            << "3. Unicast\n"
            << "4. List\n"
            << "5. File\n"
            << "\n\n";
    }

    ~ClientUDP() {
        //shutdown(ClientFD, SHUT_RDWR);
        close(ClientFD);
        cout << "*********************Disconnected**********************\n";
    }

    int display_interface() {
        int opt = -1;
        while (logged) {
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
        switch (opt) {
        case 1:                     // Logout
            send_logout();
            break;

        case 2:                     // Broadcast
            send_broadcast();
            break;

        case 3:                     // Unicast
            send_unicast();
            break;

        case 4:                     // List
            send_list();
            break;

        case 5:                     // File
            send_file();
            break;

        return 1;
    }

private:
    const int DATAGRAM_SIZE = 500;

    bool logged;
    set<string> receivedFiles;

    struct sockaddr_in stSockAddr;
    int Res;
    int ClientFD;
    char buffer[256];
    int n;
    string nickname;

    void send_login() {
        cout << "*******************************************************\n"
            << "********************ClientUDP Login********************\n"
            << "*******************************************************\n";


        int retry = 1;
        cout << "Enter nickname: ";
        cin >> nickname;

        while (true) {

            if(retry > 1) cout << retry << " retry...\n";
            if(++retry >= 5){
                cout << "Closing program due to no response\n";
                exit(EXIT_FAILURE);
            }

            string msg = LOGIN_KEY + fill(4,nickname.size()) + nickname;
            padding(DATAGRAM_SIZE-msg.size(),msg);

            int bytes_sent = send(ClientFD, msg.data(), msg.size(), 0);
            if (bytes_sent == -1) {
                perror("Error sending login");
                continue;
            }

            char recv_buffer[DATAGRAM_SIZE];
            memset(recv_buffer, 0, sizeof(recv_buffer));

            int bytes_received = recv(ClientFD, recv_buffer, sizeof(recv_buffer) - 1, 0);

            if (bytes_received == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    cout << "---> [TIMEOUT]: Server is not responding. Retrying login...\n\n";
                } else {
                    perror("Critical error in recv");
                }
                continue;
            }


            recv_buffer[bytes_received] = '\0';
            string response(recv_buffer);

            if (!response.empty()) {
                string key = response.substr(0, 1);

                if (key == OK_KEY) {
                    cout << "Logged in successfully!\n\n";
                    logged = true;
                    break;
                }
                else if (key == ERROR_KEY) {
                    string error_msg = (response.size() > 1) ? response.substr(1) : "Unknown error";
                    cout << "---> Server Error: " << error_msg << "\nPlease try another nickname.\n\n";
                    cout << "Enter nickname: ";
                    cin >> nickname;
                    retry = 1;
                }
                else {
                    cout << "---> Unexpected response format: " << response << "\n\n";
                }
            }
        }
    }

    void send_logout() {
        cout << "*******************************************************\n"
             << "********************ClientUDP Logout*******************\n"
             << "*******************************************************\n";

        if (!logged) {
            cout << "You are not logged in.\n\n";
            return;
        }

        int retry = 1; // Empezamos en el intento 1

        while (true) {
            if (retry > 5) {
                cout << "No response from server. Forcing local logout...\n";
                logged = false;
                break;
            }

            if (retry > 1) {
                cout << (retry - 1) << " retry logout...\n";
            }

            string msg = LOGOUT_KEY + fill(4, nickname.size()) + nickname;
            padding(DATAGRAM_SIZE-msg.size(),msg);

            int bytes_sent = send(ClientFD, msg.data(), msg.size(), 0);
            if (bytes_sent == -1) {
                perror("Error sending logout");
                retry++;
                continue;
            }

            char recv_buffer[DATAGRAM_SIZE];
            memset(recv_buffer, 0, sizeof(recv_buffer));

            int bytes_received = recv(ClientFD, recv_buffer, sizeof(recv_buffer) - 1, 0);

            if (bytes_received == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    cout << "---> [TIMEOUT]: Server did not acknowledge logout. Retrying...\n\n";
                } else {
                    perror("Critical error in recv during logout");
                }
                retry++;
                continue;
            }

            recv_buffer[bytes_received] = '\0';
            string response(recv_buffer);

            if (!response.empty()) {
                string key = response.substr(0, 1);

                if (key == OK_KEY) {
                    cout << "Logged out successfully from server.\n\n";
                    logged = false;
                    break;
                }
                else if (key == ERROR_KEY) {
                    string error_msg = (response.size() > 1) ? response.substr(1) : "Unknown error";
                    cout << "---> Server Logout Error: " << error_msg << "\n";
                    logged = false;
                    break;
                }
                else {
                    cout << "---> Unexpected response format: " << response << "\n\n";
                }
            }
        }
    }

    void send_broadcast() {
        //cout << "*******************************************************\n"
        //     << "*******************ClientUDP Broadcast*****************\n"
        //     << "*******************************************************\n";

        if (!logged) {
            cout << "You must be logged in to broadcast messages.\n\n";
            return;
        }

        string message;
        cout << "message: ";
        cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        getline(cin, message);

        int retry = 1;

        while (true) {

            if (retry > 5) {
                cout << "---> [ERROR]: No response from server. Broadcast failed.\n\n";
                break;
            }

            if (retry > 1) {
                cout << (retry - 1) << " retry broadcast...\n";
            }

            string msg = BROADCAST_KEY + fill(7, message.size()) + message;
            padding(DATAGRAM_SIZE-msg.size(),msg);

            int bytes_sent = send(ClientFD, msg.data(), msg.size(), 0);
            if (bytes_sent == -1) {
                perror("Error sending broadcast");
                retry++;
                continue;
            }

            char recv_buffer[DATAGRAM_SIZE];
            memset(recv_buffer, 0, sizeof(recv_buffer));

            int bytes_received = recv(ClientFD, recv_buffer, sizeof(recv_buffer) - 1, 0);

            if (bytes_received == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    cout << "---> [TIMEOUT]: Server did not acknowledge broadcast. Retrying...\n\n";
                } else {
                    perror("Critical error in recv during broadcast");
                }
                retry++;
                continue;
            }

            recv_buffer[bytes_received] = '\0';
            string response(recv_buffer);

            if (!response.empty()) {
                string key = response.substr(0, 1);

                if (key == OK_KEY) {
                    cout << "Broadcast message sent successfully!\n\n";

                    break;
                }
                else if (key == ERROR_KEY) {
                    string error_msg = (response.size() > 1) ? response.substr(1) : "Unknown error";
                    cout << "---> Server Broadcast Error: " << error_msg << "\n\n";
                    break;
                }
                else {
                    cout << "---> Unexpected response format: " << response << "\n\n";
                }
            }
        }
    }

    void send_unicast() {
        //cout << "*******************************************************\n"
        //     << "********************ClientUDP Unicast******************\n"
        //     << "*******************************************************\n";

        if (!logged) {
            cout << "You must be logged in to send private messages.\n\n";
            return;
        }

        string dest_nickname, message;
        cout << "Enter destination nickname: ";
        cin >> dest_nickname;

        cout << "message: ";
        cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        getline(cin, message);

        int retry = 1; // Empezamos en el intento 1

        while (true) {

            if (retry > 5) {
                cout << "---> [ERROR]: No response from server. Unicast message failed.\n\n";
                break;
            }

            if (retry > 1) {
                cout << (retry - 1) << " retry unicast...\n";
            }

            string msg = UNICAST_KEY +
                         fill(5, dest_nickname.size()) + dest_nickname +
                         fill(7, message.size()) + message;
            padding(DATAGRAM_SIZE-msg.size(),msg);

            int bytes_sent = send(ClientFD, msg.data(), msg.size(), 0);
            if (bytes_sent == -1) {
                perror("Error sending unicast");
                retry++;
                continue;
            }

            char recv_buffer[DATAGRAM_SIZE];
            memset(recv_buffer, 0, sizeof(recv_buffer));

            int bytes_received = recv(ClientFD, recv_buffer, sizeof(recv_buffer) - 1, 0);

            if (bytes_received == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    cout << "---> [TIMEOUT]: Server did not acknowledge unicast. Retrying...\n\n";
                } else {
                    perror("Critical error in recv during unicast");
                }
                retry++;
                continue;
            }

            recv_buffer[bytes_received] = '\0';
            string response(recv_buffer);

            if (!response.empty()) {
                string key = response.substr(0, 1);

                if (key == OK_KEY) {
                    cout << "Message sent successfully to " << dest_nickname << "!\n\n";
                    break;
                }
                else if (key == ERROR_KEY) {
                    string error_msg = (response.size() > 1) ? response.substr(1) : "Unknown error";
                    cout << "---> Server Unicast Error: " << error_msg << "\n\n";
                    break;
                }
                else {
                    cout << "---> Unexpected response format: " << response << "\n\n";
                }
            }
        }
    }

    void send_list() {
        //cout << "*******************************************************\n"
        //     << "****************ClientUDP Participants List************\n"
        //     << "*******************************************************\n";

        if (!logged) {
            cout << "You must be logged in to request the participants list.\n\n";
            return;
        }

        int retry = 1;

        while (true) {
            if (retry > 5) {
                cout << "---> [ERROR]: No response from server. Could not retrieve list.\n\n";
                break;
            }

            if (retry > 1) {
                cout << (retry - 1) << " retry list request...\n";
            }

            string msg = LIST_KEY;
            padding(DATAGRAM_SIZE-msg.size(),msg);

            int bytes_sent = send(ClientFD, msg.data(), msg.size(), 0);
            if (bytes_sent == -1) {
                perror("Error sending list request");
                retry++;
                continue;
            }

            char recv_buffer[DATAGRAM_SIZE];
            memset(recv_buffer, 0, sizeof(recv_buffer));

            int bytes_received = recv(ClientFD, recv_buffer, sizeof(recv_buffer) - 1, 0);

            if (bytes_received == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    cout << "---> [TIMEOUT]: Server did not respond to list request. Retrying...\n\n";
                } else {
                    perror("Critical error in recv during list request");
                }
                retry++;
                continue;
            }

            recv_buffer[bytes_received] = '\0';
            string response(recv_buffer);

            if (!response.empty()) {
                string key = response.substr(0, 1);

                if (key == OK_KEY) {
                    cout << "Connected participants:\n";

                    if (response.size() > 1) {
                        cout << response.substr(1) << "\n\n";
                    } else {
                        cout << "(No other participants connected)\n\n";
                    }
                    break;
                }
                else if (key == ERROR_KEY) {
                    string error_msg = (response.size() > 1) ? response.substr(1) : "Unknown error";
                    cout << "---> Server List Error: " << error_msg << "\n\n";
                    break;
                }
                else {
                    cout << "---> Unexpected response format: " << response << "\n\n";
                }
            }
        }
    }

    void send_file(string dest_nickname, string filename) {

        // File load

        ifstream file(filename, std::ios::binary | std::ios::ate);

        if (!file.is_open()) {
            cerr << "The file could not be opened.\n";
            return;
        }

        streamsize file_size = file.tellg();

        file.seekg(0, std::ios::beg);

        vector<char> buffer(file_size);

        if (file.read(buffer.data(), file_size)) {
            cout << "File read successfully!\n";
            cout << "File size: " << file_size << " bytes.\n";
        } else {
        std::cerr << "Error reading file data.\n";
        }

        file.close();


        // Protocol
        const int TOTAL_SEG_SIZE = 2;
        const int CURRENT_SEG_SIZE = 2;
        const int SEQUENCE_SEG_SIZE = 4;

        //string segment_header = fill(TOTAL_SEG_SIZE,)

        std::srand(std::time(nullptr));
        int sequence_seg = std::rand() % 10000;


        string open_header = "F" +
            fill(5,dest_nickname.size()) + dest_nickname +
            fill(3,filename.size()) + filename +
            fill(5,nickname.size()) + nickname;


        int payload = 500 - 8 - 12 - 22 - 1;

        int total_seg = (file_size + int(open_header.size())) / payload + ((file_size + int(open_header.size())) % payload ? 1 : 0);

        string total_seg_s = fill(TOTAL_SEG_SIZE,total_seg);
        string sequence_seg_s = fill(SEQUENCE_SEG_SIZE,sequence_seg);       // id

        int start_byte = 0;
        for(int i = 0; i < total_seg; i++){
            int copy_size = payload;
            string padding = "";

            if(i == 0){
              copy_size = payload - open_header.size();
            }
            if(i == total_seg-1){ // start_byte + payload > file_size
                copy_size = file_size - start_byte;
                padding.assign(payload-copy_size-(i == 0 ? open_header.size() : 0),'#');
            }

            string file_batch(buffer.data() + start_byte, copy_size);

            string msg = total_seg_s +
                fill(CURRENT_SEG_SIZE,i+1) +
                sequence_seg_s +
                (i == 0 ? open_header : "")  +
                fill(12,start_byte) + //sequence number
                fill(22,copy_size) + //batch size
                file_batch +
                padding;

            msg += to_string(checkSum(msg));
            if(i == 0) start_byte += open_header.size();
            else start_byte += payload;
        }

    }

  int write_TCP(const int &FD, const vector<int> &headBytes,
                const vector<string> &content) {
    ostringstream oss;

    oss << setw(headBytes[0]) << content[0];

    for (int i = 1; i < (int)headBytes.size(); i++) {
      oss << setfill('0') << setw(headBytes[i]) << content[i].length()
          << content[i];
    }

    string packet = oss.str();

    int total_sent = write(FD, packet.data(), (int)packet.size());

    if (total_sent == -1) {
      return -1;
    }

    return total_sent;
  }

  int read_TCP(const int &FD, vector<int> &headBytes, vector<string> &content) {
    char buffer[99999];
    int received = read(FD, buffer, 1);
    if (received == -1) {
      headBytes = {ERROR_BYTES};
      content = {ERROR_KEY, "client could not read the message"};
      return -1;
    }

    buffer[received] = '\0';
    string opt = buffer;

    if (opt == OK_KEY) {
      headBytes = {OK_BYTES};
      content = {OK_KEY};
    } else if (opt == ERROR_KEY) {
      headBytes = {ERROR_BYTES};
      content = {ERROR_KEY};
    } else if (opt == BROADCAST_RESPONSE_KEY) {
      headBytes = {BROADCAST_RESPONSE_BYTES};
      content = {BROADCAST_RESPONSE_KEY, "", ""};
    } else if (opt == UNICAST_RESPONSE_KEY) {
      headBytes = {UNICAST_RESPONSE_BYTES};
      content = {UNICAST_RESPONSE_KEY, "", ""};
    } else if (opt == LIST_RESPONSE_KEY) {
      headBytes = {LIST_RESPONSE_BYTES};
      content = {LIST_RESPONSE_KEY, ""};
    } else if (opt == FILE_RESPONSE_KEY) {
      headBytes = {FILE_RESPONSE_BYTES}; // {1, 5, 5, 5}
      content = {FILE_RESPONSE_KEY, "", "", ""};
      return 1;
    }

    for (size_t i = 1; i < headBytes.size(); i++) {
      received = read(FD, buffer, headBytes[i]);
      buffer[received] = '\0';
      int msgSize = atoi(buffer);
      received = read(FD, buffer, msgSize);
      buffer[received] = '\0';
      content[i] = buffer;
      // cout << content[i] << " " << buffer << " ";
    }
    // cout << "\n";
    return received;
  }

  void threadReadSocket(const int &ClientFD) {
    // char local_buffer[1000];
    vector<int> headBytes;
    vector<string> content;
    while (read_TCP(ClientFD, headBytes, content) > 0) {
      string opt = content[0];

      if (opt == OK_KEY) {
        cout << "--> OK\n";
      } else if (opt == ERROR_KEY) {
        cout << "Error: " << content[1];
      } else if (opt == BROADCAST_RESPONSE_KEY) {
        cout << "[broadcast] " << content[1] << ": " << content[2] << "\n";
      } else if (opt == UNICAST_RESPONSE_KEY) {
        cout << "[unicast] " << content[1] << ": " << content[2] << "\n";
      } else if (opt == LIST_RESPONSE_KEY) {
        cout << "\n[list] " << content[1] << "\n";
      } else if (opt == FILE_RESPONSE_KEY) {

        char szBuf[6];
        read(ClientFD, szBuf, 5);
        szBuf[5] = '\0';
        int fileSize = atoi(szBuf);

        vector<char> fileContent(fileSize);
        int totalRead = 0;
        while (totalRead < fileSize) {
          int r = read(ClientFD, fileContent.data() + totalRead,
                       fileSize - totalRead);
          totalRead += r;
        }

        read(ClientFD, szBuf, 5);
        int nameLen = atoi(szBuf);
        vector<char> nameBuf(nameLen + 1, 0);
        read(ClientFD, nameBuf.data(), nameLen);
        string fileName(nameBuf.data());

        read(ClientFD, szBuf, 5);
        int senderLen = atoi(szBuf);
        vector<char> senderBuf(senderLen + 1, 0);
        read(ClientFD, senderBuf.data(), senderLen);
        string senderNick(senderBuf.data());

        string savePath = "rec_" + senderNick + "_" + fileName;
        ofstream outFile(savePath, ios::binary);
        outFile.write(fileContent.data(), fileSize);
        outFile.close();

        cout << "\n[File] Received from " << senderNick << ": " << savePath
             << "\n";
      }
    }
    }


    string fill(int size, int value) {
        string tmp = to_string(value);
        if (tmp.size() > size)
            return tmp;
        string filled(size - tmp.size(), '0');
        filled += tmp;
        return filled;
    }

    int checkSum(const string& s) {
        int value = 0;
        for(size_t i = 0; i < s.size(); i++){
            value += s[i];
            value %= 10;
        }
        return value;
    }

    string padding(const int& size, const string& payload){
        if(int(payload.size()) >= size) return payload;

        string pad(size - payload.size(), '#');
        string new_payload = payload + pad;
        return new_payload;
    }
};

int main(void) {
  ClientUDP client;
  while (client.display_interface());

  return 0;
}
