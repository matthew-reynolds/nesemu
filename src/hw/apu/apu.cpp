#include <nesemu/hw/apu/apu.h>

#include <nesemu/logger.h>
#include <nesemu/ui/speaker.h>


// =*=*=*=*= PPU Setup =*=*=*=*=

void hw::apu::APU::setSpeaker(ui::Speaker* speaker) {
  speaker_ = speaker;
}


// =*=*=*=*= PPU Execution =*=*=*=*=

// Clock length counter and sweep units
void hw::apu::APU::clockHalfFrame() {
  square_1.length_counter.clock();
  square_2.length_counter.clock();
  triangle.length_counter.clock();
  noise.length_counter.clock();
  dmc.length_counter.clock();

  square_1.sweep.clock();
  square_2.sweep.clock();
}

// Clock envelope and linear counter units
void hw::apu::APU::clockQuarterFrame() {
  triangle.linear_counter.clock();
}


void hw::apu::APU::clock() {

  if (frame_counter_reset_counter_ == cycle_count_) {
    cycle_count_                 = 0;
    frame_counter_reset_counter_ = 0xFFFF;

    // If 5-step mode, clock everything
    if (frame_counter_mode_) {
      clockHalfFrame();
      clockQuarterFrame();
    }
  }


  switch (cycle_count_++) {
    case 7457:
      clockQuarterFrame();
      break;

    case 14913:
      clockHalfFrame();
      clockQuarterFrame();
      break;

    case 22371:
      clockQuarterFrame();
      break;

    case 29828:  // (4-step only) Set frame interrupt flag if inhibit is clear
      if (!frame_counter_mode_ && !irq_inhibit_) {
        has_irq_ = true;
      }
      break;

    case 29829:  // (4-step only) Set frame interrupt flag if inhibit is clear
      if (!frame_counter_mode_) {
        clockHalfFrame();
        clockQuarterFrame();
        if (!irq_inhibit_) {
          has_irq_ = true;
        }
      }
      break;

    case 29830:  // (4-step only) Set frame interrupt flag if inhibit is clear
                 //               Reset counter
      if (!frame_counter_mode_) {
        if (!irq_inhibit_) {
          has_irq_ = true;
        }
        cycle_count_ = 0;
      }
      break;

    case 37281:  // (5-step only) Set frame interrupt flag if inhibit is clear
      clockHalfFrame();
      clockQuarterFrame();
      break;

    case 37282:  // (5-step only) Reset counter
      cycle_count_ = 0;
      break;
  }

  static bool even = false;
  even             = !even;
  if (even) {
    square_1.clock();
    square_2.clock();
    triangle.clock();
    noise.clock();
    dmc.clock();
  }

  // TODO: Nicer mixer? Maybe a LUT?
  float square_sum = square_1.getOutput() + square_2.getOutput();
  float square_out = 95.88 / ((square_sum == 0 ? 0 : 8128 / square_sum) + 100);
  float tnd_sum    = triangle.getOutput() / 8227 + noise.getOutput() / 12241 + dmc.getOutput() / 22638;
  float tnd_out    = 159.79 / ((tnd_sum == 0 ? 0 : 1 / tnd_sum) + 100);

  uint8_t buffer[1] = {(square_out + tnd_out) * 255};
  speaker_->update(buffer, 1);
}


uint8_t hw::apu::APU::readRegister(uint16_t address) {
  if (address != 0x4015) {
    // Unknown register, or read-only register. Note that all registers except 0x4015 are write-only
    // To ensure open bus behaviour is maintained, this func should only be called with addr == 0x4015
    logger::log<logger::Level::WARNING>("Attempted to read a write-only APU register ($%02X)!\n", address);
    return 0;
  }

  // Status
  static registers::StatusControl status = {0};
  status.ch_1                            = square_1.length_counter.counter_ > 0;
  status.ch_2                            = square_2.length_counter.counter_ > 0;
  status.ch_3                            = triangle.length_counter.counter_ > 0;
  status.ch_4                            = noise.length_counter.counter_ > 0;
  status.ch_5                            = false;  // TODO
  status.frame_interrupt                 = has_irq_;
  status.dmc_interrupt                   = false;  // TODO

  has_irq_ = false;
  logger::log<logger::DEBUG_APU>("Read $%02X from Status (0x4015)\n", status.raw);
  return status.raw;
}

void hw::apu::APU::writeRegister(uint16_t address, uint8_t data) {
  switch (address) {

    // Channel 1 (Square 1)
    case 0x4000:
      square_1.envelope.divider_.setPeriod(data & 0x0F);
      square_1.envelope.volume_       = (data & 0x0F);
      square_1.envelope.const_volume_ = (data & 0x10);
      square_1.length_counter.halt_   = (data & 0x20);
      square_1.envelope.loop_         = (data & 0x20);
      square_1.duty_cycle             = (data & 0xC0) >> 6;
      logger::log<logger::DEBUG_APU>("Write $%02X to Square 1 (0x4000)\n", data);
      break;
    case 0x4001:
      square_1.sweep.shift_count_ = (data & 0x07);
      square_1.sweep.negate_      = (data & 0x08);
      square_1.sweep.divider_.setPeriod(((data & 0x70) >> 4) + 1);
      square_1.sweep.enable_ = (data & 0x80);
      square_1.sweep.reload_ = true;
      logger::log<logger::DEBUG_APU>("Write $%02X to Square 1 (0x4001)\n", data);
      break;
    case 0x4002:
      square_1.timer &= 0xFF00;
      square_1.timer |= data;
      logger::log<logger::DEBUG_APU>("Write $%02X to Square 1 (0x4002)\n", data);
      break;
    case 0x4003:
      square_1.timer &= 0x00FF;
      square_1.timer |= data & 0x7;
      square_1.envelope.start_ = true;
      if (sound_en_.ch_1) {
        square_1.length_counter.load(data >> 3);
      }
      square_1.seq_ = 0;
      logger::log<logger::DEBUG_APU>("Write $%02X to Square 1 (0x4003)\n", data);
      break;

      // Channel 2 (Square 2)
    case (0x4004):
      square_2.envelope.divider_.setPeriod(data & 0x0F);
      square_2.envelope.volume_       = (data & 0x0F);
      square_2.envelope.const_volume_ = (data & 0x10);
      square_2.length_counter.halt_   = true;  //(data & 0x20);
      square_2.envelope.loop_         = (data & 0x20);
      square_2.duty_cycle             = (data & 0xC0) >> 6;
      logger::log<logger::DEBUG_APU>("Write $%02X to Square 2 (0x4004)\n", data);
      break;
    case (0x4005):
      square_2.sweep.shift_count_ = (data & 0x07);
      square_2.sweep.negate_      = (data & 0x08);
      square_2.sweep.divider_.setPeriod(((data & 0x70) >> 4) + 1);
      square_2.sweep.enable_ = (data & 0x80);
      square_2.sweep.reload_ = true;
      logger::log<logger::DEBUG_APU>("Write $%02X to Square 2 (0x4005)\n", data);
      break;
    case (0x4006):
      square_2.timer &= 0xFF00;
      square_2.timer |= data;
      logger::log<logger::DEBUG_APU>("Write $%02X to Square 2 (0x4006)\n", data);
      break;
    case (0x4007):
      square_2.timer &= 0x00FF;
      square_2.timer |= data & 0x7;
      square_2.envelope.start_ = true;
      if (sound_en_.ch_2) {
        square_2.length_counter.load(data >> 3);
      }
      square_2.seq_ = 0;
      logger::log<logger::DEBUG_APU>("Write $%02X to Square 2 (0x4007)\n", data);
      break;

    // Triangle
    case 0x4008:
      triangle.linear_counter.reload_value_ = (data & 0x7F);
      triangle.linear_counter.control_      = (data & 0x80);
      triangle.length_counter.halt_         = (data & 0x80);
      logger::log<logger::DEBUG_APU>("Write $%02X to Triangle (0x4008)\n", data);
      break;
    case 0x4009:
      // Unused
      break;
    case 0x400A:
      triangle.timer &= 0xFF00;
      triangle.timer |= data;
      logger::log<logger::DEBUG_APU>("Write $%02X to Triangle (0x400A)\n", data);
      break;
    case 0x400B:
      triangle.timer &= 0x00FF;
      triangle.timer |= data & 0x7;
      if (sound_en_.ch_3) {
        triangle.length_counter.load(data >> 3);
      }
      // TODO: Set linear counter reload
      logger::log<logger::DEBUG_APU>("Write $%02X to Triangle (0x400B)\n", data);
      break;

    // Noise
    case 0x400C:
      noise.envelope.divider_.setPeriod(data & 0x0F);
      noise.envelope.volume_       = (data & 0x0F);
      noise.envelope.const_volume_ = (data & 0x10);
      noise.length_counter.halt_   = (data & 0x20);
      noise.envelope.loop_         = (data & 0x20);
      logger::log<logger::DEBUG_APU>("Write $%02X to Noise (0x400C)\n", data);
      break;
    case 0x400D:
      // Unused
      break;
    case 0x400E:
      noise.loadPeriod(data & 0x0F);
      noise.mode = (data & 0x80);
      logger::log<logger::DEBUG_APU>("Write $%02X to Noise (0x400E)\n", data);
      break;
    case 0x400F:
      noise.envelope.start_ = true;
      if (sound_en_.ch_4) {
        noise.length_counter.load(data >> 3);
      }
      logger::log<logger::DEBUG_APU>("Write $%02X to Noise (0x400F)\n", data);
      break;

    // TODO: DMC
    case 0x4010:
    case 0x4011:
    case 0x4012:
    case 0x4013:
      break;

    // Control
    case (0x4015):
      sound_en_.raw = data;
      if (!sound_en_.ch_1) {
        square_1.length_counter.counter_ = 0;
      }
      if (!sound_en_.ch_2) {
        square_2.length_counter.counter_ = 0;
      }
      if (!sound_en_.ch_3) {
        triangle.length_counter.counter_ = 0;
      }
      if (!sound_en_.ch_4) {
        noise.length_counter.counter_ = 0;
      }
      if (!sound_en_.ch_5) {
        ;  // TODO: Set bytes remaining to 0
      } else {
        ;  // TODO: Restart DMC sample only if bytes remaining > 0
      }
      logger::log<logger::DEBUG_APU>("Write $%02X to Status (0x4015)\n", data);
      break;

    // Frame counter
    case 0x4017:
      irq_inhibit_        = data & 0x40;
      frame_counter_mode_ = data & 0x80;

      if (irq_inhibit_) {
        has_irq_ = false;
      }

      // TODO: These timings are wrong
      if (cycle_count_ % 2) {  // If odd:
        frame_counter_reset_counter_ = cycle_count_ + 2;
      } else {
        frame_counter_reset_counter_ = cycle_count_ + 2;
      }
      break;
  }
}
