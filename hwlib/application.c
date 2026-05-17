/*
 * Copyright (c) 2015-2017, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <semaphore.h>
#include <unistd.h>

/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Event.h>
#include <ti/sysbios/knl/Clock.h>

/* TI-RTOS Header files */
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>
#include <ti/display/Display.h>
#include <ti/drivers/GPIO.h>
#include <ti/drivers/rf/RF.h>
#include <ti/display/DisplayExt.h>

/* Board Header files */
#include "ti_drivers_config.h"
#include <ti_radio_config.h>

/* Driverlib Header files */
#include DeviceFamily_constructPath(driverlib/rf_prop_mailbox.h)
#include DeviceFamily_constructPath(driverlib/sys_ctrl.h)
#include DeviceFamily_constructPath(driverlib/cpu.h)

/* Application Header files */
#include "hwlib/application.h"

#include "commonlib/command/command.h"
#include "commonlib/command/handler.h"
#include "commonlib/console/console.h"
#include "commonlib/logger/logger.h"

// __attribute__((section(".aux_data"))) char aux_ram_data;

static uint32_t flags = 0;

#define RESET_FLAG  1 << 0

int32_t cmd_reset(vector *control, cJSON *params, cJSON **result)
{
    int32_t error = CMD_ERROR_NONE;
    flags |= RESET_FLAG;
    return error;
}

int32_t cmd_uart(vector *control, cJSON *params, cJSON **result)
{
    if(control && (control->size > 0)) {
        UART2_Handle handle = (UART2_Handle)vector_get(control, 0);
        char *test = "Hello World\n";
        size_t bytes_written = 0;
        UART2_write(handle, test, strlen(test), &bytes_written);
    }
    return 0;
}

int32_t cmd_version(vector *cntrl, cJSON *params, cJSON **result)
{
    // There is no control object in this case
    (void)cntrl;
    (void)params;

    // Error stuff
    int32_t error = CMD_ERROR_NONE;

    int32_t ret = 0;
    char version_string[64];

    // Hardware library version string
    ret = snprintf(version_string, sizeof(version_string), "%d.%d.%d",
                     HWLIB_VERSION_MAJOR, HWLIB_VERSION_MINOR, HWLIB_VERSION_PATCH);
    if(!error && (ret > 0)) {
        cJSON_AddStringToObject(*result, "hwlib", version_string);
    } else {
        error = CMD_ERROR_CMD_FAILED;
    }


    // cJSON library version string
    ret = snprintf(version_string, sizeof(version_string), "%d.%d.%d",
                     CJSON_VERSION_MAJOR, CJSON_VERSION_MINOR, CJSON_VERSION_PATCH);
    if(!error && (ret > 0)) {
        cJSON_AddStringToObject(*result, "cjson", version_string);
    } else {
        error = CMD_ERROR_CMD_FAILED;
    }

    return error;



}

void heartbeat(Timer_Handle handle, int_fast16_t status)
{
    /* Setup red LED */
    GPIO_toggle(CONFIG_GPIO_RLED);
}

/*
 *  ======== main ========
 */
void *mainThread(void *arg0)
{
    (void)arg0;
    UART2_Handle uartHandle = NULL;
    command uartCommand;
    initialize_uart(&uartHandle, &uartCommand);
    log_init(uartHandle);
    log_set_level(LOG_DEBUG);

    cmd_handler_init();

    command version_command;
    command_init(&version_command, cmd_version, "Reports version numbers");
    command reset_command;
    command_init(&reset_command, cmd_reset, "Resets the MCU");
    cmd_handler_add_cmd("version", &version_command);
    cmd_handler_add_cmd("reset", &reset_command);

    nvs_control nvs_cntrl[CONFIG_TI_DRIVERS_NVS_COUNT] = {
        {CONFIG_NVS_UAPP, NULL, {0}},
        { CONFIG_NVS_ENV, NULL, {0}},
        {CONFIG_NVS_PAPP, NULL, {0}}
    };

    timer_control timer_cntrl[CONFIG_TI_DRIVERS_TIMER_COUNT] = {
        {CONFIG_TIMER_0, NULL}
    };

    initialize_gpio();
    initialize_nvs(nvs_cntrl, CONFIG_TI_DRIVERS_NVS_COUNT);
    initialize_timer(timer_cntrl, CONFIG_TI_DRIVERS_TIMER_COUNT);

    console_handle console_cntrl;
    console_init(&console_cntrl, uartHandle);

    bool done = false;
    while(!done) {
        console_read_input_char(&console_cntrl, 48000);

        if(flags & RESET_FLAG) {
            SysCtrlSystemReset();
        }
    }

    return 0;
}

void initialize_gpio()
{
    /* Setup green LED */
    GPIO_setConfig(CONFIG_GPIO_GLED, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_write(CONFIG_GPIO_GLED, CONFIG_GPIO_LED_OFF);

    /* Setup red LED */
    GPIO_setConfig(CONFIG_GPIO_RLED, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_write(CONFIG_GPIO_RLED, CONFIG_GPIO_LED_OFF);
}

void initialize_uart(UART2_Handle *cntrl, command *cmd)
{
    // Configure the UART handler
    UART2_Params uart_params;
    UART2_Params_init(&uart_params);
    uart_params.baudRate = 115200;
    *cntrl = UART2_open(CONFIG_UART2_0, &uart_params);

    command_init(cmd, cmd_uart, "UART Command Help");
    command_add_control(cmd, (void*)(*cntrl));
    cmd->callback(&(cmd->control), NULL, NULL);
}

void initialize_nvs(nvs_control *cntrl, uint32_t num)
{
    LOG_INFO("Initializing NVS...\n");
    NVS_init();

    for(uint32_t i = 0; i < num; i++) {
        // Initialize parameters
        NVS_Params_init(&(cntrl[i].params));
        // Initialize handle
        cntrl[i].handle = NVS_open(cntrl[i].fd, &(cntrl[i].params));
        if(cntrl[i].handle == NULL) {
            LOG_WARN("Failed to open NVS handle %d\r\n", cntrl[i].fd);
        } else {
            NVS_Attrs attributes;
            NVS_getAttrs(cntrl[i].handle, &attributes);
            LOG_INFO("  NVS Region %d\r\n", cntrl[i].fd);
            LOG_INFO("    Base Address: 0x%x\r\n", attributes.regionBase);
            LOG_INFO("    Sector Size : 0x%x\r\n", attributes.sectorSize);
            LOG_INFO("    Region Size : 0x%x\r\n", attributes.regionSize);
        }
    }

}

void initialize_timer(timer_control *cntrl, uint32_t num)
{
    LOG_INFO("Initializing Timer...\n");
    Timer_init();

    for(uint32_t i = 0; i < num; i++) {
        Timer_Params params;
        Timer_Params_init(&params);
        params.periodUnits   = Timer_PERIOD_US;
        params.period        = 250000;
        params.timerMode     = Timer_CONTINUOUS_CALLBACK;
        params.timerCallback = heartbeat;
        cntrl[i].handle = Timer_open(cntrl[i].fd, &params);
        if(cntrl[i].handle == NULL) {
            LOG_WARN("Failed to open Timer handle %d\r\n", cntrl[i].fd);
        } else {
            Timer_start(cntrl[i].handle);
        }
    }
}
