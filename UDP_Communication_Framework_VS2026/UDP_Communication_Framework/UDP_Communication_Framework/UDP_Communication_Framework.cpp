// UDP_Communication_Framework.cpp : Defines the entry point for the console application.
//

#pragma comment(lib, "ws2_32.lib")
#include "stdafx.h"
#include <winsock2.h>
#include "ws2tcpip.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "md5.h"
#include "crc32.h"
#include <stdbool.h>

#define TARGET_IP	"127.0.0.1"
//#define TARGET_IP "10.1.6.72"

#define BUFFERS_LEN 1024
#define RECV_TIMEOUT_MS 3000

enum
{
	SYNC,
	DATA,
	ACK,
	NAK,
	MASK,
	STOP
};

typedef uint8_t u8;
typedef uint32_t u32;

struct PacketStruct
{
	//u32 packet_type;
	u8  packet_type;
	u32 packet_len;
	u32 crc32;
	//buff for data
	u32 pos;
	//u32 md5;
	u8 md5[16];

	u8 payload[BUFFERS_LEN];

}packet;


#define RECEIVER

#ifdef RECEIVER
#define TARGET_PORT 5222
#define LOCAL_PORT 5111
#endif // RECEIVER


void InitWinsock()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
}

void prepare(struct sockaddr_in* local, struct sockaddr_in* addrDst)
{
	local->sin_family = AF_INET;
	local->sin_port = htons(LOCAL_PORT);
	local->sin_addr.s_addr = INADDR_ANY;

	addrDst->sin_family = AF_INET;
	addrDst->sin_port = htons(TARGET_PORT);
	InetPton(AF_INET, _T(TARGET_IP), &(addrDst->sin_addr.s_addr));

}

bool isTimeout()
{
	int err = WSAGetLastError();
	return err == WSAETIMEDOUT;
}


int main()
{
	SOCKET socketS;

	InitWinsock();

	struct sockaddr_in local;
	struct sockaddr_in addrDst;

	struct PacketStruct dataReceived = { 0 };
	struct PacketStruct dataToSend = { 0 };
	int addrDstlen = sizeof(addrDst);


	prepare(&local, &addrDst);
	socketS = socket(AF_INET, SOCK_DGRAM, 0);
	if (bind(socketS, (sockaddr*)&local, sizeof(local)) != 0)
	{
		printf("Binding error!\n");
		getchar(); //wait for press Enter
		return 1;
	}

	char* res = "res.jpeg";
	FILE* fp = fopen(res, "wb");
	if (!fp)
	{
		printf("error\n");
		exit(1);
	}

	printf("Waiting for datagram ...\n");

	int receivedPacketLength = 0;

	u32 posExpected = 0;
	int count = 0;
	u32 crc32;

	//while ((receivedPacketLength = recvfrom(socketS, (char*)&dataReceived, sizeof(dataReceived), 0, (sockaddr*)&addrDst, &addrDstlen)) > 0)
	while (1)
	{
		receivedPacketLength = recvfrom(socketS, (char*)&dataReceived, sizeof(dataReceived), 0, (sockaddr*)&addrDst, &addrDstlen);
		if (receivedPacketLength <= 0)
			break;
		

		
		if (receivedPacketLength == SOCKET_ERROR)
		{
			printf("Socket error!\n");
			getchar();
			return 1;
		}

		if (dataReceived.packet_type == SYNC)
		{
			printf("start the communication\n");
			dataToSend.packet_type = SYNC;
			sendto(socketS, (char*)&dataToSend, sizeof(dataToSend), 0, (sockaddr*)&addrDst, sizeof(addrDst));
		}


		if (dataReceived.packet_type == STOP)
		{
			printf("end the communication\n");
			break;
		}

		if (dataReceived.packet_type == DATA)
		{

			//specify the position in the file to keep the communication safe 
			crc32 = CRC_CalculateCRC32(dataReceived.payload, dataReceived.packet_len);
			
			if (crc32 != dataReceived.crc32 || dataReceived.pos != posExpected)
			{
				printf("error! 0x%X : 0x%X\n", dataReceived.crc32, crc32);
				dataToSend.packet_type = NAK;
				dataToSend.pos = posExpected;
				sendto(socketS, (char*)&dataToSend, sizeof(dataToSend), 0, (sockaddr*)&addrDst, sizeof(addrDst));
				continue;
			}

			fseek(fp, dataReceived.pos, SEEK_SET);
			int check = fwrite(dataReceived.payload, sizeof(u8), dataReceived.packet_len, fp);
			if (check != dataReceived.packet_len)
			{
				printf("got :%i  expected:%lu\n", check, dataReceived.packet_len);
				printf("error in writing to a file\n");
				return 1;
			}

			posExpected += dataReceived.packet_len;
			//set ack and  set posExpected to specify the next data//
			dataToSend.pos = posExpected;
			dataToSend.packet_type = ACK;
			//
			sendto(socketS, (char*)&dataToSend, sizeof(dataToSend), 0, (sockaddr*)&addrDst, sizeof(addrDst));

		}

	}

	posExpected += dataReceived.packet_len;

	printf("total size is %lu\n", posExpected);

	char* resInput = (char*)calloc(posExpected + 1, 1);
	if (!resInput)
		printf("error in allocation!\n");

	u8 resData[16] = { 0 };
	//close to read from this file
	fclose(fp);

	FILE* fpCheckMd5 = fopen(res, "rb");
	if (!fpCheckMd5)
		printf("error  in opening the res file before getting md5!\n");

	u32 ret = fread(resInput, sizeof(u8), posExpected, fpCheckMd5);

	if (ret != posExpected)
		printf("error in reading file! %lu, %lu\n", ret, posExpected);

	resInput[ret] = '\0';
	u8 md5[16] = { 0 };

	printf("size is: %lu\n", ret);
	md5String(resInput, md5);

	//print_hash(dataReceived.md5);
	if (memcmp(md5, dataReceived.md5, 16) != 0)
	{
		printf("error in md5 !\n");
		print_hash(dataReceived.md5);
		print_hash(md5);
	}
	else
		printf("md5 corresponded\n");

	printf("result was saved to %s\n", res);

	closesocket(socketS);
	free(resInput);

	fclose(fpCheckMd5);

	getchar(); //wait for press Enter
	return 0;

}