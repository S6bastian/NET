// g++ server.cpp -o server -pthread
// g++ client.cpp -o client


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


#include <iostream>
#include <thread>
#include <mutex>
#include <map>
#include <vector>
#include <cstring>
#include <algorithm>
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



struct ClientInfo {
    sockaddr_in address;
};

struct FragmentData {
    int sequence;
    string payload;
};

struct ServerUDP{
    mutex clients_mutex;
    mutex fragments_mutex;
    map<string, ClientInfo> clients;
    map<string, vector<FragmentData>> fragment_buffer;
    bool debug = true;

    int socket_fd;
    sockaddr_in serverAddr;

    ServerUDP(){
        socket_fd = socket(AF_INET, SOCK_DGRAM, 0);

        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(PORT);
        serverAddr.sin_addr.s_addr = INADDR_ANY;

        if (bind(
                socket_fd,
                (sockaddr *)&serverAddr,
                sizeof(serverAddr)
        ) < 0){
            cerr << "Bind failed.\n";
            exit(EXIT_FAILURE);
        }
    }

    ~ServerUDP(){
        close(socket_fd);
    }

    void run(){
        cout << "UDP CHAT SERVER RUNNING\n";

        while (true) {

            char buffer[DATAGRAM_SIZE+1];
            buffer[DATAGRAM_SIZE] = '\0';

            sockaddr_in client_addr;
            socklen_t len = sizeof(client_addr);

            int received =
                recvfrom(
                    socket_fd,
                    buffer,
                    DATAGRAM_SIZE,
                    0,
                    (sockaddr *)&client_addr,
                    &len
                );
            if(debug) cout << "\n" << "READ:>>>" << buffer << "<<<" << "\n";
            if (received <= 0)
                continue;

            int idx = 0;

            char hash = buffer[idx];
            idx += HASH_SIZE;

            string flag(buffer + idx, FLAG_SIZE);
            idx += FLAG_SIZE;

            string sequence_str(buffer + idx, SEQUENCE_SIZE);
            idx += SEQUENCE_SIZE;

            int sequence = stoi(sequence_str);

            string payload(buffer + idx, PAYLOAD_SIZE);

            string client_key = build_client_key(client_addr);

            // SINGLE FRAGMENT MESSAGE
            if (flag == "11" && sequence == 0) {

                payload = trim_right(payload, '#');

                thread t(
                    &ServerUDP::receive_thread, this,
                    socket_fd,
                    payload,
                    client_addr
                );

                t.detach();

                continue;
            }

            lock_guard<mutex> lock(fragments_mutex);

            // FIRST FRAGMENT
            if (flag == "01") {

                fragment_buffer[client_key].clear();

                fragment_buffer[client_key].push_back({sequence, payload});
            }

            // MIDDLE FRAGMENT
            else if (flag == "00") {

                fragment_buffer[client_key].push_back({sequence, payload});
            }

            // LAST FRAGMENT
            else if (flag == "11") {

                payload = trim_right(payload, '@');

                fragment_buffer[client_key].push_back({sequence, payload});

                auto &fragments = fragment_buffer[client_key];

                sort(
                    fragments.begin(),
                    fragments.end(),
                    [](const FragmentData &a,
                       const FragmentData &b) {

                        return a.sequence < b.sequence;
                    }
                );

                string full_data;

                for (auto &f : fragments)
                    full_data += f.payload;

                thread t(
                    &ServerUDP::receive_thread,this,
                    socket_fd,
                    full_data,
                    client_addr
                );

                t.detach();

                fragment_buffer[client_key].clear();
            }
        }
    }

    int checksum(const string& data){
        int sum = 0;

        for(size_t i = 0; i < data.size(); i++)
            sum += (unsigned char)data[i];

        sum %= 7;
        return sum;
    }

    void print_connected_clients() {

        lock_guard<mutex> lock(clients_mutex);

        cout << "\n========== CONNECTED CLIENTS ==========\n";

        if (clients.empty()) {

            cout << "No connected clients.\n";
        }
        else {

            int count = 1;

            for (const auto &client : clients) {

                string nick_name = client.first;

                sockaddr_in addr = client.second.address;

                cout << count++ << ". ";

                cout << "Nickname: "
                     << nick_name
                     << " | IP: "
                     << inet_ntoa(addr.sin_addr)
                     << " | Port: "
                     << ntohs(addr.sin_port)
                     << "\n";
            }
        }

        cout << "=======================================\n";
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

    string build_client_key(sockaddr_in addr) {

        return string(inet_ntoa(addr.sin_addr))
               + ":"
               + to_string(ntohs(addr.sin_port));
    }

    void send_datagram(
        int socket_fd,
        sockaddr_in addr,
        const char& hash,
        const string &flag,
        int sequence,
        const string &payload
    ) {

        char datagram[DATAGRAM_SIZE+1];
        datagram[DATAGRAM_SIZE] = '\0';

        memset(datagram, 0, DATAGRAM_SIZE);

        int idx = 0;

        datagram[idx] = hash;
        idx += HASH_SIZE;

        memcpy(datagram + idx, flag.c_str(), FLAG_SIZE);
        idx += FLAG_SIZE;

        string seq = pad_number(sequence, SEQUENCE_SIZE);
        memcpy(datagram + idx, seq.c_str(), SEQUENCE_SIZE);
        idx += SEQUENCE_SIZE;

        memcpy(datagram + idx, payload.c_str(), payload.size());

        if(debug) cout << "\n" << "WRITE:>>>" << string(datagram, DATAGRAM_SIZE) << "<<<" << "\n";
        sendto(
            socket_fd,
            datagram,
            DATAGRAM_SIZE,
            0,
            (sockaddr *)&addr,
            sizeof(addr)
        );
    }

    vector<string> split_payload(const string &data) {

        vector<string> chunks;

        int offset = 0;

        while (offset < data.size()) {

            int chunk_size =
                min(PAYLOAD_SIZE, (int)data.size() - offset);

            chunks.push_back(
                data.substr(offset, chunk_size)
            );

            offset += chunk_size;
        }

        return chunks;
    }

    void send_message(
        int socket_fd,
        sockaddr_in addr,
        const string &data
    ) {



        // SINGLE DATAGRAM CASE
        if (data.size() <= PAYLOAD_SIZE) {

            string payload = data;

            payload.append(PAYLOAD_SIZE - payload.size(), '#');

            char hash = checksum(payload) + '0';

            send_datagram(
                socket_fd,
                addr,
                hash,
                "11",
                0,
                payload
            );

            return;
        }

        // MULTI FRAGMENT CASE
        vector<string> chunks = split_payload(data);

        for (int i = 0; i < chunks.size(); i++) {

            string flag = "00";

            if (i == 0)
                flag = "01";

            else if (i == chunks.size() - 1)
                flag = "11";

            string payload = chunks[i];

            if (i == chunks.size() - 1 &&
                payload.size() < PAYLOAD_SIZE) {

                payload.append(PAYLOAD_SIZE - payload.size(), '@');
            }

            char hash = checksum(payload) + '0';

            send_datagram(
                socket_fd,
                addr,
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

        protocol += action;

        protocol += pad_number(nick_name.size(), NICK_NAME_SIZE);
        protocol += nick_name;

        protocol += pad_number(destination.size(), DESTINY_NICK_NAME_SIZE);
        protocol += destination;

        protocol += pad_number(message.size(), MESSAGE_SIZE);
        protocol += message;

        protocol += pad_number(file_name.size(), FILE_NAME_SIZE);
        protocol += file_name;

        protocol += pad_number(file_data.size(), FILE_SIZE_SIZE);
        protocol += file_data;

        return protocol;
    }

    void parse_protocol(
        const string &data,
        char &action,
        string &nick_name,
        string &destination,
        string &message,
        string &file_name,
        string &file_data
    ) {

        int idx = 0;

        action = data[idx];
        idx += ACTION_SIZE;

        int nick_size = stoi(data.substr(idx, NICK_NAME_SIZE));
        idx += NICK_NAME_SIZE;

        nick_name = data.substr(idx, nick_size);
        idx += nick_size;

        int destSize = stoi(data.substr(idx, DESTINY_NICK_NAME_SIZE));
        idx += DESTINY_NICK_NAME_SIZE;

        destination = data.substr(idx, destSize);
        idx += destSize;

        int msgSize = stoi(data.substr(idx, MESSAGE_SIZE));
        idx += MESSAGE_SIZE;

        message = data.substr(idx, msgSize);
        idx += msgSize;

        int fileNameSize = stoi(data.substr(idx, FILE_NAME_SIZE));
        idx += FILE_NAME_SIZE;

        file_name = data.substr(idx, fileNameSize);
        idx += fileNameSize;

        int fileSize = stoi(data.substr(idx, FILE_SIZE_SIZE));
        idx += FILE_SIZE_SIZE;

        file_data = data.substr(idx, fileSize);
    }

    void process_message(
        int socket_fd,
        const string &full_data,
        sockaddr_in sender_addr
    ) {
        char action;
        string nick_name;
        string destination;
        string message;
        string file_name;
        string file_data;

        parse_protocol(
            full_data,
            action,
            nick_name,
            destination,
            message,
            file_name,
            file_data
        );

        {
            lock_guard<mutex> lock(clients_mutex);

            clients[nick_name] = {sender_addr};
        }

        // Print all connected clients
        print_connected_clients();

        cout << "\n========== MESSAGE ==========\n";

        cout << "FROM: " << nick_name << "\n";
        cout << "ACTION: " << action << "\n";

        if (action == 'M' || action == 'B')
            cout << "TEXT: " << message << "\n";

        if (action == 'F') {
            cout << "FILE: " << file_name << "\n";
            cout << "FILE SIZE: "
                 << file_data.size()
                 << "\n";
        }

        cout << "=============================\n";

        if (action == 'B') {

            lock_guard<mutex> lock(clients_mutex);

            for (auto &c : clients) {

                if (c.first == nick_name)
                    continue;

                send_message(
                    socket_fd,
                    c.second.address,
                    full_data
                );
            }
        }

        else if (action == 'M' || action == 'F') {

            lock_guard<mutex> lock(clients_mutex);

            if (clients.count(destination)) {

                send_message(
                    socket_fd,
                    clients[destination].address,
                    full_data
                );
            }
        }

        else if (action == 'O') {
            {
                lock_guard<mutex> lock(clients_mutex);
                clients.erase(nick_name);
            }
            cout << "Client disconnected: " << nick_name << "\n";
            print_connected_clients();
        }

        else if (action == 'L') {
            string json;
            {
                lock_guard<mutex> lock(clients_mutex);
                json = build_list_json();
            }
            // Reutilizar build_protocol para envolver el JSON en el campo message
            string response = build_protocol('L', "server", nick_name, json, "", "");
            send_message(socket_fd, sender_addr, response);
        }
    }

    void receive_thread(
        int socket_fd,
        string full_data,
        sockaddr_in client_addr
    ) {

        process_message(socket_fd, full_data, client_addr);
    }

    string build_list_json() {
        string json = "{\"clients\":[";
        bool first = true;
        for (const auto &c : clients) {
            if (!first) json += ",";
            json += "{\"nick\":\"" + c.first + "\","
                    + "\"ip\":\"" + string(inet_ntoa(c.second.address.sin_addr)) + "\","
                    + "\"port\":" + to_string(ntohs(c.second.address.sin_port)) + "}";
            first = false;
        }
        json += "]}";
        return json;
    }


};





int main() {
    ServerUDP server;

    server.run();


    return 0;
}
