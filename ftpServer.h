#pragma once
#include <winsock2.h>
#pragma comment(lib,"ws2_32.lib")
#include <stdbool.h>
#include <stdio.h>
#define SPORT 8888 // 端口号
#define PACKET_SIZE (1024-sizeof(int)*3)

// 定义标记
enum MSGTAG{
    MSG_FILENAME = 1,   // 文件名
    MSG_FILESIZE = 2,   // 文件大小
    MSG_READY_READ = 3, // 准备接受
    MSG_SEND = 4,       // 发送
    MSG_SUCCESSED = 5,  // 传输完成
    MSG_OPENFILE_FAILD = 6  // 告诉客户端文件找不到
};
#pragma pack(1)  // 设置结构体1字节对齐
struct MsgHearder{ // 封装消息头
    enum MSGTAG msgID;  //当前消息标记 枚举类型4字节
    union { // 二者只存在一个,加union命名访问不到内部struct
        struct {
            char fileName[256]; //文件名   4
            int fileSize;       //文件大小 256
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
void listenToClient();
// 处理消息
bool processMesg(SOCKET);
// 读取文件，获得文件大小
bool readFile(SOCKET, struct MsgHearder*);
// 发送文件
bool sendFile(SOCKET, struct MsgHearder*);
