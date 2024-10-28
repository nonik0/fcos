#include <weather.hpp>

// TODO: very rough, need to figure out weather provider 
// void Weather::UpdateConditions(async args) {
//     try {
//         WiFiClient wifiClient;
//         HTTPClient httpClient;
//         String requestUri = "https://api.tomorrow.io/v4/weather/forecast?location=98112&timesteps=daily&apikey=oops";

//         httpClient.begin(wifiClient, requestUri);

//         int httpResponseCode = httpClient.GET();
//         if (httpResponseCode != 200)
//         {
//             return;
//         }

//         String jsonString = httpClient.getString();
//         httpClient.end();

//         DynamicJsonDocument jsonDoc(16 * 1024);
//         DeserializationError jsonError = deserializeJson(jsonDoc, jsonString);
//         if (jsonError)
//         {
//             return;
//         }

//         auto today = jsonDoc["timelines"]["daily"][0]["values"];

//         m_temperatureHigh = today["temperatureMax"].as<int>();
//         m_temperatureLow = today["temperatureMin"].as<int>();

//         // convert C to F
//         m_temperatureHigh = m_temperatureHigh * 9 / 5 + 32;
//         m_temperatureLow = m_temperatureLow * 9 / 5 + 32;

//         int weatherCodeMin = today["weatherCodeMin"].as<int>();
//         int weatherCodeMax = today["weatherCodeMax"].as<int>();

//         // https://docs.tomorrow.io/reference/data-layers-weather-codes
//         if (weatherCodeMin < 2000) {
//             m_weatherType = WeatherType::SUNNY;
//         } else {
//             m_weatherType = WeatherType::RAINY;
//         }
//     } catch (const std::exception& e) {
//     }
// }

void Weather::UpdateForecast(int8_t tempLow, int8_t tempHigh, uint8_t humidity, uint8_t aqi, String conditions) {
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

void Weather::Update() {
    auto wheelPos = (*m_settings)["COLR"].as<uint8_t>();
    RgbColor color = wheelPos;
    m_currentColor = color;
    m_pixels->Darken();
    m_anim->Update();
    if (!m_pixels->IsDarkModeEnabled() || m_pixels->GetBrightness() >= 0.05f) {
        m_pixels->ClearRoundLEDs({0, 0, 0});
    }

    // TODO: for future async weather updates with no rest server for updating forecast
    // if (m_updateTask not active && (m_updateTimer.Ms() >= 1000 * 60 * 10 || m_weatherType == WeatherType::UNKNOWN)) {
    //     m_updateTimer.Reset();
    //     xTaskCreate(UpdateConditions, "UpdateConditions", 4096, NULL, 10, &m_updateTask);
    // }

    // TEST: uncomment to cycle through weather conditions every 8s
    // if (m_updateTimer.Ms() >= 1000 * 8) {
    //     m_updateTimer.Reset();
    //     m_conditions = static_cast<WeatherConditions>((static_cast<int>(m_conditions) + 1) % 10);
    // }

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

    // TODO: reduce the number of calls to these functions more
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

    // if not clear at night, occasionally force clear weather to show moon phase
    WeatherConditions conditionsToShow = m_conditions;
    if (isNight && m_conditions != CLEAR && m_rtc->Second() % 15 > 12) { // show moon phase 3 out of 15 seconds
        conditionsToShow = CLEAR;
    }
    
    m_pixels->DrawWeatherLEDs(conditionsToShow, m_moonPhase, cycle);
    //m_pixels->DrawWeatherLEDs(CLEAR, (MoonPhase)((m_rtc->Second() / 2) % 8), cycle);

    CheckIfWaitingToSaveSettings();
}

void Weather::SetAnimator(std::shared_ptr<Animator> anim) {
    m_anim = anim;
    m_anim->Start();
    m_anim->SetColor((*m_settings)["COLR"].as<uint8_t>());
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
