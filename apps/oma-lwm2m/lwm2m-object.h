/*
 * Copyright (c) 2015, Yanzi Networks AB.
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
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * \addtogroup apps
 * @{
 */

/**
 * \defgroup oma-lwm2m An implementation of OMA LWM2M
 * @{
 *
 * This application is an implementation of OMA Lightweight M2M.
 */

/**
 * \file
 *         Header file for the Contiki OMA LWM2M object API
 * \author
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 */

#ifndef LWM2M_OBJECT_H_
#define LWM2M_OBJECT_H_

#if defined(__ZEPHYR__)

#ifndef SYS_LOG_DOMAIN
#define SYS_LOG_DOMAIN "fota/lib/oma-lwm2m"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_FOTA_LEVEL
#include <logging/sys_log.h>
#endif

/* stdint conversions */
#include <zephyr/types.h>
#include <stddef.h>
#include <net/net_ip.h>
#include <net/zoap.h>
#include <misc/printk.h>
#include <kernel.h>

#define uint8_t  u8_t
#define uint16_t u16_t
#define uint32_t u32_t
#define int8_t   s8_t
#define int16_t  s16_t
#define int32_t  s32_t

typedef enum {
  COAP_HANDLER_STATUS_CONTINUE,
  COAP_HANDLER_STATUS_PROCESSED
} coap_handler_status_t;

/* type compatibility */
typedef struct zoap_packet coap_packet_t;
typedef struct zoap_observer coap_endpoint_t;

#define ntimer_seconds() (k_uptime_get_32() / 1000)

/* adjust console logging function */
#define printf(...) printk(__VA_ARGS__)

#else
#include "er-coap.h"
#include "er-coap-observe.h"
#endif

/* Operation permissions on the resources - read/write/execute */
#define LWM2M_RESOURCE_READ    0x10000
#define LWM2M_RESOURCE_WRITE   0x20000
#define LWM2M_RESOURCE_EXECUTE 0x40000

/* The resource id type of lwm2m objects - 16 bits for the ID - the rest
   is flags */
typedef uint32_t lwm2m_resource_id_t;

/* Defines for the resource definition array */
#define RO(x) (x | LWM2M_RESOURCE_READ)
#define WO(x) (x | LWM2M_RESOURCE_WRITE)
#define RW(x) (x | LWM2M_RESOURCE_READ | LWM2M_RESOURCE_WRITE)
#define EX(x) (x | LWM2M_RESOURCE_EXECUTE)


#define LWM2M_OBJECT_SECURITY_ID                0
#define LWM2M_OBJECT_SERVER_ID                  1
#define LWM2M_OBJECT_ACCESS_CONTROL_ID          2
#define LWM2M_OBJECT_DEVICE_ID                  3
#define LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID 4
#define LWM2M_OBJECT_FIRMWARE_ID                5
#define LWM2M_OBJECT_LOCATION_ID                6
#define LWM2M_OBJECT_CONNECTIVITY_STATISTICS_ID 7

#define LWM2M_SECURITY_SERVER_URI_ID            0
#define LWM2M_SECURITY_BOOTSTRAP_SERVER_ID      1
#define LWM2M_SECURITY_MODE_ID                  2
#define LWM2M_SECURITY_CLIENT_PKI_ID            3
#define LWM2M_SECURITY_SERVER_PKI_ID            4
#define LWM2M_SECURITY_KEY_ID                   5
#define LWM2M_SECURITY_SHORT_SERVER_ID         10

#define LWM2M_SERVER_SHORT_SERVER_ID            0
#define LWM2M_SERVER_LIFETIME_ID                1
#define LWM2M_SERVER_BINDING_ID                 7
#define LWM2M_SERVER_REG_UPDATE_TRIGGER_ID      8

#define LWM2M_DEVICE_MANUFACTURER_ID            0
#define LWM2M_DEVICE_MODEL_NUMBER_ID            1
#define LWM2M_DEVICE_SERIAL_NUMBER_ID           2
#define LWM2M_DEVICE_FIRMWARE_VERSION_ID        3
#define LWM2M_DEVICE_REBOOT_ID                  4
#define LWM2M_DEVICE_FACTORY_DEFAULT_ID         5
#define LWM2M_DEVICE_AVAILABLE_POWER_SOURCES    6
/* These do have multiple instances */
#define LWM2M_DEVICE_POWER_SOURCE_VOLTAGE       7
#define LWM2M_DEVICE_POWER_SOURCE_CURRENT       8
#define LWM2M_DEVICE_BATTERY_LEVEL              9

#define LWM2M_DEVICE_ERROR_CODE                11
#define LWM2M_DEVICE_TIME_ID                   13
#define LWM2M_DEVICE_TYPE_ID                   17


/* Pre-shared key mode */
#define LWM2M_SECURITY_MODE_PSK                 0
/* Raw Public Key mode */
#define LWM2M_SECURITY_MODE_RPK                 1
/* Certificate mode */
#define LWM2M_SECURITY_MODE_CERTIFICATE         2
/* NoSec mode */
#define LWM2M_SECURITY_MODE_NOSEC               3

#define LWM2M_OBJECT_STR_HELPER(x) (uint8_t *) #x
#define LWM2M_OBJECT_STR(x) LWM2M_OBJECT_STR_HELPER(x)

#define LWM2M_OBJECT_PATH_STR_HELPER(x) #x
#define LWM2M_OBJECT_PATH_STR(x) LWM2M_OBJECT_PATH_STR_HELPER(x)

typedef enum {
  LWM2M_OP_NONE,
  LWM2M_OP_READ,
  LWM2M_OP_DISCOVER,
  LWM2M_OP_WRITE,
  LWM2M_OP_WRITE_ATTR,
  LWM2M_OP_EXECUTE,
  LWM2M_OP_CREATE,
  LWM2M_OP_DELETE
} lwm2m_operation_t;

/* remember that we have already output a value - can be between two block's */
#define WRITER_OUTPUT_VALUE      1
#define WRITER_RESOURCE_INSTANCE 2

typedef struct lwm2m_reader lwm2m_reader_t;
typedef struct lwm2m_writer lwm2m_writer_t;
/* Data model for OMA LWM2M objects */
typedef struct lwm2m_context {
  uint16_t object_id;
  uint16_t object_instance_id;
  uint16_t resource_id;
  uint16_t resource_instance_id;

  uint8_t resource_index;
  uint8_t resource_instance_index; /* for use when stepping to next sub-resource if having multiple */
  uint8_t level;  /* 0/1/2/3 = 3 = resource */
  lwm2m_operation_t operation;

  coap_packet_t *request;
  coap_packet_t *response;

  unsigned int content_type;
  uint8_t *outbuf;
  size_t   outsize;
  unsigned outlen;
  uint8_t  out_mark_pos_oi; /* mark pos for last object instance   */
  uint8_t  out_mark_pos_ri; /* mark pos for last resource instance */

  uint8_t *inbuf;
  size_t  insize;
  int     inpos;

  uint32_t offset; /* If we do blockwise - this needs to change */

  /* Info on last_instance read/write */
  uint16_t last_instance;
  uint16_t last_value_len;

  uint8_t writer_flags; /* flags for reader/writer */
  const lwm2m_reader_t *reader;
  const lwm2m_writer_t *writer;
} lwm2m_context_t;

/* LWM2M format writer for the various formats supported */
struct lwm2m_writer {
  size_t (* init_write)(lwm2m_context_t *ctx);
  size_t (* end_write)(lwm2m_context_t *ctx);
  /* For sub-resources */
  size_t (* enter_resource_instance)(lwm2m_context_t *ctx);
  size_t (* exit_resource_instance)(lwm2m_context_t *ctx);
  size_t (* write_int)(lwm2m_context_t *ctx, uint8_t *outbuf, size_t outlen, int32_t value);
  size_t (* write_string)(lwm2m_context_t *ctx, uint8_t *outbuf, size_t outlen,  const char *value, size_t strlen);
  size_t (* write_float32fix)(lwm2m_context_t *ctx, uint8_t *outbuf, size_t outlen, int32_t value, int bits);
  size_t (* write_boolean)(lwm2m_context_t *ctx, uint8_t *outbuf, size_t outlen, int value);
};

struct lwm2m_reader {
  size_t (* read_int)(lwm2m_context_t *ctx, const uint8_t *inbuf, size_t len, int32_t *value);
  size_t (* read_string)(lwm2m_context_t *ctx, const uint8_t *inbuf, size_t len, uint8_t *value, size_t strlen);
  size_t (* read_float32fix)(lwm2m_context_t *ctx, const uint8_t *inbuf, size_t len, int32_t *value, int bits);
  size_t (* read_boolean)(
lwm2m_context_t *ctx, const uint8_t *inbuf, size_t len, int *value);
};

#define LWM2M_INSTANCE_FLAG_USED 1


#ifdef CONTIKI
static inline void
lwm2m_notify_observers(char *path)
{
  coap_notify_observers_sub(NULL, path);
}
#endif

static inline size_t
lwm2m_object_read_int(lwm2m_context_t *ctx, const uint8_t *inbuf, size_t len, int32_t *value)
{
  return ctx->reader->read_int(ctx, inbuf, len, value);
}

static inline size_t
lwm2m_object_read_string(lwm2m_context_t *ctx, const uint8_t *inbuf, size_t len, uint8_t *value, size_t strlen)
{
  return ctx->reader->read_string(ctx, inbuf, len, value, strlen);
}

static inline size_t
lwm2m_object_read_float32fix(lwm2m_context_t *ctx, const uint8_t *inbuf, size_t len, int32_t *value, int bits)
{
  return ctx->reader->read_float32fix(ctx, inbuf, len, value, bits);
}

static inline size_t
lwm2m_object_read_boolean(lwm2m_context_t *ctx, const uint8_t *inbuf, size_t len, int *value)
{
  return ctx->reader->read_boolean(ctx, inbuf, len, value);
}

static inline size_t
lwm2m_object_write_int(lwm2m_context_t *ctx, int32_t value)
{
  size_t s;
  s = ctx->writer->write_int(ctx, &ctx->outbuf[ctx->outlen],
                             ctx->outsize - ctx->outlen, value);
  ctx->outlen += s;
  return s;
}

static inline size_t
lwm2m_object_write_string(lwm2m_context_t *ctx, const char *value, size_t strlen)
{
  size_t s;
  s = ctx->writer->write_string(ctx, &ctx->outbuf[ctx->outlen],
                                ctx->outsize - ctx->outlen, value, strlen);
  ctx->outlen += s;
  return s;
}

static inline size_t
lwm2m_object_write_float32fix(lwm2m_context_t *ctx, int32_t value, int bits)
{
  size_t s;
  s = ctx->writer->write_float32fix(ctx, &ctx->outbuf[ctx->outlen],
                                    ctx->outsize - ctx->outlen, value, bits);
  ctx->outlen += s;
  return s;
}

static inline size_t
lwm2m_object_write_boolean(lwm2m_context_t *ctx, int value)
{
  size_t s;
  s = ctx->writer->write_boolean(ctx, &ctx->outbuf[ctx->outlen],
                                 ctx->outsize - ctx->outlen, value);
  ctx->outlen += s;
  return s;
}

/* Resource instance functions (_ri)*/

static inline size_t
lwm2m_object_write_enter_ri(lwm2m_context_t *ctx)
{
  if(ctx->writer->enter_resource_instance != NULL) {
    size_t s;
    s = ctx->writer->enter_resource_instance(ctx);
    ctx->outlen += s;
    return s;
  }
  return 0;
}

static inline size_t
lwm2m_object_write_exit_ri(lwm2m_context_t *ctx)
{
  if(ctx->writer->exit_resource_instance != NULL) {
    size_t s;
    s = ctx->writer->exit_resource_instance(ctx);
    ctx->outlen += s;
    return s;
  }
  return 0;
}

static inline size_t
lwm2m_object_write_int_ri(lwm2m_context_t *ctx, uint16_t id, int32_t value)
{
  size_t s;
  ctx->resource_instance_id = id;
  s = ctx->writer->write_int(ctx, &ctx->outbuf[ctx->outlen],
                             ctx->outsize - ctx->outlen, value);
  ctx->outlen += s;
  return s;
}

static inline size_t
lwm2m_object_write_string_ri(lwm2m_context_t *ctx, uint16_t id, const char *value, size_t strlen)
{
  size_t s;
  ctx->resource_instance_id = id;
  s = ctx->writer->write_string(ctx, &ctx->outbuf[ctx->outlen],
                                ctx->outsize - ctx->outlen, value, strlen);
  ctx->outlen += s;
  return s;
}

static inline size_t
lwm2m_object_write_float32fix_ri(lwm2m_context_t *ctx, uint16_t id, int32_t value, int bits)
{
  size_t s;
  ctx->resource_instance_id = id;
  s = ctx->writer->write_float32fix(ctx, &ctx->outbuf[ctx->outlen],
                                    ctx->outsize - ctx->outlen, value, bits);
  ctx->outlen += s;
  return s;
}

static inline size_t
lwm2m_object_write_boolean_ri(lwm2m_context_t *ctx, uint16_t id, int value)
{
  size_t s;
  ctx->resource_instance_id = id;
  s = ctx->writer->write_boolean(ctx, &ctx->outbuf[ctx->outlen],
                                 ctx->outsize - ctx->outlen, value);
  ctx->outlen += s;
  return s;
}

static inline int
lwm2m_object_is_final_incoming(lwm2m_context_t *ctx)
{
#if defined(__ZEPHYR__)
#else
  uint8_t more;
  if(coap_get_header_block1(ctx->request, NULL, &more, NULL, NULL)) {
    return !more;
  }
#endif
  /* If we do not know this is final... it might not be... */
  return 0;
}

#include "lwm2m-engine.h"

#endif /* LWM2M_OBJECT_H_ */
/**
 * @}
 * @}
 */
