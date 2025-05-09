#pragma once
#include <cstdint>
#include <string>
#include <vector>

// 屏幕刷新模式
enum class DisplayUpdateMode
{
    FAST, // 快速刷新
    SLOW  // 慢速刷新
};

// 字体类型
enum class FontType
{
    DEJAVU_18,
    DEJAVU_72
};

// 触摸点信息
struct TouchPoint
{
    int32_t x;
    int32_t y;
};

// 时间和日期
struct DateTime
{
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
};

// 显示接口
class Display
{
public:
    virtual ~Display() = default;
    virtual void init() = 0;
    virtual void setRotation(uint8_t rotation) = 0;
    virtual void clear(bool isWhite = true) = 0;
    virtual void drawString(const std::string &text, int32_t x, int32_t y) = 0;
    virtual void fillRect(int32_t x, int32_t y, int32_t w, int32_t h, bool isWhite) = 0;
    virtual void setTextSize(uint8_t size) = 0;
    virtual void setFont(FontType font) = 0;
    virtual void update(DisplayUpdateMode mode) = 0;
    virtual void startWrite() = 0;
    virtual void endWrite() = 0;
};

// 触摸接口
class Touch
{
public:
    virtual ~Touch() = default;
    virtual void init() = 0;
    virtual bool isTouched() = 0;
    virtual TouchPoint getTouchPoint() = 0;
};

// 按键接口
class Button
{
public:
    virtual ~Button() = default;
    virtual void init() = 0;
    virtual bool wasPressed() = 0;
};

// RTC 接口
class RTC
{
public:
    virtual ~RTC() = default;
    virtual void init() = 0;
    virtual DateTime getDateTime() = 0;
    virtual void setDateTime(const DateTime &dt) = 0;
    virtual bool isEnabled() = 0;
};

// 电池接口
class Battery
{
public:
    virtual ~Battery() = default;
    virtual void init() = 0;
    virtual uint8_t getLevel() = 0; // 返回 0-100
    virtual bool isCharging() = 0;
};

// 低功耗接口
class Power
{
public:
    virtual ~Power() = default;
    virtual void init() = 0;
    virtual void timerSleep(uint32_t seconds) = 0;
    virtual void powerOff() = 0;
    virtual void enableTouchWakeup() = 0;
    virtual void enableButtonWakeup() = 0;
};

// Wi-Fi 和 NTP 接口
class Network
{
public:
    virtual ~Network() = default;
    virtual void init() = 0;
    virtual bool connect(const std::string &ssid, const std::string &password) = 0;
    virtual bool syncNtp(const std::string &server) = 0;
};

// 存储接口（MicroSD）
class Storage
{
public:
    virtual ~Storage() = default;
    virtual void init() = 0;
    virtual std::vector<std::string> listFiles(const std::string &path) = 0;
    virtual std::string readFile(const std::string &path) = 0;
    virtual bool writeFile(const std::string &path, const std::string &content) = 0;
};
