/*
 * FILE: rdt_sender.cc
 * DESCRIPTION: Reliable data transfer sender.
 * NOTE: This implementation assumes there is no packet loss, corruption, or 
 *       reordering.  You will need to enhance it to deal with all these 
 *       situations.  In this implementation, the packet format is laid out as 
 *       the following:
 *       
 *       |<-  1 byte  ->|<-       1 byte       ->|<-  3 byte  ->|<-             the rest            ->|
 *       | payload size |<- payload seq number ->|<- checksum ->|<-             payload             ->|
 *
 *       The first byte of each packet indicates the size of the payload
 *       (excluding this single-byte header)
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rdt_struct.h"
#include "rdt_sender.h"


void fill_packet(packet &pkt, int payload_size, int header_size, const void *data) {
    pkt.data[0] = payload_size;
    memcpy(pkt.data + header_size, data, payload_size);
    int checksum = 0;
    for(int i = header_size;i < pkt.data[0] + header_size;++ i) {
        checksum += pkt.data[i];
    }
    checksum %= 1000;
    pkt.data[2] = checksum / 100;
    pkt.data[3] = checksum % 100 / 10;
    pkt.data[4] = checksum % 10;
}

/* sender initialization, called once at the very beginning */
void Sender_Init()
{
    fprintf(stdout, "At %.2fs: sender initializing ...\n", GetSimulationTime());
}

/* sender finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to take this opportunity to release some 
   memory you allocated in Sender_init(). */
void Sender_Final()
{
    fprintf(stdout, "At %.2fs: sender finalizing ...\n", GetSimulationTime());
}

/* event handler, called when a message is passed from the upper layer at the 
   sender */
void Sender_FromUpperLayer(struct message *msg)
{
    /* 1-byte header indicating the size of the payload */
    int header_size = 5;
    /* maximum payload size */
    int maxpayload_size = RDT_PKTSIZE - header_size;
    /* reuse the same packet data structure */
    packet pkt;
    /* the cursor always points to the first unsent byte in the message */
    int cursor = 0;
    
    /* send out packet recursively */
    while (msg->size-cursor > maxpayload_size) {
	    /* fill in the packet */
	    fill_packet(pkt, maxpayload_size, header_size, msg->data + cursor);
	    /* send it out through the lower layer */
	    Sender_ToLowerLayer(&pkt);
	    /* move the cursor */
	    cursor += maxpayload_size;
    }

    /* send out the last packet */
    if (msg->size > cursor) {
	    /* fill in the packet */
	    fill_packet(pkt, msg->size - cursor, header_size, msg->data + cursor);
	    /* send it out through the lower layer */
	    Sender_ToLowerLayer(&pkt);
    }
}

/* event handler, called when a packet is passed from the lower layer at the 
   sender */
void Sender_FromLowerLayer(struct packet *pkt)
{
    
}

/* event handler, called when the timer expires */
void Sender_Timeout()
{
}
