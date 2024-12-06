#include "Endpoint.h"
#include "NetUMP.h"

extern CNetUMPHandler* NetUMPHandler;

void ProcessEndpointDiscovery (uint8_t Filter)
{
    uint32_t UMPReply[4];

    if (Filter&0x01)
    {  // e bit set : request Endpoint Info notification
        UMPReply[0]=0xF0010101;     // Endpoint Info notification, V=1.1
        UMPReply[1]=0x80000100;     // Static Function Blocks, no Function Blocks, support : MIDI 1.0, don't support : MIDI 2.0, transmit JR, receive JR
        UMPReply[2]=0x00000000;     // Reserved
        UMPReply[3]=0x00000000;     // Reserved

        NetUMPHandler->SendUMPMessage(&UMPReply[0]);
    }

    if (Filter&0x02)
    {  // d bit set : request Device Identity notification
        UMPReply[0] = 0xF0020000;       // Device Identity notification
        UMPReply[1] = 0x007D0000;       // Prototype ID code
        UMPReply[2] = 0x00000100;       // Device family, Device family model : to be defined
        UMPReply[3] = 0x00010000;		// V0.1 		TODO : should come from a global definition

        NetUMPHandler->SendUMPMessage(&UMPReply[0]);	   
    }

    if (Filter&0x04)
    {  // n bit set : request Endpoint Name notification
        UMPReply[0]=0xF0035A79;     // Endpoint Name, Zy
        UMPReply[1]=0x6E746869;     // nthi
        UMPReply[2]=0x616E2056;     // an V
        UMPReply[3]=0x35000000;     // 5

        NetUMPHandler->SendUMPMessage(&UMPReply[0]);
    }

    if (Filter&0x08)
    {  // i bit set : request Product Instance ID notification
        // TODO : use MAC address or random number (as Zynthian does not have a serial number)
        UMPReply[0]=0xF0045A59;     // Product Instance ID Name, ZY
        UMPReply[1]=0x30303030;     // 0000
        UMPReply[2]=0x30303031;     // 0001
        UMPReply[3]=0x00000000;

        NetUMPHandler->SendUMPMessage(&UMPReply[0]);
    }

    if (Filter&0x10)
    {  // s bit set : request Stream Configuration notification
        UMPReply[0]=0xF0060100;     // Stream Configuration Notification, MIDI 1.0 protocol, no TX/RX jitter reduction
        UMPReply[1]=0x00000000;     // Reserved
        UMPReply[2]=0x00000000;     // Reserved
        UMPReply[3]=0x00000000;     // Reserved

        NetUMPHandler->SendUMPMessage(&UMPReply[0]);
    }
}  // ProcessEndpointDiscovery
//-----------------------------------------------------------------------------
