#pragma once

#include <default_display.hpp>

class Weather : public DefaultDisplay {
  private:
    ElapsedTime m_updateTimer;
    WeatherType m_weatherType{UNKNOWN};
    int8_t m_temperatureLow;
    int8_t m_temperatureHigh;

  public:
    Weather(std::shared_ptr<Rtc> rtc) : DefaultDisplay(rtc) {}

    void UpdateForecast(int8_t tempLow, int8_t tempHigh, String type);

    virtual void Update() override;

    void SetAnimator(std::shared_ptr<Animator> anim);

  private:
    void PrepareToSaveSettings();
    void CheckIfWaitingToSaveSettings();
    void LoadSettings();

    void UpdateConditions();
};