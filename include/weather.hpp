#pragma once

#include <default_display.hpp>

class Weather : public DefaultDisplay {
  private:
    ElapsedTime m_updateTimer;
    //auto m_updateTask = nullptr;
    WeatherConditions m_conditions{UNKNOWN};
    int8_t m_temperatureLow{32};
    int8_t m_temperatureHigh{99};
    int8_t m_humidity{50};

  public:
    Weather(std::shared_ptr<Rtc> rtc) : DefaultDisplay(rtc) {}

    void UpdateForecast(int8_t tempLow, int8_t tempHigh, int8_t humidity, String type);

    virtual void Update() override;

    void SetAnimator(std::shared_ptr<Animator> anim);

  private:
    void PrepareToSaveSettings();
    void CheckIfWaitingToSaveSettings();
    void LoadSettings();

    void UpdateConditions();
};