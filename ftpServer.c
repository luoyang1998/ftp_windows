#include <stdio.h>
#include "ftpServer.h"

char g_recvBuf[1024];   // 用来接受客户端发送的
int g_fileSize;         // 文件大小
char* g_fileBuf;        // 存储文件
// 初始化socket库
bool initSocket(){
    WSADATA wsadata;    // 存放windows socket初始化信息
    // 打开一个套接字
    if(0 != WSAStartup(MAKEWORD(2,2), &wsadata)){   // 通过进程启动winsock DLL(套接字规范，WSASATA数据结构指针)
        printf("WSAStartup faile:%d\n", WSAGetLastError());
        return false;
    }
    return true;
}
// 关闭socket库
bool closeSocket(){
    if(0 != WSACleanup()){
        printf("WSACleanup faile:%d\n", WSAGetLastError());
        return false;
    }
    return true;
}
// 监听客户端链接
void listenToClient(){
    // 创建server socket套接字 地址 端口号
    SOCKET serfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (INVALID_SOCKET == serfd){
        printf("socket faild:%d\n", WSAGetLastError());
        return;
    }
    // 给socket绑定IP地址和端口号
    struct sockaddr_in serAddr;
    serAddr.sin_family = AF_INET; // 必须和创建socket指定的一样
    serAddr.sin_port = htons(SPORT); // htons 把本地字节序转为网络字节序
    serAddr.sin_addr.S_un.S_addr = ADDR_ANY; // 监听本机所有网卡
    if (0 != bind(serfd, (struct sockaddr*)&serAddr, sizeof(serAddr))) {
        printf("bind faild:%d\n", WSAGetLastError());
        return;
    }
    // 监听客户端链接
    if (0 != listen(serfd, 10)){
        printf("listen faild:%d\n", WSAGetLastError());
        return;
    }
    printf("listen to client...\n");
    // 有客户端链接，那么就要接受链接
    struct sockaddr_in cliAddr;
    int len = sizeof (cliAddr);
    SOCKET clifd = accept(serfd, (struct sockaddr*)&cliAddr, &len);
    if (INVALID_SOCKET == clifd){
        printf("accept faild:%d\n", WSAGetLastError());
        return;
    }
    printf("new client connnect success...\n");
    // 开始处理消息
    while (processMesg(clifd)){
        Sleep(100);
    }
}
// 处理消息
bool processMesg(SOCKET clifd){
//    char recvBuf[1024] = {0};
    // 成功接受消息，返回接收到的字节数，失败返回0
    int nRes = recv(clifd, g_recvBuf, 1024, 0);
    if (nRes <= 0){
        printf("客户端下线。。。%d\n",WSAGetLastError());
    }

    // 获取接收到的消息
    struct MsgHearder* msg = (struct MsgHearder*)g_recvBuf;
    struct MsgHearder exitmsg;
    switch  (msg->msgID){
        case MSG_FILENAME:
            printf("%s\n", msg->fileInfo.fileName);
            readFile(clifd, msg);
            break;
        case MSG_SEND:
            printf("send file");
            sendFile(clifd, msg);
            break;
        case MSG_READY_READ:
            printf("read ready");
            readFile(clifd,msg);
            break;
        case MSG_SUCCESSED:
            exitmsg.msgID = MSG_SUCCESSED;
            if ( SOCKET_ERROR == send(clifd, (char*)&exitmsg, sizeof(struct MsgHearder),0)){
                printf("文件发送失败:%d\n",WSAGetLastError());
            }
            printf("传输完成~\n");
            closesocket(clifd);
            return false;
            break;
    }
//    printf("%s\n",g_recvBuf);
    return true;
}

bool readFile(SOCKET clifd, struct MsgHearder* pMsg){
    FILE *pread = fopen(pMsg->fileInfo.fileName,"rb");
    if (pread == NULL){
        printf("找不到[%s]文件...\n",pMsg->fileInfo.fileName);
        struct MsgHearder msg;
        msg.msgID = MSG_OPENFILE_FAILD;
        if ( SOCKET_ERROR == send(clifd,(char*)&msg, sizeof(struct  MsgHearder), 0)){
            printf("send faile:%d\n", WSAGetLastError());
        }
        return false;
    }
    // 获取文件大小
    fseek(pread,0,SEEK_END); // 文件结尾
    g_fileSize = ftell(pread);
    fseek(pread,0,SEEK_SET); // 文件开头
    // 把文件大小发送给客户端
    struct MsgHearder msg;
    msg.msgID = MSG_FILESIZE;
    msg.fileInfo.fileSize = g_fileSize;
    char tfname[200] = {0}, text[100];
    _splitpath(pMsg->fileInfo.fileName,NULL,NULL,tfname,text); // 分割路径
    strcat(tfname, text);
    strcpy(msg.fileInfo.fileName,tfname);
    send(clifd,(char*)&msg, sizeof(struct MsgHearder), 0);

    // 读取文件内容
    g_fileBuf = calloc(g_fileSize+1, sizeof(char));
    if (g_fileBuf == NULL){
        printf("内存不足，请重试\n");
        return false;
    }
    fread(g_fileBuf, sizeof(char), g_fileSize, pread);
    g_fileBuf[g_fileSize] = '\0';

    fclose(pread);
    return true;
}
// 发送文件
bool sendFile(SOCKET clifd, struct MsgHearder* pmsg){
    struct MsgHearder msg; // 告诉客户端准备接受文件
    msg.msgID = MSG_READY_READ;

    // 如果文件的长度大于每个数据包能传送的大小(PACKET_SIZE)那么就分块
    for (size_t i = 0; i < g_fileSize; i+=PACKET_SIZE) {
        msg.packet.nStart = i;
        // 包的大小大于文件总数据的大小
        if (i+PACKET_SIZE+1 > g_fileSize){
            msg.packet.nsize = g_fileSize - i;
        } else {
            msg.packet.nsize = PACKET_SIZE;
        }
//        strcpy(msg.packet.buf,g_fileBuf);
        memcpy(msg.packet.buf, g_fileBuf+msg.packet.nStart, msg.packet.nsize);
//        msg.packet.nsize = g_fileSize;
        if ( SOCKET_ERROR == send(clifd, (char*)&msg, sizeof(struct MsgHearder),0)){
            printf("文件发送失败:%d\n",WSAGetLastError());
        }
    }

    return true;
}

int main(){
    initSocket();
    listenToClient();
    closeSocket();
    return 0;
}

/*
 * 1.客户端请求下载文件 -> 把文件命发送给服务器
 * 2.服务器接受客户端发送的文件名 -> 根据文件命，找到文件；把文件大小发送给客户端
 * 3.客户端接受到文件大小 -> 准备开始接受，开辟内存 准备完成告诉服务器，可以开始发送
 * 4.服务器接收到开始发送指令 -> 开始发送数据
 * 5.开始接受数据，存起来 -> 接受完成，告诉服务器接受完成
 * 6.关闭链接
 */