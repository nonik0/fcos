#pragma once

#include <rtc.hpp>
#include <pixels.hpp> // for MoonPhase

class SunMoon {
  private:
    std::shared_ptr<Rtc> m_rtc;
    std::shared_ptr<Settings> m_settings;
    SunSet m_sun;

    // hacK: year and month are never updated, just calculate julian date from day offset
    int m_year{0};
    int m_month{0};
    int m_riseDay{0};
    int m_setDay{0};
    int m_sunrise{0};
    int m_sunset{0};
    
  public:
    SunMoon(std::shared_ptr<Settings> settings, std::shared_ptr<Rtc> rtc) : m_settings(settings), m_rtc(rtc) {
      m_year = m_rtc->Year();
      m_month = m_rtc->Month();
      m_riseDay = m_rtc->Day();
      m_setDay = m_rtc->Day();

      int utcOffset = m_rtc->GetTimezoneUtcOffset() / 60;
      m_sun.setPosition(LATITUDE, LONGITUDE, utcOffset);
      m_sun.setCurrentDate(m_year, m_month, m_riseDay);
    }

    int getNextSunrise() {
        // when past sunrise of current day, increment rise day and recalculate sunrise
        if (m_rtc->Day() == m_riseDay && (m_rtc->Hour() * 60 + m_rtc->Minute()) > m_sunrise) {
          m_sun.setCurrentDate(m_year, m_month, m_riseDay++); // this works at end of month because converted to julian date internally
          m_sunrise = m_sun.calcSunrise();
        }

        return m_sunrise;
    }

    int getNextSunset() {
        // when past sunset of current day, increment set day and recalculate sunset
        if (m_rtc->Day() == m_setDay && (m_rtc->Hour() * 60 + m_rtc->Minute()) > m_sunset) {
          m_setDay++; // sunrise calls setCurrentDate first
          m_sunset = m_sun.calcSunset();
        }

        return m_sunset;
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
