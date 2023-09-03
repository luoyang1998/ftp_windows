#include <stdio.h>
#include "ftpClient.h"

char g_recvBuf[1024];   // ������Ϣ�Ļ�����
char *g_fileBuf;         // �洢�ļ�����
int g_filesize;         // �ļ���С
char g_fileName[256];   // ��������������ļ���

// ��ʼ��socket��
bool initSocket(){
    WSADATA wsadata;
    // ��һ���׽���
    if(0 != WSAStartup(MAKEWORD(2,2), &wsadata)){
        printf("WSAStartup faile:%d\n", WSAGetLastError());
        return false;
    }
    return true;
}
// �ر�socket��
bool closeSocket(){
    if(0 != WSACleanup()){
        printf("WSACleanup faile:%d\n", WSAGetLastError());
        return false;
    }
    return true;
}
// �����ͻ�������
void connectToHost(){
    // ����server socket�׽��� ��ַ �˿ں�
    SOCKET serfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (INVALID_SOCKET == serfd){
        printf("socket faild:%d\n", WSAGetLastError());
        return;
    }
    // ��socket��IP��ַ�Ͷ˿ں� �ͻ��˲���Ҫ�󶨶˿ں�
    struct sockaddr_in serAddr;
    serAddr.sin_family = AF_INET; // ����ʹ���socketָ����һ��
    serAddr.sin_port = htons(SPORT); // htons �ѱ����ֽ���תΪ�����ֽ���
    serAddr.sin_addr.S_un.S_addr = inet_addr("192.168.127.1"); // ��������ַ
    // ���ӵ�������
    if(0 != connect(serfd, (struct sockaddr*)&serAddr,sizeof (serAddr))){
        printf("connect faild:%d\n", WSAGetLastError());
        return;
    }

    downloadFileName(serfd);
    // ��ʼ������Ϣ
    while (processMesg(serfd)){
//        Sleep(100);
    }
}
// ������Ϣ
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
            printf("�������~\n");
            closesocket(serfd);
            return false;
            break;
    }
    return true;
}

void downloadFileName(SOCKET serfd){
    char fileName[1024] = "hello!~";
    printf("������Ҫ���ص��ļ�����");
//    gets_s(fileName,1023);   // �����ļ���
    gets(fileName);

    struct MsgHearder file;
    file.msgID = MSG_FILENAME;
    strcpy(file.fileInfo.fileName, fileName);

    send(serfd, (char*)&file, sizeof(struct MsgHearder), 0);
}
void readyread(SOCKET serfd, struct MsgHearder* pmsg){

    // ׼���ڴ� pmsg->fileInfo.fileSize
    // ������������ MSG_READY_READ
    strcpy(g_fileName, pmsg->fileInfo.fileName);
    g_filesize = pmsg->fileInfo.fileSize;
    g_fileBuf = calloc(g_filesize+1,sizeof (char)); // ����ռ�
    if (g_fileBuf == NULL){ // �ڴ�����ʧ��
        printf("�ڴ治����������\n");
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
    // �ж��ļ��Ƿ��������������Ƿ���������
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
