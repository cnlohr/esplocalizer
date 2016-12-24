#include <esp82xxutil.h>
#include <user_interface.h>
#include "promiscuous.h"

struct RxControl {
    signed rssi:8;
    unsigned rate:4;
    unsigned is_group:1;
    unsigned:1;
    unsigned sig_mode:2;
    unsigned legacy_length:12;
    unsigned damatch0:1;
    unsigned damatch1:1;
    unsigned bssidmatch0:1;
    unsigned bssidmatch1:1;
    unsigned MCS:7;
    unsigned CWB:1;
    unsigned HT_length:16;
    unsigned Smoothing:1;
    unsigned Not_Sounding:1;
    unsigned:1;
    unsigned Aggregation:1;
    unsigned STBC:2;
    unsigned FEC_CODING:1;
    unsigned SGI:1;
    unsigned rxend_state:8;
    unsigned ampdu_cnt:8;
    unsigned channel:4;
    unsigned:12;
};
 
struct LenSeq{ 
	u16 len; // length of packet 
    u16 seq; // serial number of packet, the high 12bits are serial number,
             //    low 14 bits are Fragment number (usually be 0)  
    u8 addr3[6]; // the third address in packet  
}; 


struct sniffer_buf {
	struct RxControl rx_ctrl; 
	u8 buf[36 ]; // head of ieee80211 packet 
    u16 cnt;     // number count of packet  
    struct LenSeq lenseq[1];  //length of packet  
};

struct sniffer_buf2{
    struct RxControl rx_ctrl; 
    u8 buf[112]; 
    u16 cnt;   
    u16 len;  //length of packet  
};



void wifi_promiscuous_cb(uint8 *buf, uint16 len)
{
//    struct RxControl *rx = (struct RxControl*) buf;
//	printf( "RX: %d %d %d\n", len, rx->MCS, rx->channel );
	int i;
    struct RxControl * rx_ctrl;
	int mdata = len;
	uint8_t * ldat;
	int padct = 0;

    if (len == 12){
		//Packet too rough to get real data.
		return;
    } else if (len == 128) {
        struct sniffer_buf2 *sniffer = (struct sniffer_buf2*) buf;
	    rx_ctrl = &sniffer->rx_ctrl;
		ldat = sniffer->buf;
		mdata = 112;
		padct = sniffer->len - 112;
		if( padct < 0 ) padct = 0;
    } else {
        struct sniffer_buf *sniffer = (struct sniffer_buf*) buf;
	    rx_ctrl = &sniffer->rx_ctrl;
		ldat = sniffer->buf;
		mdata = 36;
    }

	#define RADIOTAPLEN (8+4+3)


	if( ldat && mdata > 40 )
	{
		int i, match;


		uint32_t dt = 0;
		uint32_t now = system_get_time();

		if( memcmp( ldat, "\x80\x00\x00\x00\xff\xff\xff\xff\xff\xff", 10 ) == 0 )
		{
//			printf( "%d ", rx_ctrl->rssi );
			int wordlen = ldat[37];
			if( mdata > wordlen + 37 ) 
			{
				if( wordlen >= SSIDMAX ) wordlen = SSIDMAX-1;
				uint32_t dt = 0;
				uint32_t now = system_get_time();
				for( i = 0; i < CLIENTENTRIES; i++ )
				{
					struct ClientEntry * ce = &cle[i];
					uint32_t diff = now - ce->time;
					if( diff > dt )
					{
						dt = diff;
						match = i;
					}
					if( ets_memcmp( ce->mac, ldat+10, 6 ) == 0 )
					{
						match = i;
						dt = 0xffffffff;
					}
				}

				ets_memcpy( cle[match].mac, ldat+10, 6 );
				cle[match].time = now;
				cle[match].last = rx_ctrl->rssi;
				ets_memcpy( cle[match].apname, &ldat[38], wordlen );

//				for( i = 0; i < wordlen; i++ )
	//			{
		//			printf( "%c", ldat[i+37] );
			//	}
			}
		}
	}

}

