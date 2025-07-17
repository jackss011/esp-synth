#include "Btn.hpp"

void Btn::begin()
{
    pinMode(gpio_pin, INPUT_PULLUP);
}

void Btn::update()
{
    const bool reading = !(bool)digitalRead(gpio_pin);
    const auto now = millis();
    this->_event = BtnEvent::None;

    if (reading == true)
    {
        if (this->last_reading == false)
        {
            // on press (no debounce)
            this->last_down_millis = now;
        }
        else
        {
            // kept pressed
            if ((now - this->last_down_millis) > BTN_DEBOUNCE_MILLIS && !this->_pressed)
            {
                // on press
                this->_pressed = true;
                this->_event = BtnEvent::Press;
            }
        }
    }
    else
    {
        const bool was_pressed = this->_pressed;
        this->_pressed = false;

        if (was_pressed == true)
        {
            // on release
            this->_event = BtnEvent::Release;
        }
        else
        {
            // kept released
        }
    }

    this->last_reading = reading;
}
