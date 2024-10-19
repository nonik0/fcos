#pragma once

#include <sunset.h>
#include <default_display.hpp>

class Clock : public DefaultDisplay {
  private:
    bool m_blinkerRunning{false};
    ElapsedTime m_blinkerTimer;
    SunSet m_sun;
    int m_curDay{-1};
    int m_sunrise{0};
    int m_sunset{0};

  public:
    Clock(std::shared_ptr<Rtc> rtc) : DefaultDisplay(rtc) {
        m_sun.setPosition(LATITUDE, LONGITUDE, TIMEZONE);
    }

    virtual void Update() override;

  private:
    void DrawDigits(const RgbColor color, char* text, int yPos);
    void DrawClockDigits(const RgbColor color);
    void DrawSunDigits(const RgbColor color);
    void DrawSeparator(const int x, const int y, RgbColor color);
};