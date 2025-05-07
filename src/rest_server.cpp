#include <rest_server.hpp>

RestServer::RestServer(std::shared_ptr<DisplayManager> dispManager,
                       std::shared_ptr<Rtc> rtc,
                       std::shared_ptr<SunMoon> sunMoon,
                       std::shared_ptr<Weather> weather) {
    m_dispManager = dispManager;
    m_rtc = rtc;
    m_sunMoon = sunMoon;
    m_weather = weather;

    // WebServer.on needs static ref so we capture this ptr in a lambda
    m_webServer.on("/", [this]() { this->HandleIndex(); });
    m_webServer.on("/datetime", [this]() { this->HandleDateTime(); });
    m_webServer.on("/display", [this]() { this->HandleDisplay(); });
    m_webServer.on("/message", [this]() { this->HandleMessage(0x13); });
    m_webServer.on("/message2", [this]() { this->HandleMessage(0x14); });
    m_webServer.on("/moonphase", [this]() { this->HandleMoonPhase(); });
    m_webServer.on("/scrollspeed", [this]() { this->HandleScrollSpeed(); });
    m_webServer.on("/weather", [this]() { this->HandleWeather(); });
    m_webServer.begin();

    Wire.begin();
}

void RestServer::Update() {
    CheckWifi();
    m_webServer.handleClient();
}

void RestServer::CheckWifi() {
    if (m_sinceLastWifiCheck.Ms() > m_wifiCheckInterval) {
        m_sinceLastWifiCheck.Reset();

        try {
            if (WiFi.status() != WL_CONNECTED) {
                WiFi.disconnect();
                WiFi.reconnect();
                m_wifiDisconnects++;
                log_w("Reconnected to WiFi");
            } else {
                m_wifiCheckInterval = 60 * 1000;
            }
        } catch (const std::exception& e) {
            m_wifiCheckInterval =
                10 * 60 * 1000;  // fallback to default interval on exception
        }
    }
}

void RestServer::HandleIndex() {
    // hardcode for now
    int ipPart = WiFi.localIP()[3];
    switch (ipPart) {
        case 41:
            m_webServer.send(200, "text/plain", "CC2-L");
        case 34:
            m_webServer.send(200, "text/plain", "CC2-R");
        default:
            m_webServer.send(200, "text/plain", "CC2-N");
    }
}

void RestServer::HandleDateTime() {
    if (!m_rtc) {
        m_webServer.send(400, "text/plain", "RTC not available");
        return;
    }

    int sunrise = m_sunMoon->getNextSunrise();
    int sunset = m_sunMoon->getNextSunset();

    char response[500];
    snprintf(response, sizeof(response),
             "Date: 20%d/%02d/%02d\nTime: %02d:%02d:%02d\nTZ Offset: "
             "%d\nSunrise: %02d:%02d\nSunset: %02d:%02d\n",
             m_rtc->Year(), m_rtc->Month(), m_rtc->Day(), m_rtc->Hour(),
             m_rtc->Minute(), m_rtc->Second(), m_rtc->GetTimezoneUtcOffset(),
             sunrise / 60, sunrise % 60, sunset / 60, sunset % 60);

    m_webServer.send(200, "text/plain", response);
}

void RestServer::HandleDisplay() {
    bool display = !m_dispManager->GetBlankingState();

    if (m_webServer.hasArg("plain")) {
        String body = m_webServer.arg("plain");
        body.toLowerCase();

        if (body == "off") {
            display = false;
        } else if (body == "on") {
            display = true;
        } else {
            m_webServer.send(400, "text/plain", body);
            return;
        }

        m_dispManager->SetBlankingState(!display);

        Wire.beginTransmission(0x13);
        Wire.write((uint8_t)0x00);
        Wire.write((uint8_t)display);
        Wire.endTransmission();

        Wire.beginTransmission(0x14);
        Wire.write((uint8_t)0x00);
        Wire.write((uint8_t)display);
        Wire.endTransmission();
    }

    m_webServer.send(200, "text/plain", display ? "on" : "off");
}

void RestServer::HandleMessage(uint8_t addr) {
    String newMessage = "";

    if (m_webServer.hasArg("plain")) {
        newMessage = m_webServer.arg("plain");
    } else if (m_webServer.hasArg("message")) {
        newMessage = m_webServer.arg("message");
    } else if (!m_webServer.hasArg("plain")) {
        m_webServer.send(400, "text/plain", "No message string provided");
        return;
    }

    const size_t chunkSize = 31; // 32-byte, max chunk size for I2C transmission (-1 for end byte)
    size_t messageLength = newMessage.length();
    for (size_t i = 0; i < messageLength; i += chunkSize) {
        Wire.beginTransmission(addr);
        Wire.write((uint8_t)0x01);
  
        size_t remaining = messageLength - i;
        size_t currentChunkSize = remaining > chunkSize ? chunkSize : remaining;
  
        Wire.write((const uint8_t*)newMessage.substring(i, i + currentChunkSize).c_str(), currentChunkSize);
  
        if (i + currentChunkSize >= messageLength) {
          Wire.write('\n');
        }
  
        Wire.endTransmission();
      }

    m_webServer.send(200, "text/plain", newMessage);
}

void RestServer::HandleScrollSpeed() {
    uint8_t speed = 0;
    if (m_webServer.hasArg("plain")) {
        speed = m_webServer.arg("plain").toInt();
    } else if (m_webServer.hasArg("speed")) {
        speed = m_webServer.arg("speed").toInt();
    } else {
        m_webServer.send(400, "text/plain", "No speed string provided");
        return;
    }
    speed = constrain(speed, 0, 100);
    ;

    Wire.beginTransmission(0x13);
    Wire.write((uint8_t)0x02);
    Wire.write(speed);
    Wire.endTransmission();

    m_webServer.send(200, "text/plain", String(speed));
}

void RestServer::HandleMoonPhase() {
    if (!m_sunMoon) {
        m_webServer.send(400, "text/plain", "Moon phase not available");
        return;
    }

    MoonPhase phase = m_sunMoon->getMoonPhase();

    String phaseStr = "Unknown";
    switch (phase) {
        case NEW_MOON:
            phaseStr = "New Moon";
            break;
        case WAXING_CRESCENT:
            phaseStr = "Waxing Crescent";
            break;
        case FIRST_QUARTER:
            phaseStr = "First Quarter";
            break;
        case WAXING_GIBBOUS:
            phaseStr = "Waxing Gibbous";
            break;
        case FULL_MOON:
            phaseStr = "Full Moon";
            break;
        case WANING_GIBBOUS:
            phaseStr = "Waning Gibbous";
            break;
        case LAST_QUARTER:
            phaseStr = "Last Quarter";
            break;
        case WANING_CRESCENT:
            phaseStr = "Waning Crescent";
            break;
    }

    m_webServer.send(200, "text/plain", phaseStr);
}

void RestServer::HandleWeather() {
    if (!m_weather) {
        m_webServer.send(400, "text/plain", "Weather not available");
        return;
    }

    if (!m_webServer.hasArg("forecast")) {
        m_webServer.send(400, "text/plain", "No forecast string provided");
        return;
    }

    // forecast fmt: "LowTmp|HighTmp|HumidPct|AQI|ConditionsStr"
    String forecastStr = m_webServer.arg("forecast");

    // split the forecast into its parts
    int split1 = forecastStr.indexOf('|');
    int split2 = forecastStr.indexOf('|', split1 + 1);
    int split3 = forecastStr.indexOf('|', split2 + 1);
    int split4 = forecastStr.indexOf('|', split3 + 1);

    if (split1 == -1 || split2 == -1 || split3 == -1 || split4 == -1) {
        m_webServer.send(400, "text/plain", "Invalid forecast format");
        return;
    }

    int tempLow = forecastStr.substring(0, split1).toInt();
    int tempHigh = forecastStr.substring(split1 + 1, split2).toInt();
    int humidity = forecastStr.substring(split2 + 1, split3).toInt();
    int aqi = forecastStr.substring(split3 + 1, split4).toInt();
    String conditions = forecastStr.substring(split4 + 1);
    m_weather->UpdateForecast(tempLow, tempHigh, humidity, aqi, conditions);

    m_webServer.send(200, "text/plain", forecastStr);
}