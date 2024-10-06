#if FCOS_CARDCLOCK2
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
        m_pixels->ClearRoundLEDs({1, 1, 1});
    }

    char text[10];
    sprintf(text, "72F\0");

    int yPos = 3;
    m_pixels->DrawText(0, yPos, text, m_anim->digitColors[0]);

    int cycle = 0;
    if (m_rtc->Millis() >= 333) {
        cycle = 1;
    } else if (m_rtc->Millis() >= 666) {
        cycle = 2;
    }
    m_pixels->DrawWeatherLEDs(Pixels::SUNNY, cycle);

    CheckIfWaitingToSaveSettings();
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
    return evt != Button::REPEAT;  // when held, move to SetTime display
};

bool Weather::Right(const Button::Event_e evt) {
    if (evt == Button::PRESS) {
        m_anim->Right();
    }
    return evt != Button::REPEAT;  // when held, move to ConfigMenu display
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
        uint8_t pos = 0;
        for (int i = 0; i < 0 + (m_anim->name.length() * 4); i++) {
            m_pixels->Clear();
            m_pixels->DrawText(0 - i, 3, m_anim->name, LIGHT_GRAY);
            m_pixels->Show();
            if (joy.WaitForButton(joy.press, 75)) {
                ElapsedTime::Delay(25);
            }
        }
        joy.WaitForNoButtonsPressed();
        PrepareToSaveSettings();
    } else if (evt == Button::REPEAT) {
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
#endif