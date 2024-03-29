#pragma once

#include <nesemu/hw/mapper/mapper_base.h>
#include <nesemu/logger.h>


namespace hw::mapper::internal {

// PRG ROM bank mode (bank_select_ & 0x40):
//
// 0:
//      0x8000-0x9FFF: Switchable (R6)
//      0xA000-0xBFFF: Switchable (R7)
//      0xC000-0xDFFF: Fixed, second last bank
//      0xE000-0xFFFF: Fixed, last bank
//
// 1:
//      0x8000-0x9FFF: Fixed, second last bank
//      0xA000-0xBFFF: Switchable (R7)
//      0xC000-0xDFFF: Switchable (R6)
//      0xE000-0xFFFF: Fixed, last bank

// CHR ROM/RAM bank mode (bank_select_ & 0x80):
//
// 0:
//      0x0000-0x07FF: Switchable (R0)
//      0x0800-0x0FFF: Switchable (R1)
//      0x1000-0x13FF: Switchable (R2)
//      0x1400-0x17FF: Switchable (R3)
//      0x1800-0x1BFF: Switchable (R4)
//      0x1C00-0x1FFF: Switchable (R5)
//
// 1:
//      0x0000-0x03FF: Switchable (R2)
//      0x0400-0x07FF: Switchable (R3)
//      0x0800-0x0BFF: Switchable (R4)
//      0x0C00-0x0FFF: Switchable (R5)
//      0x1000-0x17FF: Switchable (R0)
//      0x1800-0x1FFF: Switchable (R1)

class Mapper004 : public Mapper {
public:
  using Mapper::Mapper;  // "Inherit" constructor

  uint32_t decodeCPUAddress(uint16_t addr) const override {
    uint8_t bank_num = 0;

    if (bank_select_ & 0x40) {
      if (addr < 0xA000) {                // 0x8000-0x9FFF
        bank_num = (prg_banks_ * 2) - 2;  //   -> (-2)
      } else if (addr < 0xC000) {         // 0xA000-0xBFFF
        bank_num = bank_values_[7];       //   -> R7
      } else if (addr < 0xE000) {         // 0xC000-0xDFFF
        bank_num = bank_values_[6];       //   -> R6
      } else {                            // 0xE000-0xFFFF
        bank_num = (prg_banks_ * 2) - 1;  //   -> (-1)
      }
    } else {
      if (addr < 0xA000) {                // 0x8000-0x9FFF
        bank_num = bank_values_[6];       //   -> R6
      } else if (addr < 0xC000) {         // 0xA000-0xBFFF
        bank_num = bank_values_[7];       //   -> R7
      } else if (addr < 0xE000) {         // 0xC000-0xDFFF
        bank_num = (prg_banks_ * 2) - 2;  //   -> (-2)
      } else {                            // 0xE000-0xFFFF
        bank_num = (prg_banks_ * 2) - 1;  //   -> (-1)
      }
    }

    return (0x2000 * bank_num) | (addr & 0x1FFF);
  }


  uint32_t decodePPUAddress(uint16_t addr) const override {

    // Clock scanline counter on rising edge of A12, after being low for 2 (3?) M2s
    cur_a12_ = addr & 0x1000;
    if (low_count_ > 1 && cur_a12_) {
      low_count_ = 0;
      if (irq_counter_ == 0 || irq_reload_) {
        irq_counter_ = irq_latch_;
        irq_reload_  = false;
      } else {
        irq_counter_--;
      }

      if (irq_counter_ == 0 && irq_enable_) {
        logger::log<logger::DEBUG_MAPPER>("Mapper-triggered IRQ\n");
        has_irq_ = true;
      }
    }

    addr &= 0x1FFF;
    if (bank_select_ & 0x80) {
      if (addr < 0x0400) {                                   // 0x0000-0x03FF
        return (0x400 * bank_values_[2]) | (addr & 0x03FF);  //   -> R2
      } else if (addr < 0x0800) {                            // 0x0400-0x07FF
        return (0x400 * bank_values_[3]) | (addr & 0x03FF);  //   -> R3
      } else if (addr < 0x0C00) {                            // 0x0800-0x0BFF
        return (0x400 * bank_values_[4]) | (addr & 0x03FF);  //   -> R4
      } else if (addr < 0x1000) {                            // 0x0C00-0x0FFF
        return (0x400 * bank_values_[5]) | (addr & 0x03FF);  //   -> R5
      } else if (addr < 0x1800) {                            // 0x1000-0x17FF
        return (0x400 * bank_values_[0]) | (addr & 0x07FF);  //   -> R0
      } else {                                               // 0x1800-0x1FFF
        return (0x400 * bank_values_[1]) | (addr & 0x07FF);  //   -> R1
      }
    } else {
      if (addr < 0x0800) {                                   // 0x0000-0x07FF
        return (0x400 * bank_values_[0]) | (addr & 0x07FF);  //   -> R0
      } else if (addr < 0x1000) {                            // 0x0800-0x0FFF
        return (0x400 * bank_values_[1]) | (addr & 0x07FF);  //   -> R1
      } else if (addr < 0x1400) {                            // 0x1000-0x13FF
        return (0x400 * bank_values_[2]) | (addr & 0x03FF);  //   -> R2
      } else if (addr < 0x1800) {                            // 0x1400-0x17FF
        return (0x400 * bank_values_[3]) | (addr & 0x03FF);  //   -> R3
      } else if (addr < 0x1C00) {                            // 0x1800-0x1BFF
        return (0x400 * bank_values_[4]) | (addr & 0x03FF);  //   -> R4
      } else {                                               // 0x1C00-0x1FFF
        return (0x400 * bank_values_[5]) | (addr & 0x03FF);  //   -> R5
      }
    }
  }


  bool hasIRQ() const override { return has_irq_; }


  void write(uint16_t addr, uint8_t data) override {
    switch (addr & 0xE001) {
      case 0x8000:  // 0x8000-0x9FFE, even
        logger::log<logger::DEBUG_MAPPER>("Select bank %d, PRG mode %d, CHR mode %d\n",
                                          data & 0x07,
                                          !!(data & 0x40),
                                          !!(data & 0x80));
        bank_select_ = data;
        break;
      case 0x8001: {  // 0x8001-0x9FFF, odd
        const uint8_t bank = bank_select_ & 0x07;
        if (bank < 2) {
          data &= 0xFE;
        } else if (bank > 5) {
          data &= 0x3F;
        }
        bank_values_[bank] = data;
        logger::log<logger::DEBUG_MAPPER>("Set bank[%d] = $%02X\n", bank, data);
      } break;
      case 0xA000:  // 0xA000-0xBFFE, even
        if (mirroring_ != Mirroring::none) {
          mirroring_ = (data & 0x01) ? Mirroring::horizontal : Mirroring::vertical;
        }
        break;
      case 0xA001:  // 0xA001-0xBFFF, odd
        // PRG RAM write protect. Not implemented to ensure compatibility with MMC6
        break;
      case 0xC000:  // 0xC000-0xDFFE, even
        logger::log<logger::DEBUG_MAPPER>("Load latch %d\n", data);
        irq_latch_ = data;
        break;
      case 0xC001:  // 0xC001-0xDFFF, odd
        logger::log<logger::DEBUG_MAPPER>("Reload IRQ\n");
        irq_reload_ = true;
        break;
      case 0xE000:  // 0xE000-0xFFFE, even
        logger::log<logger::DEBUG_MAPPER>("Disable/Clear IRQ\n");
        irq_enable_ = false;
        has_irq_    = false;
        break;
      case 0xE001:  // 0xE001-0xFFFF, odd
        logger::log<logger::DEBUG_MAPPER>("Enable IRQ\n");
        irq_enable_ = true;
        break;
      default:
        logger::log<logger::ERROR>("Attempted to write invalid mapper addr $%02X\n", addr);
        break;
    }
  }

  void clock() override { low_count_ = cur_a12_ ? 0 : low_count_ + 1; }

private:
  uint8_t         bank_select_    = {0};      // 0x8000-0x9FFF, even
  uint8_t         bank_values_[8] = {0};      // 0x8000-0x9FFF, odd
  uint8_t         irq_latch_      = {0};      // 0xC000-0xDFFF, even
  mutable bool    irq_reload_     = {false};  // 0xC000-0xDFFF, odd
  bool            irq_enable_     = {false};  // 0xE000-0xFFFE. Even=Disable, Odd=Enable
  mutable uint8_t irq_counter_    = {0};
  mutable bool    has_irq_        = {false};
  mutable bool    cur_a12_        = {false};
  mutable uint8_t low_count_      = {0};
};

}  // namespace hw::mapper::internal
