/*
 * Copyright (c) 2017, SICS, Swedish ICT AB.
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
 *         Posix support for TinyDTLS
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#include "tinydtls.h"
#include "dtls_debug.h"
#include "dtls_config.h"
#include "dtls_time.h"
#include <stdlib.h>

#include <arpa/inet.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

extern char *loglevels[];

#ifdef HAVE_TIME_H

static inline size_t
print_timestamp(char *s, size_t len, time_t t) {
  struct tm *tmp;
  tmp = localtime(&t);
  return strftime(s, len, "%b %d %H:%M:%S", tmp);
}

#else /* alternative implementation: just print the timestamp */

static inline size_t
print_timestamp(char *s, size_t len, clock_time_t t) {
#ifdef HAVE_SNPRINTF
  return snprintf(s, len, "%u.%03u",
		  (unsigned int)(t / CLOCK_SECOND),
		  (unsigned int)(t % CLOCK_SECOND));
#else /* HAVE_SNPRINTF */
  /* @todo do manual conversion of timestamp */
  return 0;
#endif /* HAVE_SNPRINTF */
}

#endif /* HAVE_TIME_H */
/*---------------------------------------------------------------------------*/
dtls_context_t *
malloc_context()
{
  return (dtls_context_t *)malloc(sizeof(dtls_context_t));
}
/*---------------------------------------------------------------------------*/
void
free_context(dtls_context_t *context)
{
  free(context);
}
/*---------------------------------------------------------------------------*/
#ifndef NDEBUG
size_t
dsrv_print_addr(const session_t *addr, char *buf, size_t len) {
  const void *addrptr = NULL;
  in_port_t port;
  char *p = buf;

  switch(addr->addr.sin_family) {
  case AF_INET:
    if(len < INET_ADDRSTRLEN) {
      return 0;
    }

    addrptr = &addr->addr.sin_addr;
    port = ntohs(addr->addr.sin_port);
    break;
  default:
    memcpy(buf, "(unknown address type)", min(22, len));
    return min(22, len);
  }

  if(inet_ntop(addr->addr.sin_family, addrptr, p, len) == 0) {
    perror("dsrv_print_addr");
    return 0;
  }

  p += strnlen(p, len);

  if(addr->addr.sin_family == AF_INET6) {
    if(p < buf + len) {
      *p++ = ']';
    } else {
      return 0;
    }
  }

  p += snprintf(p, buf + len - p + 1, ":%d", port);

  return p - buf;
}
#endif /* NDEBUG */
/*---------------------------------------------------------------------------*/
#ifdef HAVE_VPRINTF
void
dsrv_log(log_t level, char *format, ...)
{
  static char timebuf[32];
  va_list ap;
  FILE *log_fd;

  if (dtls_get_log_level() < level)
    return;

  log_fd = level <= DTLS_LOG_CRIT ? stderr : stdout;

  if (print_timestamp(timebuf,sizeof(timebuf), time(NULL)))
    fprintf(log_fd, "%s ", timebuf);

  if (level <= DTLS_LOG_DEBUG)
    fprintf(log_fd, "%s ", loglevels[level]);

  va_start(ap, format);
  vfprintf(log_fd, format, ap);
  va_end(ap);
  fflush(log_fd);
}
#endif /* HAVE_VPRINTF */
/*---------------------------------------------------------------------------*/
void
dtls_dsrv_hexdump_log(log_t level, const char *name, const unsigned char *buf, size_t length, int extend)
{
  static char timebuf[32];
  FILE *log_fd;
  int n = 0;

  if (dtls_get_log_level() < level)
    return;

  log_fd = level <= DTLS_LOG_CRIT ? stderr : stdout;

  if (print_timestamp(timebuf, sizeof(timebuf), time(NULL)))
    fprintf(log_fd, "%s ", timebuf);

  if (level <= DTLS_LOG_DEBUG)
    fprintf(log_fd, "%s ", loglevels[level]);

  if (extend) {
    fprintf(log_fd, "%s: (%zu bytes):\n", name, length);

    while (length--) {
      if (n % 16 == 0)
	fprintf(log_fd, "%08X ", n);

      fprintf(log_fd, "%02X ", *buf++);

      n++;
      if (n % 8 == 0) {
	if (n % 16 == 0)
	  fprintf(log_fd, "\n");
	else
	  fprintf(log_fd, " ");
      }
    }
  } else {
    fprintf(log_fd, "%s: (%zu bytes): ", name, length);
    while (length--)
      fprintf(log_fd, "%02X", *buf++);
  }
  fprintf(log_fd, "\n");

  fflush(log_fd);
}
/*---------------------------------------------------------------------------*/

/* --------- time support ----------- */

static time_t dtls_clock_offset;

void
dtls_clock_init(void) {
#ifdef HAVE_TIME_H
  dtls_clock_offset = time(NULL);
#else
# ifdef __GNUC__
  /* Issue a warning when using gcc. Other prepropressors do
   *  not seem to have a similar feature. */
#  warning "cannot initialize clock"
# endif
  dtls_clock_offset = 0;
#endif
}
/*---------------------------------------------------------------------------*/
void
dtls_ticks(dtls_tick_t *t)
{
#ifdef HAVE_SYS_TIME_H
  struct timeval tv;
  gettimeofday(&tv, NULL);
  *t = (tv.tv_sec - dtls_clock_offset) * DTLS_TICKS_PER_SECOND
    + (tv.tv_usec * DTLS_TICKS_PER_SECOND / 1000000);
#else
#error "clock not implemented"
#endif
}
/*---------------------------------------------------------------------------*/
int
dtls_get_random(unsigned long *rand)
{
  FILE *urandom = fopen("/dev/urandom", "r");
  unsigned char buf[sizeof(unsigned long)];

  if (!urandom) {
    dtls_emerg("cannot initialize PRNG\n");
    return 0;
  }

  if (fread(buf, 1, sizeof(buf), urandom) != sizeof(buf)) {
    dtls_emerg("cannot initialize PRNG\n");
    return 0;
  }

  fclose(urandom);

  *rand = (unsigned long)*buf;
  return 1;
}
/*---------------------------------------------------------------------------*/
void
dtls_set_retransmit_timer(dtls_context_t *ctx, unsigned int timeout)
{
/* Do nothing for now ... */
}
/*---------------------------------------------------------------------------*/
/* Implementation of session functions */
void
dtls_session_init(session_t *session)
{
  memset(session, 0, sizeof(session_t));
  session->size = sizeof(session->addr);
}
/*---------------------------------------------------------------------------*/
int
dtls_session_equals(const session_t *a, const session_t *b)
{
  /* printf("SESSION_EQUALS: A:%d,%d B:%d,%d\n", */
  /*        a->size, a->addr.sin_family, */
  /*        b->size, b->addr.sin_family); */

  if(a->size != b->size || a->addr.sin_family != b->addr.sin_family) {
    return 0;
  }

  /* need to compare only relevant parts of sockaddr */
  switch(a->addr.sin_family) {
  case AF_INET:
    return a->addr.sin_port == b->addr.sin_port &&
      memcmp(&a->addr.sin_addr, &b->addr.sin_addr,
             sizeof(struct in_addr)) == 0;
  default:
    return 0;
  }
}
/*---------------------------------------------------------------------------*/
/* The init */
void
dtls_support_init(void)
{
}
/*---------------------------------------------------------------------------*/
