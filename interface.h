#include <netinet/in.h>
#include <stdbool.h>
#ifndef MY_INTERFACE
#define MY_INTERFACE
bool get_interface_addr(const char *if_name,sa_family_t sa_family,struct sockaddr *addr );
#endif // MY_INTERFACE
