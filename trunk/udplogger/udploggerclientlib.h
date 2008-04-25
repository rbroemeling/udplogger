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
 * values for each parameter.  This function should return 1 for success or 0 for failure.
 **/
extern int add_option_hook();


/**
 * getopt_hook(<option>)
 *
 * Implemented in udplogger clients to deal with the option character passed into it.  Return 1 on syntactically correct match,
 * a negative value on a match that didn't satisfy the argument syntax, or 0 on no match.
 * Each client should deal properly with each option character that it has added in add_option_hook().
 **/
extern int getopt_hook(char);


/**
 * log_packet_hook(<source host>, <log line>)
 *
 * There is no implementation of this function in this file.  This function should be implemented in each
 * udploggerd client program, and should take the arguments sockaddr_in * (the source host of the log line)
 * and char * (the log line data itself).
 **/
extern void inline log_packet_hook(struct sockaddr_in *, char *);


/**
 * usage_hook()
 *
 * There is no implementation of this function in this file.  This function should be implemented in each
 * udploggerd client program, and should print a usage/help description of each command-line parameter
 * that the client program supports (beyond the default udploggerclientlib ones that this file creates.
 **/
extern void usage_hook();


#endif
