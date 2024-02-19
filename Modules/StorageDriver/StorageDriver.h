/* Copyright © 2023 Georgy E. All rights reserved. */

#pragma once


#include <stdint.h>

#include "StorageAT.h"


struct StorageDriver: public IStorageDriver
{
    StorageStatus read(uint32_t address, uint8_t *data, uint32_t len) override;
    StorageStatus write(uint32_t address, uint8_t *data, uint32_t len) override;
};
