/*
 * Copyright (c) 2013, Institute for Pervasive Computing, ETH Zurich
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
 *
 * This file is part of the Contiki operating system.
 */

/**
 * \file
 *      CoAP module for reliable transport
 * \author
 *      Matthias Kovatsch <kovatsch@inf.ethz.ch>
 */

#include "er-coap-transactions.h"
#include "er-coap-observe.h"
#include "sys/ntimer.h"
#include "lib/memb.h"
#include "lib/list.h"
#include <stdlib.h>

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINTEP(ep) coap_endpoint_print(ep)
#else
#define PRINTF(...)
#define PRINTEP(ep)
#endif

/*---------------------------------------------------------------------------*/
MEMB(transactions_memb, coap_transaction_t, COAP_MAX_OPEN_TRANSACTIONS);
LIST(transactions_list);

/*---------------------------------------------------------------------------*/
static void
coap_retransmit_transaction(ntimer_t *nt)
{
  coap_transaction_t *t = ntimer_get_user_data(nt);
  if(t == NULL) {
    PRINTF("No retransmission data in ntimer!\n");
    return;
  }
  ++(t->retrans_counter);
  PRINTF("Retransmitting %u (%u)\n", t->mid, t->retrans_counter);
  coap_send_transaction(t);
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*- Internal API ------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
coap_transaction_t *
coap_new_transaction(uint16_t mid, const coap_endpoint_t *endpoint)
{
  coap_transaction_t *t = memb_alloc(&transactions_memb);

  if(t) {
    t->mid = mid;
    t->retrans_counter = 0;

    /* save client address */
    coap_endpoint_copy(&t->endpoint, endpoint);

    list_add(transactions_list, t); /* list itself makes sure same element is not added twice */
  }

  return t;
}
/*---------------------------------------------------------------------------*/
void
coap_send_transaction(coap_transaction_t *t)
{
  PRINTF("Sending transaction %u\n", t->mid);

  coap_send_message(&t->endpoint, t->packet, t->packet_len);

  if(COAP_TYPE_CON ==
     ((COAP_HEADER_TYPE_MASK & t->packet[0]) >> COAP_HEADER_TYPE_POSITION)) {
    if(t->retrans_counter < COAP_MAX_RETRANSMIT) {
      /* not timed out yet */
      PRINTF("Keeping transaction %u\n", t->mid);

      if(t->retrans_counter == 0) {
        ntimer_set_callback(&t->retrans_timer, coap_retransmit_transaction);
        ntimer_set_user_data(&t->retrans_timer, t);
        t->retrans_interval =
          COAP_RESPONSE_TIMEOUT_TICKS + (rand() %
                                         COAP_RESPONSE_TIMEOUT_BACKOFF_MASK);
        PRINTF("Initial interval %lu msec\n",
               (unsigned long)t->retrans_interval);
      } else {
        t->retrans_interval <<= 1;  /* double */
        PRINTF("Doubled (%u) interval %d s\n", t->retrans_counter,
               t->retrans_interval / 1000);
      }

      /* interval updated above */
      ntimer_set(&t->retrans_timer, t->retrans_interval);

      t = NULL;
    } else {
      /* timed out */
      PRINTF("Timeout\n");
      restful_response_handler callback = t->callback;
      void *callback_data = t->callback_data;

      /* handle observers */
      coap_remove_observer_by_client(&t->endpoint);

      coap_clear_transaction(t);

      if(callback) {
        callback(callback_data, NULL);
      }
    }
  } else {
    coap_clear_transaction(t);
  }
}
/*---------------------------------------------------------------------------*/
void
coap_clear_transaction(coap_transaction_t *t)
{
  if(t) {
    PRINTF("Freeing transaction %u: %p\n", t->mid, t);

    ntimer_stop(&t->retrans_timer);
    list_remove(transactions_list, t);
    memb_free(&transactions_memb, t);
  }
}
coap_transaction_t *
coap_get_transaction_by_mid(uint16_t mid)
{
  coap_transaction_t *t = NULL;

  for(t = (coap_transaction_t *)list_head(transactions_list); t; t = t->next) {
    if(t->mid == mid) {
      PRINTF("Found transaction for MID %u: %p\n", t->mid, t);
      return t;
    }
  }
  return NULL;
}
