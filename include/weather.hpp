#pragma once

#include <animators.hpp>
#include <button.hpp>
#include <display.hpp>
#include <dprint.hpp>
#include <elapsed_time.hpp>
#include <pixels.hpp>
#include <rtc.hpp>
#include <set_time.hpp>
#include <settings.hpp>
#include <sun_moon.hpp>

class Weather : public Display {
  private:
    std::shared_ptr<Rtc> m_rtc;
    std::shared_ptr<SunMoon> m_sunMoon;
    WeatherConditions m_conditions{UNKNOWN};
    MoonPhase m_moonPhase{NOT_NIGHT};
    int8_t m_temperatureLow{0};
    int8_t m_temperatureHigh{0};
    uint8_t m_humidity{0};
    uint8_t m_aqi{0};
    size_t m_animMode{0};
    ElapsedTime m_sinceLastForecastUpdate;
    bool m_shouldSaveSettings{false};

    ElapsedTime m_waitingToSaveSettings;

    RgbColor m_currentColor{0};
    std::shared_ptr<Animator> m_anim;

  public:
    Weather(std::shared_ptr<Rtc> rtc, std::shared_ptr<SunMoon> sunMoon)
        : Display(), m_rtc(rtc), m_sunMoon(sunMoon) {}

    virtual void Activate();
    virtual void Update() override;
    void UpdateForecast(int8_t tempLow,
                        int8_t tempHigh,
                        uint8_t humidity,
                        uint8_t aqi,
                        String conditions);

    void SetAnimator(std::shared_ptr<Animator> anim);

  protected:
    virtual void Up(const Button::Event_e evt) override;
    virtual void Down(const Button::Event_e evt) override;
    virtual bool Left(const Button::Event_e evt) override;
    virtual bool Right(const Button::Event_e evt) override;
    virtual void Press(const Button::Event_e evt) override;

  private:
    void PrepareToSaveSettings();
    void CheckIfWaitingToSaveSettings();
    void LoadSettings();

    void UpdateConditions();
};