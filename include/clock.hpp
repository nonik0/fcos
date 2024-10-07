#pragma once

#include <default_display.hpp>

class Clock : public DefaultDisplay {
  private:
    bool m_blinkerRunning{false};
    ElapsedTime m_blinkerTimer;

  public:
    Clock(std::shared_ptr<Rtc> rtc) : DefaultDisplay(rtc) {}

    virtual void Update() override;

  private:
    void DrawClockDigits(const RgbColor color);
    void DrawSeparator(const int x, RgbColor color);
};