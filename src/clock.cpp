#include <clock.hpp>

void Clock::Update() {
    auto wheelPos = (*m_settings)["COLR"].as<uint8_t>();
    RgbColor color = wheelPos;
    m_currentColor = color;
    m_pixels->Darken();
#if FCOS_FOXIECLOCK
    m_anim->Update();
    DrawClockDigits(m_currentColor);
#elif FCOS_CARDCLOCK || FCOS_CARDCLOCK2
    m_anim->Update();
    if (!m_pixels->IsDarkModeEnabled() || m_pixels->GetBrightness() >= 0.05f) {
        m_pixels->ClearRoundLEDs({1, 1, 1});
    }

    int brightestLED = 0;
    if (m_rtc->Millis() >= 666)
        brightestLED = 2;
    else if (m_rtc->Millis() >= 333)
        brightestLED = 1;

    m_pixels->DrawSecondLEDs(m_rtc->Second(), m_anim->digitColors[0],
                             brightestLED);

    m_pixels->DrawMinuteLED(m_rtc->Minute(), m_anim->digitColors[1]);
    m_pixels->DrawHourLED(m_rtc->Hour12(), m_anim->GetColonColor());
    DrawClockDigits(m_currentColor);

#endif

    CheckIfWaitingToSaveSettings();
}

void Clock::DrawClockDigits(const RgbColor color) {
    char text[10];
    if ((*m_settings)["24HR"] == "24") {
        sprintf(text, "%02d:%02d", m_rtc->Hour(), m_rtc->Minute());
    } else {
        sprintf(text, "%2d:%02d", m_rtc->Hour12(), m_rtc->Minute());
    }
    int yPos = 0;
#if FCOS_FOXIECLOCK
    m_pixels->DrawChar(0, text[0], m_anim->digitColors[0]);
    m_pixels->DrawChar(20, text[1], m_anim->digitColors[1]);
    //                         [2] is the colon
    m_pixels->DrawChar(42, text[3], m_anim->digitColors[2]);
    m_pixels->DrawChar(62, text[4], m_anim->digitColors[3]);

    if ((m_pixels->GetBrightness() >= 0.05f || (*m_settings)["MINB"] != "0")) {
    }
#elif FCOS_CARDCLOCK || FCOS_CARDCLOCK2
#if FCOS_CARDCLOCK2
    yPos = 3;
#endif
    m_pixels->DrawChar(0, yPos, text[0], m_anim->digitColors[0]);
    m_pixels->DrawChar(4, yPos, text[1], m_anim->digitColors[1]);
    //                         [2] is the colon
    m_pixels->DrawChar(10, yPos, text[3], m_anim->digitColors[2]);
    m_pixels->DrawChar(14, yPos, text[4], m_anim->digitColors[3]);
#endif  // FCOS_CARDCLOCK || FCOS_CARDCLOCK2
    if (!m_pixels->IsDarkModeEnabled() || m_pixels->GetBrightness() >= 0.05f) {
        DrawSeparator(8, m_anim->GetColonColor());
    } else {
#if FCOS_FOXIECLOCK
        // m_pixels->DrawChar(8, yPos, ':', m_anim->GetColonColor());
#elif FCOS_CARDCLOCK || FCOS_CARDCLOCK2
        m_pixels->DrawChar(8, yPos, ':', m_anim->GetColonColor());
#endif
    }
}

void Clock::DrawSeparator(const int x, RgbColor color) {
    if (m_rtc->Second() % 2 && !m_blinkerRunning) {
        m_blinkerRunning = true;
        m_blinkerTimer.Reset();
    }
    const auto scale = &Pixels::ScaleBrightness;
    RgbColor bottomColor = m_anim->GetColonColor();
    RgbColor topColor = bottomColor;

    if (m_blinkerRunning) {
        const float progress = m_blinkerTimer.Ms() / 1900.0f;
        float brightness = progress < 0.5f ? progress : 1.0f - progress;
        bottomColor = scale(bottomColor, 0.55f - brightness);
        topColor = scale(topColor, 0.05f + brightness);

        if (m_blinkerTimer.Ms() >= 1900 ||
            (!(m_rtc->Second() % 2) && m_rtc->Millis() > 900)) {
            m_blinkerRunning = false;
        }
    } else {
        bottomColor = scale(bottomColor, 0.55f);
        topColor = scale(topColor, 0.05f);
    }

#if FCOS_FOXIECLOCK
    m_pixels->Set(40, bottomColor);
    m_pixels->Set(41, topColor);
#elif FCOS_CARDCLOCK
    m_pixels->Set(x, 1, bottomColor);
    m_pixels->Set(x, 3, topColor);
#elif FCOS_CARDCLOCK2
    m_pixels->Set(x, 3 + 1, bottomColor);
    m_pixels->Set(x, 3 + 3, topColor);
#endif
}