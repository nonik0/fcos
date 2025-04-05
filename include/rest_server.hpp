#pragma once
#include <WebServer.h>
#include <memory>  // for std::shared_ptr

#include <display.hpp>
#include <weather.hpp>

class RestServer {
  private:
    WebServer m_webServer{80};
    ElapsedTime m_sinceLastWifiCheck;
    unsigned int m_wifiDisconnects{0};  // number of times disconnected from WiFi
    unsigned int m_wifiCheckInterval{60*1000};

    std::shared_ptr<DisplayManager> m_dispManager;
    std::shared_ptr<Rtc> m_rtc;
    std::shared_ptr<SunMoon> m_sunMoon;
    std::shared_ptr<Weather> m_weather;

  public:
    RestServer(std::shared_ptr<DisplayManager> dispManager,
               std::shared_ptr<Rtc> rtc = nullptr,
               std::shared_ptr<SunMoon> sunMoon = nullptr,
               std::shared_ptr<Weather> weather = nullptr);

    void Update();

  private:
    void CheckWifi();

    void HandleIndex();
    void HandleDateTime();
    void HandleDisplay();
    void HandleMessage();
    void HandleMoonPhase();
    void HandleScrollSpeed();
    void HandleWeather();
};
