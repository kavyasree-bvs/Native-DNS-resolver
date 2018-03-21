#pragma once
#define LOOKUP_STR_MAX_LENGTH 100
#define IP_STR_MAX_LENGTH 20

#define IP_TYPE 1
#define HOST_TYPE 2
#define MAX_DNS_SIZE 512 // largest valid UDP packet

/* DNS query types */ 
#define DNS_A		1 /* name -> IP */ 
#define DNS_NS		2 /* name server */
#define DNS_CNAME	5 /* canonical name */
#define DNS_PTR		12 /* IP -> name */ 
#define DNS_HINFO	13 /* host info/SOA */ 
#define DNS_MX		15 /* mail exchange */
#define DNS_AXFR	252 /* request for zone transfer */
#define DNS_ANY		255	/* all records */

/* query classes */
#define DNS_INET	1

/* flags */ 
#define DNS_QUERY (0 << 15)
#define DNS_RESPONSE (1 << 15)

#define DNS_STDQUERY (0 << 11) /* opcode-4bits*/

#define DNS_AA (1 << 10)/* authoritative ans*/
#define DNS_TC (1 << 9)	/* truncated*/
#define DNS_RD (1<<8)	/*recursion deleted*/
#define DNS_RA (1<<7)	/*recursion available*/

#define DNS_OK 0		/*success*/
#define DNS_FORMAT 1	/*format error(unable to interpret)*/
#define DNS_SERVERFAIL 2/* can't find authority nameserver*/
#define DNS_ERROR 3		/* no DNS entry*/
#define DNS_NOTIMPL 4	/* not implemented */
#define DNS_REFUSED 5	/* server refused the query */