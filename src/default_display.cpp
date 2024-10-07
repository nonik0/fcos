#include <default_display.hpp>

void DefaultDisplay::Activate() {
    LoadSettings();
    SetAnimator(CreateAnimator(m_pixels, m_settings, m_rtc,
                               (AnimatorType_e)m_animMode));
}

void DefaultDisplay::SetAnimator(std::shared_ptr<Animator> anim) {
    m_anim = anim;
    m_anim->Start();
    m_anim->SetColor((*m_settings)["COLR"].as<uint8_t>());
}

void DefaultDisplay::Up(const Button::Event_e evt) {
    if (evt == Button::PRESS || evt == Button::REPEAT) {
        m_anim->Up();
        PrepareToSaveSettings();
    }
};

void DefaultDisplay::Down(const Button::Event_e evt) {
    if (evt == Button::PRESS || evt == Button::REPEAT) {
        m_anim->Down();
        PrepareToSaveSettings();
    }
};

bool DefaultDisplay::Left(const Button::Event_e evt) {
    if (evt == Button::PRESS) {
        m_anim->Left();
        // temporary hack for photos
        // m_rtc->SetTime(12, 34, 00);
    }
    return evt != Button::REPEAT;  // when held, move to SetTime display
};

bool DefaultDisplay::Right(const Button::Event_e evt) {
    if (evt == Button::PRESS) {
        m_anim->Right();
    }
    return evt != Button::REPEAT;  // when held, move to ConfigMenu display
};

// toggles between configured MINB=user and temporary MINB=0
void DefaultDisplay::Press(const Button::Event_e evt) {
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
    } else if (evt == Button::REPEAT) {
        m_pixels->ToggleDarkMode();
        toggledDarkMode = true;
    }
}

void DefaultDisplay::PrepareToSaveSettings() {
    m_shouldSaveSettings = true;
    m_waitingToSaveSettings.Reset();
}

void DefaultDisplay::CheckIfWaitingToSaveSettings() {
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

void DefaultDisplay::LoadSettings() {
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
