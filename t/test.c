/*
 * Copyright (c) 2017 Fastly, Kazuho Oku
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <openssl/bio.h>
#include <openssl/crypto.h>
#include <openssl/engine.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include "picotls.h"
#include "picotls/openssl.h"
#include "quicly.h"
#include "quicly/defaults.h"
#include "quicly/streambuf.h"
#include "../lib/quicly.c"
#include "test.h"

#define RSA_PRIVATE_KEY                                                                                                            \
    "-----BEGIN RSA PRIVATE KEY-----\n"                                                                                            \
    "MIIEowIBAAKCAQEA5soWzSG7iyawQlHM1yaX2dUAATUkhpbg2WPFOEem7E3zYzc6\n"                                                           \
    "A/Z+bViFlfEgL37cbDUb4pnOAHrrsjGgkyBYh5i9iCTVfCk+H6SOHZJORO1Tq8X9\n"                                                           \
    "C7WcNcshpSdm2Pa8hmv9hsHbLSeoPNeg8NkTPwMVaMZ2GpdmiyAmhzSZ2H9mzNI7\n"                                                           \
    "ntPW/XCchVf+ax2yt9haZ+mQE2NPYwHDjqCtdGkP5ZXXnYhJSBzSEhxfGckIiKDy\n"                                                           \
    "OxiNkLFLvUdT4ERSFBjauP2cSI0XoOUsiBxJNwHH310AU8jZbveSTcXGYgEuu2MI\n"                                                           \
    "uDo7Vhkq5+TCqXsIFNbjy0taOoPRvUbPsbqFlQIDAQABAoIBAQCWcUv1wjR/2+Nw\n"                                                           \
    "B+Swp267R9bt8pdxyK6f5yKrskGErremiFygMrFtVBQYjws9CsRjISehSkN4GqjE\n"                                                           \
    "CweygJZVJeL++YvUmQnvFJSzgCjXU6GEStbOKD/A7T5sa0fmzMhOE907V+kpAT3x\n"                                                           \
    "E1rNRaP/ImJ1X1GjuefVb0rOPiK/dehFQWfsUkOvh+J3PU76wcnexxzJgxhVxdfX\n"                                                           \
    "qNa7UDsWzTImUjcHIfnhXc1K/oSKk6HjImQi/oE4lgoJUCEDaUbq0nXNrM0EmTTv\n"                                                           \
    "OQ5TVP5Lds9p8UDEa55eZllGXam0zKjhDKtkQ/5UfnxsAv2adY5cuH+XN0ExfKD8\n"                                                           \
    "wIZ5qINtAoGBAPRbQGZZkP/HOYA4YZ9HYAUQwFS9IZrQ8Y7C/UbL01Xli13nKalH\n"                                                           \
    "xXdG6Zv6Yv0FCJKA3N945lEof9rwriwhuZbyrA1TcKok/s7HR8Bhcsm2DzRD5OiC\n"                                                           \
    "3HK+Xy+6fBaMebffqBPp3Lfj/lSPNt0w/8DdrKBTw/cAy40g0n1zEu07AoGBAPHJ\n"                                                           \
    "V4IfQBiblCqDh77FfQRUNR4hVbbl00Gviigiw563nk7sxdrOJ1edTyTOUBHtM3zg\n"                                                           \
    "AT9sYz2CUXvsyEPqzMDANWMb9e2R//NcP6aM4k7WQRnwkZkp0WOIH95U2o1MHCYc\n"                                                           \
    "5meAHVf2UMl+64xU2ZfY3rjMmPLjWMt0hKYsOmtvAoGAClIQVkJSLXtsok2/Ucrh\n"                                                           \
    "81TRysJyOOe6TB1QNT1Gn8oiKMUqrUuqu27zTvM0WxtrUUTAD3A7yhG71LN1p8eE\n"                                                           \
    "3ytAuQ9dItKNMI6aKTX0czCNU9fKQ0fDp9UCkDGALDOisHFx1+V4vQuUIl4qIw1+\n"                                                           \
    "v9adA+iFzljqP/uy6DmEAyECgYAyWCgecf9YoFxzlbuYH2rukdIVmf9M/AHG9ZQg\n"                                                           \
    "00xEKhuOd4KjErXiamDmWwcVFHzaDZJ08E6hqhbpZN42Nhe4Ms1q+5FzjCjtNVIT\n"                                                           \
    "jdY5cCdSDWNjru9oeBmao7R2I1jhHrdi6awyeplLu1+0cp50HbYSaJeYS3pbssFE\n"                                                           \
    "EIWBhQKBgG3xleD4Sg9rG2OWQz5IrvLFg/Hy7YWyushVez61kZeLDnt9iM2um76k\n"                                                           \
    "/xFNIW0a+eL2VxRTCbXr9z86hjc/6CeSJHKYFQl4zsSAZkaIJ0+HbrhDNBAYh9b2\n"                                                           \
    "mRdX+OMdZ7Z5J3Glt8ENFRqe8RlESMpAKxjR+dID0bjwAjVr2KCh\n"                                                                       \
    "-----END RSA PRIVATE KEY-----\n"

#define RSA_CERTIFICATE                                                                                                            \
    "-----BEGIN CERTIFICATE-----\n"                                                                                                \
    "MIICqDCCAZACCQDI5jeEvExN+TANBgkqhkiG9w0BAQUFADAWMRQwEgYDVQQDEwtl\n"                                                           \
    "eGFtcGxlLmNvbTAeFw0xNjA5MzAwMzQ0NTFaFw0yNjA5MjgwMzQ0NTFaMBYxFDAS\n"                                                           \
    "BgNVBAMTC2V4YW1wbGUuY29tMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKC\n"                                                           \
    "AQEA5soWzSG7iyawQlHM1yaX2dUAATUkhpbg2WPFOEem7E3zYzc6A/Z+bViFlfEg\n"                                                           \
    "L37cbDUb4pnOAHrrsjGgkyBYh5i9iCTVfCk+H6SOHZJORO1Tq8X9C7WcNcshpSdm\n"                                                           \
    "2Pa8hmv9hsHbLSeoPNeg8NkTPwMVaMZ2GpdmiyAmhzSZ2H9mzNI7ntPW/XCchVf+\n"                                                           \
    "ax2yt9haZ+mQE2NPYwHDjqCtdGkP5ZXXnYhJSBzSEhxfGckIiKDyOxiNkLFLvUdT\n"                                                           \
    "4ERSFBjauP2cSI0XoOUsiBxJNwHH310AU8jZbveSTcXGYgEuu2MIuDo7Vhkq5+TC\n"                                                           \
    "qXsIFNbjy0taOoPRvUbPsbqFlQIDAQABMA0GCSqGSIb3DQEBBQUAA4IBAQAwZQsG\n"                                                           \
    "E/3DQFBOnmBITFsaIVJVXU0fbfIjy3p1r6O9z2zvrfB1i8AMxOORAVjE5wHstGnK\n"                                                           \
    "3sLMjkMYXqu1XEfQbStQN+Bsi8m+nE/x9MmuLthpzJHXUmPYZ4TKs0KJmFPLTXYi\n"                                                           \
    "j0OrP0a5BNcyGj/B4Z33aaU9N3z0TWBwx4OPjJoK3iInBx80sC1Ig2PE6mDBxLOg\n"                                                           \
    "5Ohm/XU/43MrtH8SgYkxr3OyzXTm8J0RFMWhYlo1uqR+pWV3TgacixNnUq5w5h4m\n"                                                           \
    "sqXcikh+j8ReNXsKnMOAfFo+HbRqyKWNE3DekCIiiQ5ds4A4SfT7pYyGAmBkAxht\n"                                                           \
    "sS919x2o8l97kaYf\n"                                                                                                           \
    "-----END CERTIFICATE-----\n"

static void on_destroy(quicly_stream_t *stream, int err);
static void on_egress_stop(quicly_stream_t *stream, int err);
static void on_ingress_receive(quicly_stream_t *stream,  size_t off, const void *src, size_t len);
static void on_ingress_reset(quicly_stream_t *stream, int err);

quicly_address_t fake_address;
int64_t quic_now;
quicly_context_t quic_ctx;
quicly_stream_callbacks_t stream_callbacks = {on_destroy,     quicly_streambuf_egress_shift,    quicly_streambuf_egress_emit,
                                              on_egress_stop, on_ingress_receive, on_ingress_reset};
size_t on_destroy_callcnt;

static int64_t get_now_cb(quicly_now_t *self)
{
    return quic_now;
}

static quicly_now_t get_now = {get_now_cb};

void on_destroy(quicly_stream_t *stream, int err)
{
    test_streambuf_t *sbuf = stream->data;
    sbuf->is_detached = 1;
    ++on_destroy_callcnt;
}

void on_egress_stop(quicly_stream_t *stream, int err)
{
    assert(QUICLY_ERROR_IS_QUIC_APPLICATION(err));
    test_streambuf_t *sbuf = stream->data;
    sbuf->error_received.stop_sending = err;
}

void on_ingress_receive(quicly_stream_t *stream,  size_t off, const void *src, size_t len)
{
    quicly_streambuf_ingress_receive(stream, off, src, len);
}

void on_ingress_reset(quicly_stream_t *stream, int err)
{
    assert(QUICLY_ERROR_IS_QUIC_APPLICATION(err));
    test_streambuf_t *sbuf = stream->data;
    sbuf->error_received.reset_stream = err;
}

const quicly_cid_plaintext_t *new_master_id(void)
{
    static quicly_cid_plaintext_t master = {UINT32_MAX};
    ++master.master_id;
    return &master;
}

static int on_stream_open(quicly_stream_open_t *self, quicly_stream_t *stream)
{
    test_streambuf_t *sbuf;
    int ret;

    ret = quicly_streambuf_create(stream, sizeof(*sbuf));
    assert(ret == 0);
    sbuf = stream->data;
    sbuf->error_received.stop_sending = -1;
    sbuf->error_received.reset_stream = -1;
    stream->callbacks = &stream_callbacks;

    return 0;
}

quicly_stream_open_t stream_open = {on_stream_open};

static void test_vector(void)
{
    static const uint8_t expected_payload[] = {
        0x06, 0x00, 0x40, 0xc4, 0x01, 0x00, 0x00, 0xc0, 0x03, 0x03, 0x66, 0x60, 0x26, 0x1f, 0xf9, 0x47, 0xce, 0xa4, 0x9c, 0xce,
        0x6c, 0xfa, 0xd6, 0x87, 0xf4, 0x57, 0xcf, 0x1b, 0x14, 0x53, 0x1b, 0xa1, 0x41, 0x31, 0xa0, 0xe8, 0xf3, 0x09, 0xa1, 0xd0,
        0xb9, 0xc4, 0x00, 0x00, 0x06, 0x13, 0x01, 0x13, 0x03, 0x13, 0x02, 0x01, 0x00, 0x00, 0x91, 0x00, 0x00, 0x00, 0x0b, 0x00,
        0x09, 0x00, 0x00, 0x06, 0x73, 0x65, 0x72, 0x76, 0x65, 0x72, 0xff, 0x01, 0x00, 0x01, 0x00, 0x00, 0x0a, 0x00, 0x14, 0x00,
        0x12, 0x00, 0x1d, 0x00, 0x17, 0x00, 0x18, 0x00, 0x19, 0x01, 0x00, 0x01, 0x01, 0x01, 0x02, 0x01, 0x03, 0x01, 0x04, 0x00,
        0x23, 0x00, 0x00, 0x00, 0x33, 0x00, 0x26, 0x00, 0x24, 0x00, 0x1d, 0x00, 0x20, 0x4c, 0xfd, 0xfc, 0xd1, 0x78, 0xb7, 0x84,
        0xbf, 0x32, 0x8c, 0xae, 0x79, 0x3b, 0x13, 0x6f, 0x2a, 0xed, 0xce, 0x00, 0x5f, 0xf1, 0x83, 0xd7, 0xbb, 0x14, 0x95, 0x20,
        0x72, 0x36, 0x64, 0x70, 0x37, 0x00, 0x2b, 0x00, 0x03, 0x02, 0x03, 0x04, 0x00, 0x0d, 0x00, 0x20, 0x00, 0x1e, 0x04, 0x03,
        0x05, 0x03, 0x06, 0x03, 0x02, 0x03, 0x08, 0x04, 0x08, 0x05, 0x08, 0x06, 0x04, 0x01, 0x05, 0x01, 0x06, 0x01, 0x02, 0x01,
        0x04, 0x02, 0x05, 0x02, 0x06, 0x02, 0x02, 0x02, 0x00, 0x2d, 0x00, 0x02, 0x01, 0x01, 0x00, 0x1c, 0x00, 0x02, 0x40, 0x01};
    uint8_t datagram[] = {
        0xc0, 0xff, 0x00, 0x00, 0x1b, 0x08, 0x83, 0x94, 0xc8, 0xf0, 0x3e, 0x51, 0x57, 0x08, 0x00, 0x00, 0x44, 0x9e, 0x3b, 0x34,
        0x3a, 0xa8, 0x53, 0x50, 0x64, 0xa4, 0x26, 0x8a, 0x0d, 0x9d, 0x7b, 0x1c, 0x9d, 0x25, 0x0a, 0xe3, 0x55, 0x16, 0x22, 0x76,
        0xe9, 0xb1, 0xe3, 0x01, 0x1e, 0xf6, 0xbb, 0xc0, 0xab, 0x48, 0xad, 0x5b, 0xcc, 0x26, 0x81, 0xe9, 0x53, 0x85, 0x7c, 0xa6,
        0x2b, 0xec, 0xd7, 0x52, 0x4d, 0xaa, 0xc4, 0x73, 0xe6, 0x8d, 0x74, 0x05, 0xfb, 0xba, 0x4e, 0x9e, 0xe6, 0x16, 0xc8, 0x70,
        0x38, 0xbd, 0xbe, 0x90, 0x8c, 0x06, 0xd9, 0x60, 0x5d, 0x9a, 0xc4, 0x90, 0x30, 0x35, 0x9e, 0xec, 0xb1, 0xd0, 0x5a, 0x14,
        0xe1, 0x17, 0xdb, 0x8c, 0xed, 0xe2, 0xbb, 0x09, 0xd0, 0xdb, 0xbf, 0xee, 0x27, 0x1c, 0xb3, 0x74, 0xd8, 0xf1, 0x0a, 0xbe,
        0xc8, 0x2d, 0x0f, 0x59, 0xa1, 0xde, 0xe2, 0x9f, 0xe9, 0x56, 0x38, 0xed, 0x8d, 0xd4, 0x1d, 0xa0, 0x74, 0x87, 0x46, 0x87,
        0x91, 0xb7, 0x19, 0xc5, 0x5c, 0x46, 0x96, 0x8e, 0xb3, 0xb5, 0x46, 0x80, 0x03, 0x71, 0x02, 0xa2, 0x8e, 0x53, 0xdc, 0x1d,
        0x12, 0x90, 0x3d, 0xb0, 0xaf, 0x58, 0x21, 0x79, 0x4b, 0x41, 0xc4, 0xa9, 0x33, 0x57, 0xfa, 0x59, 0xce, 0x69, 0xcf, 0xe7,
        0xf6, 0xbd, 0xfa, 0x62, 0x9e, 0xef, 0x78, 0x61, 0x64, 0x47, 0xe1, 0xd6, 0x11, 0xc4, 0xba, 0xf7, 0x1b, 0xf3, 0x3f, 0xeb,
        0xcb, 0x03, 0x13, 0x7c, 0x2c, 0x75, 0xd2, 0x53, 0x17, 0xd3, 0xe1, 0x3b, 0x68, 0x43, 0x70, 0xf6, 0x68, 0x41, 0x1c, 0x0f,
        0x00, 0x30, 0x4b, 0x50, 0x1c, 0x8f, 0xd4, 0x22, 0xbd, 0x9b, 0x9a, 0xd8, 0x1d, 0x64, 0x3b, 0x20, 0xda, 0x89, 0xca, 0x05,
        0x25, 0xd2, 0x4d, 0x2b, 0x14, 0x20, 0x41, 0xca, 0xe0, 0xaf, 0x20, 0x50, 0x92, 0xe4, 0x30, 0x08, 0x0c, 0xd8, 0x55, 0x9e,
        0xa4, 0xc5, 0xc6, 0xe4, 0xfa, 0x3f, 0x66, 0x08, 0x2b, 0x7d, 0x30, 0x3e, 0x52, 0xce, 0x01, 0x62, 0xba, 0xa9, 0x58, 0x53,
        0x2b, 0x0b, 0xbc, 0x2b, 0xc7, 0x85, 0x68, 0x1f, 0xcf, 0x37, 0x48, 0x5d, 0xff, 0x65, 0x95, 0xe0, 0x1e, 0x73, 0x9c, 0x8a,
        0xc9, 0xef, 0xba, 0x31, 0xb9, 0x85, 0xd5, 0xf6, 0x56, 0xcc, 0x09, 0x24, 0x32, 0xd7, 0x81, 0xdb, 0x95, 0x22, 0x17, 0x24,
        0x87, 0x64, 0x1c, 0x4d, 0x3a, 0xb8, 0xec, 0xe0, 0x1e, 0x39, 0xbc, 0x85, 0xb1, 0x54, 0x36, 0x61, 0x47, 0x75, 0xa9, 0x8b,
        0xa8, 0xfa, 0x12, 0xd4, 0x6f, 0x9b, 0x35, 0xe2, 0xa5, 0x5e, 0xb7, 0x2d, 0x7f, 0x85, 0x18, 0x1a, 0x36, 0x66, 0x63, 0x38,
        0x7d, 0xdc, 0x20, 0x55, 0x18, 0x07, 0xe0, 0x07, 0x67, 0x3b, 0xd7, 0xe2, 0x6b, 0xf9, 0xb2, 0x9b, 0x5a, 0xb1, 0x0a, 0x1c,
        0xa8, 0x7c, 0xbb, 0x7a, 0xd9, 0x7e, 0x99, 0xeb, 0x66, 0x95, 0x9c, 0x2a, 0x9b, 0xc3, 0xcb, 0xde, 0x47, 0x07, 0xff, 0x77,
        0x20, 0xb1, 0x10, 0xfa, 0x95, 0x35, 0x46, 0x74, 0xe3, 0x95, 0x81, 0x2e, 0x47, 0xa0, 0xae, 0x53, 0xb4, 0x64, 0xdc, 0xb2,
        0xd1, 0xf3, 0x45, 0xdf, 0x36, 0x0d, 0xc2, 0x27, 0x27, 0x0c, 0x75, 0x06, 0x76, 0xf6, 0x72, 0x4e, 0xb4, 0x79, 0xf0, 0xd2,
        0xfb, 0xb6, 0x12, 0x44, 0x29, 0x99, 0x04, 0x57, 0xac, 0x6c, 0x91, 0x67, 0xf4, 0x0a, 0xab, 0x73, 0x99, 0x98, 0xf3, 0x8b,
        0x9e, 0xcc, 0xb2, 0x4f, 0xd4, 0x7c, 0x84, 0x10, 0x13, 0x1b, 0xf6, 0x5a, 0x52, 0xaf, 0x84, 0x12, 0x75, 0xd5, 0xb3, 0xd1,
        0x88, 0x0b, 0x19, 0x7d, 0xf2, 0xb5, 0xde, 0xa3, 0xe6, 0xde, 0x56, 0xeb, 0xce, 0x3f, 0xfb, 0x6e, 0x92, 0x77, 0xa8, 0x20,
        0x82, 0xf8, 0xd9, 0x67, 0x7a, 0x67, 0x67, 0x08, 0x9b, 0x67, 0x1e, 0xbd, 0x24, 0x4c, 0x21, 0x4f, 0x0b, 0xde, 0x95, 0xc2,
        0xbe, 0xb0, 0x2c, 0xd1, 0x17, 0x2d, 0x58, 0xbd, 0xf3, 0x9d, 0xce, 0x56, 0xff, 0x68, 0xeb, 0x35, 0xab, 0x39, 0xb4, 0x9b,
        0x4e, 0xac, 0x7c, 0x81, 0x5e, 0xa6, 0x04, 0x51, 0xd6, 0xe6, 0xab, 0x82, 0x11, 0x91, 0x18, 0xdf, 0x02, 0xa5, 0x86, 0x84,
        0x4a, 0x9f, 0xfe, 0x16, 0x2b, 0xa0, 0x06, 0xd0, 0x66, 0x9e, 0xf5, 0x76, 0x68, 0xca, 0xb3, 0x8b, 0x62, 0xf7, 0x1a, 0x25,
        0x23, 0xa0, 0x84, 0x85, 0x2c, 0xd1, 0xd0, 0x79, 0xb3, 0x65, 0x8d, 0xc2, 0xf3, 0xe8, 0x79, 0x49, 0xb5, 0x50, 0xba, 0xb3,
        0xe1, 0x77, 0xcf, 0xc4, 0x9e, 0xd1, 0x90, 0xdf, 0xf0, 0x63, 0x0e, 0x43, 0x07, 0x7c, 0x30, 0xde, 0x8f, 0x6a, 0xe0, 0x81,
        0x53, 0x7f, 0x1e, 0x83, 0xda, 0x53, 0x7d, 0xa9, 0x80, 0xaf, 0xa6, 0x68, 0xe7, 0xb7, 0xfb, 0x25, 0x30, 0x1c, 0xf7, 0x41,
        0x52, 0x4b, 0xe3, 0xc4, 0x98, 0x84, 0xb4, 0x28, 0x21, 0xf1, 0x75, 0x52, 0xfb, 0xd1, 0x93, 0x1a, 0x81, 0x30, 0x17, 0xb6,
        0xb6, 0x59, 0x0a, 0x41, 0xea, 0x18, 0xb6, 0xba, 0x49, 0xcd, 0x48, 0xa4, 0x40, 0xbd, 0x9a, 0x33, 0x46, 0xa7, 0x62, 0x3f,
        0xb4, 0xba, 0x34, 0xa3, 0xee, 0x57, 0x1e, 0x3c, 0x73, 0x1f, 0x35, 0xa7, 0xa3, 0xcf, 0x25, 0xb5, 0x51, 0xa6, 0x80, 0xfa,
        0x68, 0x76, 0x35, 0x07, 0xb7, 0xfd, 0xe3, 0xaa, 0xf0, 0x23, 0xc5, 0x0b, 0x9d, 0x22, 0xda, 0x68, 0x76, 0xba, 0x33, 0x7e,
        0xb5, 0xe9, 0xdd, 0x9e, 0xc3, 0xda, 0xf9, 0x70, 0x24, 0x2b, 0x6c, 0x5a, 0xab, 0x3a, 0xa4, 0xb2, 0x96, 0xad, 0x8b, 0x9f,
        0x68, 0x32, 0xf6, 0x86, 0xef, 0x70, 0xfa, 0x93, 0x8b, 0x31, 0xb4, 0xe5, 0xdd, 0xd7, 0x36, 0x44, 0x42, 0xd3, 0xea, 0x72,
        0xe7, 0x3d, 0x66, 0x8f, 0xb0, 0x93, 0x77, 0x96, 0xf4, 0x62, 0x92, 0x3a, 0x81, 0xa4, 0x7e, 0x1c, 0xee, 0x74, 0x26, 0xff,
        0x6d, 0x92, 0x21, 0x26, 0x9b, 0x5a, 0x62, 0xec, 0x03, 0xd6, 0xec, 0x94, 0xd1, 0x26, 0x06, 0xcb, 0x48, 0x55, 0x60, 0xba,
        0xb5, 0x74, 0x81, 0x60, 0x09, 0xe9, 0x65, 0x04, 0x24, 0x93, 0x85, 0xbb, 0x61, 0xa8, 0x19, 0xbe, 0x04, 0xf6, 0x2c, 0x20,
        0x66, 0x21, 0x4d, 0x83, 0x60, 0xa2, 0x02, 0x2b, 0xeb, 0x31, 0x62, 0x40, 0xb6, 0xc7, 0xd7, 0x8b, 0xbe, 0x56, 0xc1, 0x30,
        0x82, 0xe0, 0xca, 0x27, 0x26, 0x61, 0x21, 0x0a, 0xbf, 0x02, 0x0b, 0xf3, 0xb5, 0x78, 0x3f, 0x14, 0x26, 0x43, 0x6c, 0xf9,
        0xff, 0x41, 0x84, 0x05, 0x93, 0xa5, 0xd0, 0x63, 0x8d, 0x32, 0xfc, 0x51, 0xc5, 0xc6, 0x5f, 0xf2, 0x91, 0xa3, 0xa7, 0xa5,
        0x2f, 0xd6, 0x77, 0x5e, 0x62, 0x3a, 0x44, 0x39, 0xcc, 0x08, 0xdd, 0x25, 0x58, 0x2f, 0xeb, 0xc9, 0x44, 0xef, 0x92, 0xd8,
        0xdb, 0xd3, 0x29, 0xc9, 0x1d, 0xe3, 0xe9, 0xc9, 0x58, 0x2e, 0x41, 0xf1, 0x7f, 0x3d, 0x18, 0x6f, 0x10, 0x4a, 0xd3, 0xf9,
        0x09, 0x95, 0x11, 0x6c, 0x68, 0x2a, 0x2a, 0x14, 0xa3, 0xb4, 0xb1, 0xf5, 0x47, 0xc3, 0x35, 0xf0, 0xbe, 0x71, 0x0f, 0xc9,
        0xfc, 0x03, 0xe0, 0xe5, 0x87, 0xb8, 0xcd, 0xa3, 0x1c, 0xe6, 0x5b, 0x96, 0x98, 0x78, 0xa4, 0xad, 0x42, 0x83, 0xe6, 0xd5,
        0xb0, 0x37, 0x3f, 0x43, 0xda, 0x86, 0xe9, 0xe0, 0xff, 0xe1, 0xae, 0x0f, 0xdd, 0xd3, 0x51, 0x62, 0x55, 0xbd, 0x74, 0x56,
        0x6f, 0x36, 0xa3, 0x87, 0x03, 0xd5, 0xf3, 0x42, 0x49, 0xde, 0xd1, 0xf6, 0x6b, 0x3d, 0x9b, 0x45, 0xb9, 0xaf, 0x2c, 0xcf,
        0xef, 0xe9, 0x84, 0xe1, 0x33, 0x76, 0xb1, 0xb2, 0xc6, 0x40, 0x4a, 0xa4, 0x8c, 0x80, 0x26, 0x13, 0x23, 0x43, 0xda, 0x3f,
        0x3a, 0x33, 0x65, 0x9e, 0xc1, 0xb3, 0xe9, 0x50, 0x80, 0x54, 0x0b, 0x28, 0xb7, 0xf3, 0xfc, 0xd3, 0x5f, 0xa5, 0xd8, 0x43,
        0xb5, 0x79, 0xa8, 0x4c, 0x08, 0x91, 0x21, 0xa6, 0x0d, 0x8c, 0x17, 0x54, 0x91, 0x5c, 0x34, 0x4e, 0xea, 0xf4, 0x5a, 0x9b,
        0xf2, 0x7d, 0xc0, 0xc1, 0xe7, 0x84, 0x16, 0x16, 0x91, 0x22, 0x09, 0x13, 0x13, 0xeb, 0x0e, 0x87, 0x55, 0x5a, 0xbd, 0x70,
        0x66, 0x26, 0xe5, 0x57, 0xfc, 0x36, 0xa0, 0x4f, 0xcd, 0x19, 0x1a, 0x58, 0x82, 0x91, 0x04, 0xd6, 0x07, 0x5c, 0x55, 0x94,
        0xf6, 0x27, 0xca, 0x50, 0x6b, 0xf1, 0x81, 0xda, 0xec, 0x94, 0x0f, 0x4a, 0x4f, 0x3a, 0xf0, 0x07, 0x4e, 0xee, 0x89, 0xda,
        0xac, 0xde, 0x67, 0x58, 0x31, 0x26, 0x22, 0xd4, 0xfa, 0x67, 0x5b, 0x39, 0xf7, 0x28, 0xe0, 0x62, 0xd2, 0xbe, 0xe6, 0x80,
        0xd8, 0xf4, 0x1a, 0x59, 0x7c, 0x26, 0x26, 0x48, 0xbb, 0x18, 0xbc, 0xfc, 0x13, 0xc8, 0xb3, 0xd9, 0x7b, 0x1a, 0x77, 0xb2,
        0xac, 0x3a, 0xf7, 0x45, 0xd6, 0x1a, 0x34, 0xcc, 0x47, 0x09, 0x86, 0x5b, 0xac, 0x82, 0x4a, 0x94, 0xbb, 0x19, 0x05, 0x80,
        0x15, 0xe4, 0xe4, 0x2d, 0x38, 0xd3, 0xb7, 0x79, 0xd7, 0x2e, 0xdc, 0x00, 0xc5, 0xcd, 0x08, 0x8e, 0xff, 0x80, 0x2b, 0x05};
    quicly_decoded_packet_t packet;
    struct st_quicly_cipher_context_t ingress, egress;
    uint64_t pn, next_expected_pn = 0;
    ptls_iovec_t payload;
    int ret;

    ok(quicly_decode_packet(&quic_ctx, &packet, datagram, sizeof(datagram)) == sizeof(datagram));
    ret = setup_initial_encryption(&ptls_openssl_aes128gcmsha256, &ingress, &egress, packet.cid.dest.encrypted, 0, NULL);
    ok(ret == 0);
    ok(decrypt_packet(ingress.header_protection, aead_decrypt_fixed_key, ingress.aead, &next_expected_pn, &packet, &pn, &payload) ==
       0);
    ok(pn == 2);
    ok(sizeof(expected_payload) <= payload.len);
    ok(memcmp(expected_payload, payload.base, sizeof(expected_payload)) == 0);

    dispose_cipher(&ingress);
    dispose_cipher(&egress);
}

void test_retry_aead(void)
{
    quicly_cid_t odcid = {{0x29, 0xb0, 0x09, 0x2f, 0xf9, 0x2b, 0xb4, 0xe8}, 8};
    static const uint8_t packet_bytes[] = {
        0xf2, 0xff, 0x00, 0x00, 0x1b, 0x04, 0x64, 0x9c, 0x87, 0x9e, 0x08, 0xd6, 0xb0, 0x09, 0x2f, 0xf9, 0x2b, 0xb4, 0xe8, 0xc6,
        0xa3, 0x3c, 0xfb, 0x60, 0xbe, 0xbe, 0xb2, 0x10, 0x0c, 0xee, 0x31, 0x4f, 0x35, 0x11, 0x4e, 0x6b, 0xb2, 0xc5, 0xab, 0x68,
        0xb4, 0x85, 0xf0, 0x7a, 0x4b, 0x51, 0xd5, 0xcc, 0xaa, 0xf1, 0xaa, 0x3f, 0x70, 0xba, 0x33, 0xc0, 0x61, 0xe4, 0x45, 0x5d,
        0x3b, 0xa2, 0x73, 0xb3, 0x13, 0x7b, 0x06, 0x05, 0x29, 0x1d, 0x1e, 0x5a, 0xd6, 0xac, 0xf7, 0xa3, 0xcb, 0x34, 0x92, 0x17,
        0xb1, 0x2a, 0x43, 0x03, 0xfa, 0xf0, 0xeb, 0xf0, 0x11, 0x47, 0xb4, 0x8f, 0xc9, 0xe2, 0x9d, 0x00};

    quicly_decoded_packet_t decoded;
    size_t decoded_len = quicly_decode_packet(&quic_ctx, &decoded, packet_bytes, sizeof(packet_bytes));
    ok(decoded_len == sizeof(packet_bytes));

    ptls_aead_context_t *retry_aead = create_retry_aead(&quic_ctx, 0);
    ok(validate_retry_tag(&decoded, &odcid, retry_aead));
    ptls_aead_free(retry_aead);
}

void free_packets(quicly_datagram_t **packets, size_t cnt)
{
    size_t i;
    for (i = 0; i != cnt; ++i)
        quic_ctx.packet_allocator->free_packet(quic_ctx.packet_allocator, packets[i]);
}

size_t decode_packets(quicly_decoded_packet_t *decoded, quicly_datagram_t **raw, size_t cnt)
{
    size_t ri, dc = 0;

    for (ri = 0; ri != cnt; ++ri) {
        size_t off = 0;
        do {
            size_t dl = quicly_decode_packet(&quic_ctx, decoded + dc, raw[ri]->data.base + off, raw[ri]->data.len - off);
            assert(dl != SIZE_MAX);
            ++dc;
            off += dl;
        } while (off != raw[ri]->data.len);
    }

    return dc;
}

int buffer_is(ptls_buffer_t *buf, const char *s)
{
    return buf->off == strlen(s) && memcmp(buf->base, s, buf->off) == 0;
}

size_t transmit(quicly_conn_t *src, quicly_conn_t *dst)
{
    quicly_datagram_t *datagrams[32];
    size_t num_datagrams, i;
    quicly_decoded_packet_t decoded[32];
    int ret;

    num_datagrams = sizeof(datagrams) / sizeof(datagrams[0]);
    ret = quicly_send(src, datagrams, &num_datagrams);
    ok(ret == 0);

    if (num_datagrams != 0) {
        size_t num_packets = decode_packets(decoded, datagrams, num_datagrams);
        for (i = 0; i != num_packets; ++i) {
            ret = quicly_receive(dst, NULL, &fake_address.sa, decoded + i);
            ok(ret == 0 || ret == QUICLY_ERROR_PACKET_IGNORED);
        }
        free_packets(datagrams, num_datagrams);
    }

    return num_datagrams;
}

int max_data_is_equal(quicly_conn_t *client, quicly_conn_t *server)
{
    uint64_t client_sent, client_consumed;
    uint64_t server_sent, server_consumed;

    quicly_get_max_data(client, NULL, &client_sent, &client_consumed);
    quicly_get_max_data(server, NULL, &server_sent, &server_consumed);

    if (client_sent != server_consumed)
        return 0;
    if (server_sent != client_consumed)
        return 0;

    return 1;
}

static void test_next_packet_number(void)
{
    /* prefer lower in case the distance in both directions are equal; see https://github.com/quicwg/base-drafts/issues/674 */
    uint64_t n = quicly_determine_packet_number(0xc0, 8, 0x139);
    ok(n == 0xc0);
    n = quicly_determine_packet_number(0xc0, 8, 0x140);
    ok(n == 0x1c0);
    n = quicly_determine_packet_number(0x9b32, 16, 0xa82f30eb);
    ok(n == 0xa82f9b32);

    n = quicly_determine_packet_number(31, 16, 65259);
    ok(n == 65567);
}

static void test_address_token_codec(void)
{
    static const uint8_t zero_key[PTLS_MAX_SECRET_SIZE] = {0};
    ptls_aead_context_t *enc = ptls_aead_new(&ptls_openssl_aes128gcm, &ptls_openssl_sha256, 1, zero_key, ""),
                        *dec = ptls_aead_new(&ptls_openssl_aes128gcm, &ptls_openssl_sha256, 0, zero_key, "");
    quicly_address_token_plaintext_t input, output;
    ptls_buffer_t buf;

    input = (quicly_address_token_plaintext_t){1, 234};
    input.remote.sin.sin_family = AF_INET;
    input.remote.sin.sin_addr.s_addr = htonl(0x7f000001);
    input.remote.sin.sin_port = htons(443);
    set_cid(&input.retry.odcid, ptls_iovec_init("abcdefgh", 8));
    input.retry.cidpair_hash = 12345;
    strcpy((char *)input.appdata.bytes, "hello world");
    input.appdata.len = strlen((char *)input.appdata.bytes);
    ptls_buffer_init(&buf, "", 0);

    ok(quicly_encrypt_address_token(ptls_openssl_random_bytes, enc, &buf, 0, &input) == 0);
    ptls_openssl_random_bytes(&output, sizeof(output));
    ok(quicly_decrypt_address_token(dec, &output, buf.base, buf.off, 0) == 0);

    ok(input.is_retry == output.is_retry);
    ok(input.issued_at == output.issued_at);
    ok(input.remote.sa.sa_family == output.remote.sa.sa_family);
    ok(input.remote.sin.sin_addr.s_addr == output.remote.sin.sin_addr.s_addr);
    ok(input.remote.sin.sin_port == output.remote.sin.sin_port);
    ok(input.retry.odcid.len == output.retry.odcid.len);
    ok(memcmp(input.retry.odcid.cid, output.retry.odcid.cid, input.retry.odcid.len) == 0);
    ok(input.retry.cidpair_hash == output.retry.cidpair_hash);
    ok(input.appdata.len == output.appdata.len);
    ok(memcmp(input.appdata.bytes, output.appdata.bytes, input.appdata.len) == 0);

    ptls_buffer_dispose(&buf);
    ptls_aead_free(enc);
    ptls_aead_free(dec);
}

int main(int argc, char **argv)
{
    static ptls_iovec_t cert;
    static ptls_openssl_sign_certificate_t cert_signer;
    static ptls_context_t tlsctx = {ptls_openssl_random_bytes,
                                    &ptls_get_time,
                                    ptls_openssl_key_exchanges,
                                    ptls_openssl_cipher_suites,
                                    {&cert, 1},
                                    NULL,
                                    NULL,
                                    NULL,
                                    &cert_signer.super,
                                    NULL,
                                    0,
                                    0,
                                    0,
                                    NULL,
                                    1};
    quic_ctx = quicly_spec_context;
    quic_ctx.tls = &tlsctx;
    quic_ctx.transport_params.max_streams_bidi = 10;
    quic_ctx.stream_open = &stream_open;
    quic_ctx.now = &get_now;

    fake_address.sa.sa_family = AF_INET;

    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();
#if !defined(OPENSSL_NO_ENGINE)
    /* Load all compiled-in ENGINEs */
    ENGINE_load_builtin_engines();
    ENGINE_register_all_ciphers();
    ENGINE_register_all_digests();
#endif

    {
        BIO *bio = BIO_new_mem_buf(RSA_CERTIFICATE, strlen(RSA_CERTIFICATE));
        X509 *x509 = PEM_read_bio_X509(bio, NULL, NULL, NULL);
        assert(x509 != NULL || !!"failed to load certificate");
        BIO_free(bio);
        cert.len = i2d_X509(x509, &cert.base);
        X509_free(x509);
    }

    {
        BIO *bio = BIO_new_mem_buf(RSA_PRIVATE_KEY, strlen(RSA_PRIVATE_KEY));
        EVP_PKEY *pkey = PEM_read_bio_PrivateKey(bio, NULL, NULL, NULL);
        assert(pkey != NULL || !"failed to load private key");
        BIO_free(bio);
        ptls_openssl_init_sign_certificate(&cert_signer, pkey);
        EVP_PKEY_free(pkey);
    }

    quicly_amend_ptls_context(quic_ctx.tls);

    subtest("next-packet-number", test_next_packet_number);
    subtest("address-token-codec", test_address_token_codec);
    subtest("ranges", test_ranges);
    subtest("frame", test_frame);
    subtest("maxsender", test_maxsender);
    subtest("sentmap", test_sentmap);
    subtest("test-vector", test_vector);
    subtest("test-retry-aead", test_retry_aead);
    subtest("simple", test_simple);
    subtest("stream-concurrency", test_stream_concurrency);
    subtest("loss", test_loss);

    return done_testing();
}
