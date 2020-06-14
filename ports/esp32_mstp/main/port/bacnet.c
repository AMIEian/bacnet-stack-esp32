/**************************************************************************
 *
 * Copyright (C) 2011 Steve Karg <skarg@users.sourceforge.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *********************************************************************/
#include <stdint.h>
#include <stdbool.h>
/* hardware layer includes */
// #include "hardware.h"
#include "bacnet/basic/sys/mstimer.h"
#include "rs485.h"
/* BACnet Stack includes */
#include "bacnet/datalink/datalink.h"
#include "bacnet/npdu.h"
#include "bacnet/basic/services.h"
// #include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/dcc.h"
#include "bacnet/iam.h"
/* BACnet objects */
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/object/bo.h"
/* me */
#include "bacnet.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
static const char *TAG = "bacnet.c";

/* timer for device communications control */
static struct mstimer DCC_Timer;
#define DCC_CYCLE_SECONDS 1

void bacnet_init(void)
{
    dlmstp_set_mac_address(4);          // war bei 255
    dlmstp_set_max_master(10);
    /* initialize datalink layer */
    dlmstp_init(NULL);
    /* initialize objects */
    Device_Init(NULL);

    /* set up our confirmed service unrecognized service handler - required! */
    apdu_set_unrecognized_service_handler_handler(handler_unrecognized_service);
    /* we need to handle who-is to support dynamic device binding */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS, handler_who_is);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_HAS, handler_who_has);
    /* Set the handlers for any confirmed services that we support. */
    /* We must implement read property - it's required! */
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROPERTY, handler_read_property);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROP_MULTIPLE, handler_read_property_multiple);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_REINITIALIZE_DEVICE, handler_reinitialize_device);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_WRITE_PROPERTY, handler_write_property);
    /* handle communication so we can shutup when asked */
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL,
        handler_device_communication_control);
    /* start the cyclic 1 second timer for DCC */
    mstimer_set(&DCC_Timer, DCC_CYCLE_SECONDS * 1000);
    /* Hello World! */
    // vTaskDelay(2000/portTICK_PERIOD_MS);
    // ESP_LOGI(TAG, "Send Hello World");
     Send_I_Am(&Handler_Transmit_Buffer[0]);
 
    // ESP_LOGI(TAG, "Send Global WhoIs");
    // Send_WhoIs_Global(0,12);
     
}

static uint8_t PDUBuffer[MAX_MPDU];
void bacnet_task()
{
    uint16_t pdu_len;
    BACNET_ADDRESS src; /* source address */
    // BACNET_BINARY_PV binary_value = BINARY_INACTIVE;
    // BACNET_POLARITY polarity;
    // bool out_of_service;
    struct mstimer Blink_Timer;

    mstimer_set(&Blink_Timer, 125);

    while(true)
    {

        // /* Binary Output */
        // for (uint8_t i = 0; i < MAX_BINARY_OUTPUTS; i++) {
        //     out_of_service = Binary_Output_Out_Of_Service(i);
        //     if (!out_of_service) {
        //         binary_value = Binary_Output_Present_Value(i);
        //         polarity = Binary_Output_Polarity(i);
        //         if (polarity != POLARITY_NORMAL) {
        //             if (binary_value == BINARY_ACTIVE) {
        //                 binary_value = BINARY_INACTIVE;
        //             } else {
        //                 binary_value = BINARY_ACTIVE;
        //             }
        //         }
        //         if (binary_value == BINARY_ACTIVE) {
        //             if (i == 0) {
        //                 /* led_on(LED_2); */
        //             } else {
        //                 /* led_on(LED_3); */
        //             }
        //         } else {
        //             if (i == 0) {
        //                 /* led_off(LED_2); */
        //             } else {
        //                 /* led_off(LED_3); */
        //             }
        //         }
        //     }
        // }

        if (mstimer_expired(&Blink_Timer)) {
            mstimer_reset(&Blink_Timer);
        }

        /* handle the communication timer */
        if (mstimer_expired(&DCC_Timer)) {
            mstimer_reset(&DCC_Timer);
            dcc_timer_seconds(DCC_CYCLE_SECONDS);
        }
        /* handle the messaging */
        // ESP_LOGI(TAG, "Handling the messaging");
        pdu_len = datalink_receive(&src, &PDUBuffer[0], sizeof(PDUBuffer), 0);
        if (pdu_len) {
            npdu_handler(&src, &PDUBuffer[0], pdu_len);
        }
    }
   //  vTaskDelay(5000/ portTICK_PERIOD_MS);
}