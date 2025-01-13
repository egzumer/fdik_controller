#pragma once

#include <stdint.h>

namespace UI {
    extern uint16_t normalModeBtn;
    extern uint16_t primeModeBtn;
    extern uint16_t fanModeBtn;
    extern uint16_t stopModeBtn;
    extern uint16_t pwrSlider;
    extern uint16_t pwrSliderMin;
    extern uint16_t pwrSliderMax;

    extern uint16_t onStatusSwitch;
    extern uint16_t fanStatusSwitch;
    extern uint16_t glowPlugStatusSwitch;
    extern uint16_t injectorStatusSwitch;
    extern uint16_t heatingStatusSwitch;
    extern uint16_t tempLabel;
    extern uint16_t voltageLabel;
    extern uint16_t errorLabel;
    extern uint16_t stateLabel;    

void init();

}
