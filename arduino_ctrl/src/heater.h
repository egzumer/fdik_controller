#include <CRC8.h>
#include <SoftwareSerial.h>
#include <RingBuf.h>

class Heater 
{
  public:
    uint8_t readRegister(uint8_t addr, void *buf, uint8_t size) 
    {
      sendPacket(0x30, &addr, 1);
      
      auto start = millis();
      while(millis() - start < 50) {
        int c = _serial.read();
        if(c != -1) {
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

      _buffer.clear();
      return 0;
    }

    enum class EMode {Normal, Prime, Fan};

    Heater(int commPin) : 
        _serial(commPin, commPin),
        _crc(CRC8_DALLAS_MAXIM_POLYNOME, CRC8_DALLAS_MAXIM_INITIAL, CRC8_DALLAS_MAXIM_XOR_OUT, CRC8_DALLAS_MAXIM_REV_IN, CRC8_DALLAS_MAXIM_REV_OUT)
    {
      
    }

    void begin()
    {
      _serial.enableTxGPIOOpenDrain(true);
      _serial.begin(9600);
    }

    void loop()
    {
      if(_heartbeatEnabled && millis() - _lastHeartbeat > _heartbeatInteval) {
        _lastHeartbeat = millis();

        //sendHeartbeat();
      }
      if(millis() - _lastRead > _readInteval) {
        _lastRead = millis();

        // float v = readVoltage();
        // Serial.println(v);

        // auto act = readCompAct();
        // if(act.fan) Serial.print("fan ");
        // if(act.glowPlug) Serial.print("glowPlug ");
        // if(act.heating) Serial.print("heating ");
        // if(act.injector) Serial.print("injector ");
        // if(act.turnedOn) Serial.print("turnedOn ");
        // if(act.invalid) Serial.print("invalid ");
        // Serial.println();
      }
    }

    void turnOn(EMode mode)
    {
      writeInt8(0x01, 0x02, int(mode));
      writeInt8(0x01, 0x02, int(mode));
      writeInt8(0x01, 0x02, int(mode));
      writeInt8(0x01, 0x03, 1);
      _heartbeatEnabled = true;
    }

    void turnOff()
    {
      writeInt8(0x01, 0x03, 0);
      writeInt8(0x01, 0x03, 0);
      writeInt8(0x01, 0x03, 0);
      _heartbeatEnabled = false;
    }

    void startPumping()
    {
      writeInt8(0x01, 0x02, int(EMode::Normal));
      writeInt8(0x01, 0x02, int(EMode::Normal));
      writeInt8(0x01, 0x02, int(EMode::Normal));
      writeFloat(0x02, 0x01, 8);
      writeInt32(0x02, 0x02, 0xaa555a00);
    }

    void stopPumping()
    {
      writeInt32(0x02, 0x02, 0);
    }

    void setFanSpeed(uint32_t speed)
    {
      writeInt32(0x01, 0x05, speed);
    }

    void setPower(float power)
    {
      writeFloat(0x20, 0x01, power);
    }

    void sendRoomTemp(float temp)
    {
      writeFloat(0x20, 0x00, temp);
    }

    float readVoltage()
    {
      float val;
      readRegister(RegSupVoltage, &val, sizeof(val));
      return val;
    }

    float readTemp()
    {
      float val;
      readRegister(RegBurnerTemp, &val, sizeof(val));
      return val;
    }

    uint8_t readError()
    {
      uint8_t val;
      readRegister(RegError, &val, sizeof(val));
      return val;
    }    

    uint32_t readState()
    {
      uint32_t val;
      readRegister(RegState, &val, sizeof(val));
      return val;
    }

  struct CompAct {
    bool glowPlug : 1;
    bool injector : 1;
    bool fan : 1;
    bool heating : 1;
    bool : 1;
    bool turnedOn : 1;
    bool : 2;
    bool invalid: 1;
  };

    CompAct readCompAct()
    {
      //uint32_t act = readRegister(RegCompAct);
      //return *(CompAct*)(&act);
      CompAct compAct;
      readRegister(RegCompAct, &compAct, sizeof(compAct));
      return compAct;
    }


  private:
    enum EReadRegister {  Reg01 = 0x01,
                          RegMaxPower = 0x03,
                          RegMinPower = 0x04,
                          Reg05 = 0x05,
                          RegError = 0x0b,
                          RegState = 0x0c,
                          RegBurnerTemp = 0x20,
                          Reg23 = 0x23,
                          RegSupVoltage = 0x24,
                          RegCompAct = 0x50,
                          Reg66 = 0x66 };

    SoftwareSerial _serial;

    const unsigned _readInteval = 1000;
    unsigned long _lastRead = 0;
    
    bool _heartbeatEnabled = false;
    const unsigned _heartbeatInteval = 500;
    unsigned long _lastHeartbeat = 0;

    RingBuf<uint8_t, 16> _buffer;
    CRC8 _crc;

    unsigned long _lastSendPacket = 0;

    void sendPacket(uint16_t cmd, const void *data, uint8_t size)
    {
      _crc.restart();
      _crc.add(0xa5);
      _crc.add((uint8_t*)&cmd, sizeof(cmd));
      _crc.add(size);
      _crc.add((uint8_t*)data, size);

      while(millis() - _lastSendPacket < 15);
      _serial.enableTx(true);
      _serial.write(0xa5);
      _serial.write((uint8_t*)&cmd, sizeof(cmd));
      _serial.write(size);
      _serial.write((uint8_t*)data, size);
      _serial.write(_crc.calc());
      _serial.enableTx(false);
      _lastSendPacket = millis();
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