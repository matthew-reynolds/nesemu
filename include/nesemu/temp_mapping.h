#pragma once

#include <cstdint>

// TODO: Load from .pal file
inline uint32_t decodeColor(uint8_t color) {
  switch (color) {
    case 0x00:
      return 0x00757575;
    case 0x01:
      return 0x00271B8F;
    case 0x02:
      return 0x000000AB;
    case 0x03:
      return 0x0047009F;
    case 0x04:
      return 0x008F0077;
    case 0x05:
      return 0x00AB0013;
    case 0x06:
      return 0x00A70000;
    case 0x07:
      return 0x007F0B00;
    case 0x08:
      return 0x00432F00;
    case 0x09:
      return 0x00004700;
    case 0x0A:
      return 0x00005100;
    case 0x0B:
      return 0x00003F17;
    case 0x0C:
      return 0x001B3F5F;
    case 0x0D:
      return 0x00000000;
    case 0x0E:
      return 0x00000000;
    case 0x0F:
      return 0x00000000;

    case 0x10:
      return 0x00BCBCBC;
    case 0x11:
      return 0x000073EF;
    case 0x12:
      return 0x00233BEF;
    case 0x13:
      return 0x008300F3;
    case 0x14:
      return 0x00BF00BF;
    case 0x15:
      return 0x00E7005B;
    case 0x16:
      return 0x00DB2B00;
    case 0x17:
      return 0x00CB4F0F;
    case 0x18:
      return 0x008B7300;
    case 0x19:
      return 0x00009700;
    case 0x1A:
      return 0x0000AB00;
    case 0x1B:
      return 0x0000933B;
    case 0x1C:
      return 0x0000838B;
    case 0x1D:
      return 0x00000000;
    case 0x1E:
      return 0x00000000;
    case 0x1F:
      return 0x00000000;

    case 0x20:
      return 0x00FFFFFF;
    case 0x21:
      return 0x003FBFFF;
    case 0x22:
      return 0x005F97FF;
    case 0x23:
      return 0x00A78BFD;
    case 0x24:
      return 0x00F77BFF;
    case 0x25:
      return 0x00FF77B7;
    case 0x26:
      return 0x00FF7763;
    case 0x27:
      return 0x00FF9B3B;
    case 0x28:
      return 0x00F3BFF3;
    case 0x29:
      return 0x0083D313;
    case 0x2A:
      return 0x004FDF4B;
    case 0x2B:
      return 0x0058F898;
    case 0x2C:
      return 0x0000EBDB;
    case 0x2D:
      return 0x003C3C3C;
    case 0x2E:
      return 0x00000000;
    case 0x2F:
      return 0x00000000;

    case 0x30:
      return 0x00FFFFFF;
    case 0x31:
      return 0x00ABE7FF;
    case 0x32:
      return 0x00C7D7FF;
    case 0x33:
      return 0x00D7CBFF;
    case 0x34:
      return 0x00FFC7FF;
    case 0x35:
      return 0x00FFC7DB;
    case 0x36:
      return 0x00FFBFB3;
    case 0x37:
      return 0x00FFDBAB;
    case 0x38:
      return 0x00FFE7A3;
    case 0x39:
      return 0x00E3FFA3;
    case 0x3A:
      return 0x00ABF3BF;
    case 0x3B:
      return 0x00B3FFCF;
    case 0x3C:
      return 0x009FFFF3;
    case 0x3D:
      return 0x00A0A2A0;
    case 0x3E:
      return 0x00000000;
    case 0x3F:
      return 0x00000000;

    default:
      return 0;
  }
}

template <uint8_t CHANNEL_POS>
uint32_t attenuate_channel(uint32_t color, float attenuation) {
  constexpr uint32_t MASK = 0xFF << CHANNEL_POS;

  uint8_t channel = (color & MASK) >> CHANNEL_POS;
  channel *= attenuation;
  return (color & ~MASK) | (channel << CHANNEL_POS);
}

constexpr auto attenuate_red   = attenuate_channel<16>;
constexpr auto attenuate_green = attenuate_channel<8>;
constexpr auto attenuate_blue  = attenuate_channel<0>;
