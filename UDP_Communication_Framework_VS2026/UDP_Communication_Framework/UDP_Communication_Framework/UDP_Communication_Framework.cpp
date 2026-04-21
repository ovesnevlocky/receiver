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

#define TARGET_IP	"127.0.0.1"
//#define TARGET_IP "10.1.6.72"

#define BUFFERS_LEN 1024


enum
{
	SYNC,
	DATA,
	ACK,
	LACK,
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
	u32 md5;

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

	char* res = "monk.jpeg";
	FILE* fp = fopen(res, "wb");
	if (!fp)
	{
		printf("error\n");
		exit(1);
	}

	printf("Waiting for datagram ...\n");
	
	int receivedPacketLength = 0;

	u32 posExpected = 0;
	
	u32 crc32;
	
	while ((receivedPacketLength = recvfrom(socketS, (char*)&dataReceived, sizeof(dataReceived), 0, (sockaddr*)&addrDst, &addrDstlen)) > 0)
	{

		if (receivedPacketLength == SOCKET_ERROR)
		{
			printf("Socket error!\n");
			getchar();
			return 1;
		}
		if (dataReceived.packet_type == STOP)
		{
			printf("end the communication\n");
			break;
		}
		if (dataReceived.packet_type == SYNC)
		{
			printf("start the communication\n");
		}


		if (posExpected != dataReceived.pos)
		{
			
			printf("expected: %i, got %i\n", posExpected, dataReceived.pos);
			memset(&dataToSend, 0, sizeof(dataToSend));
			dataToSend.pos = posExpected;
			dataToSend.packet_type = ACK;
			sendto(socketS, (char*)&dataToSend, sizeof(dataToSend), 0, (sockaddr*)&addrDst, sizeof(addrDst));
		}

	
		if (dataReceived.packet_type == DATA)
		{

			//specify the position in the file to keep the communication safe 
			fseek(fp, dataReceived.pos, SEEK_SET);
			int check = fwrite(dataReceived.payload, sizeof(u8), dataReceived.packet_len, fp);
			if (check != dataReceived.packet_len)
			{
				printf("got :%i  expected:%lu\n", check, dataReceived.packet_len);
				printf("error in writing to a file\n");
				return 1;
			}

			crc32 = CRC_CalculateCRC32(dataReceived.payload, dataReceived.packet_len);
			//printf("0x%X : 0x%X\n", dataReceived.crc32, crc32);
			if (crc32 != dataReceived.crc32)
			{
				printf("error! 0x%X : 0x%X\n", dataReceived.crc32, crc32);
			}
			posExpected += dataReceived.packet_len;
			//set ack 
			dataToSend.pos = posExpected;
			dataToSend.packet_type = ACK;
		
			sendto(socketS, (char*)&dataToSend, sizeof(dataToSend), 0, (sockaddr*)&addrDst, sizeof(addrDst));

		}

	}

	posExpected += dataReceived.packet_len;

	printf("total size is %lu\n", posExpected);

	char* resInput = (char*)calloc(posExpected + 1, 1);
	if (!resInput)
		printf("error in allocation!\n");

	u8 resData[16] = { 0 };
	
	fclose(fp);

	FILE* fpCheckMd5 = fopen(res, "rb");
	if (!fpCheckMd5)
		printf("error!\n");

	u32 ret = fread(resInput, sizeof(u8), posExpected, fpCheckMd5);
	
	if (ret != posExpected)
		printf("error in reading file! %lu, %lu\n", ret, posExpected);

	resInput[ret] = '\0';

	md5String(resInput, resData);
	print_hash(resData);

	printf("result was saved to %s\n", res);
	
	closesocket(socketS);
	free(resInput);
	
	fclose(fpCheckMd5);

	getchar(); //wait for press Enter
	return 0;

}
