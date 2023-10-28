#include <unistd.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/time.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <asm/socket.h>

#include "../lib/types.h"
#include "../lib/logger.h"
#include "../lib/colors.h"
RestaurantState state = CLOSED;
SupplierInfo suppliers[MAX_SUPPLIER];
int suppliersCount = 0;

OrderInfo orders[MAX_ORDER];
int ordersCount = 0;

SaleInfo sales[MAX_SALE];
int salesCount = 0;

int isSupplierUnique(const char *username) {
    for (int i = 0; i < suppliersCount; i++) {
        if (strcmp(suppliers[i].username, username) == 0) {
            return 0;  // Username is not unique
        }
    }
    return 1;  // Username is unique
}

void addSupplier(const char *username, unsigned short port) {
    SupplierInfo* newSupplier = &suppliers[suppliersCount];
    suppliersCount++;
    memset(newSupplier->username, '\0', ID_SIZE);
    strcpy(newSupplier->username, username);
    newSupplier->port = port;
}

void addOrder(const char *username, unsigned short port, const char *food) {
    OrderInfo* newOrder = &orders[ordersCount];
    ordersCount++;
    memset(newOrder->username, '\0', ID_SIZE);
    strcpy(newOrder->username, username);
    newOrder->port = port;
    memset(newOrder->food, '\0', MAX_FOOD_NAME);
    strcpy(newOrder->food, food);
}

void addSale(const char *username, const char *food, OrderResult result) {
    SaleInfo* newSale = &sales[salesCount];
    salesCount++;
    memset(newSale->username, '\0', ID_SIZE);
    strcpy(newSale->username, username);
    memset(newSale->food, '\0', MAX_FOOD_NAME);
    strcpy(newSale->food, food);
    newSale->result = result;
}

void printSuppliers() {
    if (suppliersCount == 0) {
        printf("%sNo available supplier%s\n", ANSI_RED, ANSI_RST);
        return;
    }
    printf("--------------------\n");
    printf("<username>::<port>\n");
    for (int i = 0; i < suppliersCount; i++) 
        printf("%s%s%s::%s%hu%s\n", ANSI_YEL, suppliers[i].username, ANSI_RST, ANSI_GRN, suppliers[i].port, ANSI_RST);
    printf("--------------------\n");
}

void printOrders() {
    if (ordersCount == 0) {
        printf("%sNo available order%s\n", ANSI_YEL, ANSI_RST);
        return;
    }
    printf("--------------------\n");
    printf("<username>::<port> --> <food>\n");
    for (int i = 0; i < ordersCount; i++) 
        printf("%s%s%s::%s%hu%s --> %s%s%s\n", ANSI_YEL, orders[i].username, ANSI_RST, ANSI_GRN,
                     orders[i].port, ANSI_RST, ANSI_PUR, orders[i].food, ANSI_RST);
    printf("--------------------\n");
}

void printSales() {
    if (salesCount == 0) {
        printf("%sNo available sale%s\n", ANSI_YEL, ANSI_RST);
        return;
    }
    char *accStr = "accepted";
    char *denStr = "rejected";
    printf("--------------------\n");
    printf("<username>--<food> --> <result>\n");
    for (int i = 0; i < salesCount; i++)  {
         printf("%s%s%s--%s%s%s --> \n", ANSI_YEL, sales[i].username, ANSI_RST, ANSI_GRN,
                    sales[i].food, ANSI_RST);
        (sales[i].result == ACCEPTED) ? pritnf("%s%s%s", ANSI_GRN, "accepted", ANSI_RST) :
                                            pritnf("%s%s%s", ANSI_RED, "rejected", ANSI_RST);
    }
    printf("--------------------\n");
}

int makeBroadcast(struct sockaddr_in* addrOut, int port){
    int broadcastFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (broadcastFd < 0) return broadcastFd;

    struct sockaddr_in addr;
    int broadcast = 1;
    int reuseport = 1;
    setsockopt(broadcastFd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    setsockopt(broadcastFd, SOL_SOCKET, SO_REUSEPORT, &reuseport, sizeof(reuseport));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr("192.168.1.255");
    *addrOut = addr;
    return broadcastFd;
}

int makeTCP(struct sockaddr_in* addrOut){
    int tcpFd;
    tcpFd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr;
    int reuseport = 1;
    setsockopt(tcpFd, SOL_SOCKET, SO_REUSEPORT, &reuseport, sizeof(reuseport));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = inet_addr("192.168.1.255");
    *addrOut = addr;
    return tcpFd;
}

int connectServer(int fd, struct sockaddr_in server, int port) {
    
    server.sin_family = AF_INET; 
    server.sin_port = htons(port); 
    server.sin_addr.s_addr = inet_addr("192.168.1.255");

    if (connect(fd, (struct sockaddr *)&server, sizeof(server)) < 0) { // checking for errors
        printf("Error in connecting to server\n");
    }

    return fd;
}

CLIResult handleCLI(){
    CLIResult answer;
    answer.result = 1;
    memset(answer.buffer, '\0', BUFFER_SIZE);
    read(0, &answer.buffer[ID_SIZE], BUFFER_SIZE - ID_SIZE);
    if (strncmp(&answer.buffer[ID_SIZE], "start working", strlen("start working")) == 0) {
        if (state == OPEN) {
            logTerminalWarning("Restaurant is already open");
            // TODO log file
            answer.result = 0;
            return answer;
        }
        state = OPEN;
    }
    if (state == CLOSED) {
        logTerminalError("Restaurant is closed");
        answer.result = 0;
        return answer;
    }
    if (strncmp(&answer.buffer[ID_SIZE], "break", strlen("break")) == 0) {
        if (state == CLOSED) {
            logTerminalWarning("Restaurant is already closed");
            // TODO log file
            answer.result = 0;
            return answer;
        }
        state = CLOSED;
    }
    else if (strncmp(&answer.buffer[ID_SIZE], "show suppliers", strlen("show suppliers")) == 0) {
        printSuppliers();
        // TODO log file
    }
    else if (strncmp(&answer.buffer[ID_SIZE], "show requests list", strlen("show requests list")) == 0) {
        printOrders();
        // TODO log file
    }
    return answer;
    // TODO log file
}
void handleIncomingBC(char* buffer, char* identifier){
    if (strncmp(buffer, identifier, strlen(identifier)) == 0) // check self broadcasting
        return;
    char test[100];
    memset(test, '\0', sizeof(test));
    if (state == OPEN && strncmp(&buffer[ID_SIZE], "hello supplier", strlen("hello supplier")) == 0) {
        strtok(&buffer[ID_SIZE], "-");
        char* username = strtok(NULL, "-"); // username
        char* port = strtok(NULL, "-");
        if (!isSupplierUnique(username))
            return;
        
        addSupplier(username, atoi(port));
        return;
    }
    write(0, buffer, BUFFER_SIZE);
}

int main(int argc, char const *argv[]) {
    CLIResult ans;

    int tcpSock, bcSock, broadcast = 1, opt = 1;
    char buffer[1024] = {0};
    struct sockaddr_in bcAddress, tcpAddress;
    fd_set readfds;
        char identifier[100];

    sprintf(identifier, "%d", getpid());
    char username[100];
    memset(username, '\0', ID_SIZE);
    memcpy(username, "rest1", strlen("rest1"));
    
    tcpSock = makeTCP(&tcpAddress);
    bcSock = makeBroadcast(&bcAddress, 1234);
    // setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    // setsockopt(tcpSock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

    bind(bcSock, (struct sockaddr *)&bcAddress, sizeof(bcAddress));
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(tcpSock, &readfds);
        FD_SET(bcSock, &readfds);
        FD_SET(0, &readfds);

        select(bcSock + 1, &readfds, NULL, NULL, NULL);
        
        if (FD_ISSET(0, &readfds)) {
            memset(buffer, '\0', 1024);
            ans = handleCLI();
            if (ans.result == 1) {
                memcpy(ans.buffer, username, ID_SIZE);
                sendto(bcSock, ans.buffer, BUFFER_SIZE, 0,(struct sockaddr *)&bcAddress, sizeof(bcAddress));
            }
        }

        if (FD_ISSET(bcSock, &readfds)) {
            memset(buffer, 0, 1024);
            recv(bcSock, buffer, 1024, 0);
            handleIncomingBC(buffer, username);

    
        }
    }
    // write(0, "Enter username: ", sizeof("Enter username: "));
    // memcpy(buffer+strlen(identifier), "hello", strlen("hello"));
    // read(0, username, 100);
    // memcpy(buffer+strlen(identifier)+strlen("hello"), username, strlen(username));
    
    // sendto(tcpSock, buffer, strlen(buffer), 0,(struct sockaddr *)&bcAddress, sizeof(bcAddress));

    return 0;
}