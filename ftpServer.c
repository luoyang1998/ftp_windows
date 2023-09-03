#include <stdio.h>
#include "ftpServer.h"

char g_recvBuf[1024];   // �������ܿͻ��˷��͵�
int g_fileSize;         // �ļ���С
char* g_fileBuf;        // �洢�ļ�
// ��ʼ��socket��
bool initSocket(){
    WSADATA wsadata;    // ���windows socket��ʼ����Ϣ
    // ��һ���׽���
    if(0 != WSAStartup(MAKEWORD(2,2), &wsadata)){   // ͨ����������winsock DLL(�׽��ֹ淶��WSASATA���ݽṹָ��)
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
void listenToClient(){
    // ����server socket�׽��� ��ַ �˿ں�
    SOCKET serfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (INVALID_SOCKET == serfd){
        printf("socket faild:%d\n", WSAGetLastError());
        return;
    }
    // ��socket��IP��ַ�Ͷ˿ں�
    struct sockaddr_in serAddr;
    serAddr.sin_family = AF_INET; // ����ʹ���socketָ����һ��
    serAddr.sin_port = htons(SPORT); // htons �ѱ����ֽ���תΪ�����ֽ���
    serAddr.sin_addr.S_un.S_addr = ADDR_ANY; // ����������������
    if (0 != bind(serfd, (struct sockaddr*)&serAddr, sizeof(serAddr))) {
        printf("bind faild:%d\n", WSAGetLastError());
        return;
    }
    // �����ͻ�������
    if (0 != listen(serfd, 10)){
        printf("listen faild:%d\n", WSAGetLastError());
        return;
    }
    printf("listen to client...\n");
    // �пͻ������ӣ���ô��Ҫ��������
    struct sockaddr_in cliAddr;
    int len = sizeof (cliAddr);
    SOCKET clifd = accept(serfd, (struct sockaddr*)&cliAddr, &len);
    if (INVALID_SOCKET == clifd){
        printf("accept faild:%d\n", WSAGetLastError());
        return;
    }
    printf("new client connnect success...\n");
    // ��ʼ������Ϣ
    while (processMesg(clifd)){
        Sleep(100);
    }
}
// ������Ϣ
bool processMesg(SOCKET clifd){
//    char recvBuf[1024] = {0};
    // �ɹ�������Ϣ�����ؽ��յ����ֽ�����ʧ�ܷ���0
    int nRes = recv(clifd, g_recvBuf, 1024, 0);
    if (nRes <= 0){
        printf("�ͻ������ߡ�����%d\n",WSAGetLastError());
    }

    // ��ȡ���յ�����Ϣ
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
                printf("�ļ�����ʧ��:%d\n",WSAGetLastError());
            }
            printf("�������~\n");
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
        printf("�Ҳ���[%s]�ļ�...\n",pMsg->fileInfo.fileName);
        struct MsgHearder msg;
        msg.msgID = MSG_OPENFILE_FAILD;
        if ( SOCKET_ERROR == send(clifd,(char*)&msg, sizeof(struct  MsgHearder), 0)){
            printf("send faile:%d\n", WSAGetLastError());
        }
        return false;
    }
    // ��ȡ�ļ���С
    fseek(pread,0,SEEK_END); // �ļ���β
    g_fileSize = ftell(pread);
    fseek(pread,0,SEEK_SET); // �ļ���ͷ
    // ���ļ���С���͸��ͻ���
    struct MsgHearder msg;
    msg.msgID = MSG_FILESIZE;
    msg.fileInfo.fileSize = g_fileSize;
    char tfname[200] = {0}, text[100];
    _splitpath(pMsg->fileInfo.fileName,NULL,NULL,tfname,text); // �ָ�·��
    strcat(tfname, text);
    strcpy(msg.fileInfo.fileName,tfname);
    send(clifd,(char*)&msg, sizeof(struct MsgHearder), 0);

    // ��ȡ�ļ�����
    g_fileBuf = calloc(g_fileSize+1, sizeof(char));
    if (g_fileBuf == NULL){
        printf("�ڴ治�㣬������\n");
        return false;
    }
    fread(g_fileBuf, sizeof(char), g_fileSize, pread);
    g_fileBuf[g_fileSize] = '\0';

    fclose(pread);
    return true;
}
// �����ļ�
bool sendFile(SOCKET clifd, struct MsgHearder* pmsg){
    struct MsgHearder msg; // ���߿ͻ���׼�������ļ�
    msg.msgID = MSG_READY_READ;

    // ����ļ��ĳ��ȴ���ÿ�����ݰ��ܴ��͵Ĵ�С(PACKET_SIZE)��ô�ͷֿ�
    for (size_t i = 0; i < g_fileSize; i+=PACKET_SIZE) {
        msg.packet.nStart = i;
        // ���Ĵ�С�����ļ������ݵĴ�С
        if (i+PACKET_SIZE+1 > g_fileSize){
            msg.packet.nsize = g_fileSize - i;
        } else {
            msg.packet.nsize = PACKET_SIZE;
        }
//        strcpy(msg.packet.buf,g_fileBuf);
        memcpy(msg.packet.buf, g_fileBuf+msg.packet.nStart, msg.packet.nsize);
//        msg.packet.nsize = g_fileSize;
        if ( SOCKET_ERROR == send(clifd, (char*)&msg, sizeof(struct MsgHearder),0)){
            printf("�ļ�����ʧ��:%d\n",WSAGetLastError());
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
 * 1.�ͻ������������ļ� -> ���ļ������͸�������
 * 2.���������ܿͻ��˷��͵��ļ��� -> �����ļ������ҵ��ļ������ļ���С���͸��ͻ���
 * 3.�ͻ��˽��ܵ��ļ���С -> ׼����ʼ���ܣ������ڴ� ׼����ɸ��߷����������Կ�ʼ����
 * 4.���������յ���ʼ����ָ�� -> ��ʼ��������
 * 5.��ʼ�������ݣ������� -> ������ɣ����߷������������
 * 6.�ر�����
 */