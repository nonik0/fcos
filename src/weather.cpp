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

void Weather::UpdateForecast(int8_t tempLow, int8_t tempHigh, int8_t humidity, String conditions) {
    m_temperatureLow = tempLow;
    m_temperatureHigh = tempHigh;
    m_humidity = humidity;

    conditions.toLowerCase();
    
    // weird ones are probably for Home Assistant forecast conditions strings
    if (conditions == "sunny" || conditions == "clear" || conditions == "clear-night")
        m_conditions = WeatherConditions::SUNNY;
    // TODO: moon at night? with phases??
    else if (conditions == "cloudy" || conditions == "overcast" || conditions == "exceptional" )
        m_conditions = WeatherConditions::CLOUDY;
    else if (conditions == "partlycloudy" || conditions == "partlysunny" || conditions == "partlyovercast")
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
        sprintf(text, "%d\0", m_temperatureHigh);
        if (m_temperatureHigh < 10)
            tempX += 5;
    }
    else {
        tempColor = tempColor.LinearBlend(tempColor, BLUE, 0.3F);
        sprintf(text, "%d\0", m_temperatureLow);
        if (m_temperatureLow < 10)
            tempX += 5;
    }
    m_pixels->DrawText(tempX, 1, text, tempColor, true);
    m_pixels->DrawText(9, 1, "*F",  m_anim->digitColors[0], true); // TODO: need to handle negative and >100 temps

    // draw humidity
    sprintf(text, "%d%%\0", m_humidity);
    m_pixels->DrawText(4, 6, text, m_anim->digitColors[1], true);

    // weather animation cycle increments every 1/3 second, cycles from 0 to 11 every 4 seconds
    int8_t secondPart = 0;
    if (m_rtc->Millis() >= 666)
        secondPart = 2;
    else if (m_rtc->Millis() >= 333)
        secondPart = 1;
    int8_t cycle = 3 * (m_rtc->Second() % 4) + secondPart;
    
    m_pixels->DrawWeatherLEDs(m_conditions, cycle);

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
