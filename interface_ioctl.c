#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "interface.h"
#include <errno.h>

bool get_interface_addr(const char *if_name,sa_family_t sa_family,struct sockaddr *if_addr) {
	struct ifreq ifr;
	int s;

	s = socket(AF_INET, SOCK_DGRAM, 0);
	if(s < 0) {
		return(false);
	}

	memset(ifr.ifr_name, 0, sizeof(ifr.ifr_name));
	strncpy(ifr.ifr_name, if_name, sizeof(ifr.ifr_name)-1);
    ifr.ifr_addr.sa_family = sa_family;
	ifr.ifr_name[sizeof(ifr.ifr_name)-1] = '\0';
	if(ioctl(s, SIOCGIFADDR, &ifr)==-1) {
		close(s);
		return(false);
	}
	close(s);
    
    memcpy(if_addr,&ifr.ifr_addr,sizeof(struct sockaddr));
	return(true);
}
