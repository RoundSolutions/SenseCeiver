/*
 * m2m_cloud_task_callbacks.c
 *
 *  Created on: 17/nov/2014
 *      Author: FabioPi
 */

#include <stdlib.h>
#include "m2m_cloud_utils.h"
#include "m2m_cloud_methods_api.h"


extern char dwDATAMatrix[DW_FIELDS_NUM][DW_FIELD_LEN];

INT32 M2M_dwMsgReceive(INT32 type, INT32 param1, INT32 param2)
{
	char* response = (char*)param1;
	M2M_CLOUD_DWSTATUS dwStatus;

	//int ipAddr;

	if(checkATResponse(response, 5, "OK\r\n", "CONNECT\r\n", ">", "#DWDATA", "#DWRDATA"))
	{
		dwPrintDebug(M2M_CLOUD_LOG_VERBOSE, M2M_TRUE, "LOG--%s", response ); //module response
		setCloudRespState(0);
	}
	else if(checkATResponse(response, 3, "BUSY\r\n", "ERROR\r\n", "NO CARRIER\r\n"))
	{
		dwPrintDebug(M2M_CLOUD_LOG_ERRORS, M2M_TRUE, "%s", response ); //module response
		setCloudRespState(-1);
	}



	if(strstr(response, "#DWRING"))
	{
		m2m_os_send_message_to_task(getM2M_Cloud_ParseMessageTaskID(), M2M_CLOUD_DWRING_TYPE, (long)response, getDWRingAction());
	}
	else if(strstr(response, "#DWDATA"))
	{

		m2m_os_send_message_to_task(getM2M_Cloud_ParseMessageTaskID(), M2M_CLOUD_DWDATA_TYPE, (long)response, (INT32)NULL);
	}
	else if(strstr(response, "#DWRDATA")){
		m2m_os_send_message_to_task(getM2M_Cloud_ParseMessageTaskID(), M2M_CLOUD_DWRDATA_TYPE, (long)response, (INT32)NULL);
	}
	else if (strstr(response, "#DWSEND:") || strstr(response, "#DWSENDR:") )
	{
		//retrieves the last msgID
		setLastMsgID(getdwSendMsgID(response));
	}
	else if (strstr(response, "#SSLEN:")){
		lockDWDATALock(10000);
		strcpy(dwDATAMatrix[0],response);
	}

	//ipAddr = dwGetIpAddress(response, ipAddr);


	dwStatus = dwCheckStatus(response);
	if ( dwStatus > DW_ERROR )  // -1 => not a DWSTATUS response
	{
		setDWStatus(dwStatus);
		unLockDWStatusLock();
	}

	return 0;
}





INT32 M2M_dwParseMessage(INT32 type, INT32 param1, INT32 param2)
{
	char* response = (char*)param1;
	M2M_CLOUD_DWRING_STRUCT dwRingStruct;

	char dwRcvBuff[30];
	int  i, dwRing, msgType;

	int ret = -1;

	switch(type)
	{
		case M2M_CLOUD_DWRING_TYPE:
			dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_TRUE, "RSP--%s", response ); //module response

			dwRing = dwFillRingStruct(response, &dwRingStruct );
			if (dwRing == 0 )
			{
					ret = dwRingStruct.msgID;
					msgType = dwRingStruct.msgType;
					pushToRingQueue( dwRingStruct );

					/*Get the lock to avoid concurrent writings in the fields structure*/
					lockDWDATALock(10000);

					switch(param2)
					{
						case M2M_CLOUD_AUTORETRIEVE:  //automatically retrieves the message from the queue, thus making the corresponding queue slot free for another message.
							if (msgType == 2) //raw JSON message
							{
								sprintf(dwRcvBuff,"AT#DWRCVR=%d\r", ret);
								dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "RCV--Receiving RAW DW message.. %s\n", dwRcvBuff );
							}
							else
							{
								sprintf(dwRcvBuff,"AT#DWRCV=%d\r", ret);
								dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "RCV--Receiving DW message.. %s\n", dwRcvBuff );
							}

							dwSendATCommand(dwRcvBuff, M2M_TRUE);
							break;
						case M2M_CLOUD_LEAVE: //messages are left in queue
							dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "RCV--DW msgID: %d\n", ret );
							break;
					}
			}
			break;

		case M2M_CLOUD_DWDATA_TYPE:

			ret = dwParseDWData(response);

			if(ret == 1)
			{
				dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "Parsing dwdata\n", response ); //module response

				//searching in the RingQueue for the present received message
				msgType = searchMsgTypeFromMsgIDInRingQueue(atoi(dwDATAMatrix[0]));

				dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "MsgType: %d\n", msgType );

				switch(msgType)
				{
					case 0: //response to a type 0 format message (api command execution message)
							/*Unlocking the DWDATA lock (locked by case M2M_CLOUD_DWRING_TYPE ), now the global data structure will be available for the next writing*/
						unLockDWDATALock();
						break;
					case 1:
						dwPrintDebug(M2M_CLOUD_LOG_VERBOSE, M2M_FALSE, "DWDATA-- Type: 0; MsgID: %s; Error: %s; Len: %s\nDATA: ", dwDATAMatrix[0],  dwDATAMatrix[1], dwDATAMatrix[2]);
						if (getDWDebugState() >= M2M_CLOUD_LOG_VERBOSE)
						{
							for (i=3; i< DW_FIELDS_NUM; i++)
							{
								if (dwDATAMatrix[i][0] != '\0') {
									internalPrint("%s | ", dwDATAMatrix[i]);

								}
							}
							internalPrint("\n");

						}

						if( getLastMsgID() != 0)
						{
							unLockDWSendLock();
						}

						break;
					case 2:
						/*Unlocking the DWDATA lock (locked by case M2M_CLOUD_DWRING_TYPE ), now the global data structure will be available for the next writing*/
						unLockDWDATALock();
						break;

					case 3: //Message incoming from method execution request
						dwPrintDebug(M2M_CLOUD_LOG_VERBOSE, M2M_FALSE, "Method execution request incoming!\n");
						dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"Free Space at start: %d\n", m2m_os_get_mem_info((UINT32 *)NULL));
						m2m_os_send_message_to_task(getM2M_Cloud_MethodsHandlerTaskID(), 0, (INT32)NULL, (INT32)NULL);

						break;
					default:
						dwPrintDebug(M2M_CLOUD_LOG_ERRORS,M2M_FALSE, "Error! Unable to recognize DWDATA Message Type! Value: %d", dwDATAMatrix[0]);
						unLockDWDATALock();
						break;
				}
			}
			break;

		case M2M_CLOUD_DWRDATA_TYPE:

			if( getLastMsgID() != 0)
			{
				dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"raw message: %s\n", response);
				setRawResponse(response);
				unLockDWSendLock();
			}
			break;

	}

	return ret;
}



INT32 M2M_dwMethodsHandlerTask(INT32 type, INT32 param1, INT32 param2)
{
	int res;
	char (*dwMethodMatrix)[DW_FIELD_LEN] = m2m_os_mem_alloc(sizeof(char) * DW_FIELDS_NUM * DW_FIELD_LEN);

	memcpy(dwMethodMatrix, dwDATAMatrix, sizeof(dwDATAMatrix));
	dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"Free Space after allocation: %d\n", m2m_os_get_mem_info((UINT32 *)NULL));

	/*Unlocking the DWDATA lock (locked by M2M_dwParseMessage() ), now the global data structure will be available for the next writing*/
	unLockDWDATALock();

	res = m2m_cloud_method_handler(dwMethodMatrix[M2M_CLOUD_METHOD_MSG_METHOD_KEY], dwMethodMatrix[M2M_CLOUD_METHOD_MSG_METHOD_ID], dwMethodMatrix);

	m2m_os_mem_free(dwMethodMatrix);

	dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE,"Free Space: %d\n", m2m_os_get_mem_info((UINT32 *)NULL));


	return res;

}
