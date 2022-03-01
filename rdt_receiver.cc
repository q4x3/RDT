/*
 * FILE: rdt_receiver.cc
 * DESCRIPTION: Reliable data transfer receiver.
 * NOTE: This implementation assumes there is no packet loss, corruption, or 
 *       reordering.  You will need to enhance it to deal with all these 
 *       situations.  In this implementation, the packet format is laid out as 
 *       the following:
 *       
 *       |<-  1 byte  ->|<-       3 byte       ->|<-  3 byte  ->|<-             the rest            ->|
 *       | payload size |<- payload seq number ->|<- checksum ->|<-             payload             ->|
 *
 *       The first byte of each packet indicates the size of the payload
 *       (excluding this single-byte header)
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rdt_struct.h"
#include "rdt_receiver.h"

static int expected_seq_num = 0;
static packet *sndpkt = new packet;
static const int header_size = 7;

static int get_seq_num(struct packet *pkt) {
    return pkt->data[1] * 128 * 128 + pkt->data[2] * 128 + pkt->data[3];
}

bool checksum(struct packet *pkt, int header_size) {
    int checksum = pkt->data[6] + pkt->data[5] * 10 + pkt->data[4] * 100;
    int seq_num = get_seq_num(pkt);
    int realsum = seq_num;
    for(int i = header_size;i < pkt->data[0] + header_size;++ i) {
        realsum += pkt->data[i];
    }
    realsum %= 1000;
    // return checksum == realsum && seq_num == Receiver_Seq_Num;
    return checksum == realsum;
}

/* receiver initialization, called once at the very beginning */
void Receiver_Init()
{
    fprintf(stdout, "At %.2fs: receiver initializing ...\n", GetSimulationTime());
}

/* receiver finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to use this opportunity to release some 
   memory you allocated in Receiver_init(). */
void Receiver_Final()
{
    // if(sndpkt) delete sndpkt;
    fprintf(stdout, "At %.2fs: receiver finalizing ...\n", GetSimulationTime());
}

/* event handler, called when a packet is passed from the lower layer at the 
   receiver */
void Receiver_FromLowerLayer(struct packet *pkt)
{
    /* construct a message and deliver to the upper layer */
    struct message *msg = (struct message*) malloc(sizeof(struct message));
    ASSERT(msg!=NULL);

    msg->size = pkt->data[0];

    /* sanity check in case the packet is corrupted */
    if (msg->size<0) msg->size=0;
    if (msg->size>RDT_PKTSIZE-header_size) msg->size=RDT_PKTSIZE-header_size;

    if(checksum(pkt, header_size) && get_seq_num(pkt) == expected_seq_num) {
        msg->data = (char*) malloc(msg->size);
        ASSERT(msg->data!=NULL);
        memcpy(msg->data, pkt->data+header_size, msg->size);
        sndpkt = pkt;
        Receiver_ToUpperLayer(msg);
        Receiver_ToLowerLayer(pkt);
        ++ expected_seq_num;
    } else {
        // Receiver_ToLowerLayer(sndpkt);
        // fprintf(stdout, "Check failed! Packet seq num: %d, expected seq num: %d\n", get_seq_num(pkt), expected_seq_num);
    }

    /* don't forget to free the space */
    if (msg->data!=NULL) free(msg->data);
    if (msg!=NULL) free(msg);
}
