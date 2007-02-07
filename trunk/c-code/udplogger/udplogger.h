/**
 * Common Defines
 **/
#ifndef __UDPLOGGER_H__
#define __UDPLOGGER_H__

#define LOGGER_PID_T	u_int32_t
#define LOGGER_CNT_T	u_int32_t

#define LOGGER_BUFSIZE (1024)
#define LOGGER_COMPRESSLEVEL 0

// this should not be modified (it includes the zlib buffer size)
// Packet is:
//	[u_int32_t pid][u_int32_t id][zlib data]
#define LOGGER_ZBUFSZ (int)((LOGGER_BUFSIZE * 1.1) + 12)
#define LOGGER_HDRSZ (sizeof(LOGGER_PID_T) + sizeof(LOGGER_CNT_T))
#define LOGGER_SENDBUF (LOGGER_HDRSZ + LOGGER_ZBUFSZ)


#endif //!__UDPLOGGER_H__
