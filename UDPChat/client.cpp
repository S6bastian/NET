// g++ server.cpp -o server -pthread
// g++ client.cpp -o client -pthread

/*

c++ program that uses a UDP socket.
-----------------------------------
The server should support multiple clients and use Threads.
The application is a chat.
The datagram is fixed to 500 Bytes.
The datagram has 3 encapsulations.

Encapsulation 1
---------------
size: 1 Byte,  for the action.
size: 3 Bytes,  for nick_name size.
size: variable size based on the nick_name length.
size: 3 Bytes,  for the nick_name size for the destination client.
size: variable size based on the nick_name destination length.
size: 5 Bytes,  for message.
size: variable size based on the message  length.
size: 11 Bytes,  for file name.
size: variable size based on the file name  length.
size: 20 Bytes for the actual file.
size: variable size based on actual  file  length.

Actions
-------
There are 3 actions:
M: message to a particular client.
B: message to all clients as a broadcast
F: send to be sent instead of a message.

Encapsulation 2
---------------
We have two cases:
1. If the datagram from encapsulation 1 is less than 494 Bytes, pad it with # to reach 496 Bytes.
2. If the datagram from encapsulation 2 exceeds 494 Bytes, the message should be split into 496 Bytes, and each split chunk should be encapsulated with encapsulation 3.

Encapsulation 3
--------------
All messages should be encapsulated.
This encapsulation adds 2 fields.
2 bytes to indicate if it is the first chunk in a set of n, with the value 01. If it is the last chunk in a set of n, with the value of 11. other way 00
4 bytes to indicate the sequence number of chunks in the stream of chunks. The sequence number corresponds to the chunk from the encapsulation 2.

If the chunk is the one on the stream of chunks,  the first 2 bytes will be set to 11 and the sequence number to 0000.
If the last datagram is less than 494, pad it with @
All datagrams to be sent over the socket should be 500 Bytes.

*/

#include <cstddef>
#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>
#include <map>
#include <vector>
#include <algorithm>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

constexpr int PORT = 45000;

constexpr int DATAGRAM_SIZE = 500;
constexpr int HASH_SIZE = 1;
constexpr int FLAG_SIZE = 2;
constexpr int SEQUENCE_SIZE = 4;
constexpr int HEADER_SIZE = HASH_SIZE + FLAG_SIZE + SEQUENCE_SIZE;
constexpr int PAYLOAD_SIZE = DATAGRAM_SIZE - HEADER_SIZE;

constexpr int ACTION_SIZE = 1;
constexpr int NICK_NAME_SIZE = 3;
constexpr int DESTINY_NICK_NAME_SIZE = 3;
constexpr int MESSAGE_SIZE = 5;
constexpr int FILE_NAME_SIZE = 11;
constexpr int FILE_SIZE_SIZE = 20;


struct FragmentData {
    int sequence;
    string payload;
};

struct ClientUDP{
    mutex fragmentsMutex;
    map<string, vector<FragmentData>> fragmentBuffer;
    string nick_name;
    int socket_fd;
    sockaddr_in clientAddr;
    sockaddr_in server_addr;
    bool debug = true;


    ClientUDP(){
        socket_fd = socket(AF_INET, SOCK_DGRAM, 0);

        if (socket_fd < 0) {
            cerr << "Socket creation failed.\n";
            exit(EXIT_FAILURE);
        }

        // BIND CLIENT SOCKET

        clientAddr.sin_family = AF_INET;
        clientAddr.sin_addr.s_addr = INADDR_ANY;
        clientAddr.sin_port = htons(0);

        if (bind(
                socket_fd,
                (sockaddr *)&clientAddr,
                sizeof(clientAddr)
            ) < 0) {

            cerr << "Bind failed.\n";
            exit(EXIT_FAILURE);
        }



        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(PORT);

        inet_pton(
            AF_INET,
            "127.0.0.1",
            &server_addr.sin_addr
        );

        // START RECEIVER THREAD
        thread receiverThread(
            &ClientUDP::receive_messages, this,
            socket_fd
        );

        receiverThread.detach();
    }

    void run(){
        cout << "Enter nick_name: ";
        getline(cin, nick_name);

        while (true) {

            char action;

            cout << "\n=========================\n";
            cout << "[M] Private Message\n";
            cout << "[B] Broadcast\n";
            cout << "[F] Send File\n";
            cout << "[L] List\n";
            cout << "[O] Logout\n";
            cout << "Choice: ";

            cin >> action;

            cin.ignore();

            string destination;
            string message;
            string file_name;
            string file_data;

            // DESTINATION
            if (action == 'M' ||
                action == 'F') {

                cout << "Destination: ";

                getline(cin, destination);
            }

            // MESSAGE
            if (action == 'M' || action == 'B') {

                cout << "Message: ";

                getline(cin, message);
            }

            // FILE
            if (action == 'F') {

                cout << "file_name: ";

                getline(cin, file_name);

                file_data = read_file(file_name);

                if (file_data.empty()) {

                    cout << "Cannot read file.\n";

                    continue;
                }
            }

            if (action == 'O') {
                string protocol = build_protocol('O', nick_name, "", "", "", "");
                send_message(socket_fd, server_addr, protocol);
                cout << "Logged out.\n";
                close(socket_fd);
                return;
            }

            if (action == 'L') {
                string protocol = build_protocol('L', nick_name, "", "", "", "");
                send_message(socket_fd, server_addr, protocol);
                cout << "List requested.\n";
                continue;
            }

            // BUILD PROTOCOL
            string protocol =
                build_protocol(
                    action,
                    nick_name,
                    destination,
                    message,
                    file_name,
                    file_data
                );


            // SEND
            send_message(
                socket_fd,
                server_addr,
                protocol
            );

            cout << "Data sent successfully.\n";
        }

    }

    int checksum(const string& data){
        int sum = 0;

        for(size_t i = 0; i < data.size(); i++)
            sum += (unsigned char)data[i];

        sum %= 7;
        return sum;
    }


    string pad_number(int number, int width) {

        string s = to_string(number);

        while (s.size() < width)
            s = "0" + s;

        return s;
    }

    string trim_right(const string &s, char c) {

        size_t end = s.find_last_not_of(c);

        if (end == string::npos)
            return "";

        return s.substr(0, end + 1);
    }

    string read_file(const string &file_name) {

        ifstream file(file_name, ios::binary);

        if (!file)
            return "";

        return string(
            (istreambuf_iterator<char>(file)),
            istreambuf_iterator<char>()
        );
    }

    void send_datagram(
        int socket_fd,
        sockaddr_in server_addr,
        const char& hash,
        const string& flag,
        int sequence,
        const string &payload
    ) {

        char datagram[DATAGRAM_SIZE+1];
        datagram[DATAGRAM_SIZE] = '\0';

        memset(datagram, 0, DATAGRAM_SIZE);

        int idx = 0;

        // 1 BYTE HASH
        datagram[idx] = hash;
        idx += HASH_SIZE;

        // 2 BYTES FLAG
        memcpy(datagram + idx, flag.c_str(), FLAG_SIZE);
        idx += FLAG_SIZE;

        // 4 BYTES SEQUENCE NUMBER
        string seq = pad_number(sequence, SEQUENCE_SIZE);

        memcpy(datagram + idx, seq.c_str(), SEQUENCE_SIZE);
        idx += SEQUENCE_SIZE;

        // 493 BYTES PAYLOAD
        memcpy(datagram + idx, payload.c_str(), payload.size());

        sendto(
            socket_fd,
            datagram,
            DATAGRAM_SIZE,
            0,
            (sockaddr *)&server_addr,
            sizeof(server_addr)
        );

        if(debug) cout << "\n" << "WRITE:>>>" << string(datagram, DATAGRAM_SIZE) << "<<<" << "\n";
    }

    vector<string> split_payload(
        const string &data
    ) {

        vector<string> chunks;

        int offset = 0;

        while (offset < data.size()) {

            int size = min(PAYLOAD_SIZE, (int)data.size() - offset);

            chunks.push_back(data.substr(offset, size));

            offset += size;
        }

        return chunks;
    }


    void send_message(
        int socket_fd,
        sockaddr_in server_addr,
        const string &data
    ) {

        // APPLY CHECKSUM


        // SINGLE DATAGRAM CASE
        if (data.size() <= PAYLOAD_SIZE) {

            string payload = data;

            payload.append(PAYLOAD_SIZE - payload.size(), '#');

            char hash = checksum(payload) + '0';

            send_datagram(
                socket_fd,
                server_addr,
                hash,
                "11",
                0,
                payload
            );

            return;
        }

        // MULTIPLE DATAGRAMS
        vector<string> chunks = split_payload(data);

        for (int i = 0; i < chunks.size(); i++) {

            string flag = "00";

            if (i == 0)
                flag = "01";

            else if (i == chunks.size() - 1)
                flag = "11";

            string payload = chunks[i];

            // LAST FRAGMENT PADDING
            if (i == chunks.size() - 1 && payload.size() < PAYLOAD_SIZE) {
                payload.append(PAYLOAD_SIZE - payload.size(), '@');
            }

            char hash = checksum(payload) + '0';

            send_datagram(
                socket_fd,
                server_addr,
                hash,
                flag,
                i,
                payload
            );
        }
    }


    string build_protocol(
        char action,
        const string &nick_name,
        const string &destination,
        const string &message,
        const string &file_name,
        const string &file_data
    ) {

        string protocol;

        // ACTION
        protocol += action;

        // nick_name
        protocol += pad_number(nick_name.size(), NICK_NAME_SIZE);

        protocol += nick_name;

        // DESTINATION
        protocol += pad_number(destination.size(), DESTINY_NICK_NAME_SIZE);

        protocol += destination;

        // MESSAGE
        protocol += pad_number(message.size(), MESSAGE_SIZE);

        protocol += message;

        // file_name
        protocol += pad_number(file_name.size(), FILE_NAME_SIZE);

        protocol += file_name;

        // FILE DATA
        protocol += pad_number(file_data.size(), FILE_SIZE_SIZE);

        protocol += file_data;

        return protocol;
    }


    // RECEIVER SECTION


    void parse_protocol(const string &data) {

        int idx = 0;

        // ACTION
        char action = data[idx];
        idx += ACTION_SIZE;

        // NICK_NAME
        int nick_size = stoi(data.substr(idx, NICK_NAME_SIZE));

        idx += NICK_NAME_SIZE;

        string nick_name = data.substr(idx, nick_size);

        idx += nick_size;

        // DESTINATION
        int destination_size = stoi(data.substr(idx, DESTINY_NICK_NAME_SIZE));

        idx += DESTINY_NICK_NAME_SIZE;

        string destination = data.substr(idx, destination_size);

        idx += destination_size;

        // MESSAGE
        int messsage_size = stoi(data.substr(idx, MESSAGE_SIZE));

        idx += MESSAGE_SIZE;

        string message = data.substr(idx, messsage_size);

        idx += messsage_size;

        // file_name
        int file_name_size = stoi(data.substr(idx, FILE_NAME_SIZE));

        idx += FILE_NAME_SIZE;

        string file_name = data.substr(idx, file_name_size);

        idx += file_name_size;

        // FILE SIZE
        int file_size = stoi(data.substr(idx, FILE_SIZE_SIZE));

        idx += FILE_SIZE_SIZE;

        string file_data = data.substr(idx, file_size);

        cout << "\n=================================\n";

        cout << "FROM: "
             << nick_name
             << "\n";

        cout << "ACTION: "
             << action
             << "\n";

        if (action == 'M' || action == 'B') {

            cout << "MESSAGE: "
                 << message
                 << "\n";
        }

        if (action == 'F') {

            cout << "FILE RECEIVED: "
                 << file_name
                 << "\n";

            cout << "FILE SIZE: "
                 << file_data.size()
                 << " bytes"
                 << "\n";

            // SAVE FILE

            ofstream outFile(
                "received_" + file_name,
                ios::binary
            );

            outFile.write(
                file_data.c_str(),
                file_data.size()
            );

            outFile.close();

            cout << "FILE SAVED AS: "
                 << "received_" + file_name
                 << "\n";
        }

        if (action == 'L') {
            cout << "CONNECTED CLIENTS (JSON):\n" << message << "\n";
        }

        cout << "=================================\n";
    }



    void receive_messages(int socket_fd) {

        while (true) {

            char buffer[DATAGRAM_SIZE+1];

            sockaddr_in senderAddr{};
            socklen_t len = sizeof(senderAddr);

            int received =
                recvfrom(
                    socket_fd,
                    buffer,
                    DATAGRAM_SIZE,
                    0,
                    (sockaddr *)&senderAddr,
                    &len
                );

            buffer[500] = '\0';

            if(debug) cout << "\n" << "READ:>>>" << buffer << "<<<" << "\n";
            if (received <= 0)
                continue;

            // HEADER
            int idx = 0;
            idx += HASH_SIZE;

            string flag(buffer + idx, FLAG_SIZE);
            idx += FLAG_SIZE;

            string seqStr(buffer + idx, SEQUENCE_SIZE);
            idx += SEQUENCE_SIZE;

            int sequence = stoi(seqStr);

            // PAYLOAD
            string payload(buffer + idx, PAYLOAD_SIZE);

            string senderKey =
                string(inet_ntoa(senderAddr.sin_addr))
                + ":"
                + to_string(ntohs(senderAddr.sin_port));

            // SINGLE DATAGRAM
            if (flag == "11" &&
                sequence == 0) {

                payload = trim_right(payload, '#');

                parse_protocol(payload);

                continue;
            }

            lock_guard<mutex> lock(fragmentsMutex);

            // FIRST FRAGMENT
            if (flag == "01") {

                fragmentBuffer[senderKey].clear();

                fragmentBuffer[senderKey].push_back({
                    sequence,
                    payload
                });
            }

            // MIDDLE FRAGMENT
            else if (flag == "00") {

                fragmentBuffer[senderKey].push_back({
                    sequence,
                    payload
                });
            }

            // LAST FRAGMENT
            else if (flag == "11") {

                payload =
                    trim_right(payload, '@');

                fragmentBuffer[senderKey].push_back({
                    sequence,
                    payload
                });

                auto &fragments =
                    fragmentBuffer[senderKey];

                sort(
                    fragments.begin(),
                    fragments.end(),
                    [](const FragmentData &a,
                       const FragmentData &b) {

                        return a.sequence <
                               b.sequence;
                    }
                );

                string fullData;

                for (auto &f : fragments) {

                    fullData += f.payload;
                }

                parse_protocol(fullData);

                fragmentBuffer[senderKey].clear();
            }
        }
    }
};

int main(){
    ClientUDP client;
    client.run();

    return 0;
}
