//Copyright 2021-2024(c) John Sullivan
//github.com/doppelhub/Honda_Insight_LiBCM
//Lithium battery BMS for G1 Honda Insight.  Replaces OEM BCM module.

#include "src/libcm.h"

void setup() 
{
    //getting here takes ~02 milliseconds after poweron reset
    //getting here takes ~16 milliseconds after IMA switch on
    gpio_begin();
    wdt_disable();
    LTC68042configure_initialize();
    Serial.begin(115200); //USB
    METSCI_begin();
    BATTSCI_begin();

    if (gpio_keyStateNow() == GPIO_KEY_ON) { keyOn_coldBootTasks();          }
    else                                   { debugUSB_printWelcomeMessage(); }
    
    heater_begin();
    LiDisplay_begin();
    LiControl_begin();
    eeprom_begin();

    bringupTester_gridcharger(); 
    bringupTester_motherboard();

    wdt_enable(WDTO_2S); //set watchdog reset vector to 2 seconds
}

void loop()
{
    key_stateChangeHandler();
    time_handler();
    temperature_handler();
    SoC_handler();
    fan_handler();
    heater_handler();
    gridCharger_handler();
    buzzer_handler();
    lcdState_handler();
    LiDisplay_handler();
    batteryHistory_handler();
    cellBalance_handler();

    if (key_getSampledState() == KEYSTATE_ON)
    {
        if (eeprom_expirationStatus_get() != FIRMWARE_EXPIRED) { BATTSCI_sendFrames(); } //P1648 when firmware expired

        LTC68042cell_nextVoltages(); //round-robin handler measures QTY3 cell voltages per call
        METSCI_processLatestFrame();
        adc_updateBatteryCurrent();
        vPackSpoof_setVoltage();
        debugUSB_printLatestData();
        LiControl_handler();
    }
    else if (key_getSampledState() == KEYSTATE_OFF)
    {
        if (time_isItTimeToPerformKeyOffTasks() == YES)
        {
            LTC68042cell_acquireAllCellVoltages();
            SoC_updateUsingLatestOpenCircuitVoltage();
            SoC_turnOffLiBCM_ifPackEmpty();
            debugUSB_printLatest_data_gridCharger();
        }
    }

    USB_userInterface_handler();
    wdt_reset(); //Feed watchdog
    blinkLED2(); //Heartbeat
    time_waitForLoopPeriod(); //wait here until next iteration
}
