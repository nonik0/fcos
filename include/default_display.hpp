#pragma once
#include <stdint.h>

#include <animators.hpp>
#include <button.hpp>
#include <display.hpp>
#include <dprint.hpp>
#include <elapsed_time.hpp>
#include <pixels.hpp>
#include <rtc.hpp>
#include <set_time.hpp>
#include <settings.hpp>

class DefaultDisplay : public Display {
  protected:
    std::shared_ptr<Rtc> m_rtc;
    size_t m_animMode{0};
    bool m_shouldSaveSettings{false};

    ElapsedTime m_waitingToSaveSettings;

    RgbColor m_currentColor{0};
    std::shared_ptr<Animator> m_anim;

  public:
    DefaultDisplay(std::shared_ptr<Rtc> rtc) : Display(), m_rtc(rtc) {}

    virtual void Activate();
    virtual void Update() = 0;

    void SetAnimator(std::shared_ptr<Animator> anim);

  protected:
    virtual void Up(const Button::Event_e evt) override;
    virtual void Down(const Button::Event_e evt) override;
    virtual bool Left(const Button::Event_e evt) override;
    virtual bool Right(const Button::Event_e evt) override;
    virtual void Press(const Button::Event_e evt) override;

  protected:
    void PrepareToSaveSettings();
    void CheckIfWaitingToSaveSettings();
    void LoadSettings();
};