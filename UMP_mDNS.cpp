/*
 * UMP_mDNS.c
 *
 *  Created on: 2 avr. 2023
 *      Author: Benoit
 */

#include <string.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <errno.h>
#include "network.h"

typedef struct {
    unsigned short TransactionID;
    unsigned short Flags;
    unsigned short Questions;
    unsigned short AnswerRRs;
    unsigned short AuthorityRRs;
    unsigned short AdditionalRRs;
} TMDNS_Header;

static char MIDI2ProtocolName [] = "_midi2";
static char UDPProtocolName [] = "_udp";
static char LocalDomainName [] = "local";
static char TargetName [] = "Zynthian V5";
static char EndpointName [] = "Zynthian V5";
#define PRODUCT_INSTANCE_ID_LEN     17
static char ProductInstanceID [PRODUCT_INSTANCE_ID_LEN+1] = "ZYV5_000000000000";

static char ProductInstanceIdTagStr [] = "ProductInstanceId=";
#define PRODUCT_INSTANCEID_TAG_LEN 18       // Length of ProductInstanceIdStr

static unsigned int mDNSPacketLen;
static unsigned char mDNSPacket_ [512];

static TSOCKTYPE mDNSSocket = INVALID_SOCKET;

//! Transforms hex digit into ASCII
static unsigned char hex2asc (unsigned char hex)
{
    if (hex<0x0A) return 0x30+hex;      // Digit 0..9
    else return 55+hex;                 // Letter A..F
}  // hex2asc
// -------------------------------------------------------------

void initUMP_mDNS(void)
{
    unsigned char* mDNSPacket;
    char ProductName[64] = "UMPEndpointName=";
    char* ProductInstanceIDPtr;
    struct ifreq ifr{};
    uint8_t mac_address [6];

    CreateUDPSocket (&mDNSSocket, 0, false);

    // Generate product instance ID from MAC Address of ETH0
    strcpy (ifr.ifr_name, "eth0");
    ioctl (mDNSSocket, SIOCGIFHWADDR, &ifr);
    memcpy (mac_address, ifr.ifr_hwaddr.sa_data, 6);

    ProductInstanceID[5] = hex2asc(mac_address[0]>>4);
    ProductInstanceID[6] = hex2asc(mac_address[0]&0x0F);
    ProductInstanceID[7] = hex2asc(mac_address[1]>>4);
    ProductInstanceID[8] = hex2asc(mac_address[1]&0x0F);
    ProductInstanceID[9] = hex2asc(mac_address[2]>>4);
    ProductInstanceID[10] = hex2asc(mac_address[2]&0x0F);
    ProductInstanceID[11] = hex2asc(mac_address[3]>>4);
    ProductInstanceID[12] = hex2asc(mac_address[3]&0x0F);
    ProductInstanceID[13] = hex2asc(mac_address[4]>>4);
    ProductInstanceID[14] = hex2asc(mac_address[4]&0x0F);
    ProductInstanceID[15] = hex2asc(mac_address[5]>>4);
    ProductInstanceID[16] = hex2asc(mac_address[5]&0x0F);

    mDNSPacket = &mDNSPacket_[0];
    strcat (&ProductName[0], &EndpointName[0]);
    ProductInstanceIDPtr=(char*)&ProductInstanceID[0];

    TMDNS_Header* Header = (TMDNS_Header*)mDNSPacket;

    // Compute size of the various strings
    unsigned int MIDI2ProtocolNameLen = strlen (&MIDI2ProtocolName[0]);
    unsigned int UDPProtocolNameLen = strlen (&UDPProtocolName[0]);
    unsigned int LocalDomainNameLen = strlen (&LocalDomainName[0]);
    //unsigned int KBServiceNameLen = strlen (&KBServiceName[0]);
    unsigned int TargetNameLen = strlen (&TargetName[0]);
    unsigned int ProductNameLen = strlen (&ProductName[0]);
    unsigned int BufferPos;

    Header->TransactionID = 0;
    Header->Flags = htons (0x8400);
    Header->Questions = 0;
    Header->AnswerRRs = htons(4);    // Change depending on what follows
    Header->AuthorityRRs = 0;
    Header->AdditionalRRs = htons (0);       // Change depending on what follows

    // Add the AnswerRR (from byte 12)
    // Build the Name
    BufferPos = 13;
    mDNSPacket[12] = MIDI2ProtocolNameLen;
    memcpy (&mDNSPacket[BufferPos], &MIDI2ProtocolName[0], MIDI2ProtocolNameLen);
    BufferPos+=MIDI2ProtocolNameLen;
    mDNSPacket[BufferPos++] = UDPProtocolNameLen;
    memcpy (&mDNSPacket[BufferPos], &UDPProtocolName[0], UDPProtocolNameLen);
    BufferPos+=UDPProtocolNameLen;
    mDNSPacket[BufferPos++] = LocalDomainNameLen;
    memcpy (&mDNSPacket[BufferPos], &LocalDomainName[0], LocalDomainNameLen);
    BufferPos+=LocalDomainNameLen;
    mDNSPacket[BufferPos++] = 0;        // NULL terminator after the string

    mDNSPacket [BufferPos++] = 0x00;
    mDNSPacket [BufferPos++] = 0x0C;        // 0x000C = Type PTR
    mDNSPacket [BufferPos++] = 0x00;
    mDNSPacket [BufferPos++] = 0x01;        // 0x0001 = Class IN
    mDNSPacket [BufferPos++] = 0x00;
    mDNSPacket [BufferPos++] = 0x00;
    mDNSPacket [BufferPos++] = 0x70;
    mDNSPacket [BufferPos++] = 0x80;        // TTL = 0x00007080 (8 hours)
    mDNSPacket [BufferPos++] = 0x00;        // Length MSB
    mDNSPacket [BufferPos++] = PRODUCT_INSTANCE_ID_LEN+MIDI2ProtocolNameLen+UDPProtocolNameLen+LocalDomainNameLen+4+1;   // Add length byte + NULL terminator

    mDNSPacket [BufferPos++] = PRODUCT_INSTANCE_ID_LEN;
    memcpy (&mDNSPacket[BufferPos], ProductInstanceIDPtr, PRODUCT_INSTANCE_ID_LEN);
    BufferPos+=PRODUCT_INSTANCE_ID_LEN;
    mDNSPacket[BufferPos++] = MIDI2ProtocolNameLen;
    memcpy (&mDNSPacket[BufferPos], &MIDI2ProtocolName[0], MIDI2ProtocolNameLen);
    BufferPos+=MIDI2ProtocolNameLen;
    mDNSPacket[BufferPos++] = UDPProtocolNameLen;
    memcpy (&mDNSPacket[BufferPos], &UDPProtocolName[0], UDPProtocolNameLen);
    BufferPos+=UDPProtocolNameLen;
    mDNSPacket[BufferPos++] = LocalDomainNameLen;
    memcpy (&mDNSPacket[BufferPos], &LocalDomainName[0], LocalDomainNameLen);
    BufferPos+=LocalDomainNameLen;
    mDNSPacket[BufferPos++] = 0;        // NULL terminator after the string

    // Add the Additional Records
    // Add the SRV block
    mDNSPacket [BufferPos++] = PRODUCT_INSTANCE_ID_LEN;
    memcpy (&mDNSPacket[BufferPos], ProductInstanceIDPtr, PRODUCT_INSTANCE_ID_LEN);
    BufferPos+=PRODUCT_INSTANCE_ID_LEN;
    mDNSPacket[BufferPos++] = MIDI2ProtocolNameLen;
    memcpy (&mDNSPacket[BufferPos], &MIDI2ProtocolName[0], MIDI2ProtocolNameLen);
    BufferPos+=MIDI2ProtocolNameLen;
    mDNSPacket[BufferPos++] = UDPProtocolNameLen;
    memcpy (&mDNSPacket[BufferPos], &UDPProtocolName[0], UDPProtocolNameLen);
    BufferPos+=UDPProtocolNameLen;
    mDNSPacket[BufferPos++] = LocalDomainNameLen;
    memcpy (&mDNSPacket[BufferPos], &LocalDomainName[0], LocalDomainNameLen);
    BufferPos+=LocalDomainNameLen;
    mDNSPacket[BufferPos++] = 0;        // NULL terminator after the string

    mDNSPacket [BufferPos++] = 0x00;
    mDNSPacket [BufferPos++] = 0x21;        // 0x000C = Type SRV
    mDNSPacket [BufferPos++] = 0x00;
    mDNSPacket [BufferPos++] = 0x01;        // 0x0001 = Class IN
    mDNSPacket [BufferPos++] = 0x00;
    mDNSPacket [BufferPos++] = 0x00;
    mDNSPacket [BufferPos++] = 0x00;
    mDNSPacket [BufferPos++] = 0x78;        // TTL = 120 seconds
    mDNSPacket [BufferPos++] = 0x00;
    mDNSPacket [BufferPos++] = 6+TargetNameLen+2;        // Data length
    mDNSPacket [BufferPos++] = 0x00;
    mDNSPacket [BufferPos++] = 0x00;        // Priority
    mDNSPacket [BufferPos++] = 0x00;
    mDNSPacket [BufferPos++] = 0x00;        // Weight
    mDNSPacket [BufferPos++] = 0x15;
    mDNSPacket [BufferPos++] = 0x80;        // Port 5504
    mDNSPacket [BufferPos++] = TargetNameLen;
    memcpy (&mDNSPacket[BufferPos], &TargetName[0], TargetNameLen);
    BufferPos+=TargetNameLen;
    mDNSPacket[BufferPos++] = 0;     // NULL terminator after the string

    // Add the TXT block
    // Name is built in the same way as for SRV
    mDNSPacket [BufferPos++] = PRODUCT_INSTANCE_ID_LEN;
    memcpy (&mDNSPacket[BufferPos], ProductInstanceIDPtr, PRODUCT_INSTANCE_ID_LEN);
    BufferPos+=PRODUCT_INSTANCE_ID_LEN;
    mDNSPacket[BufferPos++] = MIDI2ProtocolNameLen;
    memcpy (&mDNSPacket[BufferPos], &MIDI2ProtocolName[0], MIDI2ProtocolNameLen);
    BufferPos+=MIDI2ProtocolNameLen;
    mDNSPacket[BufferPos++] = UDPProtocolNameLen;
    memcpy (&mDNSPacket[BufferPos], &UDPProtocolName[0], UDPProtocolNameLen);
    BufferPos+=UDPProtocolNameLen;
    mDNSPacket[BufferPos++] = LocalDomainNameLen;
    memcpy (&mDNSPacket[BufferPos], &LocalDomainName[0], LocalDomainNameLen);
    BufferPos+=LocalDomainNameLen;
    mDNSPacket[BufferPos++] = 0;        // NULL terminator after the string

    mDNSPacket [BufferPos++] = 0x00;
    mDNSPacket [BufferPos++] = 0x10;        // 0x0010 = Type TXT
    mDNSPacket [BufferPos++] = 0x00;
    mDNSPacket [BufferPos++] = 0x01;        // 0x0001 = Class IN
    mDNSPacket [BufferPos++] = 0x00;
    mDNSPacket [BufferPos++] = 0x00;
    mDNSPacket [BufferPos++] = 0x11;
    mDNSPacket [BufferPos++] = 0x94;        // TTL = 4500 seconds
    mDNSPacket [BufferPos++] = 0x00;        // Length MSB
	
	/*
    mDNSPacket [BufferPos++] = ProductNameLen+1;        // Data length
    mDNSPacket [BufferPos++] = ProductNameLen;
    memcpy (&mDNSPacket[BufferPos], &ProductName[0], ProductNameLen);
    BufferPos+=ProductNameLen;
	*/
	
	mDNSPacket [BufferPos++] = ProductNameLen+1+PRODUCT_INSTANCE_ID_LEN+1+PRODUCT_INSTANCEID_TAG_LEN;        // Data length (18 = size of "ProductInstanceID=" string)
    mDNSPacket [BufferPos++] = ProductNameLen;      // First string
    memcpy (&mDNSPacket[BufferPos], &ProductName[0], ProductNameLen);
    BufferPos+=ProductNameLen;

    mDNSPacket [BufferPos++] = PRODUCT_INSTANCE_ID_LEN+PRODUCT_INSTANCEID_TAG_LEN;      // Second string
	/*
    mDNSPacket [BufferPos++] = PRODUCT_INSTANCE_ID_LEN+18;      // Second string
    mDNSPacket [BufferPos++] = 'P';
    mDNSPacket [BufferPos++] = 'r';
    mDNSPacket [BufferPos++] = 'o';
    mDNSPacket [BufferPos++] = 'd';
    mDNSPacket [BufferPos++] = 'u';
    mDNSPacket [BufferPos++] = 'c';
    mDNSPacket [BufferPos++] = 't';
    mDNSPacket [BufferPos++] = 'I';
    mDNSPacket [BufferPos++] = 'n';
    mDNSPacket [BufferPos++] = 's';
    mDNSPacket [BufferPos++] = 't';
    mDNSPacket [BufferPos++] = 'a';
    mDNSPacket [BufferPos++] = 'n';
    mDNSPacket [BufferPos++] = 'c';
    mDNSPacket [BufferPos++] = 'e';
    mDNSPacket [BufferPos++] = 'I';
    mDNSPacket [BufferPos++] = 'd';
    mDNSPacket [BufferPos++] = '=';
	*/
	
    memcpy (&mDNSPacket[BufferPos], &ProductInstanceIdTagStr, PRODUCT_INSTANCEID_TAG_LEN);
    BufferPos+=PRODUCT_INSTANCEID_TAG_LEN;	
    memcpy (&mDNSPacket[BufferPos], ProductInstanceIDPtr, PRODUCT_INSTANCE_ID_LEN);
    BufferPos+=PRODUCT_INSTANCE_ID_LEN;

    // Add the A block (IP v4 address)
    mDNSPacket [BufferPos++] = TargetNameLen;
    memcpy (&mDNSPacket[BufferPos], &TargetName[0], TargetNameLen);
    BufferPos+=TargetNameLen;
    mDNSPacket [BufferPos++] = 0x00;    // Add NULL terminator
    mDNSPacket [BufferPos++] = 0x00;
    mDNSPacket [BufferPos++] = 0x01;        // 0x0010 = Type A
    mDNSPacket [BufferPos++] = 0x00;
    mDNSPacket [BufferPos++] = 0x01;        // 0x0001 = Class IN
    mDNSPacket [BufferPos++] = 0x00;
    mDNSPacket [BufferPos++] = 0x00;
    mDNSPacket [BufferPos++] = 0x00;
    mDNSPacket [BufferPos++] = 0x78;        // TTL = 120 seconds
    mDNSPacket [BufferPos++] = 0x00;
    mDNSPacket [BufferPos++] = 0x04;        // Length = 4

    // Get system IP address for eth0
    strcpy (ifr.ifr_name, "eth0");
    ifr.ifr_addr.sa_family = AF_INET;       // We want IPv4 address
    ioctl (mDNSSocket, SIOCGIFADDR, &ifr);

    // Add IP address
    struct in_addr in = ((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr;
    uint32_t IPV4Addr = htonl(in.s_addr);

    mDNSPacket [BufferPos++] = (IPV4Addr>>24)&0xFF;
    mDNSPacket [BufferPos++] = (IPV4Addr>>16)&0xFF;
    mDNSPacket [BufferPos++] = (IPV4Addr>>8)&0xFF;
    mDNSPacket [BufferPos++] = IPV4Addr&0xFF;

    mDNSPacketLen = BufferPos;
}  // initUMP_MDNS
// -------------------------------------------------------------

void TerminatemDNS (void)
{
    if (mDNSSocket!=INVALID_SOCKET)
    {
        CloseSocket(&mDNSSocket);
        mDNSSocket = INVALID_SOCKET;
    }
}  // TerminatemDNS
// -------------------------------------------------------------

void SendUMPmDNS (void)
{
	sockaddr_in AdrEmit;

    if (mDNSSocket==INVALID_SOCKET) return;

    memset (&AdrEmit, 0, sizeof(sockaddr_in));
	AdrEmit.sin_family=AF_INET;
	AdrEmit.sin_addr.s_addr=htonl(0xE00000FB);
	//AdrEmit.sin_addr.s_addr=htonl(0xC0A80028);
	AdrEmit.sin_port=htons(5353);
	sendto(mDNSSocket, (const char*)&mDNSPacket_, mDNSPacketLen, 0, (const sockaddr*)&AdrEmit, sizeof(sockaddr_in));
}  //SendUMPmDNS
// -------------------------------------------------------------
