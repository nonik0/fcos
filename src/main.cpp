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
#include <rest_server.hpp>
#include <rtc.hpp>
#include <settings.hpp>
#include <weather.hpp>

void setup() {
    auto settings = std::make_shared<Settings>();
    auto pixels = std::make_shared<Pixels>(settings);
    auto rtc = std::make_shared<Rtc>(settings);
    auto sunMoon = std::make_shared<SunMoon>(settings, rtc);
    auto joy = std::make_shared<Joystick>();
    auto develUpdates = std::make_shared<DevelUpdates>(pixels);

    DoHardwareStartupTests(pixels, settings, rtc, joy);

    // These are the "displays" that are available, but more can be added. The
    // DisplayMgr navigates between them using left/right motions, if the
    // Display allows it (e.g. holding left at the Clock activates SetTime)
    auto displayMgr =
        std::make_shared<DisplayManager>(pixels, settings, rtc, joy);

    displayMgr->Add(std::make_shared<SetTime>(rtc));

    // TODO: eventually support multiple default displays? rename to primary display or something similar?
#ifdef WEATHER
    auto weather = std::make_shared<Weather>(rtc, sunMoon);
    displayMgr->Add(weather);
    auto restServer = std::make_shared<RestServer>(displayMgr, rtc, sunMoon, weather); 
#else
    displayMgr->Add(std::make_shared<Clock>(rtc, sunMoon));
    auto restServer = std::make_shared<RestServer>(displayMgr, rtc, sunMoon); 
    #endif
    
    displayMgr->Add(std::make_shared<ConfigMenu>());

    // 0 = SetTime   <=>   1 = Clock/Weather   <=>   3 = ConfigMenu
    displayMgr->SetDefaultAndActivateDisplay(1);

    for (;;) {  // forever, instead of loop(), because I avoid globals ;)
        ShowSerialStatusMessage(pixels, rtc);
        rtc->Update();
        joy->Update();
        develUpdates->Update();
        displayMgr->Update();
        restServer->Update();
        yield();  // allow the ESP platform tasks to run
    }
}

void loop() {}  // for loop above is used instead
