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
    uint16_t pwrSliderMin;
    uint16_t pwrSliderMax;

    uint16_t onStatusSwitch;
    uint16_t fanStatusSwitch;
    uint16_t glowPlugStatusSwitch;
    uint16_t injectorStatusSwitch;
    uint16_t heatingStatusSwitch;
    uint16_t tempLabel;
    uint16_t voltageLabel;
    uint16_t errorLabel;
    uint16_t stateLabel;

void init() 
{
    ESPUI.begin("FDIK");

    auto mainTab = ESPUI.addControl(Tab, "", "Main");

    auto fanSliderCallback = [](Control* sender, int type) {
        Serial.printf("fanSliderCallback %d\n", type);
        if(type == SL_VALUE) {
            int sliderValue = sender->value.toInt(); // Get the slider value
            heater.setFanSpeed(sliderValue);
        }
    };

    auto pwrSliderCallback = [](Control* sender, int type) {
        Serial.printf("fanSliderCallback %d\n", type);
        if(type == SL_VALUE) {
            int sliderValue = sender->value.toInt(); // Get the slider value
            heater.setPower(sliderValue);
        }
    };



    auto heatingModeClbk = [=](int type, Heater::EMode mode) {
        if(type == B_UP) {
            auto slider = ESPUI.getControl(pwrSlider);

            float minPwr = heater.getMinPower();
            float maxPwr = heater.getMaxPower();
            ESPUI.getControl(pwrSliderMin)->value = isnanf(minPwr) ? "1000" : String(minPwr);
            ESPUI.getControl(pwrSliderMax)->value = isnanf(maxPwr) ? "4600" : String(maxPwr);

            float pwr;
            if(mode == Heater::EMode::Fan) {
                pwr = heater.getFanSpeed();
                slider->callback = fanSliderCallback;
            } else {
                pwr = heater.getPower();
                slider->callback = pwrSliderCallback;
            }

            if(!isnanf(pwr))
                slider->value = int(pwr);

            //slider->enabled = true;
            ESPUI.updateControl(pwrSlider);
            heater.turnOn(mode);
        }
    };

    normalModeBtn = ESPUI.button("Mode", [=](Control* sender, int type) { heatingModeClbk(type, Heater::EMode::Normal);}, ControlColor::Dark, "Normal");

    ESPUI.getControl(normalModeBtn)->parentControl = mainTab;

    primeModeBtn = ESPUI.addControl(ControlType::Button, "", "Prime", ControlColor::None, normalModeBtn, 
                                    [=](Control* sender, int type) { heatingModeClbk(type, Heater::EMode::Prime);});

    fanModeBtn = ESPUI.addControl(ControlType::Button, "", "Fan", ControlColor::None, normalModeBtn, 
                                    [=](Control* sender, int type) { heatingModeClbk(type, Heater::EMode::Fan);});

    stopModeBtn = ESPUI.addControl(ControlType::Button, "", "Stop", ControlColor::Alizarin, normalModeBtn, [](Control* sender, int type) {
        if(type == B_UP) {
            auto slider = ESPUI.getControl(pwrSlider);
            slider->callback = nullptr;
            //slider->enabled = false;
            ESPUI.updateControl(pwrSlider);
            heater.turnOff();
        }
    });


    pwrSlider = ESPUI.addControl(ControlType::Slider, "Speed", String(1000), ControlColor::Dark, normalModeBtn);
    //ESPUI.getControl(pwrSlider)->enabled = false;
    float minPwr = heater.getMinPower();
    float maxPwr = heater.getMaxPower();
    pwrSliderMin = ESPUI.addControl(ControlType::Min, "", isnanf(minPwr) ? "1000" : String(minPwr), None, pwrSlider);
    pwrSliderMax = ESPUI.addControl(ControlType::Max, "", isnanf(maxPwr) ? "4600" : String(maxPwr), None, pwrSlider);


    String clearLabelStyle = "background-color: unset; width: 100%;";
    String switcherLabelStyle = "width: 60px; margin-left: .3rem; margin-right: .3rem; background-color: unset;";

    onStatusSwitch = ESPUI.switcher("Status", nullptr, ControlColor::Dark);
    fanStatusSwitch = ESPUI.switcher("Fan", nullptr, ControlColor::Dark);
    glowPlugStatusSwitch = ESPUI.switcher("Glow plug", nullptr, ControlColor::Dark);
    injectorStatusSwitch = ESPUI.switcher("Injector", nullptr, ControlColor::Dark);
    heatingStatusSwitch = ESPUI.switcher("Heating", nullptr, ControlColor::Dark);

    //ESPUI.getControl(onStatusSwitch)->parentControl = mainTab;
    ESPUI.getControl(fanStatusSwitch)->parentControl = onStatusSwitch;
    ESPUI.getControl(glowPlugStatusSwitch)->parentControl = onStatusSwitch;
    ESPUI.getControl(injectorStatusSwitch)->parentControl = onStatusSwitch;
    ESPUI.getControl(heatingStatusSwitch)->parentControl = onStatusSwitch;    

    ESPUI.setVertical(onStatusSwitch);
    ESPUI.setVertical(fanStatusSwitch);
    ESPUI.setVertical(glowPlugStatusSwitch);
    ESPUI.setVertical(injectorStatusSwitch);
    ESPUI.setVertical(heatingStatusSwitch);

    ESPUI.setElementStyle(ESPUI.addControl(Label, "", "", None, onStatusSwitch), clearLabelStyle);
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", "On", None, onStatusSwitch), switcherLabelStyle);
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", "Fan", None, onStatusSwitch), switcherLabelStyle);
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", "GlowP", None, onStatusSwitch), switcherLabelStyle);
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", "Inject", None, onStatusSwitch), switcherLabelStyle);
    ESPUI.setElementStyle(ESPUI.addControl(Label, "", "Heat", None, onStatusSwitch), switcherLabelStyle);

    errorLabel = ESPUI.addControl(Label, "", "Error", None, onStatusSwitch);
    stateLabel = ESPUI.addControl(Label, "", "State", None, onStatusSwitch);
    voltageLabel = ESPUI.addControl(Label, "", "Voltage", None, onStatusSwitch);
    tempLabel = ESPUI.addControl(Label, "", "Temp", None, onStatusSwitch);
        
    auto serviceTab = ESPUI.addControl(Tab, "", "Service");

    auto resetBtn = ESPUI.button("", [](Control* sender, int type) {heater.reset();}, ControlColor::Alizarin, "Reset");
    ESPUI.getControl(resetBtn)->parentControl = serviceTab;
 
    auto regCtrlId = ESPUI.number("Read reg", [](Control* sender, int type){}, ControlColor::None, 0, 0, 255);
    ESPUI.getControl(regCtrlId)->parentControl = serviceTab;
    auto regBtn = ESPUI.addControl(Button, "", "Read", ControlColor::None, regCtrlId, [](Control* sender, int type) {});
    auto regTxt = ESPUI.addControl(Text, "", "RegVal", ControlColor::None, regCtrlId, [](Control* sender, int type) {});

    ESPUI.getControl(regBtn)->callback = ESPUI.getControl(regCtrlId)->callback = [regTxt, regCtrlId](Control* sender, int type){
        if(type == B_UP || type == N_VALUE) {
            uint8_t buf[16];
            char buf1[16];
            uint8_t reg = ESPUI.getControl(regCtrlId)->value.toInt();
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
        }
    };


    auto pumping = ESPUI.button("Pumping", [](Control* sender, int type) {
        if(type == B_UP) {
            heater.startPumping();
        }
    }, ControlColor::Dark, "Start");
    ESPUI.getControl(pumping)->parentControl = serviceTab;

    ESPUI.addControl(ControlType::Button, "", "Stop", ControlColor::None, pumping, [&](Control* sender, int type) {
        if(type == B_UP) {
            heater.stopPumping();
        }
    });     
}

}