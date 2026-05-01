// Copyright 2026 insomniaTV Contributors. All rights reserved.

#ifndef SRC_IR_IRDRIVER_H_
#define SRC_IR_IRDRIVER_H_

#include <string>
#include "../hal/IIrDriver.h"

namespace InsomniaTV {

class IrDriver : public IIrDriver {
public:
    IrDriver(uint8_t tx_pin, uint8_t rx_pin);
    ~IrDriver() override = default;

    void begin() override;
    bool send(const std::string& protocol, uint64_t code,
              uint16_t bits) override;
    void receive() override;
    uint16_t* learn_raw(uint16_t& out_len) override;
    bool hasDecoded() const override;
    std::string lastProtocol() const override;
    uint64_t lastCode() const override;
    uint16_t lastBits() const override;

private:
    uint8_t _tx_pin;
    uint8_t _rx_pin;
};

}  // namespace InsomniaTV

#endif  // SRC_IR_IRDRIVER_H_
