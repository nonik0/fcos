#include <rest_server.hpp>

RestServer::RestServer(std::shared_ptr<DisplayManager> dispManager, std::shared_ptr<Weather> weather) {
  m_dispManager = dispManager;
  m_weather = weather;

  // WebServer.on needs static ref so we capture this ptr in a lambda
  m_webServer.on("/", [this]() { this->HandleIndex(); });
  m_webServer.on("/display", [this]() { this->HandleDisplay(); });
  m_webServer.on("/weather", [this]() { this->HandleWeather(); });
  m_webServer.begin();
}

void RestServer::Update() {
  m_webServer.handleClient();
}

void RestServer::HandleIndex() {
  m_webServer.send(200, "text/plain", "CC2");
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
  }

  m_webServer.send(200, "text/plain", display ? "on" : "off");
}

void RestServer::HandleWeather() {
  if (!m_webServer.hasArg("forecast")) {
    m_webServer.send(400, "text/plain", "No forecast string provided");
    return;
  }

  // forecast fmt: "LowTmp|HighTmp|HumidPct|ConditionsStr"
  String forecastStr = m_webServer.arg("forecast");

  // split the forecast into its parts
  int split1 = forecastStr.indexOf('|');
  int split2 = forecastStr.indexOf('|', split1 + 1);
  int split3 = forecastStr.indexOf('|', split2 + 1);

  if (split1 == -1 || split2 == -1 || split3 == -1) {
    m_webServer.send(400, "text/plain", "Invalid forecast format");
    return;
  }

  int tempLow = forecastStr.substring(0, split1).toInt();
  int tempHigh = forecastStr.substring(split1 + 1, split2).toInt();
  int humidity = forecastStr.substring(split2 + 1, split3).toInt();
  String conditions = forecastStr.substring(split3 + 1);
  m_weather->UpdateForecast(tempLow, tempHigh, humidity, conditions);

  m_webServer.send(200, "text/plain", forecastStr);
}