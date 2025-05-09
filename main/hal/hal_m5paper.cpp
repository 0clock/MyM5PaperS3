#include "hal_m5paper.hpp"
#include "hal.hpp"
#include <M5Unified.h>
#include <esp_sleep.h>
#include <sdmmc_cmd.h>
#include <driver/sdmmc_host.h>
#include <fstream>
#include <esp_wifi.h>
#include <esp_sntp.h>
#include <nvs_flash.h>
#include <esp_event.h>
#include <time.h>

void M5PaperDisplay::init()
{
    auto cfg = M5.config();
    cfg.external_speaker.module_display = true;
    cfg.clear_display = false;
    M5.begin(cfg);
    if (M5.Display.width() > M5.Display.height())
    {
        M5.Display.setRotation(M5.Display.getRotation() ^ 1);
    }
    M5.Display.setEpdMode(epd_mode_t::epd_quality);
}

void M5PaperDisplay::setRotation(uint8_t rotation)
{
    M5.Display.setRotation(rotation);
}

void M5PaperDisplay::clear(bool isWhite)
{
    M5.Display.clearDisplay();
    if (isWhite)
    {
        M5.Display.fillScreen(TFT_WHITE);
    }
}

void M5PaperDisplay::drawString(const std::string &text, int32_t x, int32_t y)
{
    M5.Display.setCursor(x, y);
    M5.Display.print(text.c_str());
}

void M5PaperDisplay::fillRect(int32_t x, int32_t y, int32_t w, int32_t h, bool isWhite)
{
    M5.Display.fillRect(x, y, w, h, isWhite ? TFT_WHITE : TFT_BLACK);
}

void M5PaperDisplay::setTextSize(uint8_t size)
{
    M5.Display.setTextSize(size);
}

void M5PaperDisplay::setFont(FontType font)
{
    switch (font)
    {
    case FontType::DEJAVU_18:
        M5.Display.setFont(&fonts::DejaVu18);
        break;
    case FontType::DEJAVU_72:
        M5.Display.setFont(&fonts::DejaVu72);
        break;
    }
}

void M5PaperDisplay::update(DisplayUpdateMode mode)
{
    // M5PaperS3 墨水屏无需显式调用 update
}

void M5PaperDisplay::startWrite()
{
    M5.Display.startWrite();
}

void M5PaperDisplay::endWrite()
{
    M5.Display.endWrite();
}

void M5PaperTouch::init()
{
    M5.begin();
}

bool M5PaperTouch::isTouched()
{
    M5.update();
    // return M5.Touch.ispressed();
    return 0;
}

TouchPoint M5PaperTouch::getTouchPoint()
{
    M5.update();
    auto detail = M5.Touch.getDetail();
    return {detail.x, detail.y};
}

void M5PaperButton::init()
{
    M5.begin();
}

bool M5PaperButton::wasPressed()
{
    M5.update();
    return M5.BtnPWR.wasPressed();
}

void M5PaperRTC::init()
{
    M5.begin();
}

DateTime M5PaperRTC::getDateTime()
{
    auto dt = M5.Rtc.getDateTime();
    return {
        static_cast<uint16_t>(dt.date.year),
        static_cast<uint8_t>(dt.date.month),
        static_cast<uint8_t>(dt.date.date),
        static_cast<uint8_t>(dt.time.hours),
        static_cast<uint8_t>(dt.time.minutes),
        static_cast<uint8_t>(dt.time.seconds)};
}

void M5PaperRTC::setDateTime(const DateTime &dt)
{
    m5::rtc_datetime_t m5_dt;
    m5_dt.date.year = dt.year;
    m5_dt.date.month = dt.month;
    m5_dt.date.date = dt.day;
    m5_dt.time.hours = dt.hour;
    m5_dt.time.minutes = dt.minute;
    m5_dt.time.seconds = dt.second;
    M5.Rtc.setDateTime(&m5_dt);
}

bool M5PaperRTC::isEnabled()
{
    return M5.Rtc.isEnabled();
}

void M5PaperBattery::init()
{
    M5.begin();
}

uint8_t M5PaperBattery::getLevel()
{
    return M5.Power.getBatteryLevel();
}

bool M5PaperBattery::isCharging()
{
    return M5.Power.isCharging();
}

void M5PaperPower::init()
{
    M5.begin();
}

void M5PaperPower::timerSleep(uint32_t seconds)
{
    M5.Power.timerSleep(seconds);
}

void M5PaperPower::powerOff()
{
    M5.Power.powerOff();
}

void M5PaperPower::enableTouchWakeup()
{
    // TODO: 配置 GT911 触摸中断唤醒
}

void M5PaperPower::enableButtonWakeup()
{
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_15, 0); // PWR 键
}

void M5PaperNetwork::init()
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
}

bool M5PaperNetwork::connect(const std::string &ssid, const std::string &password)
{
    wifi_config_t wifi_config = {};
    strncpy((char *)wifi_config.sta.ssid, ssid.c_str(), sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, password.c_str(), sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    esp_err_t ret = esp_wifi_connect();
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    return ret == ESP_OK;
}

bool M5PaperNetwork::syncNtp(const std::string &server)
{
    esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, server.c_str());
    esp_sntp_init();

    setenv("TZ", "CST-8", 1);
    tzset();

    time_t now = 0;
    struct tm timeinfo = {};
    int retry = 0;
    const int retry_count = 10;

    while (timeinfo.tm_year < (2022 - 1900) && ++retry < retry_count)
    {
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }

    if (timeinfo.tm_year < (2022 - 1900))
    {
        return false;
    }

    DateTime dt;
    dt.year = timeinfo.tm_year + 1900;
    dt.month = timeinfo.tm_mon + 1;
    dt.day = timeinfo.tm_mday;
    dt.hour = timeinfo.tm_hour;
    dt.minute = dt.minute;
    dt.second = timeinfo.tm_sec;

    M5PaperRTC rtc;
    rtc.setDateTime(dt);
    return true;
}

void M5PaperStorage::init()
{
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 1;
    esp_err_t ret = sdmmc_host_init();
    if (ret != ESP_OK)
    {
        printf("SD card init failed\n");
    }
}

std::vector<std::string> M5PaperStorage::listFiles(const std::string &path)
{
    std::vector<std::string> files;
    // TODO: 实现文件列表读取
    return files;
}

std::string M5PaperStorage::readFile(const std::string &path)
{
    std::ifstream file(path);
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    return content;
}

bool M5PaperStorage::writeFile(const std::string &path, const std::string &content)
{
    std::ofstream file(path);
    if (!file.is_open())
        return false;
    file << content;
    return true;
}
