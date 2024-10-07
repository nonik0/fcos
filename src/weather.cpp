#include <weather.hpp>

// void Weather::UpdateConditions() {
//     try {
//         WiFiClient wifiClient;
//         HTTPClient httpClient;
//         String requestUri = "https://api.tomorrow.io/v4/weather/forecast?location=98112&timesteps=daily&apikey=aIZ4ID3MjSTlSgei5t4oNi4lTfQRucl4";

//         httpClient.begin(wifiClient, requestUri);

//         int httpResponseCode = httpClient.GET();
//         if (httpResponseCode != 200)
//         {
//             m_temperatureHigh = httpResponseCode;
//             m_weatherType = WeatherType::RAINY;
//             return;
//         }

//         String jsonString = httpClient.getString();
//         httpClient.end();

//         DynamicJsonDocument jsonDoc(16 * 1024);
//         DeserializationError jsonError = deserializeJson(jsonDoc, jsonString);
//         if (jsonError)
//         {
//             m_temperatureHigh = 2;
//             m_weatherType = WeatherType::RAINY;
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
//         m_temperatureHigh = 3;
//         m_weatherType = WeatherType::RAINY;
//     }
// }

void Weather::UpdateForecast(int8_t tempLow, int8_t tempHigh, String conditions) {
    m_temperatureLow = tempLow;
    m_temperatureHigh = tempHigh;
    
    if (conditions == "sunny" || conditions == "clear-night")
        m_weatherType = WeatherType::SUNNY;
    else if (conditions == "cloudy" || conditions == "exceptional")
        m_weatherType = WeatherType::CLOUDY;
    else if (conditions == "partlycloudy")
        m_weatherType = WeatherType::PARTLY_CLOUDY;
    else if (conditions == "rainy" || conditions == "pouring")
        m_weatherType = WeatherType::RAINY;
    else if (conditions == "snowy")
        m_weatherType = WeatherType::SNOWY;
    else if (conditions == "windy" || conditions == "windy-variant")
        m_weatherType = WeatherType::WINDY;
    else if (conditions == "lightning" || conditions == "lightning-rainy")
        m_weatherType = WeatherType::THUNDERSTORM;
    else if (conditions == "fog")
        m_weatherType = WeatherType::FOGGY;
    else if (conditions == "hail")
        m_weatherType = WeatherType::HAIL;
    else if (conditions == "snowy-rainy")
        m_weatherType = WeatherType::SLEET;
    else
        m_weatherType = WeatherType::UNKNOWN;
}

void Weather::Update() {
    auto wheelPos = (*m_settings)["COLR"].as<uint8_t>();
    RgbColor color = wheelPos;
    m_currentColor = color;
    m_pixels->Darken();
    m_anim->Update();
    if (!m_pixels->IsDarkModeEnabled() || m_pixels->GetBrightness() >= 0.05f) {
        m_pixels->ClearRoundLEDs({1, 1, 0});
    }

    // if (m_updateTimer.Ms() >= 1000 * 60 * 60 || m_weatherType == WeatherType::UNKNOWN) {
    //     m_updateTimer.Reset();
    //     UpdateConditions();
    // }

    char text[10];
    sprintf(text, "%dF\0", m_temperatureHigh);

    m_pixels->DrawText(1, 1, text, m_anim->digitColors[0], true);

    sprintf(text, "%dF\0", m_temperatureLow);
    m_pixels->DrawText(1, 6, text, m_anim->digitColors[1], true);

    int cycle = (m_rtc->Second() % 12 << 1) + (m_rtc->Millis() > 500); // 0-11 in 6s
    m_pixels->DrawWeatherLEDs(m_weatherType, cycle);

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
