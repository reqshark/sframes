/*

  Copyright (c) 2015 Martin Sustrik  All rights reserved

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom
  the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
  IN THE SOFTWARE.

*/

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "sframes.h"

struct sfsock {
    tcpsock u;
    chan sch;
    chan rch;
};

struct sframe {
    void *buf;
    size_t len;
};

static coroutine void sfsender(tcpsock u, chan ch) {
    struct sframe frame;
    while(1) {
        frame = chr(ch, struct sframe);
        uint64_t len = frame.len;
        size_t nbytes = tcpsend(u, &len, sizeof(len), -1);
        assert(errno == 0);
        assert(nbytes == sizeof(len));
        nbytes = tcpsend(u, frame.buf, frame.len, -1);
        assert(errno == 0);
        assert(nbytes == frame.len);
        free(frame.buf);
    }
}

static coroutine void sfreceiver(tcpsock u, chan ch) {
    struct sframe frame;
    uint64_t len;
    while(1) {
        size_t nbytes = tcprecv(u, &len, sizeof(len), -1);
        assert(errno == 0);
        assert(nbytes == sizeof(len));
        char *buf = malloc(len);
        assert(buf);
        nbytes = tcprecv(u, buf, len, -1);
        assert(errno == 0);
        assert(nbytes == len);
        frame.buf = (void*)buf;
        frame.len = len;
        chs(ch, struct sframe, frame);
    }
}

sfsock sfattach(tcpsock s) {
    struct sfsock *sfs = (struct sfsock*)malloc(sizeof(struct sfsock));
    if(!sfs) {
        errno = ENOMEM;
        return NULL;
    }
    sfs->u = s;
    sfs->sch = chmake(struct sframe, 0);
    assert(sfs->sch);
    go(sfsender(sfs->u, sfs->sch));
    sfs->rch = chmake(struct sframe, 0);
    assert(sfs->rch);
    go(sfreceiver(sfs->u, sfs->rch));
    return sfs;
}

void sfsend(sfsock s, const void *buf, size_t len, int64_t deadline) {
    struct sframe frame;
    frame.buf = malloc(len);
    assert(frame.buf);
    memcpy(frame.buf, buf, len);
    frame.len = len;
    choose {
    out(s->sch, struct sframe, frame):;
        return;
    end
    }
}

size_t sfrecv(sfsock s, void *buf, size_t len, int64_t deadline) {
    choose {
    in(s->rch, struct sframe, frame):;
        size_t sz = len < frame.len ? len : frame.len;
        memcpy(buf, frame.buf, sz);
        free(frame.buf);
        return sz;
    end
    }
}

void sfclose(sfsock s) {
    chclose(s->rch);
    chclose(s->sch);
    tcpclose(s->u);
    free(s);
}

