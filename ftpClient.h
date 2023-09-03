#pragma once
#include <winsock2.h>
#pragma comment(lib,"ws2_32.lib")
#include <stdbool.h>
#define SPORT 8888 // 端口号
#define PACKET_SIZE (1024-sizeof(int)*3)

// 定义标记
enum MSGTAG{
    MSG_FILENAME = 1,   // 文件名
    MSG_FILESIZE = 2,   // 文件大小
    MSG_READY_READ = 3, // 准备接受
    MSG_SEND = 4,       // 发送
    MSG_SUCCESSED = 5,   // 传输完成
    MAG_OPENFILE_FALID = 6   // 告诉客户端找不到
};

#pragma pack(1)  // 设置结构体1字节对齐
struct MsgHearder{ // 封装消息头
    enum MSGTAG msgID;  //当前消息标记 枚举类型4字节
    union { // 二者只存在一个,加union命名访问不到内部struct
        struct {
            char fileName[256]; //文件名   256
            int fileSize;       //文件大小 4
        }fileInfo;
        struct{
            int nStart;         // 包的编号
            int nsize;          // 该包数据大小
            char buf[PACKET_SIZE];
        }packet;
    };
};
#pragma pack()  // 恢复对齐

// 初始化socket库
bool initSocket();
// 关闭socket库
bool closeSocket();
// 监听客户端链接
void connectToHost();
// 处理消息
bool processMesg(SOCKET);
void downloadFileName(SOCKET serfd);
void readyread(SOCKET serfd, struct MsgHearder*);
// 写文件
bool writeFile(SOCKET, struct MsgHearder*);
