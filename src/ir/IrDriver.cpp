// Copyright 2026 insomniaTV Contributors. All rights reserved.

#include "IrDriver.h"
#include <string>

namespace InsomniaTV {

IrDriver::IrDriver(uint8_t tx_pin, uint8_t rx_pin)
    : _tx_pin(tx_pin), _rx_pin(rx_pin) {}

void IrDriver::begin() {
    // Initialization of IRremoteESP8266 pins
}

bool IrDriver::send(const std::string& protocol, uint64_t code, uint16_t bits) {
    // Send IR implementation
    return true;
}

void IrDriver::receive() {
    // Start receive mode
}

uint16_t* IrDriver::learn_raw(uint16_t& out_len) {
    out_len = 0;
    return nullptr;
}

bool IrDriver::hasDecoded() const {
    return false;
}

std::string IrDriver::lastProtocol() const {
    return "";
}

uint64_t IrDriver::lastCode() const {
    return 0;
}

uint16_t IrDriver::lastBits() const {
    return 0;
}

}  // namespace InsomniaTV
