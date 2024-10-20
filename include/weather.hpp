#pragma once
#include <default_display.hpp>

class Weather : public DefaultDisplay {
  private:
    ElapsedTime m_updateTimer;
    //auto m_updateTask = nullptr;
    WeatherConditions m_conditions{UNKNOWN};
    MoonPhase m_moonPhase{NOT_NIGHT};
    int8_t m_temperatureLow{1};
    int8_t m_temperatureHigh{99};
    uint8_t m_humidity{50};
    uint8_t m_aqi{123};

  public:
    Weather(std::shared_ptr<Rtc> rtc, std::shared_ptr<SunMoon> sunMoon) : DefaultDisplay(rtc, sunMoon) {}

    void UpdateForecast(int8_t tempLow, int8_t tempHigh, uint8_t humidity, uint8_t aqi, String conditions);

    virtual void Update() override;

    void SetAnimator(std::shared_ptr<Animator> anim);

  private:
    void PrepareToSaveSettings();
    void CheckIfWaitingToSaveSettings();
    void LoadSettings();

    void UpdateConditions();
};