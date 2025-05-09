#include "hal.hpp"
#include "hal_m5paper.hpp"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <memory>
#include <esp_log.h>

// 锁屏管理类
class LockScreen
{
public:
    LockScreen(std::unique_ptr<Display> disp, std::unique_ptr<RTC> rtc,
               std::unique_ptr<Battery> bat, std::unique_ptr<Power> pwr,
               std::unique_ptr<Network> net)
        : display(std::move(disp)), rtc(std::move(rtc)), battery(std::move(bat)),
          power(std::move(pwr)), network(std::move(net)), prev_battery(-1) {}

    void begin()
    {
        display->init();
        rtc->init();
        battery->init();
        power->init();
        network->init();
    }

    void displayTest()
    {
        if (!rtc->isEnabled())
        {
            display->drawString("RTC error", 0, 0);
            display->update(DisplayUpdateMode::SLOW);
            return;
        }

        DateTime dt = rtc->getDateTime();
        if (dt.hour == 2 && dt.minute == 0)
        {
            network->connect("YOURSSID", "YOURPASSWD");
            network->syncNtp("pool.ntp.org");
        }

        if (dt.minute % 10 == 0)
        {
            display->clear(true);
        }
        else
        {
            display->startWrite();
            display->fillRect(150 + 165, 150, 50, 60, true);
            display->endWrite();
        }

        display->setFont(FontType::DEJAVU_72);
        display->startWrite();
        char buf[16];
        snprintf(buf, sizeof(buf), "%02d:%02d", dt.hour, dt.minute);
        display->drawString(buf, 150, 150);
        display->endWrite();

        display->setFont(FontType::DEJAVU_18);
        int battery_level = battery->getLevel();
        if (prev_battery != battery_level)
        {
            prev_battery = battery_level;
            display->startWrite();
            char bat_buf[16];
            snprintf(bat_buf, sizeof(bat_buf), "bat:%03d", battery_level);
            display->drawString(bat_buf, 450, 20);
            display->endWrite();
        }

        display->update(DisplayUpdateMode::SLOW);
    }

    void powerDown()
    {
        DateTime dt = rtc->getDateTime();
        uint32_t sleep_seconds = 60;
        if (rtc->isEnabled())
        {
            sleep_seconds = (60 - dt.second) % 60; // 下一分钟
        }
        power->timerSleep(sleep_seconds);
        power->powerOff();
        display->startWrite();
        display->drawString("power off fail!!!", 0, 200);
        display->endWrite();
        display->update(DisplayUpdateMode::SLOW);
    }

private:
    std::unique_ptr<Display> display;
    std::unique_ptr<RTC> rtc;
    std::unique_ptr<Battery> battery;
    std::unique_ptr<Power> power;
    std::unique_ptr<Network> network;
    int prev_battery;
};

extern "C" void app_main()
{
    auto display = std::make_unique<M5PaperDisplay>();
    auto rtc = std::make_unique<M5PaperRTC>();
    auto battery = std::make_unique<M5PaperBattery>();
    auto power = std::make_unique<M5PaperPower>();
    auto network = std::make_unique<M5PaperNetwork>();

    LockScreen lock_screen(std::move(display), std::move(rtc), std::move(battery),
                           std::move(power), std::move(network));
    lock_screen.begin();
    lock_screen.displayTest();
    lock_screen.powerDown();
}