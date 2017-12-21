/*
 * m2m_cloud_typedefs.h
 *
 *  Created on: 17/nov/2014
 *      Author: FabioPi
 */

#ifndef M2M_CLOUD_TYPEDEFS_H_
#define M2M_CLOUD_TYPEDEFS_H_
	#include "m2m_type.h"
	#include "m2m_clock_api.h"
	#include "m2m_cloud_types.h"

	typedef struct
	{
		int msgType;
		int msgID;
		int msgLen;
	} M2M_CLOUD_DWRING_STRUCT;


	typedef struct{
		int last;
		M2M_CLOUD_DWRING_STRUCT ringQueue[20];
	} M2M_CLOUD_DWRING_QUEUE;


	typedef struct {
	    double 			latitude;
	    double 			longitude;
	    float 			cog; //course over ground
	    float			altitude;
	    float 			speed;
	    unsigned short 	nsat;
	    short 			fix;
	    struct M2M_T_RTC_TIMEVAL time;

	} DW_GPSPositionData;



	typedef enum
	{
		M2M_CLOUD_LEAVE=20,
		M2M_CLOUD_AUTORETRIEVE
	} M2M_CLOUD_DWRING_ACTION;

	typedef enum
	{
		M2M_CLOUD_DWRING_TYPE,
		M2M_CLOUD_DWDATA_TYPE,
		M2M_CLOUD_DWRDATA_TYPE
	} M2M_CLOUD_DWMSG_TYPE;



	/* ====DEFINES ==== */

	#define DW_RAW_BUF_LEN 			1500

	#define DW_FIELD_LEN 			512
	#define DW_FIELDS_NUM			32

	#define M2M_CLOUD_AT_RESPONSE_TASK_PRIORITY 8
	#define M2M_CLOUD_PARSE_MESSAGE_TASK_PRIORITY 9
	#define M2M_CLOUD_METHODS_TASK_PRIORITY 10



	// -- GLOBAL VARIABLES STRUCT -- //
	typedef struct
	{
		M2M_CLOUD_AZ_LOGPORT	cloud_LogPortN;

		INT32 					M2M_CLOUD_AT_RESPONSE_TASK_ID;
		INT32 					M2M_CLOUD_PARSE_MESSAGE_TASK_ID;
		INT32 					M2M_CLOUD_METHODS_HANDLER_TASK_ID;

		M2M_CLOUD_DWSTATUS	 	cloudStatus;
		INT8					cloudRespState;
		M2M_CLOUD_BOOL 			dwIsInit;
		M2M_CLOUD_DBG_LEVEL		dwDebugLevel;
		M2M_USB_CH 				USB_LOG;

		M2M_CLOUD_DWRING_QUEUE	dw_RING_QUEUE;
		M2M_CLOUD_DWRING_ACTION DW_RING_ACTION;
		int						DW_LAST_MSG_ID;

		UINT32					timeout;

		char *CACertificatePath;

	} M2M_CLOUD_GLOBALS;


	static const char 		m2m_cloudDWConn[] = "AT#DWCONN=1\r";
	static const char 		m2m_cloudDWDisconn[] = "AT#DWCONN=0\r";
	static const char		m2m_cloudDWStat[] = "AT#DWSTATUS\r";





#endif /* M2M_CLOUD_TYPEDEFS_H_ */
