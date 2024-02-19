#pragma once

#include <array>
#include <memory>
#include <optional>

#include <QByteArray>

#include "EleparamDefine.h"

namespace edm {

namespace power {

class EleparamDecodeResult final {
public:
    using ptr = std::shared_ptr<EleparamDecodeResult>;

public:
    EleparamDecodeResult()
        : can_buffer_{QByteArray{8, 0x00}, QByteArray{8, 0x00}}, io_1_(0x00),
          io_2_(0x00) {}
    ~EleparamDecodeResult() = default;

    auto& can_buffer() { return can_buffer_; }
    const auto& can_buffer() const { return can_buffer_; }

    auto& io_1() { return io_1_; }
    const auto& io_1() const { return io_1_; }

    auto& io_2() { return io_2_; }
    const auto& io_2() const { return io_2_; }

private:
    std::array<QByteArray, 2> can_buffer_;
    uint32_t io_1_;
    uint32_t io_2_;

public:
    // 定义电参数控制的 io_1 和 io_2 的掩码
    constexpr static const uint32_t io_1_mask = 0xFF;
    constexpr static const uint32_t io_2_mask = 0xFF;
};

class EleparamDecodeInput final {
public:
    using ptr = std::shared_ptr<EleparamDecodeInput>;

public:
    EleparamDecodeInput(EleParam_dkd_t::ptr ele_param, uint8_t highpower_flag,
                        uint8_t machpower_flag)
        : ele_param_(ele_param), highpower_flag_(highpower_flag),
          machpower_flag_(machpower_flag) {}
    ~EleparamDecodeInput() = default;

    auto ele_param() const { return ele_param_; }
    auto highpower_flag() const { return highpower_flag_; }
    auto machpower_flag() const { return machpower_flag_; }

private:
    EleParam_dkd_t::ptr ele_param_;
    uint8_t highpower_flag_;
    uint8_t machpower_flag_; // can 帧 mach 高频使能位
};

class EleparamDecoder {
public:
    static EleparamDecodeResult::ptr decode(EleparamDecodeInput::ptr input);

private:
    EleparamDecoder() = default;
    ~EleparamDecoder() = default;

};

} // namespace power

} // namespace edm
