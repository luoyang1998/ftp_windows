#include <stdio.h>
#include "ftpClient.h"

char g_recvBuf[1024];   // 接受消息的缓冲区
char *g_fileBuf;         // 存储文件内容
int g_filesize;         // 文件大小
char g_fileName[256];   // 保存服务器发送文件名

// 初始化socket库
bool initSocket(){
    WSADATA wsadata;
    // 打开一个套接字
    if(0 != WSAStartup(MAKEWORD(2,2), &wsadata)){
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
void connectToHost(){
    // 创建server socket套接字 地址 端口号
    SOCKET serfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (INVALID_SOCKET == serfd){
        printf("socket faild:%d\n", WSAGetLastError());
        return;
    }
    // 给socket绑定IP地址和端口号 客户端不需要绑定端口号
    struct sockaddr_in serAddr;
    serAddr.sin_family = AF_INET; // 必须和创建socket指定的一样
    serAddr.sin_port = htons(SPORT); // htons 把本地字节序转为网络字节序
    serAddr.sin_addr.S_un.S_addr = inet_addr("192.168.127.1"); // 服务器地址
    // 连接到服务器
    if(0 != connect(serfd, (struct sockaddr*)&serAddr,sizeof (serAddr))){
        printf("connect faild:%d\n", WSAGetLastError());
        return;
    }

    downloadFileName(serfd);
    // 开始处理消息
    while (processMesg(serfd)){
//        Sleep(100);
    }
}
// 处理消息
bool processMesg(SOCKET serfd){

    recv(serfd, g_recvBuf, 1024, 0);
    struct MsgHearder* msg = (struct MsgHearder*)g_recvBuf;
    switch(msg->msgID){
        case MAG_OPENFILE_FALID:
            downloadFileName(serfd);
            break;
        case MSG_FILESIZE:
            readyread(serfd, msg);
            break;
        case MSG_READY_READ:
            printf("read ready\n");
            writeFile(serfd,msg);
            break;
        case MSG_SUCCESSED:
            printf("传输完成~\n");
            closesocket(serfd);
            return false;
            break;
    }
    return true;
}

void downloadFileName(SOCKET serfd){
    char fileName[1024] = "hello!~";
    printf("请输入要下载的文件名：");
//    gets_s(fileName,1023);   // 发送文件名
    gets(fileName);

    struct MsgHearder file;
    file.msgID = MSG_FILENAME;
    strcpy(file.fileInfo.fileName, fileName);

    send(serfd, (char*)&file, sizeof(struct MsgHearder), 0);
}
void readyread(SOCKET serfd, struct MsgHearder* pmsg){

    // 准备内存 pmsg->fileInfo.fileSize
    // 给服务器发送 MSG_READY_READ
    strcpy(g_fileName, pmsg->fileInfo.fileName);
    g_filesize = pmsg->fileInfo.fileSize;
    g_fileBuf = calloc(g_filesize+1,sizeof (char)); // 申请空间
    if (g_fileBuf == NULL){ // 内存申请失败
        printf("内存不足吗，请重试\n");
    } else {
        struct MsgHearder msg;
        msg.msgID = MSG_SEND;
        if (SOCKET_ERROR == send(serfd,(char*)&msg,sizeof(struct MsgHearder), 0)) {
            printf("send error: %d", WSAGetLastError());
            return;
        }
    }
    printf("size:%d filename:%s\n",pmsg->fileInfo.fileSize,pmsg->fileInfo.fileName);

}

bool writeFile(SOCKET serfd, struct MsgHearder* pMsg){
    if (g_fileBuf == NULL) {
        return false;
    }
    int nStart = pMsg->packet.nStart;
    int nsize = pMsg->packet.nsize;
    memcpy(g_fileBuf+nStart, pMsg->packet.buf,nsize);
//    printf("packet:%d %d" ,nStart+nsize,g_filesize);
    // 判断文件是否完整，服务器是否发完了数据
    if (nStart + nsize >= g_filesize){
        FILE *pwrite = fopen(g_fileName,"wb");
        if (pwrite == NULL) {
            printf("write file error..\n");
            return false;
        }
        fwrite(g_fileBuf, sizeof(char), g_filesize, pwrite);
        fclose(pwrite);
        free(g_fileBuf);
        g_fileBuf = NULL;
        struct MsgHearder msg;
        msg.msgID = MSG_SUCCESSED;
        send(serfd, (char*)&msg, sizeof(struct MsgHearder), 0);

        return false;
    }

    return true;
}

int main(){
    initSocket();
    connectToHost();
    closeSocket();
    return 0;
}
