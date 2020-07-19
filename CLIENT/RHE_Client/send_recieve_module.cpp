#include "send_recieve_module.h"

Send_Recieve_Module::Send_Recieve_Module() {
    WORD version;
    WSADATA wsaData;
    version = MAKEWORD(2, 2);
    WSAStartup(version, dynamic_cast<LPWSADATA>(&wsaData));
    hostEntry = gethostbyname("192.168.1.10");
    if(!hostEntry) {
        printf(">>> ERROR (hostEntry NULL)\n");
        WSACleanup();
    }
}

void Send_Recieve_Module::send_data(QByteArray *data) {
    SOCKET theSocket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serverInfo;
    serverInfo.sin_family = AF_INET;
    serverInfo.sin_addr = *(reinterpret_cast<LPIN_ADDR>(*hostEntry->h_addr_list));
    serverInfo.sin_port = htons(8888);
    connect(theSocket, reinterpret_cast<LPSOCKADDR>(&serverInfo), sizeof(serverInfo));
    send(theSocket, data->data(), data->size(), 0);
}
