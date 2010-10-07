/**
 * The MIT License (http://www.opensource.org/licenses/mit-license.php)
 * 
 * Copyright (c) 2010 Nexopia.com, Inc.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 **/

#ifndef __UDPLOGGER_H__
#define __UDPLOGGER_H__

/* 
 * Log data format is:
 *   accesslog.format = "%m%s%b%I%O%T%X%U%q%a%{User-agent}i%{X-Forwarded-For}i%{Referer}i"
 * Or:
 *   accesslog.format = "%m%s%b%I%O%T%X%U%q%a%{User-agent}i%{X-Forwarded-For}i%{Referer}i%{X-LIGHTTPD-userid}o%{X-LIGHTTPD-age}o%{X-LIGHTTPD-sex}o%{X-LIGHTTPD-loc}o%{X-LIGHTTPD-usertype}o"
 */
 
/* Beacon packet format is: [beacon string]. */
#define BEACON_PACKET_SIZE 32
#define BEACON_STRING "UDPLOGGER BEACON"

/* The character to be used to delimit log fields.  We use U+001E, decimal 30, octal 036 (the information/record separator character). */
#define DELIMITER_CHARACTER 30U

/* The maximum length of a single log line (as read from stdin). */
#define INPUT_BUFFER_SIZE (1024 * 8)

/* Log packet format is:     [serial]   [tag]      [log data]. */
#define PACKET_MAXIMUM_SIZE ((20 + 1) + (10 + 1) + INPUT_BUFFER_SIZE)

/* The maximum length (characters) that a log line tag is allowed to be. */
#define TAG_MAXIMUM_LENGTH 10

/* The default port that udplogger will use to communicate. */
#define UDPLOGGER_DEFAULT_PORT 43824U

#endif
