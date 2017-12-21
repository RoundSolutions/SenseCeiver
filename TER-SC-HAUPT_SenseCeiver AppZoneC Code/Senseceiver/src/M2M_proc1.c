/*==================================================================================================
                            INCLUDE FILES
==================================================================================================*/

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


#include "m2m_cloud_api.h"
#include "m2m_cloud_methods_api.h"

#include "Senseceiver_utils.h"


/*==================================================================================================
                            LOCAL CONSTANT DEFINITION
==================================================================================================*/

/*==================================================================================================
                            LOCAL TYPES DEFINITION
==================================================================================================*/
#define TRUE 1
#define FW_VERSION	"1.0.3"
/*==================================================================================================
                            LOCAL FUNCTION PROTOTYPES
==================================================================================================*/
INT32 M2M_msgProc2(INT32 type, INT32 param1, INT32 param2);
void SendToUSB (const CHAR *fmt, ... );
INT32 M2M_msgProc3(INT32 type, INT32 param1, INT32 param2);
INT32 InitAuxUART();
void LoadConfigurationFile();
INT32 ConfSetParam(CHAR* _CompleteBuf,INT32 _BufSize ,CHAR* _Param,INT32 _StartIndex);
INT32 ConnectToCloud();
void ParseSensorData(UINT8 * SensorData);
void ReadSensorData();
UINT8 SetDigitalOutput(UINT8 PortNumber, UINT8 PortStatus);
void SendSMSNotification();
void SendEmailNotification();

/*==================================================================================================
                            GLOBAL FUNCTIONS PROTOTYPES
==================================================================================================*/

/*==================================================================================================
                            LOCAL MACROS
==================================================================================================*/

/*==================================================================================================
                            LOCAL VARIABLES
==================================================================================================*/






CHAR	username_buf[32];  /*!< Stores the Username of the operator being used. Default is empty unless otherwise specified in the config file*/


CHAR	password_buf[32]; /*!< Stores the Password of the operator being used. Default is empty unless otherwise specified in the config file*/


char 	appToken[32];/*!< Stores the token of the organization in the Telit Cloud. You can replace this with your organization Token Found under the menu Developer --> Applications in the Telit IoT Porta
 	 	 	 	 	 	 *  Default is js4jJnnK4FmNh2QU which connects to Round Solutions Organization. You can replace the default token in
 	 	 	 	 	 	 *  the function LoadConfigurationsFile()*/

INT8 EnableSMS = 0;  /*!< Enables or disables SMS notifications*/

char   	SMSNumber[32]; /*!< Stores the Number to which SMS notifications are sent to*/

INT8 EnableEmail = 0;  /*!< Enables or disables SMS notifications*/




char APN[32]; /*!< Stores the APN of the operator being used. Default is internetm2m.air.com unless otherwise specified in the config file*/

INT32 TxInterval; /*!< Defines the interval at which the application will attempt to connect to the Platform and update the sensor Data*/

char serverURL[] 	= "api-de.devicewise.com"; /*!< Server API address*/


M2M_EMAIL_INFO email_server_info;	 /*!< A Structure that contains the variables for sending an email:
 	 	 	 	 	 	 	 	 	 	* - SMTP Server Address
 	 	 	 	 	 	 	 	 	 	* - SMTP Port
 	 	 	 	 	 	 	 	 	 	* - Email From/Username
 	 	 	 	 	 	 	 	 	 	* - Email Password
 	 	 	 	 	 	 	 	 	 	* - Email To (email recipient)
 	 	 	 	 	 	 	 	 	 	**/



float Current1=0; /*!< Holds the value of Current1 in mA. Value is updated after the call ReadSensorData()*/

float Current2=0; /*!< Holds the value of Current2 in mA. Value is updated after the call ReadSensorData()*/

float Analog1=0; /*!< Holds the value of Analog1 in mV. Value is updated after the call ReadSensorData()*/

float Analog2=0;  /*!< Holds the value of Analog2 in mV. Value is updated after the call ReadSensorData()*/

float MainVoltage = 0; /*!< Holds the value of Main Voltage in V. Value is updated after the call ReadSensorData()*/

UINT8 DigitalIN1=0; /*!< Holds the status of Digital Input 1. Value is updated after the call ReadSensorData()*/

UINT8 DigitalIN2=0; /*!< Holds the status of Digital Input 2. Value is updated after the call ReadSensorData()*/

UINT8 DigitalOUT1=0; /*!< Holds the status of Digital Output 1. Value is updated after the call ReadSensorData()*/

UINT8 DigitalOUT2=0; /*!< Holds the status of Digital Output 2. Value is updated after the call ReadSensorData()*/


M2M_T_HW_UART_HANDLE global_aux_uart = M2M_HW_UART_HANDLE_INVALID; /*!< Global handle for the AUX UART to read sensor data*/

M2M_T_OS_LOCK _lock_handle = NULL; /*!< Used to lock function call to prevent simultaneous opening of USB port for Example*/
M2M_T_HW_UART_IO_HW_OPTIONS hw_settings; /*!< Used to define the UAX UART Port settings like baud rate, parity, etc... */



UINT8 UART_buf[300];	/*!< a generic buffer used to read data from the AUX UART */
INT32 UART_buf_Index = 0;	/*!< An index used to keep track of where to write the next byte in UART_buf*/
INT8 EnableDebug = 1;	/*!< enables of disables debug messages on USB or UART */
INT8 UseUART = 0;	 /*!< defines  if the debug messages should be sent out on UART or USB */
/*==================================================================================================
                            GLOBAL VARIABLES
==================================================================================================*/

/*==================================================================================================
                            LOCAL FUNCTIONS IMPLEMENTATION
==================================================================================================*/

/*==================================================================================================
                            GLOBAL FUNCTIONS IMPLEMENTATION
==================================================================================================*/

//##################################################################################################################################
/**
 *
 *  \brief     Handles events sent to process 1
 *
 *  \param       type:	event id
 *  \param       param1: addition info
 *  \param       param2: addition info
 *
 *  \return      None.
 *
 *
 *  \details This process has the highest priority.
 *  				        Running complex code here will block other events coming to this task.
 */
//##################################################################################################################################
INT32 M2M_msgProc1(INT32 type, INT32 param1, INT32 param2)
{
	int res;
	m2m_os_sleep_ms(5000);
	m2m_hw_gpio_write(2,0);
	m2m_info_get_model(UART_buf);
	if(UART_buf[0]=='G')
		UseUART = 1;
	SendToUSB("module name= %s",UART_buf);
	SendToUSB("\r\nApplication has started...\r\nFW Version: %s",FW_VERSION);
	SendToUSB("\r\nApplication has started...\r\n");
	LoadConfigurationFile();
	/*********************************************/
	/*The following task can be used to read sensor data incase data monitoring is needed to trigger events*/
	/*Otherwise Sensor data can be requested once needed through the function ReadSensorData()*/
	/************/
	//m2m_os_create_task(M2M_OS_TASK_STACK_L,2,M2M_OS_TASK_MBOX_L,M2M_msgProc2);
	//m2m_os_send_message_to_task(2,0,0,0);
	//m2m_os_sleep_ms(1000);
	/************/
	/*********************************************/

	m2m_sms_set_text_mode_format();
	m2m_os_create_task(M2M_OS_TASK_STACK_M,3,M2M_OS_TASK_MBOX_M,M2M_msgProc3);
	m2m_sms_enable_new_message_indication();
	m2m_hw_gpio_write(2,1);
	m2m_os_sleep_ms(2000);
	ReadSensorData();

	/******Init Cloud********/
	//AZ2 connected to AT2, Small HEAP size (64k), Log Level to debug, logs printed on USB0 channel (HighSpeed Modem)
	res = m2m_cloud_init(M2M_AZ2, AT1, M2M_CLOUD_HEAP_S, M2M_CLOUD_LOG_NONE, USB_CH_NONE);
	//serverURL, deviceIDSelector, appToken, security, heartBeat, autoReconnect, handle, atrunInstanceId, serviceTimeout
	res = m2m_cloud_config( serverURL, IMEI, appToken, M2M_CLOUD_SSL_OFF, 86400, M2M_CLOUD_DISABLED, M2M_CLOUD_FIFO, 4, 5);
	if (res != M2M_CLOUD_SUCCESS)
	{
		//dwPrintDebug(M2M_CLOUD_LOG_ERRORS, M2M_FALSE, "Error configuring cloud connection!\n" );
		return M2M_CLOUD_FAILURE;
	}

	//dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "Res: %d\n", res);

	//enabling remoteAT feature
	res = m2m_cloud_remoteAT(M2M_CLOUD_DW_ENABLED);
	//dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "Res: %d\n", res);
	if (res != M2M_CLOUD_SUCCESS)
	{
		//dwPrintDebug(M2M_CLOUD_LOG_ERRORS, M2M_FALSE, "Error initializing cloud. Return value: %d\n", res);

		//exit from main execution
		return M2M_CLOUD_FAILURE;
	}
	while(1)
	{
		m2m_os_sleep_ms(5000);	//needed in order to start other tasks if created before initiating a connection to Cloud
		SendToUSB("Reading Sensor Data....\r\n");
		ReadSensorData();
		SendToUSB("Sending data to Cloud....\r\n");
		ConnectToCloud();
		SendSMSNotification();
		SendEmailNotification();
		SendToUSB("\r\nWaiting till next TX Interval....\r\n");

		m2m_os_sleep_ms((TxInterval-1)*60000);//the TX process is on average 1 minute.
	}
  return res;
}
//##################################################################################################################################
/**
 *  \brief      Connects to the Telit Cloud Portal and sends the measurements of the Analog and Current inputs
 *
 *  \param       None.
 *
 *  \return      None.
 *
 *
 *  \details Function activates context, creates the device in the cloud if it is not already created, uploads
 * 					the values and then disconnects. It uses USB0 to send debug and status messages
 */
//##################################################################################################################################
INT32 ConnectToCloud()
{
	int res;
	/*******************
	* Starting cloud configuration...
	*******************/
	CHAR sendvalue[10];




	/*******************
	 * Enabling PDP context
	 *******************/
	res = m2m_cloud_pdp_context_activate(APN, username_buf, password_buf);
	if (res != M2M_CLOUD_SUCCESS)
	{
		//dwPrintDebug(M2M_CLOUD_LOG_ERRORS, M2M_FALSE, "Error activating PDP context. Return value: %d\n", res);

		return M2M_CLOUD_FAILURE;
	}


	//connecting to the cloud server
	res = m2m_cloud_connect(M2M_TRUE);
	//dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "m2m_cloud_connect Res: %d\n", res);

	if(res != M2M_CLOUD_SUCCESS) {
		//dwPrintDebug(M2M_CLOUD_LOG_ERRORS, M2M_FALSE, "Error Connecting to the Cloud Server. Return value: %d\n", res);

		return M2M_CLOUD_FAILURE;
	}


	if (m2m_cloud_status() == DW_CONNECTED)
	{

		/* ====================================================================================================================================
		 * ====================================================================================================================================
		 * ===																																===
		 * ===													CONNECTED TO CLOUD															===																	===
		 * ===																																===
		 * ====================================================================================================================================
		 * ====================================================================================================================================*/



		//dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE, "Connection to the Cloud server established!\n\n" );



		/*==     INSERT USER CODE HERE      ==*/
		/* Publish properties to the cloud */
		sprintf(&sendvalue[0],"%.3f",Current1);
		m2m_cloud_property_publish("Current 1",&sendvalue[0],NULL,NULL,M2M_TRUE);

		sprintf(&sendvalue[0],"%.3f",Current2);
		m2m_cloud_property_publish("Current 2",&sendvalue[0],NULL,NULL,M2M_TRUE);

		sprintf(&sendvalue[0],"%.2f",Analog1);
		m2m_cloud_property_publish("Analog 1",&sendvalue[0],NULL,NULL,M2M_TRUE);

		sprintf(&sendvalue[0],"%.2f",Analog2);
		m2m_cloud_property_publish("Analog 2",&sendvalue[0],NULL,NULL,M2M_TRUE);

		sprintf(&sendvalue[0],"%.2f",MainVoltage);
		m2m_cloud_property_publish("Supply Voltage",&sendvalue[0],NULL,NULL,M2M_TRUE);

		sprintf(&sendvalue[0],"%d",DigitalIN1);
		m2m_cloud_property_publish("Digital IN 1",&sendvalue[0],NULL,NULL,M2M_TRUE);

		sprintf(&sendvalue[0],"%d",DigitalIN2);
		m2m_cloud_property_publish("Digital IN 2",&sendvalue[0],NULL,NULL,M2M_TRUE);

		//function to post position if GPS data is available
		//m2m_cloud_location_publish(50.0433,8.6837,NULL,NULL,NULL,NULL,NULL,NULL,NULL,M2M_TRUE);

		//waiting 30 seconds before tear down
		m2m_os_sleep_ms(30000);

		//dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE, "Disconnecting from cloud..\n");
		m2m_cloud_disconnect(M2M_TRUE);
		//dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE, "Done.\n");

		//dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE, "Disconnecting context..\n");
		m2m_cloud_pdp_context_deactivate();
		//dwPrintDebug(M2M_CLOUD_LOG_LOG, M2M_FALSE, "Done.\n");
	}
			//error in dwstatus
	else
		//dwPrintDebug(M2M_CLOUD_LOG_ERRORS, M2M_FALSE, "Error connecting: %d\n", m2m_cloud_status());


	  return res;
}
void M2M_msgProcCompl ( INT8 procId, INT32 type, INT32 result )
{
  /* write code fulfilling the requirements of the M2M project */
}
//##################################################################################################################################
/**
 *  \brief      Parses the string received on the AUX port and calculates the sensor data
 *
 *  \param       SensorData: a pointer to string that contains the Sensor data
 *
 *  \return         None.
 *
 *
 *  \details The string format is the following:
 * 					- Byte at index 0 is a header and is represented by the character 'A'
 * 					- First and Second bytes represents the 16 bits value read from the Analog1 ADC register.
 * 					  Analog1 value is then computed and value is represented in mV
 * 					- Third and Fourth bytes represents the 16 bits value read from the Analog2 ADC register
 * 					  Analog2 value is then computed and value is represented in mV
 * 					- Fifth and Sixth bytes represents the 16 bits value read from the Current1 Differential ADC register
 * 					  Current1 value is then computed and value is represented in mA
 *	 				- Seventh and Eighth bytes represents the 16 bits value read from the Current2 Differential ADC register
 * 					  Current2 value is then computed and value is represented in mA
 *	 				- Ninth and Tenth bytes represents the 16 bits value read from the Main Voltage ADC register
 * 					  Main Voltage value is then computed and value is represented in V
 * 					- Eleventh byte represents the status of the digital input 1 in ASCII code '0' or '1'
 * 					- Twelfth  byte represents the status of the digital input 2 in ASCII code '0' or '1'
 * 					- Thirteenth byte represents the status of the digital output 1 in ASCII code '0' or '1'
 * 					- Fourteenth byte represents the status of the digital output 2 in ASCII code '0' or '1'
 *
 */
//##################################################################################################################################
void ParseSensorData(UINT8 * SensorData)
{
	Analog1 = SensorData[1] * 256;
	Analog1+= SensorData[2];
	Analog1 = Analog1 * 625 * 122;
	Analog1 = Analog1/22;
	Analog1 = Analog1/10000;

	Analog2 = SensorData[3] * 256;
	Analog2+= SensorData[4];
	Analog2 = Analog2 * 625 * 122;
	Analog2 = Analog2/22;
	Analog2 = Analog2/10000;

	Current1 =  SensorData[5] * 256;
	Current1+= SensorData[6];
	if(Current1>32767)
		Current1 = 65563-  Current1;
	Current1 = Current1 * 625;
	Current1 = Current1 / 10000;
	Current1 = Current1 / 49.9;

	Current2 =  SensorData[7] * 256;
	Current2+= SensorData[8];
	if(Current2>32767)
		Current2 = 65563-  Current1;
	Current2 = Current2 * 625;
	Current2 = Current2 / 10000;
	Current2 = Current2 / 49.9;

	MainVoltage = SensorData[9] * 256;
	MainVoltage += SensorData[10];
	MainVoltage = MainVoltage * 1000 * 19 *18;
	MainVoltage = MainVoltage / 1023;
	MainVoltage = MainVoltage / 10000;

	DigitalIN1 = SensorData[11]-48;
	DigitalIN2 = SensorData[12]-48;
	DigitalOUT1 = SensorData[13]-48;
	DigitalOUT2 = SensorData[14]-48;
	/**/
	SendToUSB("Analog 1= %.2f mV\r\n",Analog1);
	SendToUSB("Analog 2= %.2f mV\r\n",Analog2);
	SendToUSB("Current 1= %.2f mA\r\n",Current1);
	SendToUSB("Current 2= %.2f mA\r\n",Current2);
	SendToUSB("Main Voltage= %.2f V\r\n",MainVoltage);
	SendToUSB("Digital Input 1= %d \r\n",DigitalIN1);
	SendToUSB("Digital Input 2= %d \r\n",DigitalIN2);
	SendToUSB("Digital Output 1= %d \r\n",DigitalOUT1);
	SendToUSB("Digital Output 2= %d \r\n",DigitalOUT2);

}
//##################################################################################################################################
/**
 *  \brief      Initialize and open the AUX UART Port
 *
 *  \param       None.
 *
 *  \return        0 is failure and 1 is success.
 *
 *
 *  \details Unless otherwise specified, the hw_settings should not be changed
 */
//##################################################################################################################################
INT32 InitAuxUART()
{
	global_aux_uart = m2m_hw_uart_aux_open();
	if(M2M_HW_UART_HANDLE_INVALID==global_aux_uart)
	{
		SendToUSB("failed to open AUX UART");
		return 0;
	}
	m2m_os_sleep_ms(1000);
	hw_settings.baudrate = 115200;
	hw_settings.databits = 8;
	hw_settings.flow_ctrl = 0;
	hw_settings.parity = 0;
	m2m_hw_uart_ioctl (global_aux_uart, M2M_HW_UART_IO_HW_OPTIONS_SET,(INT32) &hw_settings);
	m2m_hw_uart_ioctl (global_aux_uart, M2M_HW_UART_TX_BLOCKING_SET,(INT32) M2M_HW_UART_IO_BLOCKING_OFF);
	m2m_hw_uart_ioctl (global_aux_uart, M2M_HW_UART_RX_BLOCKING_SET,(INT32) M2M_HW_UART_IO_BLOCKING_OFF);

	return 1;
}
//##################################################################################################################################
/**
 *  \brief      Extracts the variable value from a comma separated string
 *
 *  \param       	_CompleteBuf:Complete String that contains all variables comma separated.
 *  \param			_BufSize: The length of the complete buffer
 *  \param			_Param: A reference to the pointer where the extracted value should be written
 *  \param   		_StartIndex: The index at which the function should start copying
 *
 *  \return        None.
 *
 *
 *  \details This function start coping the characters/bytes from the complete buffer string  to the Param point
 * 					starting form the index _StartIndex until a comma or a NULL character is reached and then it stops
 */
//##################################################################################################################################
INT32 ConfSetParam(CHAR* _CompleteBuf,INT32 _BufSize ,CHAR* _Param,INT32 _StartIndex)
{
    INT32 _ParamIndex = 0;
    while(_StartIndex<_BufSize)
    {
        if(_CompleteBuf[_StartIndex]==',' || _CompleteBuf[_StartIndex]=='\0' )
        {
            break;
        }
        _Param[_ParamIndex] = _CompleteBuf[_StartIndex++];
        _ParamIndex++;
    }
    _Param[_ParamIndex] = '\0';
    return _StartIndex;
}
//##################################################################################################################################
/**
 *  \brief      Loads configuration data if the the file cell.txt exists on the root directory
 *
 *  \param      None.
 *
 *  \return        None.
 *
 *
 *  \details It is important to have the file comma separated in the correct format:
 * 					M2M_APN,M2M_Username,M2M_Password,APP_Token,TxInterval,EnableSMS,SMSNotificationNumber,EnableEmail,Email_Username,EmailPassword,EmailTo,SMTP_Address,SMTP_PORT
 * 					Tx Interval is number that represents the minutes between Transmission of sensor data to the cloud
 * 					EnableSMS should be either 0 or 1
 * 					EnableEMail should be either 0 or 1
 * 					SMSNotificationNumber should have the format as in the following example +49177.......
 * 					The password must not contain a comma
 */
//##################################################################################################################################
void LoadConfigurationFile()
{
	CHAR  *file_name = "A:\\MOD\\cell.txt";
	M2M_API_RESULT  API_Result;
	M2M_T_FS_HANDLE file_handle = NULL;
	INT32 File_Size = 0;
	INT32 StartIndex = 0;
	file_handle = m2m_fs_open(file_name, M2M_FS_OPEN_READ);
	if(file_handle == NULL)
	{
		SendToUSB("Configurations File Doesn't exist");
		sprintf(&APN[0],"surfo2");
		sprintf(&username_buf[0],"");
		sprintf(&password_buf[0],"");
		//sprintf(&appToken[0],"js4jJnnK4FmNh2QU");
		sprintf(&appToken[0],"jriTTOj5ik942fOh");
		sprintf(&SMSNumber[0],"+4917645680891");
		sprintf(email_server_info.SMTPServer,"SMTP_DEFAULT_SERVER");
		email_server_info.SMTPPORT = 25;
		sprintf(&email_server_info.EmailTo[0],"EMAIL_TO_ADDRESS");
		sprintf(&email_server_info.Password[0],"EMAIl_PASSWORD");
		sprintf(&email_server_info.Username[0],"USERNAME/EMAIL_FROM");
		EnableSMS = 0;
		EnableEmail = 0;
		TxInterval = 2;
	}
	else
	{
		SendToUSB("Loading Configuration data");
		CHAR	TempBuffer[10];
		CHAR 	File_buf[256]; /*General Buffer to  load config file contents*/
		File_Size = m2m_fs_get_size_with_handle(file_handle);
		m2m_fs_read(file_handle,File_buf,File_Size);
		m2m_fs_close(file_handle);
		StartIndex = ConfSetParam(&File_buf[0],File_Size,&APN[0],StartIndex);
		StartIndex = ConfSetParam(&File_buf[0],File_Size,&username_buf[0],StartIndex+1);
		StartIndex = ConfSetParam(&File_buf[0],File_Size,&password_buf[0],StartIndex+1);
		StartIndex = ConfSetParam(&File_buf[0],File_Size,&appToken[0],StartIndex+1);
		StartIndex = ConfSetParam(&File_buf[0],File_Size,&TempBuffer[0],StartIndex+1);
		TxInterval = atoi(TempBuffer);
		StartIndex = ConfSetParam(&File_buf[0],File_Size,&TempBuffer[0],StartIndex+1);
		EnableSMS = atoi(TempBuffer);
		StartIndex = ConfSetParam(&File_buf[0],File_Size,&SMSNumber[0],StartIndex+1);
		StartIndex = ConfSetParam(&File_buf[0],File_Size,&TempBuffer[0],StartIndex+1);
		EnableEmail = atoi(TempBuffer);
		StartIndex = ConfSetParam(&File_buf[0],File_Size,&email_server_info.Username[0],StartIndex+1);
		StartIndex = ConfSetParam(&File_buf[0],File_Size,&email_server_info.Password[0],StartIndex+1);
		StartIndex = ConfSetParam(&File_buf[0],File_Size,&email_server_info.EmailTo[0],StartIndex+1);
		StartIndex = ConfSetParam(&File_buf[0],File_Size,&email_server_info.SMTPServer[0],StartIndex+1);
		StartIndex = ConfSetParam(&File_buf[0],File_Size,&TempBuffer[0],StartIndex+1);
		SendToUSB("SMTP Port Str=%s\r\n",TempBuffer);
		email_server_info.SMTPPORT = atoi(TempBuffer);
	}

	if(TxInterval<2)
		TxInterval = 2;
	SendToUSB("APN=%s\r\n",APN);
	SendToUSB("Username=%s\r\n",username_buf);
	SendToUSB("Password=%s\r\n",password_buf);
	SendToUSB("AppToken=%s\r\n",appToken);
	SendToUSB("Transmit Interval=%d\r\n",TxInterval);
	SendToUSB("Enable SMS=%d\r\n",EnableSMS);
	SendToUSB("SMS Notification Number=%s\r\n",SMSNumber);
	SendToUSB("Enable Email=%d\r\n",EnableEmail);
	SendToUSB("Email username=%s\r\n",email_server_info.Username);
	SendToUSB("Email password=%s\r\n",email_server_info.Password);
	SendToUSB("Email To=%s\r\n",email_server_info.EmailTo);
	SendToUSB("SMTP Address=%s\r\n",email_server_info.SMTPServer);
	SendToUSB("SMTP Port=%d\r\n",email_server_info.SMTPPORT);


}
//##################################################################################################################################
/**
 *  \brief     Populates the Sensor data into their corresponding variables
 *
 *  \param      None.
 *
 *  \return         None.
 *
 * PRE-CONDITIONS:  None.
 *
 * POST-CONDITIONS: None.
 *
 *  \details This function opens the AUX Port, sends the character 0xAA that indicates that the module is requesting the
 * 					the sensor data. The data is sent back on the AUX port and passed on to the function ParseSensorData to parse it
 */
//##################################################################################################################################
void ReadSensorData()
{
	UINT8 ReadRequest[1];
	ReadRequest[0] = 0xAA;
	INT32	readlen=0;
	INT32 sent=0;
	m2m_os_sleep_ms(300);
	if(InitAuxUART()==0)
	{
		//failed to open AUX UART
		return;
	}

	m2m_hw_uart_write(global_aux_uart,ReadRequest , 1, &sent);
	m2m_os_sleep_ms(100);
	m2m_hw_uart_read(global_aux_uart, &UART_buf[0], 256, &readlen);
	if(readlen>0)
	{
		if(readlen==16)
		{
			ParseSensorData(&UART_buf[0]);
		}
	}
	else
	{
		SendToUSB("No bytes received from the PIC...\r\n");


	}
	m2m_hw_uart_close(global_aux_uart);
	m2m_os_sleep_ms(1000);

}
//##################################################################################################################################
/**
 *  \brief      Sets or clears the selected Digital output pin
 *
 *  \param      PortNumber: supports values 1 and 2.
 *  \param		PortStatus: 0 to clear the port and 1 to set it
 *
 *  \return         0 if wrong values are passed or if the command couldn't be sent.
 * 					1 is success
 *
 *  \details You can verify the state of the digital output pin by reading it from the function
 * 					ReadSensoeData()
 */
//##################################################################################################################################
UINT8 SetDigitalOutput(UINT8 PortNumber, UINT8 PortStatus)
{
	UINT8 WriteRequest[1];
	WriteRequest[0] = 0x00;
	INT32 sent=0;

	if(PortNumber==1)
		WriteRequest[0] = 0x10;
	else
		if(PortNumber==2)
				WriteRequest[0] = 0x20;
		else
		{
			SendToUSB("Incorrect Port Number");
			return 0;
		}
	if(PortStatus==0 || PortStatus==1)
			WriteRequest[0] += PortStatus;
		else
		{
			SendToUSB("Incorrect Port Status");
			return 0;
		}
	InitAuxUART();
	m2m_hw_uart_write(global_aux_uart,WriteRequest , 1, &sent);
	m2m_os_sleep_ms(100);

	m2m_hw_uart_close(global_aux_uart);
	m2m_os_sleep_ms(100);
	if(sent!=1)
	{
		SendToUSB("Failed to send Set Digital Output command over AUX Port");
		return 0;
	}
	return 1;
}
//##################################################################################################################################
/**
 *  \brief      Handles events sent to process 2
 *
 *  \param      type:	event id
 *  \param      param1: addition info
 *  \param      param2: addition info
 *
 *  \return         None.
 *

 *
 *  \details In case of continuous monitoring of the sensor values is needed to fire a trigger based on
 *					a certain value received, activate this task by enabling it in M2M_msgProc1 function
 */
//##################################################################################################################################
INT32 M2M_msgProc2(INT32 type, INT32 param1, INT32 param2)
{
	SendToUSB("Task 2 has started...\r\n");
	UINT8 ReadRequest[1];
	ReadRequest[0] = 0xAA;
	INT32	readlen=0;
	INT32 sent=0;
	InitAuxUART();
	while(TRUE)
	{
		m2m_hw_uart_write(global_aux_uart,ReadRequest , 1, &sent);
		m2m_os_sleep_ms(100);
		m2m_hw_uart_read(global_aux_uart, &UART_buf[0], 256, &readlen);
		if(readlen>0)
		{
			if(readlen==16)
			{

				ParseSensorData(&UART_buf[0]);

			}
		}
		else
		{
			SendToUSB("No bytes received...Closing and reopening the AUX Port\r\n");
			m2m_hw_uart_close(global_aux_uart);
			m2m_os_sleep_ms(1000);
			InitAuxUART();
		}
		m2m_os_sleep_ms(1000);
	}
	return 0;
}
//##################################################################################################################################
/**
 *  \brief      Sends Messages to USB0 for debugging purposes and information exchange
 *
 *  \param      type:	String to be sent with arg
 *  \param      arg...: addition info/parameter
 *
 *  \return         None.
 *
 *
 *  \details This functions always closes the USB port after use in case of multiple uses of the USB port by different functions
 *  		 for 2G/GE910 modules, debug data is sent out to the main UART instead of USB
 */
//##################################################################################################################################
void SendToUSB (const CHAR *fmt, ... )
{


	INT32 Res,Sent;
	M2M_T_HW_USB_HANDLE m_USB_Handle;
	va_list arg;
	CHAR	buf[1024];


	//Lock
	if (NULL == _lock_handle) {
		_lock_handle = m2m_os_lock_init(M2M_OS_LOCK_CS);
	}

	m2m_os_lock_lock(_lock_handle);
	va_start(arg, fmt);
	vsnprintf(buf, sizeof(buf)-1, fmt, arg);

 	if(EnableDebug==1)
 		if(UseUART==0)//if it is a 2G module, then we send the debug messages over UART instead
 		{
 			if(m2m_hw_usb_cable_check()== USB_CABLE_DETACHED)
 			{
 				return;
 			}
			if(m2m_hw_usb_open(USB_CH0,&m_USB_Handle)!=M2M_HW_UART_HANDLE_INVALID)
			{
				m2m_hw_usb_write(m_USB_Handle,buf, strlen(buf),&Sent);
				m2m_hw_usb_write(m_USB_Handle,"\r\n", 2,&Sent);
				m2m_hw_usb_close (m_USB_Handle);
			}
 		}
		else
		{
			M2M_T_HW_UART_HANDLE local_uart = M2M_HW_UART_HANDLE_INVALID;
			local_uart = m2m_hw_uart_open();
			m2m_hw_uart_ioctl (local_uart, M2M_HW_UART_IO_HW_OPTIONS_SET,(INT32) (&hw_settings));
			if (M2M_HW_UART_HANDLE_INVALID != local_uart) {
				m2m_hw_uart_write(local_uart, buf, strlen(buf), &Sent);
				m2m_hw_uart_write(local_uart, "\r\n", 2, &Sent);

				/* in case of concurrency using m2m_hw_uart... comment the next API to avoid the closing */
				m2m_hw_uart_close(local_uart);
			}
		}
	va_end(arg);
	//UnLock
	m2m_os_lock_unlock(_lock_handle);
}
//##################################################################################################################################
/**
 *  \brief      Task that is triggered when an SMS is received
 *
 *  \param      type:	event id
 *  \param      param1: addition info
 *  \param      param2: addition info
 *
 *  \return         None.
 *
 *
 *  \details Add your code to handle and parse the Message received
 */
//##################################################################################################################################
INT32 M2M_msgProc3(INT32 type, INT32 param1, INT32 param2)
{
	CHAR atCom[200] = "";
	char key_word [] = "CONF:";
	INT32 atCommandSend = 0;
	M2M_T_SMS_INFO sms_info;
	m2m_os_iat_set_at_command_instance(1,1);
	m2m_sms_set_text_mode_format();
	m2m_sms_get_text_message(param1, &sms_info);

	SendToUSB("SMS received: \"%s\"\r\n",sms_info.data);

	if (strncmp (sms_info.data, key_word,strlen(key_word)) == 0)
	{
		//example that shows to handle SMS received
	}

  return 0;
}
void SendEmail(CHAR*USEREMAIL, CHAR*USERPASS)
{

}
//##################################################################################################################################
/**
 *  \brief      Function that sends the sensor data via SMS
 *
 *  \param      type:	event id
 *  \param      param1: addition info
 *  \param      param2: addition info
 *
 *  \return         None.
 *
 *
 *  \details It checks if SMS notifications are enabled, after that it attempts to send a message
 *  		 with the sensor data to the defined number
 */
//##################################################################################################################################
void SendSMSNotification()
{
	if(EnableSMS==1)
	{
		SendToUSB("\r\nSending SMS...\r\n");
		CHAR SMS_Body[164];
		sprintf(&SMS_Body[0],"Current1= %.2f mA\r\nCurrent2= %.2f mA\r\nAnalog1= %.2f mV\r\nAnalog2= %.2f mV\r\nDigitalIN1=%d\r\nDigitalIN2=%d",Current1,Current2,Analog1,Analog2,DigitalIN1,DigitalIN2);
		m2m_sms_send_SMS(&SMSNumber[0],&SMS_Body[0]);
		SendToUSB("SMS Sent\r\n");
	}
}
//##################################################################################################################################
/**
 *  \brief      Function that sends the sensor data via Email
 *
 *  \param      EmailInfo:	a struct that has the SMTP server details and account details
 *
 *  \return         None.
 *
 *
 *  \details It checks if Email notifications are enabled, after that it attempts to send an email
 *  		 with the sensor data to the defined email address using a regular TCP socket
 */
//##################################################################################################################################
void SendEmailNotification()
{
	if(EnableEmail==1)
	{
		Setup_Email_Connection(&email_server_info);
	}
}
//##################################################################################################################################
/**
 *  \brief      Sends Messages to the main UART of the module for debugging purposes and/or information exchange
 *
 *  \param      type:	String to be sent with arg
 *  \param      arg...: addition info/parameter
 *
 *  \return         None.
 *
 *
 *  \details If this function is called once, the AT parser will no longer be available on the main UART externally
 */
//##################################################################################################################################

void PrintToUART ( const CHAR *fmt, ... )
{
	INT32	sent;
	va_list arg;
	CHAR	buf[1024];
	M2M_T_HW_UART_HANDLE local_uart = M2M_HW_UART_HANDLE_INVALID;


	//Lock
	if (NULL == _lock_handle) {
		_lock_handle = m2m_os_lock_init(M2M_OS_LOCK_CS);
	}

	m2m_os_lock_lock(_lock_handle);
	va_start(arg, fmt);
	vsnprintf(buf, sizeof(buf)-1, fmt, arg);

		/* Get a UART handle first */
	local_uart = m2m_hw_uart_open();
	m2m_hw_uart_ioctl (local_uart, M2M_HW_UART_IO_HW_OPTIONS_SET,(INT32) (&hw_settings));
	if (M2M_HW_UART_HANDLE_INVALID != local_uart) {
		m2m_hw_uart_write(local_uart, buf, strlen(buf), &sent);
		m2m_hw_uart_write(local_uart, "\r\n", 2, &sent);

		/* in case of concurrency using m2m_hw_uart... comment the next API to avoid the closing */
		m2m_hw_uart_close(local_uart);
	}


	va_end(arg);
	//UnLock
	m2m_os_lock_unlock(_lock_handle);

}
