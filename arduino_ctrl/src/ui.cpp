#include "ui.h"

#include <ESPUI.h>
#include "heater.h"

extern Heater heater;

namespace UI {

    uint16_t normalModeBtn;
    uint16_t primeModeBtn;
    uint16_t fanModeBtn;
    uint16_t stopModeBtn;
    uint16_t pwrSlider;

void init() 
{
    ESPUI.begin("FDIK");

     normalModeBtn = ESPUI.button("Mode", [](Control* sender, int type) {
        if(type == B_UP) {
        heater.turnOn(Heater::EMode::Normal);
        }
    }, ControlColor::Dark, "Normal");

    primeModeBtn = ESPUI.addControl(ControlType::Button, "", "Prime", ControlColor::None, normalModeBtn, [&](Control* sender, int type) {
        if(type == B_UP) {
        heater.turnOn(Heater::EMode::Prime);
        }
    });

    fanModeBtn = ESPUI.addControl(ControlType::Button, "", "Fan", ControlColor::None, normalModeBtn, [](Control* sender, int type) {
        if(type == B_UP) {
        heater.turnOn(Heater::EMode::Fan);
        }
    });

    stopModeBtn = ESPUI.addControl(ControlType::Button, "", "Stop", ControlColor::Alizarin, normalModeBtn, [](Control* sender, int type) {
        if(type == B_UP) {
        heater.turnOff();
        }
    });

    pwrSlider = ESPUI.slider("Speed", [](Control* sender, int type) {
        if(type == SL_VALUE) {
        int sliderValue = sender->value.toInt(); // Get the slider value
        heater.setFanSpeed(sliderValue);
        }
    }, ControlColor::Dark, 1000, 1000, 4600);

 
    auto regCtrlId = ESPUI.number("Read reg", [](Control* sender, int type){}, ControlColor::None, 0, 0, 255);
    auto regTxt = ESPUI.addControl(ControlType::Text, "", "RegVal", ControlColor::None, regCtrlId, [](Control* sender, int type) {});

    ESPUI.getControl(regCtrlId)->callback = [regTxt](Control* sender, int type){
            uint8_t buf[16];
            char buf1[16];
            uint8_t reg = sender->value.toInt();
            sprintf(buf1, "0x%02x ", reg);

            uint8_t size = heater.readRegister(reg, buf, sizeof(buf));
            String txt = String(buf1) + " - size: " + String(size) + " / ";
            int32_t intVal = 0;
            for(uint8_t i = 0; i < size; i++) {
                sprintf(buf1, "%02x ", buf[i]);
                txt += buf1;
                intVal += buf[i] << (8*i);
            }
            txt += String(" / ") + intVal;
            if(size == 4) {
                float floatVal = *((float*)&intVal);
                txt += " / " + String(floatVal);
            }
            ESPUI.updateText(regTxt, txt);
    };
    // ESPUI.addControl(ControlType::Button, "", "Read", ControlColor::None, regCtrlId, [regCtrlId, regTxt](Control* sender, int type) {
    //     if(type == B_UP) {
    //         uint8_t buf[16];
    //         Serial.println( ESPUI.getControl(regCtrlId)->value.toInt());
    //         uint8_t size = heater.readRegister(ESPUI.getControl(regCtrlId)->value.toInt(), buf, sizeof(buf));
    //         String txt = "size: " + String(size) + " / ";
    //         int32_t intVal = 0;
    //         for(uint8_t i = 0; i < size; i++) {
    //             char buf1[16];
    //             sprintf(buf1, "%02x ", buf[i]);
    //             txt += buf1;
    //             intVal += buf[i] << (8*i);
    //         }
    //         txt += String(" / ") + intVal;
    //         if(size == 4) {
    //             float floatVal = *((float*)&intVal);
    //             txt += " / " + String(floatVal);
    //         }
    //         ESPUI.updateText(regTxt, txt);
    //     }
    // });

    auto pumping = ESPUI.button("Pumping", [](Control* sender, int type) {
        if(type == B_UP) {
            heater.startPumping();
        }
    }, ControlColor::Dark, "Start");

    ESPUI.addControl(ControlType::Button, "", "Stop", ControlColor::None, pumping, [&](Control* sender, int type) {
        if(type == B_UP) {
            heater.stopPumping();
        }
    });     
}

}