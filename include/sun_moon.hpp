#pragma once

#include <rtc.hpp>
#include <pixels.hpp> // for MoonPhase

class SunMoon {
  private:
    std::shared_ptr<Rtc> m_rtc;
    std::shared_ptr<Settings> m_settings;
    SunSet m_sun;
    
  public:
    SunMoon(std::shared_ptr<Settings> settings, std::shared_ptr<Rtc> rtc) : m_settings(settings), m_rtc(rtc) {}

    int getNextSunrise() {
        m_sun.setPosition(LATITUDE, LONGITUDE, m_rtc->GetTimezoneUtcOffset() / 60);
        m_sun.setCurrentDate(m_rtc->Year(), m_rtc->Month(), m_rtc->Day());

        return m_sun.calcSunrise();
    }

    int getNextSunset() {
        m_sun.setPosition(LATITUDE, LONGITUDE, m_rtc->GetTimezoneUtcOffset() / 60);
        m_sun.setCurrentDate(m_rtc->Year(), m_rtc->Month(), m_rtc->Day());

        return m_sun.calcSunset();
    }

    MoonPhase getMoonPhase() {
      std::tm time_info = {};
      time_info.tm_year = m_rtc->Year() + 100; // years since 1900
      time_info.tm_mon = m_rtc->Month() - 1; // Jan = 0
      time_info.tm_mday = m_rtc->Day(); 
      time_info.tm_hour = m_rtc->Hour();
      time_info.tm_min = m_rtc->Minute();
      time_info.tm_sec = m_rtc->Second();
    
      std::time_t time = std::mktime(&time_info);

      int phase = m_sun.moonPhase(static_cast<int>(time));
      MoonPhase moonPhase;

      //double percentageFull = (std::abs(15 - phase) / 15.0) * 100;

      if (phase == 0 || phase == 29) {
          moonPhase = NEW_MOON;
      } else if (phase >= 1 && phase <= 6) {
          moonPhase = WAXING_CRESCENT;
      } else if (phase == 7) {
          moonPhase = FIRST_QUARTER;
      } else if (phase >= 8 && phase <= 14) {
          moonPhase = WAXING_GIBBOUS;
      } else if (phase == 15) {
          moonPhase = FULL_MOON;
          //percentageFull = 100.0;  // Full moon is 100% full
      } else if (phase >= 16 && phase <= 22) {
          moonPhase = WANING_GIBBOUS;
      } else if (phase == 23) {
          moonPhase = LAST_QUARTER;
      } else if (phase >= 24 && phase <= 28) {
          moonPhase = WANING_CRESCENT;
      }

      return moonPhase;
    }
};
