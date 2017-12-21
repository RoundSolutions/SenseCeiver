/*
 * Senseceiver_utils.h
 *
 *  Created on: 14.08.2017
 *      Author: Mazen Jrab
 */

#ifndef HDR_SENSECEIVER_UTILS_H_
#define HDR_SENSECEIVER_UTILS_H_

typedef struct _M2M_SOCKET_INFO
{
	UINT16 SocketID;
	UINT16 PORT;
	CHAR IP_SERVER[32];
} M2M_SOCKET_INFO;

/*! \Struct M2M_EMAIL_INFO
    \brief A Structure that contains the necessary variable to send email using SMTP commands
*/
typedef struct _M2M_EMAIL_INFO
{
	UINT16 SMTPPORT;
	CHAR SMTPServer[32];
	CHAR Username[40];
	CHAR Password[16];
	CHAR EmailTo[40];
} M2M_EMAIL_INFO;
M2M_SOCKET_BSD_SOCKET Setup_TCP_Socket(CHAR* SERVER_ADDRESS, INT16 TCP_PORT, INT8* SocketStatus);
void Setup_Email_Connection(M2M_EMAIL_INFO *email_info);
void SendStringToSocket(const CHAR* buf_send,M2M_SOCKET_BSD_SOCKET SocketFD);
void ReceiveDataFromSocket(CHAR* buf_recv,M2M_SOCKET_BSD_SOCKET SocketFD,UINT32 _delay_ms);
void Socket_Connection_Close(M2M_SOCKET_BSD_SOCKET SocketFD);
#endif /* HDR_SENSECEIVER_UTILS_H_ */
