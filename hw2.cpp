// hw2.cpp : Defines the entry point for the console application.
//
/*
hw 2: Native DNS Resolver
Name: Venkata Satya Kavya Sree Bondapalli
UIN: 725006670
Course: CSCE 612 Networks and Distributed Processing
Semester: Spring 2018
Instructor: Dmitri Loguinov
*/
/*
REFERENCES
https://stackoverflow.com/questions/14420924/aligning-output-values-in-c
*/

/*TO-DO
//what about in ip reversing
//nslookup -querytype=ptr 12.138.194.128.in-addr.arpa
*/
#include "stdafx.h"
#include "string.h"
#include <iostream>
#include <winsock2.h>
#include "macros.h"
#include <stdlib.h>
#include <time.h>

#define SUCCESS 1
#define FAIL 0
using namespace std;

#pragma pack(push,1) //sets struct padding/alignment to 1 byte
class QueryHeader {
public:
	USHORT qType;
	USHORT qClass;
};
class FixedDNSheader {
public:
	USHORT ID;
	USHORT flags;
	USHORT nQuestions;
	USHORT nAnswers;
	USHORT nAuthority;
	USHORT nAdditional;
};
#pragma pack(pop) //restores old packing
#pragma pack(push,1)
class FixedRR {
public:
	u_short qType;
	u_short qClass;
	u_short TTL1;
	u_short TTL2;
	u_short length;
	//...
};
#pragma pack(pop)
void printArray(char * ptr,int len,int yes);
int DecideQueryType(char* str);
int QueryConstructor(char* host, int type, char* dnsServerIP);
string ReversedIP(char*ip);
int iResult = 0;
char* recv_fixed_header_ptr;
int main(int argc, char** argv)
{
	//cout << argc << endl;
	if (argc != 3)
	{
		printf("incorrect number of arguments\n");
		return 0;
	}
	 
	char lookupString[LOOKUP_STR_MAX_LENGTH];
	char dnsServerIP[IP_STR_MAX_LENGTH];
	memcpy(lookupString, argv[1], strlen(argv[1]) + 1);
	lookupString[strlen(argv[1])] = '\0';
	
	memcpy(dnsServerIP, argv[2], strlen(argv[2]) + 1);
	dnsServerIP[strlen(argv[2])] = '\0';
	string queryString;
	printf("%-12s%s%s\n", "Lookup",": ", lookupString);
	char* ress = NULL;
	if (DecideQueryType(lookupString) == HOST_TYPE)
	{
		int res = QueryConstructor(lookupString, 1, dnsServerIP);
	}
	else if (DecideQueryType(lookupString) == IP_TYPE) {
		queryString = ReversedIP(lookupString);
		char q[100];
		memcpy(q, queryString.c_str(), strlen(queryString.c_str()));
		q[strlen(queryString.c_str())] = '\0';
		int res = QueryConstructor(q, 12, dnsServerIP);
	}
	WSACleanup();
	return 0;
}
void makeDNSquestion(char* lol, char *host) 
{
	//cout << "buf "<< lol << endl;
	char* buf = lol + sizeof(FixedDNSheader);
	const char dot = '.';
	char* res;
	char* temp;
	temp = host;
	res = strchr(temp, dot);

	//char buf[256 * 10];
	int strSize = 0;
	//cout << "HERE HELLO" << buf << endl;
	int i = 0;
	do
	{
		char a[256];
		memcpy(a, temp, strlen(temp) - strlen(res));
		a[strlen(temp) - strlen(res)] = '\0';
		strSize = strlen(a);
		buf[i++] = strSize;
		memcpy(buf + i, a, strlen(a));
		i = i + strSize;
		res = res + 1;
		temp = res;
		res = strchr(temp, dot);
		if (res == NULL)
		{
			char* b = temp;
			strSize = strlen(temp);
			buf[i++] = strSize;
			memcpy(buf + i, temp, strlen(temp));
			i = i + strSize;
			buf[i] = 0;
			break;
		}
	} while (strlen(res) != 0);
	i++;
	//buf[i] = 0; // last word NULL-terminated
}
char* jumpFunction(char* recvBuf, char* currptr)
{

	if (currptr[0] == 0)
	{
		//cout << "CASE 0" << endl;
		//printArray(currptr);
		//currptr = currptr + 1;
		return currptr+1;
	}
		
	if ((unsigned char)currptr[0] >= 0xc0)
	{
		//cout << "CASE 1" << endl;
		//printArray(currptr);
		//cout << "COMPRESSED HENCE JUMP TO ";
		int off = (((unsigned char)currptr[0] & 0x3f) << 8) + (unsigned char)currptr[1];
		//printf("\n{%d}\n", currptr - recvBuf);
		//printf("%x %x\n", currptr[0], currptr[1]);
		//printf("%d \n", recvBuf + off - recv_fixed_header_ptr);
		//printf("offset is %d\n", off);
		if (recvBuf + off - recv_fixed_header_ptr > 0 && recvBuf + off - recv_fixed_header_ptr < 12)
		{
			printf("++\tinvalid record: jump into fixed header\n");
			WSACleanup();
			exit(EXIT_FAILURE);
		}
		if (currptr+1 - recvBuf > iResult-1)
		{
			printf("++\tinvalid record: truncated jump offset\n");
			WSACleanup();
			exit(EXIT_FAILURE);
		}
		if (off > iResult)
		{
			printf("++\tinvalid record: jump beyond packet boundary\n");
			WSACleanup();
			exit(EXIT_FAILURE);
		}
		//cout << off << endl;
		//FixedHeader *rand = (FixedHeader*)(recvBuf + off);
		char *tee = recvBuf + off;
		if ((unsigned char)tee[0] >= 0xc0)
		{
			printf("++\tinvalid record: jump loop\n");
			WSACleanup();
			exit(EXIT_FAILURE);
		}
		//cout << "\t\t here" << endl;
		printArray(recvBuf + off, 10,1);
		printArray(recv_fixed_header_ptr, 10, 1);
		//cout << "\t\t hereeee" << endl;
		//cout << "HHHH" << endl;
		char* ren = jumpFunction(recvBuf, recvBuf + off);
		//currptr = currptr + 2;
		
		return currptr+2;
	}
	else
	{
		//cout << "CASE 2" << endl;
		//printArray(currptr);
		int size = currptr[0];
		if (size == 0)
		{
			//currptr = currptr + 1; //break;
			return 0;
		}
		currptr = currptr + 1;
		

		char temp[100];
		memcpy(temp, currptr, size+1);
		
		if (currptr + size - recvBuf>=iResult)
		{
			temp[currptr + size - recvBuf - iResult] = 0;
			printf("%s", temp);
			printf(" ++\tinvalid record: truncated name\n");
			WSACleanup();
			exit(EXIT_FAILURE);
		}
		temp[size] = '\0';
		printf("%s", temp);
		currptr = currptr + size;
		if (currptr[0] != 0)
			printf(".");
		else
			printf(" ");
		currptr=jumpFunction(recvBuf, currptr);
		return currptr;
	}
}
void printArray(char * ptr,int len,int yes)
{
#if 0
	cout << "\t\t\tHERE" << endl;
	for (int jjj = 0; jjj<len; jjj++)
		printf("\t\tj %d c %c x %02x\n", jjj, (unsigned char)ptr[jjj], ptr[jjj]);
	cout << endl;
#endif
}
int QueryConstructor(char* host, int type, char* dnsServerIP)
{
	int pkt_size = strlen(host) + 2 + sizeof(FixedDNSheader) + sizeof(QueryHeader);
	char *buf = new char[pkt_size];
	FixedDNSheader *fdh = (FixedDNSheader*)buf;
	QueryHeader *qh = (QueryHeader*)(buf + pkt_size - sizeof(QueryHeader));
	fdh->ID = htons(1);
	fdh->flags = htons(DNS_QUERY | DNS_RD | DNS_STDQUERY);// 0x10;
	fdh->nQuestions = htons(1);
	fdh->nAnswers = htons(0);
	fdh->nAuthority = htons(0);
	fdh->nAdditional = htons(0);
	if (type == 1)
		qh->qType = htons(DNS_A);
	else if (type == 12)
		qh->qType = htons(DNS_PTR);//10A 11B 12C
	qh->qClass = htons(DNS_INET);
	makeDNSquestion(buf, host);

	printf("%-12s%s%s%s%d%s%04x\n", "Query", ": ", host, ", type ", type, ", TXID 0x", fdh->ID);
	printf("%-12s%s%s\n", "Server", ": ", dnsServerIP);

	cout << "********************************" << endl;
#define MAX_ATTEMPTS 3
	WSADATA wsaData;
	//Initialize WinSock; once per program run
	WORD wVersionRequested = MAKEWORD(2, 2);
	if (WSAStartup(wVersionRequested, &wsaData) != 0) {
		printf("WSAStartup error %d\n", WSAGetLastError());
		delete buf;
		return FAIL;
	}
	int count = 0;
	while (count++ < MAX_ATTEMPTS)
	{
		printf("Attempt %d with %d bytes... ", count-1, pkt_size);
		SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
		//handle errors
		if (sock == INVALID_SOCKET)
		{
			printf("socket() generated error %d\n", WSAGetLastError());
			delete buf;
			return FAIL;
		}
		struct sockaddr_in local;
		memset(&local, 0, sizeof(local));
		local.sin_family = AF_INET;
		local.sin_addr.s_addr = INADDR_ANY;
		local.sin_port = htons(0);
		if (bind(sock, (struct sockaddr*)&local, sizeof(local)) == SOCKET_ERROR)
		{
			printf("socket() binding error %d\n", WSAGetLastError());
			delete buf;
			return FAIL;
		}
		
		struct sockaddr_in remote;
		memset(&remote, 0, sizeof(remote));
		remote.sin_family = AF_INET;
		remote.sin_addr.s_addr = inet_addr(dnsServerIP);
		remote.sin_port = htons(53);
	
		clock_t start, end;
		double cpu_time_used;
		char * res;
	
		if (sendto(sock, buf, pkt_size, 0, (struct sockaddr*)&remote, sizeof(remote)) == SOCKET_ERROR)
		{
			printf("socket() send to error %d\n", WSAGetLastError());
			delete buf;
			return FAIL;
		}
		
		char recvBuf[MAX_DNS_SIZE];
		struct sockaddr_in response;
		int size = sizeof(response);
		
		fd_set fd;
		FD_ZERO(&fd);
		FD_SET(sock, &fd);
		timeval tp;
		tp.tv_sec = 10;
		tp.tv_usec = 0;
		start = clock();
		int available = select(0, &fd, NULL, NULL, &tp);
		iResult = 0;
		if (available > 0)
		{
			iResult = recvfrom(sock, recvBuf, MAX_DNS_SIZE, 0, (struct sockaddr*)&response, &size);
			if (iResult == SOCKET_ERROR)
			{
				//error processing
				printf(" socket error %d\n", WSAGetLastError());
				delete buf;
				return FAIL;
			}
			end = clock();
			cpu_time_used = 1000 * ((double)(end - start)) / CLOCKS_PER_SEC;//in ms
			printf("response in %d ms with %d bytes\n", int(cpu_time_used), iResult);
			if (response.sin_addr.s_addr != remote.sin_addr.s_addr || response.sin_port != remote.sin_port)
			{
				printf(" bogus reply\n");
				delete buf;
				return FAIL;
			}
			res = strstr(recvBuf, buf);
			//printf("start = %d", res - recvBuf);
			recv_fixed_header_ptr = res;
			printArray(recvBuf,iResult,0);
			printArray(buf,iResult,0);
			if (res == NULL)
			{
				printf("fixed DNS Header not found\n");
				delete buf;
				return FAIL;
			}
			FixedDNSheader *recv_fdh = (FixedDNSheader*)res;
			if (recv_fdh == NULL)
			{
				printf("Error while getting the recv fixed header\n");
				delete buf;
				return FAIL;
			}
			if (iResult < sizeof(FixedDNSheader))
			{
				printf("++\tinvalid reply: smaller than fixed header\n");
				delete buf;
				return FAIL;
			}
			printf("\tTXID 0x%04x", htons(recv_fdh->ID));
			printf(" flags 0x%04x", htons(recv_fdh->flags));
			printf(" questions %d", (int)htons(recv_fdh->nQuestions));
			printf(" answers %d", (int)htons(recv_fdh->nAnswers));
			printf(" authority %d", (int)htons(recv_fdh->nAuthority));
			printf(" additional %d\n", (int)htons(recv_fdh->nAdditional));

			if (htons(recv_fdh->ID) != htons(fdh->ID))
			{
				printf("\t++ invalid reply: TXID mismatch, sent 0x%04x, received 0x%04x\n", htons(fdh->ID), htons(recv_fdh->ID));
				delete buf;
				return FAIL;
			}
			int code = htons(recv_fdh->flags) & 0x000f;
			if (code == 0)
				printf("\tsucceeded with Rcode = %d\n", code);
			else
			{
				printf("\tfailed with Rcode = %d\n", code);
				delete buf;
				return FAIL;
			}

			res = res + sizeof(FixedDNSheader);
			if (htons(recv_fdh->nQuestions) != 0)
				printf("\t---------- [questions] ----------\n");
			for (int j = 0; j < htons(recv_fdh->nQuestions); j++)
			{
				printf("\t\t");
				//print the question name
				int first = 0;
				while (true)
				{
					int size = res[0];
					if (size == 0)
					{
						res = res + 1; break;
					}
					if (first == 1)
						printf(".");
					res = res + 1;
					char temp[MAX_DNS_SIZE];
					memcpy(temp, res, size);
					temp[size] = '\0';
					printf("%s", temp);
					res = res + size;
					first = 1;
				}
				printf(" ");
				//print the qclass and qtype
				QueryHeader* temp_qh = (QueryHeader*)res;
				printf("type %d class %d\n", htons(temp_qh->qType), htons(temp_qh->qClass));
				res = res + sizeof(QueryHeader);
			}
			
			if (htons(recv_fdh->nAnswers) != 0)
				printf("\t---------- [answers] ----------\n");
			for (int jj = 0; jj < htons(recv_fdh->nAnswers); jj++)
			{
				//printf("%d", res - recvBuf);
				if (res - recvBuf >= iResult)
				{
					printf("++\tinvalid section: not enough records\n");
					delete buf;
					return FAIL;
				}
					

				//cout << "ANS COUNT " << jj << endl;
				//cout << "\tb4 jump printed" << endl;
				//printArray(res,10);
				printf("\t\t");
				res = jumpFunction(recvBuf, res);
				//cout << "\t\tjumped and Qname printed b4 rr header taken" << endl;
				printArray(res,10,0);
				if (res + sizeof(FixedRR) - recvBuf>iResult)
				{
					printf("++\tinvalid record: truncated fixed RR header\n");
					delete buf;
					return FAIL;
				}
				FixedRR* frr = (FixedRR*)(res);
				res = res + sizeof(FixedRR);
				
				//cout << "\tAf RR header taken" << endl;
				printArray(res,10,0);
				if ((int)htons(frr->qType) == DNS_A)
				{
					printf("A ");
					if (res + (int)htons(frr->length) - recvBuf > iResult)
					{
						printf("\t ++ invalid record: RR value length beyond packet\n");
						delete buf;
						return FAIL;
					}
					//cout <<"len "<< (int)htons(frr->length) << endl;
					printf("%d.", 16 * (unsigned char(res[0]) >> 4) + (unsigned char(res[0]) & 0x0f));
					printf("%d.", 16 * (unsigned char(res[1]) >> 4) + (unsigned char(res[1]) & 0x0f));
					printf("%d.", 16 * (unsigned char(res[2]) >> 4) + (unsigned char(res[2]) & 0x0f));
					printf("%d ", 16 * (unsigned char(res[3]) >> 4) + (unsigned char(res[3]) & 0x0f));
					printf(" TTL = %d\n", (16 * 16 * (int)htons(frr->TTL1)) + ((int)htons(frr->TTL2)));
					res = res + 4;
					continue;
				}
				else if ((int)htons(frr->qType) == DNS_PTR || 
					(int)htons(frr->qType) == DNS_NS ||
					(int)htons(frr->qType) == DNS_CNAME)
				{
					if ((int)htons(frr->qType) == DNS_PTR)
						printf("PTR "); 
					if((int)htons(frr->qType) == DNS_NS)
						printf("NS ");
					if ((int)htons(frr->qType) == DNS_CNAME)
						printf("CNAME ");
					if (res + (int)htons(frr->length) - recvBuf > iResult)
					{
						printf("\t ++ invalid record: RR value length beyond packet\n");
						delete buf;
						return FAIL;
					}
					//cout << "b4 printing value" << endl;
					printArray(res,10,0);
					res = jumpFunction(recvBuf, res);
					//cout << "af printing value" << endl;
					printArray(res,10,0);
					printf(" ");
					printf(" TTL = %d\n", (16 * 16 * (int)htons(frr->TTL1)) + ((int)htons(frr->TTL2)));
					continue;
				}
				else
				{
					continue;
				}
			}
#if 1
			if (htons(recv_fdh->nAuthority) != 0)
				printf("\t---------- [authority] ----------\n");
			for (int jj = 0; jj < htons(recv_fdh->nAuthority); jj++)
			{
				//printf("%d", res - recvBuf);
				if (res - recvBuf >= iResult)
				{
					printf("++\tinvalid section: not enough records\n");
					delete buf;
					return FAIL;
				}//out << "ANS COUNT " << jj << endl;
				//cout << "\tb4 jump printed" << endl;
				//printArray(res,10);
				printf("\t\t");
				res = jumpFunction(recvBuf, res);
				//cout << "\t\tjumped and Qname printed b4 rr header taken" << endl;
				printArray(res,10,0);
				if (res+sizeof(FixedRR)-recvBuf>iResult)
				{
					printf("++\tinvalid record: truncated fixed RR header\n");
					delete buf;
					return FAIL;
				}
				FixedRR* frr = (FixedRR*)(res);
				res = res + sizeof(FixedRR);
				
				//cout << "\tAf RR header taken" << endl;
				printArray(res,10,0);
				if ((int)htons(frr->qType) == DNS_A)
				{
					printf("A ");
					if (res + (int)htons(frr->length) - recvBuf > iResult)
					{
						printf("\t ++ invalid record: RR value length beyond packet\n");
						delete buf;
						return FAIL;
					}
					//cout <<"len "<< (int)htons(frr->length) << endl;
					printf("%d.", 16 * (unsigned char(res[0]) >> 4) + (unsigned char(res[0]) & 0x0f));
					printf("%d.", 16 * (unsigned char(res[1]) >> 4) + (unsigned char(res[1]) & 0x0f));
					printf("%d.", 16 * (unsigned char(res[2]) >> 4) + (unsigned char(res[2]) & 0x0f));
					printf("%d ", 16 * (unsigned char(res[3]) >> 4) + (unsigned char(res[3]) & 0x0f));
					printf(" TTL = %d\n", (16 * 16 * (int)htons(frr->TTL1)) + ((int)htons(frr->TTL2)));
					res = res + 4;
					continue;
				}
				else if ((int)htons(frr->qType) == DNS_PTR ||
					(int)htons(frr->qType) == DNS_NS ||
					(int)htons(frr->qType) == DNS_CNAME)
				{
					if ((int)htons(frr->qType) == DNS_PTR)
						printf("PTR ");
					if ((int)htons(frr->qType) == DNS_NS)
						printf("NS ");
					if ((int)htons(frr->qType) == DNS_CNAME)
						printf("CNAME ");
					if (res + (int)htons(frr->length) - recvBuf > iResult)
					{
						printf("\t ++ invalid record: RR value length beyond packet\n");
						delete buf;
						return FAIL;
					}
					//cout << "b4 printing value" << endl;
					//printArray(res,10);
					res = jumpFunction(recvBuf, res);
					//cout << "af printing value" << endl;
					//printArray(res,10);
					printf(" ");
					printf(" TTL = %d\n", (16 * 16 * (int)htons(frr->TTL1)) + ((int)htons(frr->TTL2)));
					continue;
				}
				else
				{
					continue;
				}
			}
			
			if (htons(recv_fdh->nAdditional) != 0)
				printf("\t---------- [additional] ----------\n");
			for (int jj = 0; jj < htons(recv_fdh->nAdditional); jj++)
			{
				//printf("%d", res - recvBuf);
				if (res - recvBuf >= iResult)
				{
					printf("++\tinvalid section: not enough records\n");
					delete buf;
					return FAIL;
				}
				//cout << "ANS COUNT " << jj << endl;
				//cout << "\tb4 jump printed" << endl;
				//printArray(res,10);
				printf("\t\t");
				res = jumpFunction(recvBuf, res);
				//cout << "\t\tjumped and Qname printed b4 rr header taken" << endl;
				//printArray(res,10);
				if (res + sizeof(FixedRR) - recvBuf>iResult)
				{
					printf("++\tinvalid record: truncated fixed RR header\n");
					delete buf;
					return FAIL;
				}
				FixedRR* frr = (FixedRR*)(res);
				res = res + sizeof(FixedRR);
				
				//cout << "\tAf RR header taken" << endl;
				//printArray(res,10);
				if ((int)htons(frr->qType) == DNS_A)
				{
					printf("A ");
					if (res + (int)htons(frr->length) - recvBuf > iResult)
					{
						printf("\t ++ invalid record: RR value length beyond packet\n");
						delete buf;
						return FAIL;
					}
					//cout <<"len "<< (int)htons(frr->length) << endl;
					printf("%d.", 16 * (unsigned char(res[0]) >> 4) + (unsigned char(res[0]) & 0x0f));
					printf("%d.", 16 * (unsigned char(res[1]) >> 4) + (unsigned char(res[1]) & 0x0f));
					printf("%d.", 16 * (unsigned char(res[2]) >> 4) + (unsigned char(res[2]) & 0x0f));
					printf("%d ", 16 * (unsigned char(res[3]) >> 4) + (unsigned char(res[3]) & 0x0f));
					printf(" TTL = %d\n", (16 * 16 * (int)htons(frr->TTL1)) + ((int)htons(frr->TTL2)));
					res = res + 4;
					continue;
				}
				else if ((int)htons(frr->qType) == DNS_PTR ||
					(int)htons(frr->qType) == DNS_NS ||
					(int)htons(frr->qType) == DNS_CNAME)
				{
					if ((int)htons(frr->qType) == DNS_PTR)
						printf("PTR ");
					if ((int)htons(frr->qType) == DNS_NS)
						printf("NS ");
					if ((int)htons(frr->qType) == DNS_CNAME)
						printf("CNAME ");
					if (res + (int)htons(frr->length) - recvBuf > iResult)
					{
						printf("\t ++ invalid record: RR value length beyond packet\n");
						delete buf;
						return FAIL;
					}
					//cout << "b4 printing value" << endl;
					//printArray(res,10);
					res = jumpFunction(recvBuf, res);
					//cout << "af printing value" << endl;
					//printArray(res,10);
					printf(" ");
					printf(" TTL = %d\n", (16 * 16 * (int)htons(frr->TTL1)) + ((int)htons(frr->TTL2)));
					continue;
				}
				else
				{
					continue;
				}
			}
#endif
			delete buf;
			return SUCCESS;
		}
		else if (available < 0)
		{
			printf("failed with %d on recv\n", WSAGetLastError());
			delete buf;
			return FAIL;
		}
		end = clock();
		cpu_time_used = 1000 * ((double)(end - start)) / CLOCKS_PER_SEC;//in ms
		printf("timeout in %d ms\n", int(cpu_time_used));
		continue;
	}
	delete buf;
	return FAIL;
}
int DecideQueryType(char* str)
{
	unsigned long res = inet_addr(str);
	if (res == INADDR_NONE)
	{
		return HOST_TYPE;
	}
	return IP_TYPE;
}
string ReversedIP(char*ip)
{
	const char dot = '.';
	char* res;
	char* temp;
	
	res = strchr(ip, dot);
	char a[4];
	memcpy(a, ip, strlen(ip) - strlen(res));
	a[strlen(ip) - strlen(res)] = '\0';
	//cout << "a " << a << endl;
	res = res + 1;

	temp = strchr(res, dot);
	char b[4];
	memcpy(b, res, strlen(res) - strlen(temp));
	b[strlen(res) - strlen(temp)] = '\0';
	//cout << "b " << b << endl;
	temp = temp + 1;
	
	char* temp1;
	temp1 = strchr(temp, dot);
	char c[4];
	//cout << strlen(temp) <<" "<< strlen(temp1) << endl;

	memcpy(c, temp, strlen(temp) - strlen(temp1));
	c[strlen(temp) - strlen(temp1)] = '\0';
	//cout << "c " << c << endl;
	temp1 = temp1 + 1;
	//cout << "d " << temp1 << endl;

	string ret = string(temp1) + "." + string(c) + "." + string(b) + "." + string(a) + ".in-addr.arpa";
	//cout << "ret " << ret.c_str() << endl;

	return ret;
}