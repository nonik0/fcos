#pragma once

#include <default_display.hpp>

class Clock : public DefaultDisplay {
  private:
    bool m_blinkerRunning{false};
    ElapsedTime m_blinkerTimer;

  public:
    Clock(std::shared_ptr<Rtc> rtc, std::shared_ptr<SunMoon> sunMoon) : DefaultDisplay(rtc, sunMoon) {}

    virtual void Update() override;

  private:
    void DrawDigits(const RgbColor blendColor, char* text, int yPos);
    void DrawClockDigits(const RgbColor blendColor);
    void DrawSunDigits(const RgbColor blendColor);
    void DrawSeparator(const int x, const int y);
};