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
#include <libmill.h>

#include "../sframes.h"

static coroutine void client(void) {
    ipaddr addr = ipremote("127.0.0.1", 5555, 0, -1);
    tcpsock tcps = tcpconnect(addr, -1);
    assert(tcps);
    sfsock s = sfattach(tcps);
    assert(s);
    sfsend(s, "ABC", 3, -1);
    assert(errno == 0);
    //sfclose(s);
}

int main() {
    tcpsock ls = tcplisten(iplocal(NULL, 5555, 0), 10);
    assert(ls);
    go(client());
    tcpsock tcps = tcpaccept(ls, -1);
    assert(tcps);
    sfsock s = sfattach(tcps);
    assert(s);
    char buf[10];
    size_t nbytes = sfrecv(s, buf, sizeof(buf), -1);
    assert(errno == 0);
    assert(nbytes == 3);
    //sfclose(s);
    return 0;
}

