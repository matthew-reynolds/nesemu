#pragma once

#include <nesemu/utils/compat.h>
#include <nesemu/utils/reg_bit.h>

#include <cstdint>
#include <cstring>  //memcpy


// Forward declarations
namespace hw::mapper {
class Mapper;
}

namespace ui {
class NametableViewer;
class PatternTableViewer;
class Screen;
class SpriteViewer;
}  // namespace ui


// Ricoh RP2A03 (based on MOS6502)
// Little endian
namespace hw::ppu {

enum class MemoryMappedIO {
  PPUCTRL   = 0x2000,
  PPUMASK   = 0x2001,
  PPUSTATUS = 0x2002,
  OAMADDR   = 0x2003,
  OAMDATA   = 0x2004,
  PPUSCROLL = 0x2005,
  PPUADDR   = 0x2006,
  PPUDATA   = 0x2007,
  OAMDMA    = 0x4014
};

class PPU {
  friend class ui::NametableViewer;
  friend class ui::PatternTableViewer;
  friend class ui::SpriteViewer;

public:
  // Setup
  void loadCart(mapper::Mapper* mapper, uint8_t* chr_mem, bool is_ram);
  void setScreen(ui::Screen* screen);


  // Execution
  void    clock();
  bool    hasNMI();
  uint8_t readRegister(uint16_t cpu_address);
  void    writeRegister(uint16_t cpu_address, uint8_t data);
  void    spriteDMAWrite(uint8_t* data);  // CPU 0x4014. Load sprite memory with 256 bytes.


private:
  // Other chips
  ui::Screen* screen_ = {nullptr};


  // Registers
  union PPUReg {
    utils::RegBit<0, 15, uint16_t> raw;
    utils::RegBit<0, 8, uint16_t>  lower;
    utils::RegBit<8, 8, uint16_t>  upper;
    utils::RegBit<0, 5, uint16_t>  coarse_x_scroll;   // X offset of the current tile
    utils::RegBit<5, 5, uint16_t>  coarse_y_scroll;   // Y offset of the current tile
    utils::RegBit<10, 2, uint16_t> nametable_select;  // 0=0x2000, 1=0x2400, 2=0x2800, 3=0x2C00
    utils::RegBit<12, 3, uint16_t> fine_y_scroll;     // Y offset of the scanline within a tile
  };

  // TODO: StructField vs bool performance
  struct CtrlReg1 {
    void operator=(uint8_t val) {
      /* nametable_address.from(val); */
      vertical_write.from(val);
      sprite_pattern_table_addr.from(val);
      screen_pattern_table_addr.from(val);
      large_sprites.from(val);
      ppu_master_slave_mode.from(val);
      vblank_enable.from(val);
    }
    operator uint8_t() const {
      return /* nametable_address.to() | */
          vertical_write.to() | sprite_pattern_table_addr.to() | screen_pattern_table_addr.to() | large_sprites.to()
          | ppu_master_slave_mode.to() | vblank_enable.to();
    }
    /* utils::StructField<0, 2> nametable_address; */  // ** Mapped in temp_ register
    utils::StructField<2> vertical_write;              // 0=PPU memory address increments by 1, 1=increments by 32
    utils::StructField<3> sprite_pattern_table_addr;   // 0=0x0000, 1=0x1000
    utils::StructField<4> screen_pattern_table_addr;   // 0=0x0000, 1=0x1000
    utils::StructField<5> large_sprites;               // 0=8x8, 1=0x8x16
    utils::StructField<6> ppu_master_slave_mode;       // Unused
    utils::StructField<7> vblank_enable;               // Generate interrupts on VBlank
  };

  struct CtrlReg2 {
    void operator=(uint8_t val) {
      greyscale.from(val);
      bg_mask.from(val);
      sprite_mask.from(val);
      bg_enable.from(val);
      sprite_enable.from(val);
      tint_red.from(val);
      tint_green.from(val);
      tint_blue.from(val);
      render_enable = bg_enable | sprite_enable;
    }
    operator uint8_t() const {
      return greyscale.to() | bg_mask.to() | sprite_mask.to() | bg_enable.to() | sprite_enable.to() | tint_red.to()
             | tint_green.to() | tint_blue.to();
    }

    bool                  render_enable;  // Read only, shortcut for checking if rendering is enabled
    utils::StructField<0> greyscale;      // Produce greyscale display
    utils::StructField<1> bg_mask;        // Show left 8 columns of the background
    utils::StructField<2> sprite_mask;    // Show sprites in left 8 columns
    utils::StructField<3> bg_enable;      // 0=Blank screen, 1=Show background
    utils::StructField<4> sprite_enable;  // 0=Hide sprites, 1=Show sprites
    utils::StructField<5> tint_red;       // Attenuate non-red channels (Non-green on PAL/Dendy)
    utils::StructField<6> tint_green;     // Attenuate non-green channels (Non-red on PAL/Dendy)
    utils::StructField<7> tint_blue;      // Attenuate non-blue channels
  };

  struct StatusReg {
    void operator=(uint8_t val) {
      overflow.from(val);
      hit.from(val);
      vblank.from(val);
    }
                          operator uint8_t() const { return overflow.to() | hit.to() | vblank.to(); }
    utils::StructField<5> overflow;  // Sprite overflow
    utils::StructField<6> hit;       // Sprite refresh hit sprite #0. Reset when screen refresh starts.
    utils::StructField<7> vblank;    // PPU is in VBlank. Reset when VBlank ends or CPU reads 0x2002
  };


  // Sprite Memory
  PACKED(union SpriteAttributesPacked {
    uint8_t             raw;
    utils::RegBit<0, 2> palette;     // Palette (4 to 7) of sprite
    utils::RegBit<5, 1> priority;    // 0=In front of background, 1=Behind background
    utils::RegBit<6, 1> flip_horiz;  // Flip sprite horizontally
    utils::RegBit<7, 1> flip_vert;   // Flip sprite vertically TODO: Unsupported
  });

  struct SpriteAttributes {
    SpriteAttributes() = default;
    SpriteAttributes(const SpriteAttributesPacked& packed)
        : palette(packed.palette),
          priority(packed.priority),
          flip_horiz(packed.flip_horiz),
          flip_vert(packed.flip_vert) {}
    uint8_t palette : 2;
    bool    priority;
    bool    flip_horiz;
    bool    flip_vert;
  };

  PACKED(struct Sprite {
    Sprite& operator=(const Sprite& val) {
      // Cast used to suppress -Wclass-memaccess
      memcpy((uint8_t*) this, (uint8_t*) &val, sizeof(Sprite));
      return *this;
    }

    uint8_t y_position = {0};                   // Measured from top left
    PACKED(union {                              // 8x8 sprites use 8 bit index & pattern table select from ctrl_reg_1_
                                                // 8x16 sprites use 7 bit index & specify pattern table
      uint8_t             small_tile_index;     // Tile number within pattern table
      utils::RegBit<0, 1> large_pattern_table;  // Pattern table ($0000 or $1000)
      utils::RegBit<1, 7> large_tile_index;     // Tile number for top half of sprite within pattern table
                                                //   (Bottom half uses next tile)
    });
    SpriteAttributesPacked attributes = {0};
    uint8_t                x_position = {0};  // Measured from top left
  });


  // Sprite evaluation state machine
  struct SpriteEvaluationFSM {
    enum class State { CHECK_Y_IN_RANGE, COPY_SPRITE, OVERFLOW, DUMMY_READ, DONE } state_;
    uint8_t state_counter_ = {0};  // Number of cycles until state change
    uint8_t poam_index_    = {0};  // Position within primary OAM (0-64)*4
    uint8_t soam_index_    = {0};  // Position within secondary OAM (0-8)*4
    uint8_t latch_         = {0};
    bool    initialize_    = {false};
  } sprite_eval_fsm_;


  // Background registers
  PPUReg   t_ = {0};
  PPUReg   v_ = {0};
  uint8_t  fine_x_scroll_ : 3;        // X offset of the scanline within a tile
  bool     write_toggle_    = false;  // 0 indicates first write
  uint16_t pattern_sr_a_    = {0};    // Lower byte of pattern, controls bit 0 of the color
  uint16_t pattern_sr_b_    = {0};    // Upper byte of pattern, controls bit 1 of the color
  uint8_t  palette_sr_a_    = {0};    // Palette number, controls bit 2 of the color
  uint8_t  palette_sr_b_    = {0};    // Palette number, controls bit 3 of the color
  bool     palette_latch_a_ = {false};
  bool     palette_latch_b_ = {false};


  // Sprite registers
  union {
    uint8_t byte[0x100];
    Sprite  sprite[0x40];
  } primary_oam_ = {0};  // 256 byte/64 sprite ram, for curent frame (Object Attribute Memory)
  union {
    uint8_t byte[0x40];
    Sprite  sprite[0x08];
  } secondary_oam_                          = {0};      // 64 byte/8 sprite ram, for current scanline
  uint8_t          num_sprites_fetched_     = {0};      // The number of sprites fetched so far
  bool             oam_has_sprite_zero_     = {false};  // Whether the secondary OAM contains sprite 0 (next line)
  bool             sr_has_sprite_zero_      = {false};  // Whether the sprite registers contains sprite 0 (current line)
  bool             did_hit_sprite_zero_     = {false};  // Whether sprite zero hit has occurred this frame
  uint8_t          sprite_pattern_sr_a_[8]  = {0};      // Lower byte of pattern, controls bit 0 of the color
  uint8_t          sprite_pattern_sr_b_[8]  = {0};      // Upper byte of pattern, controls bit 1 of the color
  SpriteAttributes sprite_palette_latch_[8] = {};       //
  uint8_t          sprite_x_position_[8]    = {0};      //


  // Memory-mapped IO Registers
  uint8_t   io_latch_ = {0};  //
  CtrlReg1  ctrl_reg_1_;      // PPU Control Register 1, mapped to CPU 0x2000 (RW)
  CtrlReg2  ctrl_reg_2_;      // PPU Control Register 2, mapped to CPU 0x2001 (RW)
  StatusReg status_reg_;      // PPU Status Register, mapped to CPU 0x2002 (R)
  uint8_t   oam_addr_ = {0};  // Object Attribute Memory Address, mapped to CPU 0x2003 (W)

  uint8_t vblank_suppression_counter_ = {0};


  // Memory
  mapper::Mapper* mapper_         = {nullptr};
  uint8_t         ram_[0x2000]    = {0};        // 8KiB RAM, at address 0x2000-0x3FFF
  uint8_t*        chr_mem_        = {nullptr};  // Character VRAM/VROM, at address 0x0000-0x1FFF
  bool            chr_mem_is_ram_ = {false};    // Whether chr_mem is VRAM or VROM


  // Rendering
  uint32_t pixels_[256 * 240] = {0};  // Screen buffer
  uint16_t scanline_          = {0};  // 0 to 262
  uint16_t cycle_             = {1};  // 0 to 341
  bool     frame_is_odd_      = true;


  // Internal operations
  uint8_t readByte(uint16_t address) const;
  void    writeByte(uint16_t address, uint8_t data);
  void    renderPixel();

  inline void fetchTilesAndSprites(bool fetch_sprites);
  inline void fetchNextBGTile();
  inline void fetchNextSprite();
  inline void incrementCoarseX();
  inline void incrementFineY();
};

}  // namespace hw::ppu
