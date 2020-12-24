#pragma once

#include "internal/mapper_000.h"
#include "internal/mapper_001.h"
#include "internal/mapper_002.h"
#include "internal/mapper_003.h"
#include "internal/mapper_004.h"
#include <nesemu/hw/mapper/mapper_base.h>
#include <nesemu/logger.h>


namespace hw::mapper {

namespace internal {

template <class T>
Mapper* make(uint8_t prg_banks, uint8_t chr_banks, Mirroring mirror) {
  return new T(prg_banks, chr_banks, mirror);
}

template <int N>
Mapper* dummy(uint8_t /*prg_banks*/, uint8_t /*chr_banks*/, Mirroring /*mirror*/) {
  logger::log<logger::ERROR>("Mapper %d not supported!\n", N);
  return nullptr;
}

}  // namespace internal


Mapper* (*mappers[])(uint8_t, uint8_t, Mirroring) = {
    internal::make<internal::Mapper000>,  // Mapper 000 - Nintendo NROM
    internal::make<internal::Mapper001>,  // Mapper 001 - Nintendo MMC1
    internal::make<internal::Mapper002>,  // Mapper 002 - Nintendo UxROM
    internal::make<internal::Mapper003>,  // Mapper 003 - Nintendo CNROM
    internal::make<internal::Mapper004>,  // Mapper 004 - Nintendo MMC3
    internal::dummy<5>,                   // Mapper 005
    internal::dummy<6>,                   // Mapper 006
    internal::dummy<7>,                   // Mapper 007
    internal::dummy<8>,                   // Mapper 008
    internal::dummy<9>,                   // Mapper 009
    internal::dummy<10>,                  // Mapper 010
                                          // Etc
};

}  // namespace hw::mapper
