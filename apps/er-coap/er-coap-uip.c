/*
 * Copyright (c) 2016, SICS, Swedish ICT AB.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/**
 * \file
 *         CoAP transport implementation for uIPv6
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#include "contiki.h"
#include "sys/cc.h"
#include "er-coap.h"
#include "er-coap-engine.h"
#include "er-coap-endpoint.h"
#include "er-coap-transport.h"
#include "er-coap-transactions.h"

#define DEBUG DEBUG_NONE
#include "net/ip/uip-debug.h"

/* sanity check for configured values */
#if COAP_MAX_PACKET_SIZE > (UIP_BUFSIZE - UIP_IPH_LEN - UIP_UDPH_LEN)
#error "UIP_CONF_BUFFER_SIZE too small for REST_MAX_CHUNK_SIZE"
#endif

#define SERVER_LISTEN_PORT      UIP_HTONS(COAP_SERVER_PORT)

/* direct access into the buffer */
#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
#if NETSTACK_CONF_WITH_IPV6
#define UIP_UDP_BUF  ((struct uip_udp_hdr *)&uip_buf[uip_l2_l3_hdr_len])
#else
#define UIP_UDP_BUF  ((struct uip_udp_hdr *)&uip_buf[UIP_LLH_LEN + UIP_IPH_LEN])
#endif

PROCESS(coap_engine, "CoAP Engine");

static struct uip_udp_conn *udp_conn = NULL;
/*---------------------------------------------------------------------------*/
void
coap_endpoint_print(const coap_endpoint_t *ep)
{
  printf("[");
  uip_debug_ipaddr_print(&ep->ipaddr);
  printf("]:%u", uip_ntohs(ep->port));
}
/*---------------------------------------------------------------------------*/
void
coap_endpoint_copy(coap_endpoint_t *destination,
                   const coap_endpoint_t *from)
{
  uip_ipaddr_copy(&destination->ipaddr, &from->ipaddr);
  destination->port = from->port;
}
/*---------------------------------------------------------------------------*/
int
coap_endpoint_cmp(const coap_endpoint_t *e1, const coap_endpoint_t *e2)
{
  if(!uip_ipaddr_cmp(&e1->ipaddr, &e2->ipaddr)) {
    return 0;
  }
  return e1->port == e2->port;
}
/*---------------------------------------------------------------------------*/
const coap_endpoint_t *
coap_src_endpoint(void)
{
  static coap_endpoint_t src;
  uip_ipaddr_copy(&src.ipaddr, &UIP_IP_BUF->srcipaddr);
  src.port = UIP_UDP_BUF->srcport;
  return &src;
}
/*---------------------------------------------------------------------------*/
uip_ipaddr_t *
coap_srcipaddr(void)
{
  return &UIP_IP_BUF->srcipaddr;
}

uint16_t
coap_srcport(void)
{
  return UIP_UDP_BUF->srcport;
}
/*---------------------------------------------------------------------------*/
uint8_t *
coap_databuf(void)
{
  return uip_appdata;
}

uint16_t
coap_datalen()
{
  return uip_datalen();
}
/*---------------------------------------------------------------------------*/
void
coap_transport_init(void)
{
  process_start(&coap_engine, NULL);
}
/*---------------------------------------------------------------------------*/
static void
process_data(void)
{
  PRINTF("receiving UDP datagram from: ");
  PRINT6ADDR(&UIP_IP_BUF->srcipaddr);
  PRINTF(":%u\n  Length: %u\n", uip_ntohs(UIP_UDP_BUF->srcport),
         uip_datalen());

  coap_receive(coap_src_endpoint(), uip_appdata, uip_datalen());
}
/*---------------------------------------------------------------------------*/
void
coap_send_message(const coap_endpoint_t *ep, const uint8_t *data,
                  uint16_t length)
{
  /* configure connection to reply to client */
  uip_ipaddr_copy(&udp_conn->ripaddr, &ep->ipaddr);
  udp_conn->rport = ep->port;

  uip_udp_packet_send(udp_conn, data, length);

  PRINTF("-sent UDP datagram (%u)-\n", length);

  /* restore server socket to allow data from any node */
  memset(&udp_conn->ripaddr, 0, sizeof(udp_conn->ripaddr));
  udp_conn->rport = 0;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(coap_engine, ev, data)
{
  PROCESS_BEGIN();

  /* new connection with remote host */
  udp_conn = udp_new(NULL, 0, NULL);
  udp_bind(udp_conn, SERVER_LISTEN_PORT);
  PRINTF("Listening on port %u\n", uip_ntohs(udp_conn->lport));

  while(1) {
    PROCESS_YIELD();

    if(ev == tcpip_event) {
      if(uip_newdata()) {
        process_data();
      }
    }
  } /* while (1) */

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/