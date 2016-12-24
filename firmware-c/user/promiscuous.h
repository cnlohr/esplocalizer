#ifndef _PROMISCUOUS_H
#define _PROMISCUOUS_H

#define CLIENTENTRIES 20
#define SSIDMAX 20

struct ClientEntry
{
	uint8_t mac[6];
	char    apname[SSIDMAX-1];
	uint32_t time;
	int8_t last;
};

struct ClientEntry cle[CLIENTENTRIES];

#endif


