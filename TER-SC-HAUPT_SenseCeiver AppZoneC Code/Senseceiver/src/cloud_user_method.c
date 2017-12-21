/*
 * cloud_user_method.c
 *
 *  Created on: 16/gen/2015
 *      Author: FabioPi
 */

#include <stdlib.h>
#include "cloud_user_method.h"


/* ===========================================================================================
*
*	 This source file contains a sample method execution function, callable
*	 from m2m_cloud_method_handler() callback (see m2m_cloud_method_handler.c source file).
*
* ===========================================================================================*/

int exampleMethod(char buffer[DW_FIELDS_NUM][DW_FIELD_LEN])
{
	int intvalue = 0;
	int res, error;
	int execution_result = 0;
	char fieldValue[DW_FIELD_LEN];
	char resp_buf[32];


	if (m2m_cloud_method_searchFieldValue(buffer, "key", fieldValue) > 0){
		intvalue = atoi(fieldValue);
	}
	dwPrintDebug(M2M_CLOUD_LOG_DEBUG, M2M_FALSE, "key: %d\n", intvalue);


	/*================================================================
	*	Execute the method
	*	Assign a value to execution_result variable
	*	....
	*	....
	*	....
	*==================================================================*/

	//was it successful?
	if (execution_result == 1 )
	{
		sprintf(resp_buf, "success.");
		//respond to the cloud service
		res = m2m_cloud_method_send_response( buffer[M2M_CLOUD_METHOD_MSG_METHOD_ID], 0, 1, "completion_var", resp_buf );
	}
	//else (specify an error message, too)
	else
	{
		error = 2;
		//respond to the cloud service
		m2m_cloud_method_send_response( buffer[M2M_CLOUD_METHOD_MSG_METHOD_ID], error, 1, "ERRORE" );
		return -1;
	}


	return res;

}

