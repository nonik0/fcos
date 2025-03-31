#include <weather.hpp>

void Weather::Activate() {
    LoadSettings();
    SetAnimator(CreateAnimator(m_pixels, m_settings, m_rtc,
                               (AnimatorType_e)m_animMode));
}

void Weather::Update() {
    auto wheelPos = (*m_settings)["COLR"].as<uint8_t>();
    RgbColor color = wheelPos;
    m_currentColor = color;
    m_pixels->Darken();
    m_anim->Update();
    if (!m_pixels->IsDarkModeEnabled() || m_pixels->GetBrightness() >= 0.05f) {
        m_pixels->ClearRoundLEDs({0, 0, 0});
    }

    // clear forecast data if no recent update
    if (m_sinceLastForecastUpdate.Ms() > 60*1000*30) {
        m_sinceLastForecastUpdate.Reset();
        
        m_conditions = UNKNOWN;
        m_moonPhase = NOT_NIGHT;
        m_temperatureLow = 0;
        m_temperatureHigh = 0;
        m_humidity = 0;
        m_aqi = 0;
    }

    // alternate between showing high and low temperature, shifting colors accordingly to indicate which is being shown
    char text[10];
    RgbColor tempColor = m_anim->digitColors[0];
    int tempX = 1;
    if ((m_rtc->Second() / 6) % 2 == 0) { // swap every 5 seconds
        tempColor = tempColor.LinearBlend(tempColor, RED, 0.4F);
        sprintf(text, "%2d\0", m_temperatureHigh);
    }
    else {
        tempColor = tempColor.LinearBlend(tempColor, BLUE, 0.3F);
        sprintf(text, "%2d\0", m_temperatureLow);
    }
    m_pixels->DrawText(tempX, 1, text, tempColor, true);
    m_pixels->DrawText(9, 1, "*F",  m_anim->digitColors[0], true); // TODO: need to handle negative and >100 temps

    // alternate between humidity and aqi
    if ((m_rtc->Second() / 5) % 2 == 0) { // swap every 6 seconds
        sprintf(text, "%d%%\0", m_humidity);
        m_pixels->DrawText(4, 6, text, m_anim->digitColors[1], true);
    }
    else {
        // draw aqi
        RgbColor aqiColor = m_anim->digitColors[0];
        RgbColor aqiShiftColor = GREEN;
        if (m_aqi > 50)
            aqiShiftColor = YELLOW;
        if (m_aqi > 100)
            aqiShiftColor = ORANGE;
        if (m_aqi > 150)
            aqiShiftColor = RED;
        if (m_aqi > 200)
            aqiShiftColor = PURPLE;
        aqiColor = aqiColor.LinearBlend(aqiColor, aqiShiftColor, 0.4F);

        sprintf(text, "%3d\0", m_aqi);
        m_pixels->DrawText(0, 6, text, aqiColor, true);
        m_pixels->DrawText(12, 6, "Q",  m_anim->digitColors[0], true);
    }

    // weather animation cycle increments every 1/3 second, cycles from 0 to 11 every 4 seconds
    int8_t secondPart = 0;
    if (m_rtc->Millis() >= 666)
        secondPart = 2;
    else if (m_rtc->Millis() >= 333)
        secondPart = 1;
    int8_t cycle = 3 * (m_rtc->Second() % 4) + secondPart;

    // TODO: reduce the number of calls to these functions
    int sunrise = m_sunMoon->getNextSunrise();
    int sunset = m_sunMoon->getNextSunset();
    int minIntoDay = m_rtc->Hour() * 60 + m_rtc->Minute();
    bool isNight = minIntoDay >= sunset || minIntoDay < sunrise;
    if (isNight && m_moonPhase == NOT_NIGHT) {
        m_moonPhase = m_sunMoon->getMoonPhase();
    }
    else if (!isNight && m_moonPhase != NOT_NIGHT) {
        m_moonPhase = NOT_NIGHT;
    }

    // if not clear at night, periodically force clear weather which also shows moon phase
    WeatherConditions conditionsToShow = m_conditions;
    if (isNight && m_conditions != CLEAR && m_rtc->Second() % 15 > 12) { // show moon phase 3 out of 15 seconds
        conditionsToShow = CLEAR;
    }
    
    m_pixels->DrawWeatherLEDs(conditionsToShow, m_moonPhase, cycle);
    //m_pixels->DrawWeatherLEDs(CLEAR, (MoonPhase)((m_rtc->Second() / 2) % 8), cycle);

    CheckIfWaitingToSaveSettings();
}

void Weather::UpdateForecast(int8_t tempLow, int8_t tempHigh, uint8_t humidity, uint8_t aqi, String conditions) {
    m_sinceLastForecastUpdate.Reset();

    m_temperatureLow = tempLow;
    m_temperatureHigh = tempHigh;
    m_humidity = humidity;
    m_aqi = aqi;

    conditions.toLowerCase();

    // weird ones are probably for Home Assistant forecast conditions strings
    if (conditions == "sunny" || conditions == "clear" || conditions == "clear-night")
        m_conditions = WeatherConditions::CLEAR;
    else if (conditions == "cloudy" || conditions == "overcast" || conditions == "exceptional" )
        m_conditions = WeatherConditions::CLOUDY;
    else if (conditions == "partlycloudy" || conditions == "partlyclear" || conditions == "partlysunny" || conditions == "partlyovercast")
        m_conditions = WeatherConditions::PARTLY_CLOUDY;
    else if (conditions == "rain" || conditions == "rainy" || conditions == "pouring")
        m_conditions = WeatherConditions::RAINY;
    else if (conditions == "snowy" || conditions == "snow")
        m_conditions = WeatherConditions::SNOWY;
    else if (conditions == "windy" || conditions == "windy-variant")
        m_conditions = WeatherConditions::WINDY;
    else if (conditions == "thunderstorm" || conditions == "lightning" || conditions == "lightning-rainy")
        m_conditions = WeatherConditions::THUNDERSTORM;
    else if (conditions == "fog" || conditions == "foggy")
        m_conditions = WeatherConditions::FOGGY;
    else if (conditions == "hail")
        m_conditions = WeatherConditions::HAIL;
    else if (conditions == "sleet" || conditions == "snowy-rainy")
        m_conditions = WeatherConditions::SLEET;
    else
        m_conditions = WeatherConditions::UNKNOWN;
}

void Weather::SetAnimator(std::shared_ptr<Animator> anim) {
    m_anim = anim;
    m_anim->Start();
    m_anim->SetColor((*m_settings)["COLR"].as<uint8_t>());
}

void Weather::Up(const Button::Event_e evt) {
    if (evt == Button::PRESS || evt == Button::REPEAT) {
        m_anim->Up();
        PrepareToSaveSettings();
    }
};

void Weather::Down(const Button::Event_e evt) {
    if (evt == Button::PRESS || evt == Button::REPEAT) {
        m_anim->Down();
        PrepareToSaveSettings();
    }
};

bool Weather::Left(const Button::Event_e evt) {
    if (evt == Button::PRESS) {
        m_anim->Left();
        // temporary hack for photos
        // m_rtc->SetTime(12, 34, 00);
    }
    return evt != Button::LONG_PRESS;  // when long pressed, move to SetTime display
};

bool Weather::Right(const Button::Event_e evt) {
    if (evt == Button::PRESS) {
        m_anim->Right();
    }
    return evt != Button::LONG_PRESS;  // when long pressed, move to ConfigMenu display
};

// toggles between configured MINB=user and temporary MINB=0
void Weather::Press(const Button::Event_e evt) {
    static bool toggledDarkMode = false;
    if (evt == Button::RELEASE) {
        if (toggledDarkMode) {
            // don't want to change anim mode when toggling dark mode
            toggledDarkMode = false;
            return;
        }

        if (++m_animMode >= ANIM_TOTAL) {
            m_animMode = 0;
        }
        SetAnimator(CreateAnimator(m_pixels, m_settings, m_rtc,
                                   (AnimatorType_e)m_animMode));
        (*m_settings)["ANIM"] = m_animMode + 1;
        m_pixels->Clear();
        Joystick joy;
#if FCOS_CARDCLOCK2
        uint8_t pos = 0;
        for (int i = 0; i < 0 + (m_anim->name.length() * 4); i++) {
            m_pixels->Clear();
            m_pixels->DrawText(0 - i, 3, m_anim->name, LIGHT_GRAY);
            m_pixels->Show();
            if (joy.WaitForButton(joy.press, 75)) {
                ElapsedTime::Delay(25);
            }
        }
#else
        m_pixels->DrawText(0, 0, (*m_settings)["ANIM"], LIGHT_GRAY);
        m_pixels->Show();
        joy.WaitForButton(joy.press, 500);
#endif
        joy.WaitForNoButtonsPressed();
        PrepareToSaveSettings();
    } else if (evt == Button::LONG_PRESS) {
        m_pixels->ToggleDarkMode();
        toggledDarkMode = true;
    }
}

void Weather::PrepareToSaveSettings() {
    m_shouldSaveSettings = true;
    m_waitingToSaveSettings.Reset();
}

void Weather::CheckIfWaitingToSaveSettings() {
    if (m_shouldSaveSettings && m_waitingToSaveSettings.Ms() >= 2000) {
        // wait until 2 seconds after changing the color to save
        // settings, since the user can quickly change either one and we
        // want to save flash write cycles

        ElapsedTime saveTime;
        m_settings->Save();
        TDPRINT(m_rtc, "Saved settings in %dms                          \n",
                saveTime.Ms());  // usually ~25ms
        m_shouldSaveSettings = false;
    }
}

void Weather::LoadSettings() {
    if (!(*m_settings).containsKey("WLED")) {
        (*m_settings)["WLED"] = "ON";
    }

    if (!(*m_settings).containsKey("ANIM")) {
        (*m_settings)["ANIM"] = m_animMode + 1;
    } else {
        m_animMode = (*m_settings)["ANIM"].as<int>() - 1;
        if (m_animMode >= ANIM_TOTAL) {
            m_animMode = ANIM_NORMAL;
        }
    }
}
