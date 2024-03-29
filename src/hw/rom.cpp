#include <nesemu/hw/rom.h>

#include <nesemu/logger.h>
#include <nesemu/utils/crc.h>

#include <fstream>
#include <iomanip>
#include <sstream>


// TODO: Make more lightweight
inline std::string uint8_to_hex_string(const uint8_t* arr, const std::size_t size) {
  std::stringstream ss;
  ss << std::hex << std::setfill('0');

  for (unsigned i = 0; i < size; i++) {
    ss << "0x" << std::hex << std::setw(2) << static_cast<int>(arr[i]);
    if (i < size - 1) {
      ss << " ";
    }
  }
  return ss.str();
}


int hw::rom::parseFromFile(std::string filename, Rom* rom) {
  std::ifstream file(filename, std::ios::in | std::ios::binary | std::ios::ate);

  if (!file) {
    logger::log<logger::ERROR>("Unable to open file '%s'\n", filename.c_str());
    return 1;
  }
  logger::log<logger::INFO>("Loading ROM from file '%s'... ", filename.c_str());

  std::streampos size = file.tellg();
  file.seekg(0, std::ios::beg);

  file.read(reinterpret_cast<char*>(&rom->header), 16);

  unsigned expected_size = 16 + (rom->header.has_trainer ? 512 : 0) + (rom->header.prg_rom_size * 16 * 1024)
                           + (rom->header.chr_rom_size * 8 * 1024);

  if (!std::equal(rom->header.name, std::end(rom->header.name), header_name)) {
    logger::log<logger::ERROR>("Unable to parse file %s: Invalid header beginning with '%s'\n",
                               filename.c_str(),
                               uint8_to_hex_string(rom->header.name, 4).c_str());
    return 1;
  }

  if (expected_size != size) {
    logger::log<logger::ERROR>("Unable to parse file %s: Expected $%0X bytes, but file is $%0X bytes\n",
                               filename.c_str(),
                               expected_size,
                               size);
    return 1;
  }

  if (rom->header.has_battery) {
    rom->expansion = new uint8_t[1][0x2000];

    // TODO: Handle trainers better (https://forums.nesdev.org/viewtopic.php?t=3657)
    if (rom->header.has_trainer) {
      rom->trainer = rom->expansion[0] + 0x1000;
      file.read(reinterpret_cast<char*>(&rom->trainer), 512);
    } else {
      rom->trainer = nullptr;
    }
  } else {
    rom->expansion = nullptr;
    rom->trainer   = nullptr;
  }

  rom->prg = new uint8_t[rom->header.prg_rom_size][16 * 1024];
  file.read(reinterpret_cast<char*>(rom->prg), rom->header.prg_rom_size * 16 * 1024);

  if (rom->header.chr_rom_size == 0) {
    rom->chr = new uint8_t[1][8 * 1024];  // Cartridge uses CHR RAM
  } else {
    rom->chr = new uint8_t[rom->header.chr_rom_size][8 * 1024];
    file.read(reinterpret_cast<char*>(rom->chr), rom->header.chr_rom_size * 8 * 1024);
  }

  file.seekg(16, std::ios::beg);
  rom->crc = crc32_stream(file);

  logger::log<logger::INFO>("Done\n");
  return 0;
}
