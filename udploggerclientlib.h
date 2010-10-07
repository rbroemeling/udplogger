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

#ifndef __UDPLOGGERCLIENTLIB_H__
#define __UDPLOGGERCLIENTLIB_H__


/**
 * add_option(<long option>, <has argument>, <short option>)
 *
 * Utility function implemented in udploggerclientlib.c to add an option (described by the parameters)
 * to the list of accepted command-line parameters.  Returns 1 for success or 0 for failure.
 **/
extern int add_option(const char *, const int, const char);


/**
 * add_option_hook()
 *
 * Implemented in udplogger clients to call the add_option function in order to add each command-line parameter
 * that the client program supports (beyond the default udploggerclientlib ones).  Should also set the default
 * value for each parameter.  This function should return 1 for success or 0 for failure.
 **/
extern int add_option_hook();


/**
 * getopt_hook(<option>)
 *
 * Implemented in udplogger clients to deal with the option character passed into it.  Return 1 on syntactically correct match,
 * a negative value on a match that didn't satisfy the argument syntax, or 0 on no match.
 * Each client should deal properly with each option that the client has added in add_option_hook().
 **/
extern int getopt_hook(char);


/**
 * handle_signal_hook(<signal flags>)
 *
 * Implemented in udplogger clients to handle any signals that are marked as received in
 * the sigset_t <signal flags>.
 **/
extern void handle_signal_hook(sigset_t *);


/**
 * log_packet_hook(<source host>, <log line>)
 *
 * Implemented in udplogger clients to take the arguments sockaddr_in * (the source host of the log line)
 * and char * (the log packet data itself) and do something with it (called once per log line).
 **/
extern void inline log_packet_hook(struct sockaddr_in *, char *);


/**
 * usage_hook()
 *
 * Implemented in udplogger clients to print a usage/help description of each command-line parameter
 * that the client program supports (beyond the default udploggerclientlib ones).  Should show complete
 * usage details for each option that the client has added in add_option_hook().
 **/
extern void usage_hook();


#endif
