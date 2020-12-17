#pragma once

#include <nesemu/hw/apu/apu.h>
#include <nesemu/hw/cpu.h>
#include <nesemu/hw/joystick.h>
#include <nesemu/hw/ppu.h>
#include <nesemu/hw/system_bus.h>

#include <SDL2/SDL_events.h>


// Forward declarations
namespace hw::mapper {
class Mapper;
}

namespace hw::rom {
class Rom;
}

namespace ui {
class Screen;
}


namespace hw::console {

class Console {
public:
  explicit Console(bool allow_unofficial_opcodes);
  ~Console();

  // Setup
  void loadCart(rom::Rom* rom);
  void setScreen(ui::Screen* screen);

  // Execution
  void start();
  void update();
  void handleEvent(const SDL_Event& event);

  // Misc
  const ppu::PPU* getPPU() const { return &ppu_; }

private:
  ui::Screen* screen_ = {nullptr};

  // HW Components
  system_bus::SystemBus bus_;
  apu::APU              apu_;
  cpu::CPU              cpu_;
  ppu::PPU              ppu_;
  joystick::Joystick    joy_1_  = {1};
  joystick::Joystick    joy_2_  = {2};
  mapper::Mapper*       mapper_ = {nullptr};
};

}  // namespace hw::console
