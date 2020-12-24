//拥塞控制版本 3-3 by WCH 
//此程序为接收端
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<math.h>
#include<time.h>
#include<winsock2.h>
#include<windows.h>
#pragma comment(lib, "ws2_32.lib")
#define S_PORT 1111//发送端端口
#define R_PORT 3617//接收端端口
#define RETIME 5   //包有五次机会

// 每段DATA字节数
#define SIZEB 1014
// 路径长度
#define SIZEP 192
#define MINWIN 4
#define MAXWIN 64

int WINDOW = 8;
int lastRecv = 0;

//SendPortAddr RecvPortAddr
typedef struct sockaddr_in SockAddrIn;
SockAddrIn saddr, raddr;

typedef struct RUCPACKET {
	int seq;//序号
	int CKS;//校验码
	unsigned short len;//数据长度
	char data[SIZEB];
}ruc;
// 来自Send的包和来自Recv的包
ruc* FromSendPort, * FromRecvPort;
int partNum;//多少部分
int dgsize = sizeof(*FromSendPort);
char tosend[SIZEB];//准备发送
char fName[SIZEP];//文件名
char dfName[SIZEP * 2];//文件路径名
HANDLE handle;
DWORD hID;
int rec = 0;

WSADATA wsadata;
SOCKET rsock;
// S程序的addr,R程序的addr
SOCKADDR_IN saddr, raddr;
int addlen = sizeof(raddr);

//找到第一个0
int findzero(int size, int* array)
{
	int ret = 0;
	for (ret = 0; ret < size; ret++)
	{
		if (array[ret] == 0)
			return ret;
	}//第ret个包处于w未收到，之前的包全收到
	return size;
}

/*
	3-3版本中唯一新增的方法实现拥塞控制
	具体思路是通过比较当前的窗口大小和这一次接收到的包数来改变窗口大小
*/
void setWindow(int recvwindow)
{
	int packetNum = recvwindow - lastRecv;
	int lastWindow = WINDOW;//执行前的window大小

	if (packetNum == lastWindow)//接收到的包和窗口一样多，认为可以接收更多,WINDOW翻倍
	{
		WINDOW = 2 * lastWindow;
		if (WINDOW >= MAXWIN)
		{
			WINDOW = MAXWIN;
		}
	}
	else if(packetNum*2 < lastWindow)
	{
		WINDOW = lastWindow / 2;
		if (WINDOW <= MINWIN)
		{
			WINDOW = MINWIN;
		}
	}
	else
	{
		WINDOW += 2;
		if (WINDOW >= MAXWIN)
		{
			WINDOW = MAXWIN;
		}
	}
	lastRecv = recvwindow;
}

//初始化 打包 校验
void ruc_init(ruc* r)
{
	memset(r->data, '\0', SIZEB);
	r->len = 0;
	r->seq = 0;
	r->CKS = 0;
}
void ruc_pack(int s, char d[])
{
	memset(FromRecvPort->data, '\0', SIZEB);
	FromRecvPort->len = (unsigned short)strlen(d);
	memcpy(FromRecvPort->data, d, SIZEB);

	FromRecvPort->seq = s;
	int sum = 0;
	for (int i = 0; i < FromRecvPort->len; i++)
	{
		sum += (int)d[i];
	}
	sum += (s + FromRecvPort->len);
	FromRecvPort->CKS = sum;
}
int ruc_chek(ruc* r)
{
	if (!r->data)
	{
		printf("Packet is empty\n");
	}
	else
	{
		int sum = 0;
		for (int i = 0; i < r->len; i++)
		{
			sum += (int)r->data[i];
		}
		sum = sum + r->len + r->seq - r->CKS;
		if (sum != 0)
		{
			printf("CheckSum has something wrong! The packet has been thrown\n");
			return -1;
		}
	}
	return 0;
}

// 字符串转整型
int stoi(char* acStr)
{
	int i, iIndex = 0, iNum = 0, iSize = 0;
	if (acStr[0] == '+' || acStr[0] == '-')
		iIndex = 1;

	for (iSize = iIndex; ; iSize++)
		if (acStr[iSize] < '0' || acStr[iSize] > '9')
			break;

	for (i = iIndex; i < iSize; i++)
		iNum += (int)pow(10, iSize - i - 1) * (acStr[i] - 48);

	if (acStr[0] == '-')
		iNum = -iNum;

	return iNum;
}
// 整型转字符串
void itos(int iInt, char* acStr)
{
	int iIndex = 0, iSize, iNum, iBit, i, j;

	if (iInt < 0)
	{
		acStr[0] = '-';
		iInt = -iInt;
		iIndex = 1;
	}
	for (i = 0; ; i++)
		if (iInt < pow(10, i))
			break;
	iSize = i;

	for (i = 0; i < iSize; i++)
	{
		iNum = pow(10, iSize - i - 1);
		iBit = iInt / iNum;
		iInt -= iNum * iBit;
		acStr[i + iIndex] = iBit + 48;
	}
	if (iSize != 0)
		acStr[iSize + iIndex] = '\0';
	else
	{
		acStr[0] = '0';
		acStr[1] = '\0';
	}
}

void UDPsleep(int time)
{

	Sleep(time);

}
void UDPawake(char* ip)
{
	// Winsows 启用 socket
	if (WSAStartup(MAKEWORD(1, 1), &wsadata) == SOCKET_ERROR)
	{
		printf("启用 socket 失败\n");
		exit(0);
	}
	// 新建 socket
	if ((rsock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		printf("新建 socket 失败\n");
		exit(0);
	}
	int retime = 10000;
	// 清零
	memset(&saddr, 0, sizeof(saddr));
	memset(&raddr, 0, sizeof(raddr));

	// 设置协议 IP 地址及 Port
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(S_PORT);
	saddr.sin_addr.s_addr = inet_addr(ip);

	raddr.sin_family = AF_INET;
	raddr.sin_port = htons(R_PORT);
	raddr.sin_addr.s_addr = htonl(INADDR_ANY);

	// 绑定端口，监听端口
	if (bind(rsock, (struct sockaddr*)&raddr, sizeof(raddr)) == -1)
	{
		printf("绑定端口失败\n");
		exit(0);
	}
}
void UDPclose(void)
{

	// Winsows 关闭 socket
	closesocket(rsock);
	WSACleanup();

}

DWORD WINAPI ruc_send(LPVOID p)
{
	int r = rec;
	int i = 0;
	while (1)
	{
		/*if(i>=RETIME)
		{
			printf("重传次数超过上限\n");
			exit(1);
		}*/
		sendto(rsock, FromRecvPort, dgsize, 0, (struct sockaddr*)&saddr, addlen);

		UDPsleep(10);
		for (int j = 0; j < 5; j++)
		{
			if (r < rec)
			{
				return 0;
			}
			UDPsleep(100);
		}
		//	printf("超时，再次尝试\n");
		i++;
	}
	return 0;
}

// 接收文件
void recvFile(char* dName, char* ip)
{
	int FromRecvPortPart;
	int fSize;
	FILE* p = NULL;

	FromSendPort = (ruc*)malloc(dgsize);
	FromRecvPort = (ruc*)malloc(dgsize);
	ruc_init(FromSendPort); ruc_init(FromRecvPort);

	UDPawake(ip);
	int rept;

	printf("准备接收文件名...\n");
	for (rept = 0; rept < 5; rept++)
	{
		if (recvfrom(rsock, FromSendPort, dgsize, 0, (struct sockaddr*)&saddr, &addlen) < 0)
		{
			printf("连接中断...\n\n");
			exit(1);
		}
		if (!ruc_chek(FromSendPort) && FromSendPort->seq == -2)
		{
			strcpy(fName, FromSendPort->data);
			ruc_init(FromSendPort);
			printf("接收到文件名:%s...\n\n", fName);
			strcpy(tosend, "size");
			ruc_pack(-1, tosend);
			handle = CreateThread(NULL, NULL, ruc_send, 0, 0, &hID);
			rept = 0;
			break;
		}
	}

	printf("准备接收文件大小...\n");
	for (rept = 0; rept < 5; rept++)
	{
		memset(tosend, '\0', SIZEB);
		if (recvfrom(rsock, FromSendPort, dgsize, 0, (struct sockaddr*)&saddr, &addlen) >= 0)
		{
			if (!ruc_chek(FromSendPort))
			{
				printf("接收到文件大小:%s...\n\n", FromSendPort->data);
				fSize = stoi(FromSendPort->data);
				ruc_init(FromSendPort);
				rept = 0;
				break;
			}
		}
	}

	printf("准备接收文件...\n\n");
	strcat(dfName, dName);
	strcat(dfName, fName);
	p = fopen(dfName, "w");
	fclose(p);
	p = fopen(dfName, "ab");

	//2
	FromRecvPortPart = fSize / SIZEB;
	partNum = fSize / SIZEB;
	if ((fSize % SIZEB) != 0)
	{
		partNum++;
	}
	printf("part:%d\n", partNum);
	int* recved;
	recved = (int*)malloc(partNum * sizeof(int));
	//全部设为0
	for (int i = 0; i < partNum; i++)
	{
		recved[i] = 0;
	}
	ruc** recvRuc;//暂时存放接收到数据的地方
	recvRuc = (ruc**)malloc(partNum * sizeof(ruc*));
	for (int y = 0; y < partNum; y++)
	{
		recvRuc[y] = (ruc*)malloc(sizeof(ruc));
	}
	int recvwindow = 0;

	for (int z = 0;; z++)
	{
		if ((recvwindow != 0)&&(lastRecv!=recvwindow))
		{
			setWindow(recvwindow);
			printf("WINDOW:%d\n", WINDOW);
		}
		

		if (recvwindow == partNum) { break; }//如果接满了就跑路
		memset(tosend, '\0', SIZEB);
		itos(recvwindow, tosend);

		ruc_pack(WINDOW, tosend);//包中seq作为WINDOW,data作为第几个
		handle = CreateThread(NULL, NULL, ruc_send, 0, 0, &hID);//发送此时的窗口信息
		//printf("第%d次循环，正在接收文件(窗口:%d)...\n",z,recvwindow);
		UDPsleep(50);
		for (int t = 0; t < WINDOW; t++)
		{

			recvfrom(rsock, FromSendPort, dgsize, 0, (struct sockaddr*)&saddr, &addlen);
			if (!ruc_chek(FromSendPort))
			{
				recved[FromSendPort->seq] = 1;
				memcpy(recvRuc[FromSendPort->seq]->data, FromSendPort->data, SIZEB);
				recvwindow = findzero(partNum, recved);//设置当前的接收窗口

				//printf("SEQ：%d\nDATA:%s\n", FromSendPort->seq, recvRuc[FromSendPort->seq]->data);
				if (FromSendPort->seq == partNum - 1)
				{
					break;
				}
			}
			UDPsleep(10);
		}
	}
	printf("正在保存文件...\n\n\n");
	//最后写
	int x = 0;
	for (x; x < partNum; x++)
	{
		//如果是不足一段的剩余文件
		if (((fSize % SIZEB) != 0) && (x >= partNum - 1))
		{
			//printf("last:%d\nData:%s", x, recvRuc[x]->data);
			fwrite(recvRuc[x]->data, sizeof(char), fSize % SIZEB, p);
			break;
		}
		else
		{
			//printf("part:%d\nData:%s",x, recvRuc[x]->data);
			fwrite(recvRuc[x]->data, sizeof(char), SIZEB, p);//少10B
			//还需要解决窗口只能读一次的bug
		}

	}
	memset(tosend, '\0', SIZEB);
	strcpy(tosend, "over");
	ruc_pack(999, tosend);
	handle = CreateThread(NULL, NULL, ruc_send, 0, 0, &hID);
	UDPsleep(10);
	printf("成功接收文件...\n\n");
	fclose(p);
}

int main(void)
{
	clock_t start, end;
	char dName[SIZEP] = "C:/Users/49393/Desktop/recv/";
	//char dName[SIZEP];
	//printf("本程序为接收端，请输入需要保存的路径名:\n");
	//scanf("%s", dName);
	//printf("请输入发送端的IP地址:\n");
	//char sIP[15];
	//scanf("%s", sIP);
	char sIP[15] = "127.0.0.1";
	start = clock();
	recvFile(dName, sIP);
	end = clock();
	printf("接收用时%f\n", (double)(end - start));
	system("pause");
	UDPclose();
	return 0;
}