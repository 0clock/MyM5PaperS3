#pragma once
#include "hal.hpp"

// 声明 M5PaperS3 的 HAL 实现类
class M5PaperDisplay : public Display
{
public:
    void init() override;
    void setRotation(uint8_t rotation) override;
    void clear(bool isWhite) override;
    void drawString(const std::string &text, int32_t x, int32_t y) override;
    void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, bool isWhite) override;
    void setTextSize(uint8_t size) override;
    void setFont(FontType font) override;
    void update(DisplayUpdateMode mode) override;
    void startWrite() override;
    void endWrite() override;
};

class M5PaperTouch : public Touch
{
public:
    void init() override;
    bool isTouched() override;
    TouchPoint getTouchPoint() override;
};

class M5PaperButton : public Button
{
public:
    void init() override;
    bool wasPressed() override;
};

class M5PaperRTC : public RTC
{
public:
    void init() override;
    DateTime getDateTime() override;
    void setDateTime(const DateTime &dt) override;
    bool isEnabled() override;
};

class M5PaperBattery : public Battery
{
public:
    void init() override;
    uint8_t getLevel() override;
    bool isCharging() override;
};

class M5PaperPower : public Power
{
public:
    void init() override;
    void timerSleep(uint32_t seconds) override;
    void powerOff() override;
    void enableTouchWakeup() override;
    void enableButtonWakeup() override;
};

class M5PaperNetwork : public Network
{
public:
    void init() override;
    bool connect(const std::string &ssid, const std::string &password) override;
    bool syncNtp(const std::string &server) override;
};

class M5PaperStorage : public Storage
{
public:
    void init() override;
    std::vector<std::string> listFiles(const std::string &path) override;
    std::string readFile(const std::string &path) override;
    bool writeFile(const std::string &path, const std::string &content) override;
};