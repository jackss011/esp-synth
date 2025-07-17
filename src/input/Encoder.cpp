#include "Encoder.hpp"

void Encoder::begin()
{
    pinMode(clk_pin,    INPUT_PULLUP);
    pinMode(dt_pin,     INPUT_PULLUP);

    delay(10);
    last_clk = digitalRead(clk_pin);
}

void Encoder::update()
{
    const int clk  = digitalRead(clk_pin);
    const auto now = millis();
    this->_event = EncoderEvent::None;

    // if (clk == true)
    // {
    //     if (this->last_clk == false) // clk low->high
    //     {
    //         _last_dt = !digitalRead(dt_pin);
    //         this->last_down_millis = now;
    //     }
    //     else // clk kept high
    //     {
    //         if ((now - this->last_down_millis) > ENCODER_DEBOUNCE_MILLIS && !this->_clk_state) // debounce
    //         {
    //             this->_clk_state = true; // real state

    //             if(_last_dt != inverted) {
    //                 this->_event = EncoderEvent::Left;
    //             }
    //             else {
    //                 this->_event = EncoderEvent::Right;
    //             }
    //         }
    //     }
    // }
    // else
    // {
    //     const bool was_pressed = this->_clk_state;
    //     this->_clk_state = false;
    // }

    if(clk != last_clk) {
        int dt = digitalRead(dt_pin);

        bool is_left = true;

        if (clk) { //raise
            is_left = dt;
        }
        else { //fall
            is_left = !dt;
        }

        this->_event = is_left ? EncoderEvent::Left : EncoderEvent::Right;
    }

    this->last_clk = clk;
}
