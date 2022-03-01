/*
 * FILE: rdt_sender.cc
 * DESCRIPTION: Reliable data transfer sender.
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
#include <vector>

#include "rdt_struct.h"
#include "rdt_sender.h"

static const int WindowSize = 10;
static const int header_size = 7;
static int base = 0;
static int next_seq_num = 0;
static std::vector<packet> pkt_seq;


void fill_packet(packet &pkt, int payload_size, const void *data, int seq_num) {
    pkt.data[0] = payload_size;
    pkt.data[1] = seq_num / (128 * 128);
    pkt.data[2] = seq_num % (128 * 128) / 128;
    pkt.data[3] = seq_num % 128;
    memcpy(pkt.data + header_size, data, payload_size);
    int checksum = seq_num;
    for(int i = header_size;i < pkt.data[0] + header_size;++ i) {
        checksum += pkt.data[i];
    }
    checksum %= 1000;
    pkt.data[4] = checksum / 100;
    pkt.data[5] = checksum % 100 / 10;
    pkt.data[6] = checksum % 10;
}

static int get_seq_num(struct packet *pkt) {
    return pkt->data[1] * 128 * 128 + pkt->data[2] * 128 + pkt->data[3];
}

// ack数据格式与普通包一致
bool check_ack(struct packet *pkt, int header_size) {
    int checksum = pkt->data[6] + pkt->data[5] * 10 + pkt->data[4] * 100;
    int seq_num = get_seq_num(pkt);
    int realsum = seq_num;
    for(int i = header_size;i < pkt->data[0] + header_size;++ i) {
        realsum += pkt->data[i];
    }
    realsum %= 1000;
    return checksum == realsum;
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
    /* maximum payload size */
    int maxpayload_size = RDT_PKTSIZE - header_size;
    /* reuse the same packet data structure */
    packet pkt;
    /* the cursor always points to the first unsent byte in the message */
    int cursor = 0;
    
    /* send out packet recursively */
    while (msg->size-cursor > maxpayload_size) {
	    /* fill in the packet */
	    fill_packet(pkt, maxpayload_size, msg->data + cursor, pkt_seq.size());
	    /* put in the packet sequence */
	    pkt_seq.push_back(pkt);
	    /* move the cursor */
	    cursor += maxpayload_size;
    }

    /* send out the last packet */
    if (msg->size > cursor) {
	    /* fill in the packet */
	    fill_packet(pkt, msg->size - cursor, msg->data + cursor, pkt_seq.size());
	    /* put in the packet sequence */
	    pkt_seq.push_back(pkt);
    }
    // while(pkt_seq.size() > 0) {
    //     Sender_ToLowerLayer(&(pkt_seq.front()));
    //     pkt_seq.erase(pkt_seq.begin());
    // }
    while(next_seq_num < base + WindowSize && next_seq_num < pkt_seq.size()) {
        if(base == next_seq_num) Sender_StartTimer(0.3);
        Sender_ToLowerLayer(&(pkt_seq[next_seq_num ++]));
    }
}

/* event handler, called when a packet is passed from the lower layer at the 
   sender */
void Sender_FromLowerLayer(struct packet *pkt)
{
    if(pkt && check_ack(pkt, header_size)) {
        base = get_seq_num(pkt) + 1;
        if(base == next_seq_num) Sender_StopTimer();
        else Sender_StartTimer(0.3);
    }
}

/* event handler, called when the timer expires */
void Sender_Timeout()
{
    Sender_StartTimer(0.3);
    // fprintf(stdout, "Timeout, resend packets! base: %d, next_seq_num: %d, pkt_seq_size: %d\n", base, next_seq_num, pkt_seq.size());
    for(int i = base;i < next_seq_num && i < pkt_seq.size();++ i) {
        Sender_ToLowerLayer(&(pkt_seq[i]));
    }
}
