// TCPRequestChannel.cpp
#include "TCPRequestChannel.h"
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

using namespace std;

TCPRequestChannel::TCPRequestChannel(const string _ip_address, const string _port_no) {
    if (_ip_address.empty()) {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            // perror("Error creating socket");
            exit(-1);
        }

        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(stoi(_port_no));

        // memset(&server_addr, 0, sizeof server_addr);

        if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            // perror("Error binding socket");
            exit(-1);
        }

        if (listen(sockfd, 30) < 0) {
            // perror("Error listening for connections");
            exit(-1);
        }
    }
    else {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            // perror("Error creating socket");
            exit(-1);
        }

        struct sockaddr_in client_addr;
        client_addr.sin_family = AF_INET;
        client_addr.sin_port = htons(stoi(_port_no));

        // memset(&client_addr, 0, sizeof client_addr);

        if (inet_aton(_ip_address.c_str(), &client_addr.sin_addr) < 0) {
            // perror("Error converting IP address");
            exit(-1);
        }

        if (connect(sockfd, (struct sockaddr*)&client_addr, sizeof(client_addr)) < 0) {
            // perror("Error connecting to server");
            exit(-1);
        }
    }
}

TCPRequestChannel::TCPRequestChannel(int _sockfd) : sockfd(_sockfd) {}

TCPRequestChannel::~TCPRequestChannel() {
    close(sockfd);
}

int TCPRequestChannel::accept_conn() {
    return accept(sockfd, nullptr, nullptr);
}

int TCPRequestChannel::cread(void* msgbuf, int msgsize) {
    return recv(sockfd, msgbuf, msgsize, 0);
}

int TCPRequestChannel::cwrite(void* msgbuf, int msgsize) {
    return write(sockfd, msgbuf, msgsize);
}
