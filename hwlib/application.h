/******************************************************************************

 @file rfClient.h

 @brief rfClient Header
 *
 *  Created on: Jun 23, 2022
 *
 *****************************************************************************/
#ifndef RFCLIENT_H_
#define RFCLIENT_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#include <stdlib.h>
#include <stdint.h>

#include <ti/drivers/NVS.h>
#include <ti/drivers/UART2.h>
#include <ti/drivers/Timer.h>

#include "commonlib/command/command.h"
#include "commonlib/types/vector.h"

#define HWLIB_VERSION_MAJOR     0
#define HWLIB_VERSION_MINOR     0
#define HWLIB_VERSION_PATCH     1

/* Drivers ***********************************************/
typedef struct {
    uint32_t fd;
    NVS_Handle handle;
    NVS_Params params;
} nvs_control;

typedef struct {
    uint32_t fd;
    UART2_Handle handle;
    NVS_Params params;
} uart_control;

typedef struct {
    uint32_t fd;
    Timer_Handle handle;
} timer_control;
/*********************************************************/

void initialize_gpio();
void initialize_uart(UART2_Handle *cntrl, command *cmd);
void initialize_nvs(nvs_control *cntrl, uint32_t num);
void initialize_timer(timer_control *cntrl, uint32_t num);

/*******************************************************************************
 * DEFINES
 ******************************************************************************/

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/*******************************************************************************
 * FUNCTION PROTOTYPES
 ******************************************************************************/

#endif /* RFCLIENT_H_ */
