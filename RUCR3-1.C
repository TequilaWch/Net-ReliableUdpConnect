//起码能用版本 3-1 by WCH 
//此程序为接收端
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<math.h>
#include<time.h>
#include<winsock2.h>
#include<windows.h>
#pragma comment(lib, "ws2_32.lib")
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
ruc *FromSendPort,*FromRecvPort;
int dgsize = sizeof(*FromSendPort);
char tosend[SIZEB];//准备发送
char fName[SIZEP];//文件名
char dfName[SIZEP*2];//文件路径名
HANDLE handle;
DWORD hID;
int rec = 0;

WSADATA wsadata;
SOCKET rsock;
// S程序的addr,R程序的addr
SOCKADDR_IN saddr,raddr;
int addlen = sizeof(raddr);

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
	memset(FromRecvPort->data,'\0',SIZEB);
	FromRecvPort->len = (unsigned short)strlen(d);
	memcpy(FromRecvPort->data,d,SIZEB);
	
	FromRecvPort->seq = s;
	int sum=0;
	for(int i = 0;i< FromRecvPort->len;i++)
	{
		sum += (int)d[i];
	}
	sum += (s+FromRecvPort->len);
	FromRecvPort->CKS = sum; 
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
		for(int i =0;i< r->len;i++)
		{
			sum += (int)r->data[i];
		}
		sum = sum + r->len + r->seq - r->CKS;
		if(sum != 0)
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
	if((rsock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
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
	if(bind(rsock, (struct sockaddr*)&raddr, sizeof(raddr)) == -1)
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
	while(1)
	{
		if(i>=RETIME)
		{
			printf("重传次数超过上限\n");
			exit(1);
		}
		sendto(rsock,FromRecvPort,dgsize,0,(struct sockaddr*)&saddr,addlen);

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
 
// 接收文件
void recvFile(char* dName, char* ip)
{
	int FromRecvPortPart;
	int fSize;
	FILE *p = NULL;

	FromSendPort = (ruc *)malloc(dgsize);
	FromRecvPort = (ruc *)malloc(dgsize);
	ruc_init(FromSendPort);ruc_init(FromRecvPort);

	UDPawake(ip);
	int rept;

	printf("准备接收文件名...\n");
	for(rept=0;rept<5;rept++)
	{
		if(recvfrom(rsock,FromSendPort,dgsize,0,(struct sockaddr*)&saddr,&addlen)<0)
		{
			printf("连接中断...\n\n");
			exit(1);
		}
		if(!ruc_chek(FromSendPort)&&FromSendPort->seq == -2)
		{
			strcpy(fName,FromSendPort->data);
			ruc_init(FromSendPort);
			printf("接收到文件名:%s...\n\n",fName);
			strcpy(tosend,"size");
			ruc_pack(-1,tosend);
			handle = CreateThread(NULL,NULL,ruc_send,0,0,&hID);
			rept=0;
			break;
		}
	}

	printf("准备接收文件大小...\n");
	for(rept=0;rept<5;rept++)
	{
		memset(tosend,'\0',SIZEB);
		if(recvfrom(rsock, FromSendPort, dgsize, 0, (struct sockaddr*) & saddr, &addlen) >= 0) 
		{
			if(!ruc_chek(FromSendPort))
			{
				printf("接收到文件大小:%s...\n\n",FromSendPort->data);
				fSize = stoi(FromSendPort->data);
				ruc_init(FromSendPort);
				rept = 0;
				break;
			}
		}	
	}

	printf("准备接收文件...\n\n");
	strcat(dfName,dName);
	strcat(dfName,fName);
	p = fopen(dfName,"w");
	fclose(p);
	p = fopen(dfName,"ab");
	FromRecvPortPart = fSize / SIZEB;
	int FromRecvPortlen=0;
	int i;
	for(i=0;i<FromRecvPortPart;i++)
	{
		for(rept=0;rept<5;rept++)
		{
			memset(tosend,'\0',SIZEB);
			itos(i,tosend);
			ruc_pack(i,tosend);
			handle = CreateThread(NULL,NULL,ruc_send,0,0,&hID);

			printf("正在接收文件:%d/%d...\n",i,FromRecvPortPart);
			UDPsleep(10);
			FromRecvPortlen = recvfrom(rsock,FromSendPort,dgsize,0,(struct sockaddr*) & saddr, &addlen);
			/*if(FromSendPort->seq != i)
			{
				printf("报文序号错误，收到序号为%d,预期为%d\n",FromSendPort->seq, i);
			}
			*/
			if(!ruc_chek(FromSendPort))
			{
				if (i == FromRecvPortPart)break;
				rec++;
				printf("成功接收文件:%d/%d...\n\n",i,FromRecvPortPart);
				fwrite(FromSendPort->data,sizeof(char),SIZEB,p);
				rept=0;
				break;
			}
		}
	}
	printf("准备接收剩余文件...\n");
	int lSize = fSize%SIZEB;
	while(lSize>0)
	{
		memset(tosend,'\0',SIZEB);
		itos(FromRecvPortPart,tosend);
		ruc_pack(FromRecvPortPart,tosend);
		handle = CreateThread(NULL,NULL,ruc_send,0,0,&hID);
		printf("正在接收剩余文件...\n");
		UDPsleep(10);
		int rc = recvfrom(rsock, FromSendPort, dgsize, 0, (struct sockaddr*) & saddr, &addlen);	
		if(!ruc_chek(FromSendPort))
		{
			fwrite(FromSendPort->data,sizeof(char),lSize,p);
			memset(tosend,'\0',SIZEB);
			strcpy(tosend, "over");
			ruc_pack(FromRecvPortPart,tosend);
			handle = CreateThread(NULL,NULL,ruc_send,0,0,&hID);
			UDPsleep(10);
			printf("成功接收剩余文件...\n\n");
			rept=0;
			break;
		}		
	}
	rec++;
	UDPsleep(10);
	fclose(p);
}

// int main(void)
// {
// 	clock_t start, end;
// 	char dName[SIZEP]= "C:/Users/49393/Desktop/recv/";
// 	//printf("本程序为接收端，请输入需要保存的路径名:\n");
// 	//scanf("%s", dName);
// 	//printf("请输入发送端的IP地址:\n");
// 	//scanf("%s", sIP);
// 	char sIP[15] = "10.130.86.110";
// 	start = clock();
// 	recvFile(dName,sIP);
// 	end = clock();
// 	printf("接收用时%f\n", (double)(end - start));
// 	system("pause");
// 	UDPclose();
// 	return 0;
// }
int main(void)
{
	clock_t start, end;
	char dName[SIZEP]= "C:/Users/49393/Desktop/recv/";
	//char dName[SIZEP];
	//printf("本程序为接收端，请输入需要保存的路径名:\n");
	//scanf("%s", dName);
	//printf("请输入发送端的IP地址:\n");
	//char sIP[15];
	//scanf("%s", sIP);
	char sIP[15] = "127.0.0.1";
	start = clock();
	recvFile(dName,sIP);
	end = clock();
	printf("接收用时%f\n", (double)(end - start));
	system("pause");
	UDPclose();
	return 0;
}