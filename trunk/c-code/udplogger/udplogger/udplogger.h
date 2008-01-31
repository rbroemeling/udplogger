#ifndef __UDPLOGGER_H__
#define __UDPLOGGER_H__

#define DEFAULT_LISTEN_PORT 43824U
#define DEFAULT_MAXIMUM_TARGET_AGE 120UL

int arguments_parse(int, char **);
void arguments_show_usage();

#endif //!__UDPLOGGER_H__
