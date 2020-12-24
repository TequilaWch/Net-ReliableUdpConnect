//起码能用版本 3-1 by WCH 
//此程序为发送端
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<math.h>
#include<time.h>
#include<winsock2.h>
#include<windows.h>
#pragma comment(lib, "ws2_32.lib")
//发送FromSendPort包，接收FromRecvPort包
#define S_PORT 4939//发送端端口
#define R_PORT 3617//接收端端口
#define RETIME 5   //包有五次机会

// 每段DATA字节数
#define SIZEB 2038

// 路径长度
#define SIZEP 192
//SendPortAddr RecvPortAddr
typedef struct sockaddr_in SockAddrIn;
SockAddrIn saddr, raddr;

typedef struct RUCPACKET{
	int seq;//序号
	int CKS;//校验码
	unsigned short len;//数据长度
	char data[SIZEB];
}ruc;
// 来自Send的包和来自Recv的包
ruc *FromRecvPort,*FromSendPort;
int dgsize = sizeof(*FromRecvPort);
char tosend[SIZEB];//准备发送
char fName[SIZEP];//文件名
char dName[SIZEP];//路径名
HANDLE handle;
DWORD hID;
int rec = 0;
int namesize;

WSADATA wsadata;
SOCKET ssock;
// S程序的addr,R程序的addr
SOCKADDR_IN saddr,raddr;
int addlen = sizeof(raddr);

//分割d f
void Div_Dir_File(char* dfName)
{
	int len = strlen(dfName);
	int dlen = 0,flen = 0;
	for(dlen = len -1;dlen>0;dlen--,flen++)
	{
			if(dfName[dlen] == '\\' || dfName[dlen] == '/')break;
	}
	strncpy(dName,dfName,dlen+1);
	strncpy(fName,(dfName+dlen+1),flen);
}

//初始化 打包 校验
void ruc_init(ruc *r)
{
	memset(r->data,'\0',SIZEB);
	r->len = 0;
	r->seq = 0;
	r->CKS = 0;
}
void ruc_pack(int s,char d[])
{
	memset(FromSendPort->data,'\0',SIZEB);
	FromSendPort->len = (unsigned short)strlen(d);
	memcpy(FromSendPort->data,d,SIZEB);
	FromSendPort->seq = s;
	int sum=0;
	for(int i = 0;i< FromSendPort->len;i++)
	{
		sum += (int)d[i];
	}
	sum += (s+FromSendPort->len);
	FromSendPort->CKS = sum; 
}
int ruc_chek(ruc *r)
{
	if(!r->data)
	{
		printf("Packet is empty\n");
	}
	else
	{
		int sum = 0;
		for(int i =0;i<r->len;i++)
		{
			sum += (int)r->data[i];
		}
		sum = sum + r->len + r->seq - r->CKS;
		if(sum!=0)
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
	if(acStr[0] == '+' || acStr[0] == '-')
		iIndex = 1;
 
	for(iSize=iIndex; ; iSize++)
		if(acStr[iSize] < '0' || acStr[iSize] > '9')
			break;
 
	for(i=iIndex; i<iSize; i++)
		iNum += (int)pow(10, iSize - i - 1) * (acStr[i] - 48);
 
	if(acStr[0] == '-')
		iNum = - iNum;
 
	return iNum;
}
// 整型转字符串
void itos(int iInt, char* acStr)
{
	int iIndex = 0, iSize, iNum, iBit, i, j;
 
	if(iInt < 0)
	{
		acStr[0] = '-';
		iInt = - iInt;
		iIndex = 1;
	}
	for(i=0; ; i++)
		if(iInt < pow(10, i))
			break;
	iSize = i;
 
	for(i=0; i<iSize; i++)
	{
		iNum = pow(10, iSize - i - 1);
		iBit = iInt/iNum;
		iInt -= iNum*iBit;
		acStr[i + iIndex] = iBit + 48;
	}
	if(iSize != 0)
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
	if(WSAStartup(MAKEWORD(1, 1), &wsadata) == SOCKET_ERROR)
	{
		printf("启用 socket 失败\n");
		exit(0);
	}
	// 新建 socket
	if((ssock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
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
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
 
	raddr.sin_family = AF_INET;
	raddr.sin_port = htons(R_PORT);
	raddr.sin_addr.s_addr = inet_addr(ip);
 
	// 绑定端口，监听端口
	if(bind(ssock, (struct sockaddr*)&saddr, sizeof(saddr)) == -1)
	{
		printf("绑定端口失败\n");
		exit(0);
	}
}
void UDPclose(void)
{

	// Winsows 关闭 socket
	closesocket(ssock);
	WSACleanup();

}
 
DWORD WINAPI ruc_send(LPVOID p)
{
	int r = rec;
	int i = 0;
	while(1)
	{
		if(i>=RETIME)
		{
			printf("重传次数超过上限\n");
			exit(1);
		}
		sendto(ssock,FromSendPort,dgsize,0,(struct sockaddr*)&raddr,addlen);
		UDPsleep(10);
		for(int j=0;j<5;j++)
		{
			if(r<rec)
			{
				return 0;
			}
			UDPsleep(10);
		}
		printf("超时，再次尝试\n");
		i++;
	}
	return 0;
}
 
// 发送文件
void sendFile(char* dfName, char* ip)
{
	int fSize;
	int FromSendPortPart;
	int aSize = addlen;

	FromSendPort = (ruc *)malloc(dgsize);
	FromRecvPort = (ruc *)malloc(dgsize);
	ruc_init(FromSendPort);ruc_init(FromRecvPort);

	FILE *p = NULL;
	if(!(p = fopen(dfName,"rb")))
	{
		printf("打开文件失败，请检查是否输入了正确的路径文件名\n");
		exit(0);
	}
	fseek(p,0,SEEK_END);
	fSize = ftell(p);
	Div_Dir_File(dfName);

	printf("文件读取成功，开始发送文件名...\n");
	UDPawake(ip);
	ruc_pack(-2,fName);
	handle = CreateThread(NULL,NULL,ruc_send,0,0,&hID);

	for(;;)
	{
		if(recvfrom(ssock,FromRecvPort,dgsize,0,(struct sockaddr*)&raddr,&addlen))
		{
			rec++;
			printf("文件名已发送，开始发送文件大小...\n");
		}
		UDPsleep(10);
		if(!ruc_chek(FromRecvPort)&&!strcmp(FromRecvPort->data,"size"))
		{
			char FileSize[32];
			itos(fSize,FileSize);
			ruc_pack(-1,FileSize);
			handle = CreateThread(NULL,NULL,ruc_send,0,0,&hID);
			break;
		}	
	} 

	FromSendPortPart = fSize/(SIZEB);
	int currSeq = 0;
	int FromSendPortSeq = 0;
	for(;;)
	{
		if(recvfrom(ssock,FromRecvPort,dgsize,0,(struct sockaddr*)&raddr,&addlen))
		{
			if(!ruc_chek(FromRecvPort))
			{
				rec++;
				if(rec == 2)
				{
					printf("文件大小已发送，开始分段发送文件...\n");
				}
				memset(tosend,'\0',SIZEB);
				printf("正在发送文件:%s/%d...\n",FromRecvPort->data,FromSendPortPart);
				UDPsleep(10);
				currSeq = stoi(FromRecvPort->data);
				fseek(p,currSeq*SIZEB,SEEK_SET);
				fread(tosend,1,SIZEB,p);

				ruc_pack(currSeq,tosend);
			}
			handle = CreateThread(NULL,NULL,ruc_send,0,0,&hID);
			if(currSeq >=FromSendPortPart-1)
			{
				printf("成段文件已发送，开始发送剩余文件...\n");
				break;
			}
		}
	}

	int lSize = fSize%SIZEB;
	while(lSize>0)
	{
		if(recvfrom(ssock,FromRecvPort,dgsize,0,(struct sockaddr*)&raddr,&addlen))
		{
			UDPsleep(10);
		}
		if(!ruc_chek(FromRecvPort))
		{
			rec++;
			currSeq = FromRecvPort->seq;
			if(!strcmp(FromRecvPort->data,"over"))
			{
				printf("成功发送所有文件...\n\n"); break;
			}
			if(currSeq == FromSendPortPart)
			{
				memset(tosend,'\0',SIZEB);
				printf("正在发送文件:%s/%s...\n", FromRecvPort->data,FromRecvPort->data);
				fseek(p,currSeq*SIZEB,SEEK_SET);
				fread(tosend,1,lSize,p);
				
				ruc_pack(FromSendPortPart, tosend);
			}
			
			handle = CreateThread(NULL,NULL,ruc_send,0,0,&hID);
		}
	}
	fclose(p);
}
 

// int main(void)
// {
// 	//char dfName[SIZEP];
// 	//printf("本程序为发送端，请输入需要发送的文件名:\n");
// 	//scanf("%s", dfName);
// 	//printf("请输入接收端的IP地址:\n");
// 	//scanf("%s", rIP);
// 	clock_t start, end;
// 	char dfName[SIZEP] = "C:/Users/49393/Desktop/send/l5.zip";
// 	char rIP[15] = "10.130.86.110";
// 	start = clock();
// 	sendFile(dfName,rIP); 
// 	end = clock();
// 	printf("发送用时%f\n", (double)(end - start));
// 	system("pause");
// 	UDPclose();
// 	return 0;
// }

int main(void)
{
/*	char dfName[SIZEP];
	char rIP[15];
	printf("本程序为发送端，请输入需要发送的文件名:\n");
	scanf("%s", dfName);
	printf("请输入接收端的IP地址:\n");
	scanf("%s", rIP);*/
	clock_t start, end;
	char dfName[SIZEP] = "C:/Users/49393/Desktop/send/test2.png";
	char rIP[15] = "127.0.0.1";
	start = clock();
	sendFile(dfName,rIP); 
	end = clock();
	printf("发送用时%f\n", (double)(end - start));
	system("pause");
	UDPclose();
	return 0;
}