/*
 * Copyright (c) 2015-2020, Texas Instruments Incorporated
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

/*
 *  ======== gpiointerrupt.c ========
 */
#include <stdint.h>
#include <stddef.h>

/* Driver Header files */
#include <ti/drivers/GPIO.h>
#include <ti/drivers/Timer.h>


/* Driver configuration */
#include "ti_drivers_config.h"

char state[]= "OK";

#define DOT_DURATION 500000 // 500ms
#define DASH_DURATION 1500000 // 1500ms
#define CHAR_GAP_DURATION (3 * DOT_DURATION) // 3*500ms
#define WORD_GAP_DURATION (7 * DOT_DURATION) // 7*500ms


enum st  {DOT, DASH, PAUSE_BETWEEN_CHARACTERS, PAUSE_BETWEEN_WORDS, SOS, OK};
enum st currentState = SOS;
int sCount = 0;
int oCount = 0;
int SOSId= 1;
int OKId = 1;
int kId = 0;
Timer_Handle timer0;


/* ====== STATE MACHINE========
 *
 *
 *
 *
 *
 */
void stateMachine(Timer_Handle myHandle, int_fast16_t status)
{


        //GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_OFF);
       // GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON);
    switch (currentState) {
        case DOT:
            Timer_setPeriod( myHandle, Timer_PERIOD_US, DOT_DURATION );
            Timer_start(myHandle);
            if (sCount < 3 && SOSId==1)
            {
                GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON);
            }
            if(OKId == 1 && oCount == 1 )
            {
                GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON);
                kId = 1;
            }
            currentState = PAUSE_BETWEEN_CHARACTERS;
            break;
        case DASH:
              Timer_setPeriod( myHandle, Timer_PERIOD_US, DASH_DURATION );
              Timer_start(myHandle);
              GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_ON);

              currentState = PAUSE_BETWEEN_CHARACTERS;
            break;
        case PAUSE_BETWEEN_CHARACTERS:
            Timer_setPeriod( myHandle, Timer_PERIOD_US, CHAR_GAP_DURATION );
            Timer_start(myHandle);
            GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);
            GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_OFF);
            if (sCount < 3  )
            {

                currentState = DOT;
                sCount++;
            }
            else if(sCount > 2 && SOSId == 1 && oCount < 3)
            {
                currentState = DASH;
                oCount++;
                break;
            }
            else if (oCount < 2 && kId == 0 && SOSId != 1 )
            {
                currentState = DOT;
                oCount ++;
                break;
            }
            else if (oCount < 2 && kId == 1 && SOSId != 1 )
            {
                currentState = DASH;
                oCount++;
            }
            else
            {
                currentState = PAUSE_BETWEEN_WORDS;
                sCount = 0;
                oCount =0;
            }
            break;
        case PAUSE_BETWEEN_WORDS:
            Timer_setPeriod( myHandle, Timer_PERIOD_US, WORD_GAP_DURATION );
            Timer_start(myHandle);
            GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);
            GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_OFF);
            if( SOSId == 1 && oCount < 3 )
            {
               currentState = DASH;
            }
            currentState = DOT;
           break;
        case SOS:
            GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);
            GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_OFF);

            currentState = DOT;
            sCount = 0;
            oCount = 0;
            OKId = 0;
            SOSId  = 1;
            kId= 0;
            break;

        case OK:
            GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_OFF);
            GPIO_write(CONFIG_GPIO_LED_1, CONFIG_GPIO_LED_ON);
            currentState = DASH;
            sCount = 0;
            oCount = 0;
            OKId = 1;
            SOSId  = 0;
            break;
    }




}






/*
 * ======== Timer ========
 *
 * Function for Timer
 *
 *
 *
 *
 */
void timerCallback(Timer_Handle myHandle, int_fast16_t status)
{


    stateMachine( myHandle,  status);


}
Timer_Handle initTimer(void)
{


    Timer_Handle timer0;

    Timer_Params params;

    Timer_init();

    Timer_Params_init(&params);

    params.period = 1000000;

    params.periodUnits = Timer_PERIOD_US;

    params.timerMode = Timer_CONTINUOUS_CALLBACK;

    params.timerCallback = timerCallback;






    timer0 = Timer_open(CONFIG_TIMER_0, &params);

    if (timer0 == NULL) {

        /* Failed to initialized timer */

        while (1) {}

    }

    if (Timer_start(timer0) == Timer_STATUS_ERROR) {

        /* Failed to start timer */

        while (1) {}

    }

return timer0;
}


void changeState(){
    if (currentState != OK)
    {
        currentState = OK;
    }else
    {
        currentState = SOS;
    }
}

/*
 *  ======== gpioButtonFxn0 ========
 *  Callback function for the GPIO interrupt on CONFIG_GPIO_BUTTON_0.
 *
 *  Note: GPIO interrupts are cleared prior to invoking callbacks.
 */
void gpioButtonFxn0(uint_least8_t index)
{
    /* Toggle an LED */
    //GPIO_toggle(CONFIG_GPIO_LED_0);
    /* TOGGLE STATE*/

    changeState();

}

/*
 *  ======== gpioButtonFxn1 ========
 *  Callback function for the GPIO interrupt on CONFIG_GPIO_BUTTON_1.
 *  This may not be used for all boards.
 *
 *  Note: GPIO interrupts are cleared prior to invoking callbacks.
 */
void gpioButtonFxn1(uint_least8_t index)
{
    /* Toggle an LED */
    //GPIO_toggle(CONFIG_GPIO_LED_1);
    changeState();
}

/*
 *  ======== mainThread ========
 */
void *mainThread(void *arg0)
{
    /* Call driver init functions */
    GPIO_init();

    Timer_Handle myHandler = initTimer();
    int_fast16_t status =
    /* Configure the LED and button pins */
    GPIO_setConfig(CONFIG_GPIO_LED_0, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_setConfig(CONFIG_GPIO_LED_1, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_setConfig(CONFIG_GPIO_BUTTON_0, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);

    /* Turn on user LED */
    GPIO_write(CONFIG_GPIO_LED_0, CONFIG_GPIO_LED_ON);

    /* Install Button callback */
    GPIO_setCallback(CONFIG_GPIO_BUTTON_0, gpioButtonFxn0);

    /* Enable interrupts */
    GPIO_enableInt(CONFIG_GPIO_BUTTON_0);




        /*
           *  If more than one input pin is available for your device, interrupts
           *  will be enabled on CONFIG_GPIO_BUTTON1.
           */
       while(1){

          if (CONFIG_GPIO_BUTTON_0 != CONFIG_GPIO_BUTTON_1) {
              /* Configure BUTTON1 pin */
              GPIO_setConfig(CONFIG_GPIO_BUTTON_1, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);

              /* Install Button callback */
              GPIO_setCallback(CONFIG_GPIO_BUTTON_1, gpioButtonFxn1);

              GPIO_enableInt(CONFIG_GPIO_BUTTON_1);
          }
       }






    return (NULL);
}
