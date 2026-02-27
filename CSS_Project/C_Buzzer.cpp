#include "C_Buzzer.h"

C_Buzzer::C_Buzzer(){
    chip = gpiod_chip_open(GPIOD_NAME);
    if (!chip) {
        std::cerr << "Failed to open GPIO chip" << std::endl;
    }

    line = gpiod_chip_get_line(chip, BUZZER_PIN);
    if (!line) {
        std::cerr << "Failed to get GPIO line" << std::endl;
        gpiod_chip_close(chip);
    }

    if (gpiod_line_request_output(line, "buzzer", 0) < 0) {
        std::cerr << "Failed to request line as output" << std::endl;
        gpiod_chip_close(chip);
    }
}

C_Buzzer::~C_Buzzer(){
    if (chip) 
        gpiod_chip_close(chip);
}

void C_Buzzer::ActivateActuator(int frequency, int duration_ms) {
    std::cout << "Buzzer activated" << std::endl;
    int half_period_us = 1000000 / (2 * frequency);
    int iterations = (duration_ms * frequency) / 1000;
    for (int i = 0; i < iterations; ++i) {
        gpiod_line_set_value(line, 1);
        std::this_thread::sleep_for(std::chrono::microseconds(half_period_us));
        gpiod_line_set_value(line, 0);
        std::this_thread::sleep_for(std::chrono::microseconds(half_period_us));
    }
}

void C_Buzzer::DeactivateActuator() {
    std::cout << "Buzzer Deactivated" << std::endl;
    gpiod_line_set_value(line, 0);
}