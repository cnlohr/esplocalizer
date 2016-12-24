//Copyright 2015 <>< Charles Lohr, see LICENSE file.

#include <commonservices.h>

extern uint8_t last_leds[512*3];
extern int last_led_count;


int ICACHE_FLASH_ATTR CustomCommand(char * buffer, int retsize, char *pusrdata, unsigned short len)
{
	char * buffend = buffer;

	switch( pusrdata[1] )
	{
	case 'C': case 'c': //Custom command test
	{
		buffend += ets_sprintf( buffend, "CC" );
		return buffend-buffer;
	}
	}
	return -1;
}
