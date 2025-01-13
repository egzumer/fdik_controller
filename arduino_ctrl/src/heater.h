#include <CRC8.h>
#include <SoftwareSerial.h>
#include <RingBuf.h>

class Heater 
{
    public:
        struct __attribute__((packed)) CompAct {
        bool glowPlug : 1;
        bool injector : 1;
        bool fan : 1;
        bool heating : 1;
        bool : 1;
        bool turnedOn : 1;
        bool : 2;
        bool invalid: 1;
        };

        enum class EMode {INVALID = -1, Normal, Prime, Fan};
        
        uint8_t readRegister(uint8_t addr, void *buf, uint8_t size) 
        {
            sendPacket(0x30, &addr, 1);
            
            auto start = millis();
            while(millis() - start < 50) {
                int c = _serial.read();
                if(c != -1) {
                    _lastPacketTime = millis();
                    _buffer.push(c);
                    if(_buffer[0] != 0xa0) {
                        uint8_t _;
                        _buffer.pop(_);
                    }

                    uint8_t len = _buffer[3];
                    if(_buffer.size() >= 6 && _buffer.size() == len + 5 && _buffer[1] == 0 && _buffer[2] == 0) {
                        _crc.restart();
                        for(uint8_t i = 0; i < _buffer.size(); i++) {
                            _crc.add(_buffer[i]);
                        }
                            if(!_crc.calc()) {
                            uint32_t val = 0;
                            
                            for(uint8_t i = 0; i < min(len, size); i++) {
                                ((uint8_t *)buf)[i] = _buffer[4 + i];
                            }
                            _buffer.clear();
                            return len;
                        }
                        else {
                            uint8_t _;
                            _buffer.pop(_);
                            while(_buffer.size() && _buffer[0] != 0xa0) {
                                _buffer.pop(_);
                            }
                        }
                    }
                }
            }

            
            
            // static bool f;
            // if(!f) {
            //     f = true;
            //     pinMode(4, OUTPUT);
            // }
            // digitalWrite(4, 1);
            // delay(1);
            // digitalWrite(4, 0);
            // Serial.println("failed");

            // for(uint8_t i = 0; i < _buffer.size(); i++) {
            //     Serial.printf("%02x ", _buffer[i]);
            // }
            // Serial.println();

            _buffer.clear();
            return 0;
        }

        Heater(int commPin) : 
            _serial(commPin, commPin),
            _crc(CRC8_DALLAS_MAXIM_POLYNOME, CRC8_DALLAS_MAXIM_INITIAL, CRC8_DALLAS_MAXIM_XOR_OUT, CRC8_DALLAS_MAXIM_REV_IN, CRC8_DALLAS_MAXIM_REV_OUT)
        {
        
        }

        void begin()
        {
            _serial.enableTxGPIOOpenDrain(true);
            _serial.begin(9600);
            _serial.enableIntTx(false);
        }

        String getVersion()
        {
            char s[32];
            size_t size = readRegister(RegVerString, &s, sizeof(s));

            return String(s, min(size, sizeof(s)));
        }

        // Turn on heating or ventilation
        // Mode::Normal - start normal heating cycle
        // Mode::Prime - runs pump until ignition is detected, 
        // then goes into normal operation. Usefull for filling empty hoses.
        // Mode::Fan - ventilation mode, no heating.
        void turnOn(EMode mode)
        {
            auto err = getError();
            if(err != 255) {
                reset();
                delay(500);
            }
            _currentMode = mode;
            writeInt8(0x01, 0x02, int(mode));
            writeInt8(0x01, 0x02, int(mode));
            writeInt8(0x01, 0x02, int(mode));
            writeInt8(0x01, 0x03, 1);
            _heartbeatEnabled = true;
        }

        // Turn off heating or ventilation.
        void turnOff()
        {
            writeInt8(0x01, 0x03, 0);
            writeInt8(0x01, 0x03, 0);
            writeInt8(0x01, 0x03, 0);
            _heartbeatEnabled = false;
        }

        // Reaset heater internal controller module. Resets error state.
        void reset()
        {
            sendPacket(0x07, nullptr, 0);
            _heartbeatEnabled = false;
        }

        // Quick fuel pumping for filling empty hoses, has to be supervised.
        // Runs until stopPumping() is called. Does not run glow plug or fan.
        // Will flood the heater if left running for too long.
        void startPumping()
        {
            writeInt8(0x01, 0x02, int(EMode::Normal));
            writeInt8(0x01, 0x02, int(EMode::Normal));
            writeInt8(0x01, 0x02, int(EMode::Normal));
            writeFloat(0x02, 0x01, 8);
            writeInt32(0x02, 0x02, 0xaa555a00);
        }

        // Stop pumping
        void stopPumping()
        {
            writeInt32(0x02, 0x02, 0);
        }

        // Set fan speed for ventilation operation mode
        // Min and max value is the same as for normal mode
        // and can be read with getMinPower() and getMaxPower()
        void setFanSpeed(uint32_t speed)
        {
            writeInt32(0x01, 0x05, speed);
        }

        // Get current setting of fan speed for ventilation operation mode
        int32_t getFanSpeed()
        {
            uint32_t val;
            if(readRegister(RegFanSpeed, &val, sizeof(val)))
                return val;
            else
                return -1;
        }   

        // Set heating power level
        // Min and max value can be read with getMinPower() and getMaxPower()
        void setPower(float power)
        {
            writeFloat(0x01, 0x01, power);
        }

        // Get current heating power level
        // returns NAN if failed
        float getPower()
        {
            float val;
            if(readRegister(RegHeaterPwr, &val, sizeof(val)))
                return val;
            else
                return NAN;
        }

        // Get min allowable value for heating power level and fan speed for ventilation mode
        // returns NAN if failed
        float getMinPower()
        {
            float val;
            if(readRegister(RegMinPower, &val, sizeof(val)))
                return val;
            else
                return NAN;
        }

        // Get max allowable value for heating power level and fan speed for ventilation mode
        // returns NAN if failed
        float getMaxPower()
        {
            float val;
            if(readRegister(RegMaxPower, &val, sizeof(val)))
                return val;
            else
                return NAN;
        }

        // Send room temperature to the heater module for temperature regulation
        void sendRoomTemp(float temp)
        {
            writeFloat(0x20, 0x00, temp);
        }

        // Get power supply voltage
        // returns NAN if failed
        float getVoltage()
        {
            float val;
            if(readRegister(RegSupVoltage, &val, sizeof(val)))
                return val;
            else
                return NAN;
        }

        // Get internal heater temperature
        // returns NAN if failed
        float getTemp()
        {
            float val;
            if(readRegister(RegBurnerTemp, &val, sizeof(val)))
                return val;
            return NAN;
        }

        // Get heater error, 255 means no error present
        // returns -1 if failed
        int16_t getError()
        {
            uint8_t val;
            if(readRegister(RegError, &val, sizeof(val)))
                return val;
            else
                return -1;
        }    

        // Get heater state index
        // returns -1 if failed
        int32_t getState()
        {
            uint32_t val;
            if(readRegister(RegState, &val, sizeof(val)))
                return val;
            else
                return -1;
        }

        // Get heater internal components activity status
        // CompAct::invalid is true if failed
        CompAct getCompAct()
        {
            CompAct compAct = {invalid : true};
            if(readRegister(RegCompAct, &compAct, 1))
                compAct.invalid = false;
            return compAct;
        }

        // Get operation mode
        EMode getMode()
        {
            uint8_t val;
            if(readRegister(RegMode, &val, sizeof(val)))
                return EMode(val);
            else
                return EMode(-1);
        }        

        static String stateToString(uint32_t state)
        {
            switch (state) {
                case 1: return "standby";
                case 2: return "fan test";
                case 3: return "first ignition failed";
                case 5: return "glowplug test";
                case 6: return "preignition";
                case 7: return "fuel priming";
                case 9: return "firing up";
                case 10: return "checking ignition";
                case 11: return "heating up";
                case 13: return "stabilized";
                case 17: return "cooldown";
                case 20: return "fan ramp down";
                case 23: return "preparing for fuel filling";
	            case 24: return "fuel filling";
                case 25: return "fuel pump start failed, cooldown";
	            case 26: return "fuel pump start failed, standby";
                case 28: return "ventilating";
                case 30: return "failed ignition shutdown";
                case 31: return "shutdown";
                default:
                    return String(state);
            }
        }

        static String errorToString(uint8_t error) {
            switch (error) {
                case 0: return "control board failure";
                case 1: return "ignition failure";
                case 2: return "flameout";
                case 3: return "supply voltage";
                case 6: return "intake overtemperature";
                case 7: return "oil pump failure";
                case 8: return "fan failure";
                case 9: return "glow plug short";
                case 10: return "overheating";
                case 11: return "temp sensor failure";
                case 12: return "glow plug open";
                case 0xff: return "no error";
                default:
                    return String(error);
            }
        }

        static String modeToString(EMode mode)
        {
            switch (mode) {
                case EMode::Normal: return "Normal";
                case EMode::Prime: return "Prime";
                case EMode::Fan: return "Fan";
                default:
                    return String(int(mode));
            }
        }

    private:
        enum EReadRegister {    
            RegVerString = 0x00,
            RegSetup = 0x01,
            RegMaxPower = 0x03,
            RegMinPower = 0x04,
            Reg05 = 0x05,
            RegError = 0x0b,
            RegState = 0x0c,
            RegBurnerTemp = 0x20,
            Reg23 = 0x23,
            RegSupVoltage = 0x24,
            RegCompAct = 0x50,
            RegHeaterPwr = 0x60,
            RegSetTemp = 0x61,
            RegFanSpeed = 0x62,
            RegMode = 0x66 };

        SoftwareSerial _serial;
        const unsigned _readInteval = 1000;
        unsigned long _lastRead = 0;
        bool _heartbeatEnabled = false;
        const unsigned _heartbeatInteval = 500;
        unsigned long _lastHeartbeat = 0;
        EMode _currentMode;
        RingBuf<uint8_t, 16> _buffer;
        CRC8 _crc;
        unsigned long _lastPacketTime = 0;

        void sendPacket(uint16_t cmd, const void *data, uint8_t size)
        {
            _crc.restart();
            _crc.add(0xa5);
            _crc.add((uint8_t*)&cmd, sizeof(cmd));
            _crc.add(size);
            _crc.add((uint8_t*)data, size);

            while(millis() - _lastPacketTime < 15);
            _serial.enableTx(true);
            _serial.write(0xa5);
            _serial.write((uint8_t*)&cmd, sizeof(cmd));
            _serial.write(size);
            _serial.write((uint8_t*)data, size);
            _serial.write(_crc.calc());
            _serial.enableTx(false);
            _lastPacketTime = millis();
        }

        void writeFloat(uint16_t cmd, uint8_t addr, float val)
        {
            struct {
                uint8_t addr;
                float val;
            } __attribute__((packed)) data = {addr, val};

            sendPacket(cmd, &data, sizeof(data));
        }

        void writeInt32(uint16_t cmd, uint8_t addr, int32_t val)
        {
            struct {
                uint8_t addr;
                int32_t val;
            } __attribute__((packed)) data = {addr, val};

            sendPacket(cmd, &data, sizeof(data));
        }

        void writeInt8(uint16_t cmd, uint8_t addr, int8_t val)
        {
            struct {
                uint8_t addr;
                int8_t val;
            } __attribute__((packed)) data = {addr, val};

            sendPacket(cmd, &data, sizeof(data));
        }


        void sendHeartbeat() {
            writeFloat(0x20, 1, 0);
        }

};