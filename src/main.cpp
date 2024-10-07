#include <arduino_hal.hpp>
#include <clock.hpp>
#include <config_menu.hpp>
#include <devel_updates.hpp>
#include <display.hpp>
#include <dprint.hpp>
#include <elapsed_time.hpp>
#include <hardware.hpp>
#include <memory>
#include <pixels.hpp>
#include <rtc.hpp>
#include <settings.hpp>
#include <weather.hpp>

std::shared_ptr<Weather> weather;
WebServer restServer(80);
bool display = true;

void restIndex()
{
    restServer.send(200, "text/plain", "test");
}

void restDisplay()
{
  if (restServer.hasArg("plain")) {
    String body = restServer.arg("plain");
    body.toLowerCase();

    if (body == "off") {
      display = false;
    } else if (body == "on") {
      display = true;
    } else {
      restServer.send(400, "text/plain", body);
      return;
    }
  }

  restServer.send(200, "text/plain", display ? "on" : "off");
}

// forecast: "Lo|Hi|Hu|conditions"
void restWeather()
{
  if (!restServer.hasArg("forecast"))
  {
    restServer.send(400, "text/plain", "No forecast provided");
    return;
  }

  String forecastStr = restServer.arg("forecast");
  
  // split the forecast into its parts
  int split1 = forecastStr.indexOf('|');
  int split2 = forecastStr.indexOf('|', split1 + 1);
  int split3 = forecastStr.indexOf('|', split2 + 1);
  
  if (split1 == -1 || split2 == -1 || split3 == -1)
  {
    restServer.send(400, "text/plain", "Invalid forecast format");
    return;
  }

  int tempLow = forecastStr.substring(0, split1).toInt();
  int tempHigh = forecastStr.substring(split1 + 1, split2).toInt();
  int humidity = forecastStr.substring(split2 + 1, split3).toInt();
  String conditions = forecastStr.substring(split3 + 1);
  weather->UpdateForecast(tempLow, tempHigh, humidity, conditions);

  restServer.send(200, "text/plain", forecastStr);
}

void restSetup()
{
    restServer.on("/", restIndex);
    restServer.on("/display", restDisplay);
    restServer.on("/weather", restWeather);
    restServer.begin();
}

void setup() {
    auto settings = std::make_shared<Settings>();
    auto pixels = std::make_shared<Pixels>(settings);
    auto rtc = std::make_shared<Rtc>(settings);
    auto joy = std::make_shared<Joystick>();
    auto develUpdates = std::make_shared<DevelUpdates>(pixels);
    restSetup();

    DoHardwareStartupTests(pixels, settings, rtc, joy);

    // These are the "displays" that are available, but more can be added. The
    // DisplayMgr navigates between them using left/right motions, if the
    // Display allows it (e.g. holding left at the Clock activates SetTime)
    auto displayMgr =
        std::make_shared<DisplayManager>(pixels, settings, rtc, joy);
    
    weather = std::make_shared<Weather>(rtc);
    // rest = std::make_shared<Rest>(clock, weather); // eventually

    displayMgr->Add(std::make_shared<SetTime>(rtc));
    displayMgr->Add(std::make_shared<Clock>(rtc));
    displayMgr->Add(weather);
    displayMgr->Add(std::make_shared<ConfigMenu>());

    // 0 = SetTime   <=>   1 = Clock   <=>   2 = Weather   <=>   3 = ConfigMenu
    displayMgr->SetDefaultAndActivateDisplay(2);

    for (;;) {  // forever, instead of loop(), because I avoid globals ;) [nonik0] If anyone reads this--I know I stomped all over this, but it's a quick hack. I would keep things idiomatic at release-level quality. :)
        ShowSerialStatusMessage(pixels, rtc);
        restServer.handleClient();
        rtc->Update();
        joy->Update();
        develUpdates->Update();
        if (display) {
            displayMgr->Update();
        }
        else {
            pixels->Clear(BLACK, true, true);
            pixels->Show();
        }
        yield();  // allow the ESP platform tasks to run
    }
}

void loop() {}  // for loop above is used instead
