#include <M5Unified.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string>
#include "esp_log.h"
#include "esp_sntp.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "time.h"

#define WIFI_SSID "YOUR SSID"
#define WIFI_PASS "YOUR PASSWD"

class EInkDevice
{
public:
  EInkDevice() = default;

  /* 初始化硬件 */
  void begin()
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
    // M5.Display.fillScreen(TFT_WHITE);
  }

  void init_wifi()
  {
    static const char *TAG = "init_wifi";

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {};
    strcpy((char *)wifi_config.sta.ssid, WIFI_SSID);
    strcpy((char *)wifi_config.sta.password, WIFI_PASS);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Connecting to WiFi...");
    ESP_ERROR_CHECK(esp_wifi_connect());
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }

  void syncRtcNtp()
  {
    static const char *TAG = "rtc-sync";

    this->init_wifi();

    ESP_LOGI(TAG, "Starting SNTP...");
    esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();

    setenv("TZ", "CST-8", 1); // 设置中国时区（UTC+8）
    tzset();                  // 应用设置

    time_t now = 0;
    struct tm timeinfo = {};
    int retry = 0;
    const int retry_count = 10;

    while (timeinfo.tm_year < (2022 - 1900) && ++retry < retry_count)
    {
      ESP_LOGI(TAG, "Waiting for time... (%d)", retry);
      vTaskDelay(2000 / portTICK_PERIOD_MS);
      time(&now);
      localtime_r(&now, &timeinfo);
    }

    if (timeinfo.tm_year < (2022 - 1900))
    {
      ESP_LOGE(TAG, "Failed to sync time");
      return;
    }

    m5::rtc_datetime_t dt;
    dt.date.year = timeinfo.tm_year + 1900;
    dt.date.month = timeinfo.tm_mon + 1;
    dt.date.weekDay = timeinfo.tm_wday;

    dt.time.hours = timeinfo.tm_hour;
    dt.time.minutes = timeinfo.tm_min;
    dt.time.seconds = timeinfo.tm_sec;

    M5.Rtc.setDateTime(&dt);

    ESP_LOGI(TAG, "RTC updated: %04d-%02d-%02d %02d:%02d:%02d",
             timeinfo.tm_year + 1900,
             timeinfo.tm_mon + 1,
             timeinfo.tm_mday,
             timeinfo.tm_hour,
             timeinfo.tm_min,
             timeinfo.tm_sec);
  }

  // 显示测试文本
  void displayTest()
  {
    /* 强制刷新*/
    // M5.Display.clearDisplay();
    // M5.Display.fillScreen(TFT_WHITE);
    // M5.Display.fillRect(10,10,20,20,TFT_LIGHTGRAY);

    M5.Display.setFont(&fonts::DejaVu18);
    //------------------- RTC test
    if (M5.Rtc.isEnabled())
    {
      static constexpr const char *const wd[] = {"Sun", "Mon", "Tue", "Wed", "Thr", "Fri", "Sat", "ERR"};
      char buf[32];
      //*
      /// Get the date and time from the RTC and display it.
      m5::rtc_datetime_t dt;
      if (M5.Rtc.getDateTime(&dt))
      {
        if (0 == dt.time.minutes % 10)
        {
          M5.Display.clearDisplay();
        }
        else
        {
          M5.Display.startWrite();
          M5.Display.fillRect(150+165,150,50,60,TFT_WHITE);
          M5.Display.endWrite();
        }
        if (2 == dt.time.hours && 0 == dt.time.minutes)
        {
          this->syncRtcNtp();
        }
        /* Clock */
        M5.Display.setFont(&fonts::DejaVu72);
        M5.Display.startWrite();
        M5.Display.setCursor(150, 150);
        snprintf(buf, 11, "%02d:%02d\n", dt.time.hours, dt.time.minutes);
        M5.Display.printf(buf);

        M5.Display.endWrite();
        M5.Display.setFont(&fonts::DejaVu18);
        /* End Clock*/
      }
      else
      {
        M5.Display.printf("RTC error");
      }
      /*/
      /// In the example above, the date and time are obtained through I2C communication with the RTC.
      /// However, since M5Unified synchronizes the ESP32's internal clock at startup,
      /// it is also possible to get the date and time, as shown in the example below.
      /// ※ Note: that there will be an error of a few seconds per day.
      ///    You may want to call M5.Rtc.setSystemTimeFromRtc() periodically to synchronize.
      auto t = time(nullptr);
      auto time = localtime(&t);
      M5.Display.startWrite();
      snprintf( buf, 30, "%04d/%02d/%02d(%s)"
              , time->tm_year + 1900
              , time->tm_mon + 1
              , time->tm_mday
              , wd[time->tm_wday & 7]
              );
      M5.Display.drawString(buf, M5.Display.width() / 2, 0);
      snprintf( buf, 30, "%02d:%02d:%02d"
              , time->tm_hour
              , time->tm_min
              , time->tm_sec
              );
      M5.Display.drawString(buf, M5.Display.width() / 2, M5.Display.fontHeight());
      M5.Display.endWrite();
      //*/
    }

    /*Get Battery*/
    static int prev_battery = INT_MAX;
    int battery = M5.Power.getBatteryLevel();
    if (prev_battery != battery)
    {
      prev_battery = battery;
      M5.Display.startWrite();
      M5.Display.setCursor(450, 20);
      M5.Display.print("bat:");
      if (battery >= 0)
      {
        M5.Display.printf("%03d", battery);
      }
      else
      {
        M5.Display.print("none");
      }
      M5.Display.endWrite();
    }
  }

  void powerDown()
  {
    m5::rtc_time_t t;
    if (M5.Rtc.getTime(&t))
    {
      if(t.minutes<59)
      {
        t.minutes += 1;
      }
      else
      {
        t.hours +=1;
        t.minutes = 0;
      }
      
      M5.Power.timerSleep(t);
    }
    else
    {
      M5.Power.timerSleep(60);
    }
    M5.Power.powerOff();
    M5.Display.startWrite();
    M5.Display.setCursor(0, 200);
    M5.Display.print("power off fail!!!");
    M5.Display.endWrite();
  }
};

// 主函数
extern "C" void app_main()
{
  EInkDevice device;
  device.begin();
  device.displayTest();
  device.powerDown();
}