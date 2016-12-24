//Copyright 2015 <>< Charles Lohr, see LICENSE file.

#include "mem.h"
#include "c_types.h"
#include "user_interface.h"
#include "ets_sys.h"
#include "uart.h"
#include "osapi.h"
#include "espconn.h"
#include "esp82xxutil.h"
#include "commonservices.h"
#include <mdns.h>
#include "i2c.h"
#include "promiscuous.h"
#include "http.h"

#define procTaskPrio        0
#define procTaskQueueLen    1


static volatile os_timer_t some_timer;

//int ICACHE_FLASH_ATTR StartMDNS();

void user_rf_pre_init(void)
{
	//nothing.
}


char * strcat( char * dest, char * src )
{
	return strcat(dest, src );
}



//Tasks that happen all the time.

os_event_t    procTaskQueue[procTaskQueueLen];



#define LSMBUFFERSIZE 1024
int lsmhead = 0;
int lsmtail = 0;
int16_t lsmdata[6*LSMBUFFERSIZE];
int first_acc = 0;
int i2cmode = 0;

static void ICACHE_FLASH_ATTR procTask(os_event_t *events)
{

	while( i2cmode == 1 )
	{
		//Overflow?
		if( ((lsmhead+1)%LSMBUFFERSIZE) == lsmtail )
		{
			i2cmode = 0;
			break;
		}

		int r = ReadFastAcc( &lsmdata[6*lsmhead] );
		if( r < 0 )
		{
			i2cmode = 0;
		}
		else if( r == 0 )
		{
			break;
		}
		else if( r ==1 )
		{
			lsmhead = (lsmhead+1)%LSMBUFFERSIZE;
		}
	}

	CSTick( 0 );
	system_os_post(procTaskPrio, 0, 0 );
}


void wifi_promiscuous_cb(uint8 *buf, uint16 len);


void ICACHE_FLASH_ATTR system_init_done()
{
	//XXX Disabled: Monitor nearby stations.
	//wifi_set_channel(1);
    //wifi_promiscuous_enable(0);
	//wifi_set_promiscuous_rx_icb(wifi_promiscuous_cb);
	//wifi_promiscuous_enable(1);
}



static ICACHE_FLASH_ATTR void fastacc()
{
	//char mydat[128];
	//int len = URLDecode( mydat, 128, curhttp->pathbuffer+8 );

	char buf[1024];
	int times = 20;
	int len = 0;
	int16_t data[10];

	if( first_acc == 1 )
	{
		first_acc = 0;
		len += ets_sprintf( buf+len, "gx, gy, gz, ax, ay, az\r\n" );
	}

	for( ; times > 0 && lsmhead != lsmtail; times -- )
	{
		int16_t * data = &lsmdata[6*lsmtail];
		lsmtail = (lsmtail+1)%LSMBUFFERSIZE;
		
		len += ets_sprintf( buf+len, "%d, %d, %d, %d, %d, %d\r\n", data[0], data[1], data[2], data[3], data[4], data[5] );
	}

	if( len )
	{
		START_PACK;
		PushBlob( buf, len );
		END_TCP_WRITE( curhttp->socket );
	}

	if( i2cmode == 0 )
	{
		curhttp->state = HTTP_WAIT_CLOSE;
	}

}



static int ICACHE_FLASH_ATTR custom_http_cb_start_cb( struct HTTPConnection * hc )
{
	if( strcmp( (const char*)hc->pathbuffer, "/d/fastacc.csv" ) == 0 )
	{
		lsmhead = 0;
		lsmtail = 0;
		i2cmode = 1;
		first_acc = 1;
		SetupForFastAcc(1);
		hc->rcb = (void(*)())&fastacc;
		hc->bytesleft = 0xfffffffe;
		printf( "Got fast acc request.\n" );
		return 0;
	}
	if( strcmp( (const char*)hc->pathbuffer, "/d/stopacc" ) == 0 )
	{
		i2cmode = 0;
		SetupForFastAcc(0);
		hc->rcb = 0;
		hc->bytesleft = 0x0;
		return 0;
	}
	return -1;
}


//Timer event.
static void ICACHE_FLASH_ATTR myTimer(void *arg)
{
	static int channelchange = 0;

	CSTick( 1 );

	if( i2cmode == 0 )
		ReadI2Cs();

	int i;
	uint32_t now = system_get_time();

/*
	//XXX Disabled: monitoring for nearby stations.

	for( i = 0; i < CLIENTENTRIES; i++ )
	{
		int v = now - cle[i].time;
		if( v < 10000000 )
		{
			printf( "%d %s ", cle[i].last, cle[i].apname );
		}
	}

	channelchange++;
	if( ( channelchange & 0x00f ) == 0 )
	{
		int channel = (channelchange>>4) + 1;
		wifi_set_channel( channel );
		if( (channelchange>>4) == 13 ) channelchange = 0;
	}

*/

	system_os_post(procTaskPrio, 0, 0 );


/*
	os_delay_us(6000);
    wifi_fpm_set_sleep_type(MODEM_SLEEP_T);
	wifi_fpm_open();
	wifi_fpm_do_sleep( 0xffffffff );
	os_delay_us(1);*/
}

void ICACHE_FLASH_ATTR charrx( uint8_t c )
{
	//Called from UART.
}


void user_init(void)
{
	uart_init(BIT_RATE_115200, BIT_RATE_115200);
	uart0_sendStr("\001\xff\r\n\r\n\033cesp8266 ws2812 driver\r\n");


	{
		struct rst_info *  rs = system_get_rst_info();
		if( rs )
		{
			printf( "RS: %08x\n", rs->reason );
			printf( "EC: %08x\n", rs->exccause );
			printf( "E1: %08x\n", rs->epc1 );
			printf( "E2: %08x\n", rs->epc2 );
			printf( "E3: %08x\n", rs->epc3 );
			printf( "EA: %08x\n", rs->excvaddr );
			printf( "DP: %08x\n", rs->depc );
		}
	}


//Uncomment this to force a system restore.
//	system_restore();

	custom_http_cb_start = custom_http_cb_start_cb;

	CSSettingsLoad( 0 );
	CSPreInit();
	CSInit();

	SetServiceName( "esplocalizer" );
	AddMDNSName( "esp82xx" );
	AddMDNSService( "_http._tcp", "An ESP8266 Webserver", 80 );
	AddMDNSService( "_cn8266._udp", "ESP8266 Backend", 7878 );

	//Add a process
	system_os_task(procTask, procTaskPrio, procTaskQueue, procTaskQueueLen);

	//Timer example
	os_timer_disarm(&some_timer);
	os_timer_setfn(&some_timer, (os_timer_func_t *)myTimer, NULL);
	os_timer_arm(&some_timer, 25, 1);

	InitI2Cs();

	printf( "Boot Ok.\n" );

//	wifi_station_disconnect();
//	wifi_set_opmode(NULL_MODE);

	os_delay_us(6000);

	//wifi_set_sleep_type(LIGHT_SLEEP_T);

	//NOTE: Cannot enter sleep if always running idle task.
	system_os_post(procTaskPrio, 0, 0 );

    system_init_done_cb(system_init_done);
}


//There is no code in this project that will cause reboots if interrupts are disabled.
void EnterCritical()
{
}

void ExitCritical()
{
}


