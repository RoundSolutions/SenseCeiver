/*
 * Email_func.c
 *
 *  Created on: 14.08.2017
 *      Author: Mazen Jrab
 */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "m2m_type.h"
#include "m2m_clock_api.h"
#include "m2m_fs_api.h"
#include "m2m_hw_api.h"
#include "m2m_os_api.h"
#include "m2m_os_lock_api.h"
#include "m2m_socket_api.h"
#include "m2m_timer_api.h"
#include "m2m_sms_api.h"
#include "m2m_network_api.h"
#include "Senseceiver_utils.h"
#include "Senseceiver_base64.h"

/* SMS information */
#define ContextResetCounter		10
extern char APN[32];
extern CHAR	username_buf[32];
extern CHAR	password_buf[32];

extern float Current1;
extern float Current2;
extern float Analog1;
extern float Analog2;

extern UINT8 DigitalIN1;
extern UINT8 DigitalIN2;

//##################################################################################################################################
/**
 *  \brief      Function that sets up a TCP socket connection to a certain server address
 *
 *  \param      SERVER_ADDRESS:	a string that define the domain name of the server
 *  			TCP_PORT: TCP Port to which the socket should connect to
 *  			SocketStatus:  A pointer to a INT8 variable that is populated with 1 if the socket connects and 0 otherwise
 *
 *  \return     M2M_SOCKET_BSD_SOCKET; handle to the socket that can be used to receive and send information from the socket.
 *
 *
 *  \details The functions attempts to activate the context if not active and it will attempt that 10 time before it quits.
 *  		Further information about the Socket functionality can be found in the AppZone API Reference Guide
 */
//##################################################################################################################################
M2M_SOCKET_BSD_SOCKET Setup_TCP_Socket(CHAR* SERVER_ADDRESS, INT16 TCP_PORT, INT8* SocketStatus)
{
	M2M_SOCKET_BSD_SOCKET SocketFD;
	struct M2M_SOCKET_BSD_SOCKADDR_IN stSockAddr;
	M2M_SOCKET_BSD_FD_SET set;
	INT8 ResetCounter = 0;
	INT32 Res;
	UINT32 addr = 0;
	INT32 namelen = 0;
	INT32 on = 1;
	*SocketStatus = 0;
	while(m2m_pdp_get_status()!= M2M_PDP_STATE_ACTIVE )
	{
		SendToUSB("Context is not active...");
		ResetCounter++;
		if(ResetCounter>=ContextResetCounter)
		{
			SendToUSB("Failed to Send email. Couldn't activate context...\r\n");
			return 0;
		}
		m2m_pdp_activate(&APN[0], &username_buf[0], &password_buf[0]);
		m2m_os_sleep_ms(500);
	}
	SocketFD = m2m_socket_bsd_socket(M2M_SOCKET_BSD_PF_INET, M2M_SOCKET_BSD_SOCK_STREAM,

																		   M2M_SOCKET_BSD_IPPROTO_TCP);
	SendToUSB("Socket handle: SUCCESS\r\n");
	if (M2M_SOCKET_BSD_INVALID_SOCKET == SocketFD)
	{
		SendToUSB("Socket handle FAILURE\r\n");
		m2m_pdp_deactive();
		return 0;
	}
	SendToUSB("Context is activated\r\n");
	memset(&stSockAddr, 0, sizeof(struct M2M_SOCKET_BSD_SOCKADDR_IN));

	stSockAddr.sin_family = M2M_SOCKET_BSD_PF_INET;
	stSockAddr.sin_port = m2m_socket_bsd_htons(TCP_PORT);
	//stSockAddr.sin_addr.s_addr = m2m_socket_bsd_inet_addr(SERVER_ADDRESS);	//can be used instead of the line below if the server addres is an IP
	stSockAddr.sin_addr.s_addr = m2m_socket_bsd_get_host_by_name(SERVER_ADDRESS);
	if (M2M_SOCKET_BSD_INVALID_SOCKET ==
			m2m_socket_bsd_connect(SocketFD, (M2M_SOCKET_BSD_SOCKADDR *)&stSockAddr, sizeof(struct
												 M2M_SOCKET_BSD_SOCKADDR_IN)))
	{
		SendToUSB("Socket connects: FAILURE");
		m2m_socket_bsd_close(SocketFD);
		m2m_pdp_deactive();
		return 0;
	}
	SendToUSB("Socket connects: SUCCESS");
	Res = m2m_socket_bsd_get_sock_name(SocketFD, ( M2M_SOCKET_BSD_SOCKADDR *)&stSockAddr, &namelen );

	if (Res == 0)
	{
		SendToUSB ("IP: %s",m2m_socket_bsd_addr_str(stSockAddr.sin_addr.s_addr));
		SendToUSB ("PORT: %u", m2m_socket_bsd_ntohs(stSockAddr.sin_port));
	}
	else
	{
		SendToUSB ("get_socket_name: FAILURE");
		m2m_pdp_deactive();
		return 0;
	}
	if (0 != m2m_socket_bsd_ioctl (SocketFD, M2M_SOCKET_BSD_FIONBIO, &on))		//on=1 ==> Non blocking
	{
		SendToUSB("Error setting the socket in non blocking mode");
		return SocketFD;
	}
	*SocketStatus = 1;
	return SocketFD;
}
//##################################################################################################################################
/**
 *  \brief      Function that sends the sensor data collected by email
 *
 *  \param      M2M_EMAIL_INFO:	Structure that contains the necessary variable to send email using SMTP commands
 *  						* - SMTP Server Address
 *	 	 	 	 	 	 	* - SMTP Port
 *	 	 	 	 	 	 	* - Email From/Username
 *	 	 	 	 	 	 	* - Email Password
 * 	 	 	 	 	 	 	* - Email To
 *
 *
 *  \return     None
 *
 *
 *  \details This function has been implemented to use plain Authentication (AUTH PLAIN). Modifications are necessary if other
 *  		 authentication techniques are needed by the mail server being used. It encodes the credentials in the form \0username\0password
 *  		 in base64 and attempts the authentication afterwards.
 */
//##################################################################################################################################
void Setup_Email_Connection(M2M_EMAIL_INFO *email_info)
{
	INT8 SocketStatus = 0;
	CHAR buf_send[1024];
	CHAR buf_recv[1024];
	CHAR AutheticatioString[128];
	CHAR DeviceIMEI[20];

	m2m_info_get_serial_num(&DeviceIMEI[0]);
	SendToUSB("Serial number: %s",DeviceIMEI);

	SendToUSB("Socket connects: SUCCESS");

	buf_recv[0] = '\0';
	sprintf(&buf_recv[1],"%s",email_info->Username);
	buf_recv[strlen(email_info->Username)+1] = '\0';
	sprintf(&buf_recv[strlen(email_info->Username)+2],"%s",email_info->Password);
	Base64encode(AutheticatioString,buf_recv,strlen(email_info->Username)+strlen(email_info->Password)+2);
	SendToUSB("Authentication String: %s",AutheticatioString);

	M2M_SOCKET_BSD_SOCKET SocketFD = Setup_TCP_Socket(email_info->SMTPServer,email_info->SMTPPORT,&SocketStatus);
	if(SocketStatus==0)
	{
		return;
	}

	ReceiveDataFromSocket(buf_recv,SocketFD,2000);

	//Send ehlo to identify the device
	SendStringToSocket("EHLO  Senseceiver.roundsolutions.com\r\n",SocketFD);
	ReceiveDataFromSocket(buf_recv,SocketFD,1000);
	if(strstr(buf_recv,"AUTH LOGIN PLAIN")==NULL)
	{
		Socket_Connection_Close(SocketFD);
		return;
	}

	//request plain Authentication
	SendStringToSocket("AUTH PLAIN\r\n",SocketFD);
	ReceiveDataFromSocket(buf_recv,SocketFD,1000);
	if(strstr(buf_recv,"334")==NULL)
	{
		Socket_Connection_Close(SocketFD);
		return;
	}

	//Send Authentication information base64 encoded:  \0username\0password
	sprintf(buf_send,"%s\r\n",AutheticatioString);
	SendStringToSocket(buf_send,SocketFD);
	ReceiveDataFromSocket(buf_recv,SocketFD,2000);
	if(strstr(buf_recv,"Authentication succeeded")==NULL)
	{
		Socket_Connection_Close(SocketFD);
		return;
	}

	//Define Mail From
	sprintf(buf_send,"MAIL FROM: %s\r\n",email_info->EmailTo);
	SendStringToSocket(buf_send,SocketFD);
	ReceiveDataFromSocket(buf_recv,SocketFD,2000);
	if(strstr(buf_recv,"250")==NULL)
	{
		Socket_Connection_Close(SocketFD);
		return;
	}

	//Define Mail TO
	sprintf(buf_send,"RCPT TO: %s\r\n",email_info->EmailTo);
	SendStringToSocket(buf_send,SocketFD);
	ReceiveDataFromSocket(buf_recv,SocketFD,2000);
	if(strstr(buf_recv,"250")==NULL)
	{
		Socket_Connection_Close(SocketFD);
		return;
	}

	//Inform mail server that we are about to start sending the email information
	SendStringToSocket("DATA\r\n",SocketFD);
	ReceiveDataFromSocket(buf_recv,SocketFD,2000);

	sprintf(buf_send,"From:%s\r\nTo:%s\r\nSubject:Senseceiver Sensor Notification\r\n\r\n",email_info->Username,email_info->EmailTo);
	SendStringToSocket(buf_send,SocketFD);
	sprintf(&buf_send[0],"Device IMEI= %s\r\nCurrent1= %.2f mA\r\nCurrent2= %.2f mA\r\nAnalog1= %.2f mV\r\nAnalog2= %.2f mV\r\nDigitalIN1=%d\r\nDigitalIN2=%d",DeviceIMEI,Current1,Current2,Analog1,Analog2,DigitalIN1,DigitalIN2);
	SendStringToSocket(buf_send,SocketFD);
	//Termination sequence to send the email
	SendStringToSocket("\r\n.\r\n",SocketFD);
	ReceiveDataFromSocket(buf_recv,SocketFD,5000);

	m2m_os_sleep_ms(1000);

	Socket_Connection_Close(SocketFD);
	return;
}
//##################################################################################################################################
/**
 *  \brief      Function that closes the open socket and deactivates the Context afterwards
 *
 *  \param      SocketFD: handle to the socket that is required to be closed
 *
 *
 *  \return     None
 *
 *
 *  \details
 */
//##################################################################################################################################
void Socket_Connection_Close(M2M_SOCKET_BSD_SOCKET SocketFD)
{
	if(m2m_socket_bsd_close(SocketFD) == 0)
	{
		SendToUSB ("Close SUCCESS");
	}
	else
	{
		SendToUSB ("Close FAILURE");
	}
	m2m_pdp_deactive();
}
//##################################################################################################################################
/**
 *  \brief      Function that sends an array/string to the socket.
 *
 *  \param      buf_send: A pointer to the array/string that will be sent through the socket
 *  			SocketFD: Handle to the socket that is currently opened and will be used
 *
 *
 *  \return     None
 *
 *
 *  \details  The function checks if there is enough buffer to be allocated to the data being sent before it attempts sending
 */
//##################################################################################################################################
void SendStringToSocket(const CHAR* buf_send,M2M_SOCKET_BSD_SOCKET SocketFD)
{
	INT32 Res = m2m_socket_bsd_send_buf_size(SocketFD);
	if (Res > strlen(buf_send))
	{
		SendToUSB ("Data size: %d\r\nSending: %s \r\n",strlen(buf_send),buf_send);
		Res = m2m_socket_bsd_send(SocketFD, buf_send,strlen(buf_send), 0);
	}
	if(Res < 0)
	{
		SendToUSB ("Sending: FAILURE");
	}
}
//##################################################################################################################################
/**
 *  \brief      Function that receives data from an open socket
 *
 *  \param      buf_recv: A pointer to the array/string that will be will loaded with the received data
 *  			SocketFD: Handle to the socket that is currently opened and will be used
 *  			_delay_ms: a delay to wait before attempting to read data from the socket buffer
 *
 *
 *  \return     None
 *
 *
 *  \details  The function checks if there is data in the buffer of the socket before it attempts to read
 */
//##################################################################################################################################

void ReceiveDataFromSocket(CHAR* buf_recv,M2M_SOCKET_BSD_SOCKET SocketFD,UINT32 _delay_ms)
{
	if(_delay_ms>0)
		m2m_os_sleep_ms(_delay_ms);
	INT32 RXLen=0;
	m2m_socket_bsd_recv_data_size(SocketFD, &RXLen);
	INT32 Res=0;
	if(RXLen>0)
	{
		Res = m2m_socket_bsd_recv(SocketFD, &buf_recv[0], RXLen, 0);
		if(RXLen>1023)
			RXLen = 1023;
		if(Res < 0)
		{
			SendToUSB ("Receiving: FAILURE");
			SendToUSB ("Socket Error = %d",m2m_socket_errno());
		}
		else
		{
			SendToUSB("Received: %s", buf_recv);
		}
	}
}
