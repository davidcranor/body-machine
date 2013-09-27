/******************************************************************************
main.cpp for testing Microchip MCP2035

By David Cranor
cranor@mit.edu
8/20/2013

 * The MIT License
 *
 * Copyright (c) 2013, MIT Media Lab
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *****************************************************************************/

#include <wirish/wirish.h>

//sorry.
#include </Users/davidcranor/Documents/libmaple/libraries/MCP2035/MCP2035.h>

#define MCP2035_IO_PIN 12
#define MCP2035_CLK_ALERT_PIN 13
#define MCP2035_CS_PIN 10


MCP2035 Receiver;

int main()
{
    //For debugging
	delay(3000);

    Receiver.begin(MCP2035_IO_PIN, MCP2035_CLK_ALERT_PIN, MCP2035_CS_PIN);
}


// Force init to be called *first*, i.e. before static object allocation.
// Otherwise, statically allocated objects that need libmaple may fail.
__attribute__((constructor)) void premain() {
    init();
}
