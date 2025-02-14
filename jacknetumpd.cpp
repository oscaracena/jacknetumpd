/*
 * File:   jacknetumpd.cpp
 * NetUMP daemon with JACK interface
 * Created mainly for the Zynthian Open Source Synthesizer
 *
 *
 * Copyright (c) 2025 Benoit BOUCHEZ / KissBox
 * License : MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

/*
 Command line options

--host <hostname>        Set remote destination host
--localport <port>       Set local port for Network UMP (5504 by default)
--remoteport <port>      Set destination port when Zynthian is session initiator
--endpoint-name <name>   Set local UMP Endpoint Name ("Zynthian NetUMP" by default)
--help                   Display this help message

 */

 /*
 Release notes
 V0.2 : 23/12/2023
 - update to use MIT license BEBSDK
 - updated NetUMP library for V0.7.6 specification

 V0.3 : 05/01/2024
  - added mDNS preliminary support for NAMM demonstration

 V1.0 : 06/12/2024
  - first public release after Network Protocol being adopted by MMA

  V1.1 : 27/12/2024
  - TXT record in mDNS updated to "UMPEndpointName" rather than "EndpointName"

  V1.2 : 29/12/2024
  - mDNS TTL set to 2 minutes (8 hours can be problematic for refreshing mDNS table on some devices)

  V1.3 : 07/01/2025
  - bug corrected in SYSEX handling from JACK to UMP : SYSEX longer than 6 bytes could hang the daemon (see comment in ump_transcoder.c)
  - bug corrected in NetUMPCallback() : last word for 128-bit UMP message was not transmitted (wrong index in array)

  V1.3.2 : 21/01/2025 (changes by oscaracena)
  - added callbacks for connection and disconnection events
  - added host/port/endpoint-name options to initiate a session with a remote peer

  V1.4 : 03/02/2025
  - all printf transformed to fprintf with adequate stream (stdout or stderr)
  - local port and destination port are now defined separately
  - code cleanup in UMP_mDNS
 */

#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <jack/jack.h>
#include <jack/midiport.h>
#include <jack/metadata.h>

#include "SystemSleep.h"
#include "NetUMP.h"
#include "UMP_Transcoder.h"
#include "Endpoint.h"
#include "UMP_mDNS.h"

jack_port_t *input_port;
static jack_port_t *output_port;
bool break_request=false;
unsigned int IntermDNSPacketCounter;

CNetUMPHandler* NetUMPHandler=NULL;
TUMP_FIFO UMP2JACK;

static unsigned int UMPSize [16] = {1, 1, 1, 2, 2, 4, 1, 1, 2, 2, 2, 3, 3, 4, 4, 4};

// Function called when the UMP engine receives a valid UMP message
void NetUMPCallback (void* UserInstance, uint32_t* DataBlock)
{
    unsigned int CurrentOutPtr;
    unsigned int TempInPtr;
    unsigned int MTSize;

    // Process Endpoint related UMP messages
    if ((DataBlock[0]&0xFFFF0000)==0xF0000000)
    {
        ProcessEndpointDiscovery(DataBlock[1]);
        return;     // Do not transmit this message to Jack
    }

    // Push UMP message into the queue to JACK
    CurrentOutPtr=UMP2JACK.ReadPtr;
    TempInPtr=UMP2JACK.WritePtr;

    MTSize = UMPSize[DataBlock[0]>>28];

    // Store first UMP word in all cases
    UMP2JACK.FIFO[TempInPtr]=DataBlock[0];
    TempInPtr+=1;
    if (TempInPtr>=UMP_FIFO_SIZE)
        TempInPtr=0;
    if (TempInPtr==CurrentOutPtr)
        return;     // FIFO is full

    // Store second word
    if (MTSize>=2)
    {
        UMP2JACK.FIFO[TempInPtr]=DataBlock[1];
        TempInPtr+=1;
        if (TempInPtr>=UMP_FIFO_SIZE)
            TempInPtr=0;
        if (TempInPtr==CurrentOutPtr)
            return;     // FIFO is full

        if (MTSize>=3)
        {
            UMP2JACK.FIFO[TempInPtr]=DataBlock[2];
            TempInPtr+=1;
            if (TempInPtr>=UMP_FIFO_SIZE)
                TempInPtr=0;
            if (TempInPtr==CurrentOutPtr)
                return;     // FIFO is full

            if (MTSize==4)
            {
                UMP2JACK.FIFO[TempInPtr]=DataBlock[3];
                TempInPtr+=1;
                if (TempInPtr>=UMP_FIFO_SIZE)
                    TempInPtr=0;
                if (TempInPtr==CurrentOutPtr)
                    return;     // FIFO is full
            }
        }
    }

    // Update the pointer only when all UMP words have been stored
    UMP2JACK.WritePtr=TempInPtr;
}  // NetUMPCallback
//-----------------------------------------------------------------------------

// Callback function called when there is an audio block to process
int jack_process(jack_nframes_t nframes, void *arg)
{
    unsigned int i;
    void* in_port_buf = jack_port_get_buffer(input_port, nframes);
    void* out_port_buf = jack_port_get_buffer(output_port, nframes);
    jack_midi_event_t in_event;
    jack_nframes_t event_count;
    jack_midi_data_t* Buffer;
    unsigned int TempRead, LastBufferPos;
    size_t NumBytesInEvent;
    uint32_t UMPMsg[4];
    uint8_t MIDIMsg[8];
    unsigned int MTSize;
    unsigned int MIDI1Size;

    jack_midi_clear_buffer(out_port_buf);    // Recommended to call this at the beginning of process cycle

    // Check if we have UMP data waiting in the FIFO from NetUMP to be sent to JACK
    if (UMP2JACK.ReadPtr!=UMP2JACK.WritePtr)
    {
        // Read FIFO and generate JACK events for each MIDI message in the FIFO
        TempRead=UMP2JACK.ReadPtr;     // Local snapshot to avoid UMP thread to see pointer moving while we parse the buffer
        LastBufferPos=UMP2JACK.WritePtr;

        while (TempRead!=LastBufferPos)
        {
            // Identify message length from first word
            UMPMsg[0]=UMP2JACK.FIFO[TempRead];
            TempRead+=1;
            if (TempRead>=UMP_FIFO_SIZE)
                TempRead=0;

            MTSize = UMPSize[UMPMsg[0]>>28];
            // Read other UMP words if necessary
            if (MTSize>=2)
            {
                UMPMsg[1]=UMP2JACK.FIFO[TempRead];
                TempRead+=1;
                if (TempRead>=UMP_FIFO_SIZE)
                    TempRead=0;

                if (MTSize>=3)
                {
                    UMPMsg[2]=UMP2JACK.FIFO[TempRead];
                    TempRead+=1;
                    if (TempRead>=UMP_FIFO_SIZE)
                        TempRead=0;

                    if (MTSize==4)
                    {
                        UMPMsg[3]=UMP2JACK.FIFO[TempRead];
                        TempRead+=1;
                        if (TempRead>=UMP_FIFO_SIZE)
                            TempRead=0;
                    }
                }
            }

            MIDI1Size = TranscodeUMP_MIDI1 (&UMPMsg[0], &MIDIMsg[0]);
            if (MIDI1Size>0)        // UMP message has been transcoded successfully into MIDI1.0
            {
                Buffer=jack_midi_event_reserve (out_port_buf, 0, MIDI1Size);
                if (Buffer!=0)
                {
                    memcpy(Buffer, &MIDIMsg[0], MIDI1Size);
                }
            }

            // TODO : For now, we do not convert SYSEX as we don't see real interest for the Zynthian
        }  // loop over all events in the queue

        // Update read pointer only when we have parsed the whole buffer
        UMP2JACK.ReadPtr=TempRead;
    }  // MIDI data available from NetUMP queue

    // Generate NetUMP payload for each event sent by JACK
    if (in_port_buf)
        event_count = jack_midi_get_event_count(in_port_buf);
    else
        event_count = 0;

    if(event_count >= 1)
    {
        //printf("jackrtpumpd: %d events\n", event_count);

        for(i=0; i<event_count; i++)
        {
            jack_midi_event_get(&in_event, in_port_buf, i);
            NumBytesInEvent=in_event.size;

            if (TranscodeMIDI1_UMP (&in_event.buffer[0], NumBytesInEvent, &UMPMsg[0]))
            {
                if (NetUMPHandler)
                {
                    NetUMPHandler->SendUMPMessage(&UMPMsg[0]);
                }
            }
        }
    }

    return 0;
}  // jack_process
// ----------------------------------------------------

/* Callback function called when jack server is shut down */
void jack_shutdown(void *arg)
{
    printf ("JACK has shut down\n");
    break_request=true;
}  // jack_shutdown
// ----------------------------------------------------

void sig_handler (int signo)
{
    if (signo == SIGINT)
    {
        break_request=true;
    }
}  // sig_handler
// ----------------------------------------------------

int main(int argc, char** argv)
{
    int Ret;
    static jack_client_t *client;
    char *destHost = 0;
    char *LocalEndpointName = "Zynthian NetUMP";
    unsigned int LocalPort = 5504;
    unsigned int RemotePort = 5504;

    fprintf (stdout, "JACK <-> Network UMP bridge V1.4 for Zynthian\n");
    fprintf (stdout, "Copyright 2024/2025 Benoit BOUCHEZ (BEB)\n");
    fprintf (stdout, "Please report any issue to BEB on discourse.zynthian.org\n");

    break_request=false;
    signal (SIGINT, sig_handler);

    UMP2JACK.ReadPtr=0;
    UMP2JACK.WritePtr=0;

    // Parse command line arguments
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--host") == 0 && i + 1 < argc)
        {
            destHost = argv[i + 1];
            i++;
        }
        else if (strcmp(argv[i], "--localport") == 0 && i + 1 < argc)
        {
            LocalPort = atoi(argv[i + 1]);
            i++;
        }
        else if (strcmp(argv[i], "--remoteport") == 0 && i + 1 < argc)
        {
            RemotePort = atoi(argv[i + 1]);
            i++;
        }
        else if (strcmp(argv[i], "--endpoint-name") == 0 && i + 1 < argc)
        {
            LocalEndpointName = argv[i + 1];
            i++;
        }
        else if (strcmp(argv[i], "--help") == 0)
        {
            fprintf(stdout, "Usage: %s [options]\n", argv[0]);
            fprintf(stdout, "Options:\n");
            fprintf(stdout, "  --host <hostname>        Set remote destination host\n");
            fprintf(stdout, "  --localport <port>       Set Network UMP local port\n");
            fprintf(stdout, "  --remoteport <port>      Set Network UMP port on remote host\n");
            fprintf(stdout, "  --endpoint-name <name>   Set local UMP Endpoint Name\n");
            fprintf(stdout, "  --help                   Display this help message\n");
            return 0;
        }
        else
        {
            fprintf(stderr, "Unknown option: %s.\n", argv[i]);
            fprintf(stderr, "Use --help for usage information.\n");
            return -1;
        }
    }

    initUMP_mDNS();

    if ((client = jack_client_open ("jacknetumpd", JackNullOption, NULL)) == 0)
    {
        fprintf(stderr, "jacknetumpd : JACK server is not running\n");
        return -1;
    }

    NetUMPHandler = new CNetUMPHandler (&NetUMPCallback, 0);
    if (NetUMPHandler)
    {
        NetUMPHandler->SetEndpointName(LocalEndpointName);
        NetUMPHandler->SetProductInstanceID((char*)"ZV5_001");      // TODO : this should be random

        NetUMPHandler->SetConnectionCallback([](const char* EndpointName, unsigned int size)
        {
            fprintf (stdout, "jacknetumpd : connected to '%s'.\n", EndpointName);
            jack_uuid_t output_port_uuid = jack_port_uuid(output_port);
            jack_set_property(client, output_port_uuid, "UMPEndpointName", EndpointName, "text/plain");
        });

        NetUMPHandler->SetDisconnectCallback([]()
        {
            fprintf (stdout, "jacknetumpd : disconnected\n");
            jack_uuid_t output_port_uuid = jack_port_uuid(output_port);
            jack_remove_property(client, output_port_uuid, "UMPEndpointName");
        });

        if (destHost)
        {
            fprintf(stdout, "jacknetumpd : connecting to peer '%s:%d'...\n", destHost, RemotePort);
            struct hostent *host_entry;
            host_entry = gethostbyname(destHost);
            if (host_entry == NULL)
            {
                fprintf(stderr, "jacknetumpd : could not resolve hostname: %s\n", destHost);
                delete NetUMPHandler;
                return 1;
            }

            char *ip = inet_ntoa(*((struct in_addr*) host_entry->h_addr_list[0]));
            fprintf(stdout, "jacknetumpd : resolved hostname '%s' to IP address %s\n", destHost, ip);
            unsigned int destIP = ntohl(inet_addr(ip));
            Ret = NetUMPHandler->InitiateSession(destIP, RemotePort, LocalPort, true);
        }
        else
        {
            fprintf(stdout, "jacknetumpd : waiting for connection on port %d...\n", LocalPort);
            Ret = NetUMPHandler->InitiateSession (0, 0, LocalPort, false);
        }

        // Report if problem arises when session is activated
        if (Ret<0)
        {
            fprintf (stderr, "jacknetumpd : can not create session\n");
            delete NetUMPHandler;
            return -1;
        }
    }  // NetUMPHandler created
    else
    {
        fprintf (stderr, "jacknetumpd : can not create NetworkUMP handler! Aborting...\n");
        return -1;
    }

    // Register the various callbacks needed by a JACK application
    jack_set_process_callback (client, jack_process, 0);
    jack_on_shutdown (client, jack_shutdown, 0);

    input_port = jack_port_register (client, "netump_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
    output_port = jack_port_register (client, "netump_out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);

    if (jack_activate (client))
    {
        fprintf(stderr, "jacknetumpd : cannot activate client");
        return 1;
    }

    /* run until interrupted */
    while(break_request==false)
    {
        if (NetUMPHandler)
            NetUMPHandler->RunSession();

        // Send UMP mDNS packet every 5 seconds
        IntermDNSPacketCounter++;
        if (IntermDNSPacketCounter>=5000)
        {
            IntermDNSPacketCounter = 0;
            SendUMPmDNS();
        }
        SystemSleepMillis(1);        // Run NetUMP process every millisecond
    }
    fprintf (stdout, "Program termination requested by user\n");

    // Clean everything before we exit
    jack_client_close(client);
    if (NetUMPHandler)
    {
        fprintf (stdout, "Closing NetUMP handler...\n");
        NetUMPHandler->CloseSession();
        delete NetUMPHandler;
        NetUMPHandler=0;
    }

    TerminatemDNS();

    fprintf (stdout, "Done...\n");

    return (EXIT_SUCCESS);
}  // main
// ----------------------------------------------------
