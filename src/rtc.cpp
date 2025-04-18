#include <dprint.hpp>
#include <rtc.hpp>

Rtc::Rtc(std::shared_ptr<Settings> settings) : m_settings(settings) {
    GetTimeFromRTC();
    SetupTimezones();

    ElapsedTime::Delay(25);

    // try to get the RTC initialized for up to 50ms
    ElapsedTime initTime;
    while (!m_isInitialized && initTime.Ms() < 50) {
        Update();
        ElapsedTime::Delay(1);
    }
}

bool Rtc::IsInitialized() {
    return m_isInitialized;
}

void Rtc::Update() {
    if (!m_isInitialized) {
        Initialize();
    }

    if (m_receivedInterrupt) {
        // happens once per second
        m_receivedInterrupt = false;
        m_uptime++;
        GetTimeFromRTC();
        // m_millisAtInterrupt = millis();
    }

    CheckNTPTime();
}

void Rtc::SetTime(uint8_t hour, uint8_t minute, uint8_t second) {
    m_rtc.setTime(hour, minute, second);
    GetTimeFromRTC();
}

void Rtc::SetDateTime(uint8_t day,
                      uint8_t month,
                      uint8_t weekday,
                      uint8_t year,
                      uint8_t hour,
                      uint8_t minute,
                      uint8_t second) {
    m_rtc.setDateTime(day, day, month, weekday, year, hour, minute, second);
    GetTimeFromRTC();
}

int Rtc::Conv24to12(int hour) {
    if (hour > 12) {
        hour -= 12;
    } else if (hour == 0) {
        hour = 12;
    }
    return hour;
}

void Rtc::SetClockToZero() {
    m_rtc.zeroClock();
    GetTimeFromRTC();
}

std::vector<String> Rtc::GetTimezoneNames() {
    std::vector<String> names;
    for (const auto& tz : m_timezones) {
        names.push_back(tz.name);
    }
    return names;
}

int Rtc::GetTimezoneNumFromName(const String& name) {
    for (size_t i = 0; i < m_timezones.size(); i++) {
        if (m_timezones[i].name == name) {
            return i;
        }
    }
    return 6;  // UTC 0; always return something valid
}

int Rtc::GetTimezoneUtcOffset() {
    Timezone tz = m_timezones[(*m_settings)["TIMEZONE"]].tz;
    time_t utc = mktime(&m_timeinfo);
    time_t local = tz.toLocal(utc);
    time_t diff = local - utc;
    return diff / 60;
}

void Rtc::ForceNTPUpdate() {
    m_uptimeForNextNTPUpdate = m_uptime;
}

void Rtc::Initialize() {
    // try to set the alarm -- if it succeeds, the RTC is booted and we can
    // setup the 1 second timer
    m_rtc.getDateTime();
    m_rtc.setAlarm(1, 2, 3, 4);
    m_rtc.enableAlarm();
    if (m_rtc.getAlarmMinute() == 1 && m_rtc.getAlarmHour() == 2 &&
        m_rtc.getAlarmDay() == 3 && m_rtc.getAlarmWeekday() == 4) {
        m_rtc.clearAlarm();

        m_rtc.getDateTime();
        if (m_rtc.getTimerValue() != TIMER_FREQUENCY) {
            // Configured at 1Hz, this results in 1 interrupt per second //
            // fresh boot -- no time backup, timer was not enabled
            m_rtc.zeroClock();
        }

        m_rtc.clearStatus();
        m_rtc.clearTimer();
        // interrupt every 1 second
        m_rtc.setTimer(TIMER_FREQUENCY, TMR_1Hz, true);

        m_isInitialized = true;
        AttachInterrupt();

        GetTimeFromRTC();
    }
}

void Rtc::GetTimeFromRTC() {
    unsigned long msAtInterrupt = millis();

    m_rtc.getDateTime();
    if (m_rtc.getHour() < 24) {  // the RTC reports 33 initially
        m_day = m_rtc.getDay();
        m_month = m_rtc.getMonth();
        m_year = m_rtc.getYear();
        m_hour = m_rtc.getHour();
        m_minute = m_rtc.getMinute();
        if (m_second != m_rtc.getSecond()) {
            m_millisAtInterrupt = msAtInterrupt;
            m_second = m_rtc.getSecond();
        }
    }
}

void Rtc::CheckNTPTime() {
    if (WiFi.isConnected() && m_uptime > m_uptimeForNextNTPUpdate) {
        if (GetLocalTime(&m_timeinfo, MAX_WAIT_FOR_NTP_MS)) {
            m_uptimeForNextNTPUpdate = m_uptime + (180 * 60) + (rand() % 120);

            int selectedTimezone =
                GetTimezoneNumFromName((*m_settings)["timezone"]);
            (*m_settings)["TIMEZONE"] = selectedTimezone;
            auto local =
                m_timezones[selectedTimezone].tz.toLocal(mktime(&m_timeinfo));
            m_timeinfo = *localtime(&local);

            SetDateTime(m_timeinfo.tm_mday, m_timeinfo.tm_mon + 1,
                        m_timeinfo.tm_wday, m_timeinfo.tm_year - 100,
                        m_timeinfo.tm_hour, m_timeinfo.tm_min,
                        m_timeinfo.tm_sec);

            TDPRINT(this,
                    "Got NTP time (%02d:%02d:%02d) (TZ: %s) next update in "
                    "~3 hours @ %ds   \n",
                    m_timeinfo.tm_hour, m_timeinfo.tm_min, m_timeinfo.tm_sec,
                    m_timezones[selectedTimezone].name.c_str(),
                    m_uptimeForNextNTPUpdate);
        } else {
            TDPRINT(this, "Failed to get time, retry in ~10s         \n");
            m_uptimeForNextNTPUpdate = m_uptime + (8 + (rand() % 4));
        }
    }
}

bool Rtc::GetLocalTime(struct tm* info, uint32_t ms) {
    uint32_t start = millis();
    time_t now;
    while ((millis() - start) <= ms) {
        time(&now);
        localtime_r(&now, info);
        if (info->tm_year > (2016 - 1900)) {
            return true;
        }
        delay(10);
    }
    return false;
}

void Rtc::SetupTimezones() {
    TimeChangeRule nzDT = {"NZDT", week_t::First, Sun, Sep, 25, 780};
    TimeChangeRule nzST = {"NZST", week_t::First, Sun, Apr, 3, 720};
    TimeChangeRule aEDT = {"AEDT", week_t::First, Sun, Oct, 2, 660};
    TimeChangeRule aEST = {"AEST", week_t::First, Sun, Apr, 3, 600};
    TimeChangeRule aWDT = {"AWDT", week_t::First, Sun, Oct, 2, 540};
    TimeChangeRule aWST = {"AWST", week_t::First, Sun, Apr, 3, 480};
    TimeChangeRule msk = {"MSK", week_t::Last, Sun, Mar, 1, 180};
    TimeChangeRule utcPlus2 = {"UTC+2", week_t::Last, Sun, Oct, 30, 120};
    TimeChangeRule CEST = {"CEST", week_t::Last, Sun, Mar, 27, 120};
    TimeChangeRule CET = {"CET ", week_t::Last, Sun, Oct, 30, 60};
    TimeChangeRule BST = {"BST", week_t::Last, Sun, Mar, 1, 60};
    TimeChangeRule GMT = {"GMT", week_t::Last, Sun, Oct, 2, 0};
    TimeChangeRule utcRule = {"UTC", week_t::Last, Sun, Mar, 1, 0};
    TimeChangeRule utcMinus1 = {"UTC-1", week_t::Last, Sun, Mar, 1, -60};
    TimeChangeRule utcMinus2 = {"UTC-2", week_t::Last, Sun, Mar, 1, -120};
    TimeChangeRule utcMinus3 = {"UTC-3", week_t::Last, Sun, Mar, 1, -180};
    TimeChangeRule usEDT = {"EDT", week_t::Second, Sun, Mar, 12, -240};
    TimeChangeRule usEST = {"EST", First, Sun, Nov, 6, -300};
    TimeChangeRule usCDT = {"CDT", week_t::Second, Sun, Mar, 12, -300};
    TimeChangeRule usCST = {"CST", week_t::First, Sun, Nov, 6, -360};
    TimeChangeRule usMDT = {"MDT", week_t::Second, Sun, Mar, 12, -360};
    TimeChangeRule usMST = {"MST", week_t::First, Sun, Nov, 6, -420};
    TimeChangeRule usPDT = {"PDT", week_t::Second, Sun, Mar, 12, -420};
    TimeChangeRule usPST = {"PST", week_t::First, Sun, Nov, 6, -480};
    TimeChangeRule usAKDT = {"AKDT", week_t::Second, Sun, Mar, 12, -480};
    TimeChangeRule usAKST = {"AKST", week_t::First, Sun, Nov, 6, -540};
    TimeChangeRule usHST = {"HST", week_t::First, Sun, Nov, 6, -600};

    m_timezones.push_back(
        {"UTC +12 New Zealand Daylight Time (DST)", {nzST, nzDT}});
    m_timezones.push_back(
        {"UTC +10 Australia Eastern Time (DST)", {aEDT, aEST}});
    m_timezones.push_back(
        {"UTC +8 Australia Western Time (DST)", {aWDT, aWST}});
    m_timezones.push_back({"UTC +3 Moscow Standard Time (no DST)", {msk}});
    m_timezones.push_back({"UTC +2 (no DST)", {utcPlus2}});
    m_timezones.push_back({"UTC +1 Central European Time (DST)", {CEST, CET}});
    m_timezones.push_back({"UTC 0 London (DST)", {BST, GMT}});
    m_timezones.push_back({"UTC 0", {utcRule}});
    m_timezones.push_back({"UTC -1 (no DST)", {utcMinus1}});
    m_timezones.push_back({"UTC -2 (no DST)", {utcMinus2}});
    m_timezones.push_back({"UTC -3 (no DST)", {utcMinus3}});
    m_timezones.push_back(
        {"UTC -4 Eastern Daylight Time (DST)", {usEDT, usEST}});
    m_timezones.push_back(
        {"UTC -5 Central Daylight Time (DST)", {usCDT, usCST}});
    m_timezones.push_back(
        {"UTC -6 Mountain Daylight Time (DST)", {usMDT, usMST}});
    m_timezones.push_back({"UTC -6 Arizona (no DST)", {usMST}});
    m_timezones.push_back(
        {"UTC -7 Pacific Daylight Time (DST)", {usPDT, usPST}});
    m_timezones.push_back(
        {"UTC -8 Alaska Daylight Time (DST)", {usAKDT, usAKST}});
    m_timezones.push_back({"UTC -10 Hawaii Standard Time (no DST)", {usHST}});

    if (!(*m_settings).containsKey("TIMEZONE")) {
        (*m_settings)["TIMEZONE"] = GetTimezoneNumFromName("UTC 0");
    }
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
}

void Rtc::AttachInterrupt() {
    pinMode(PIN_RTC_INTERRUPT, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_RTC_INTERRUPT), InterruptISR,
                    FALLING);
}

bool Rtc::m_receivedInterrupt = false;

void Rtc::InterruptISR() {
    m_receivedInterrupt = true;
}
