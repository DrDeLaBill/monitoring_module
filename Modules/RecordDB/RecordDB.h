/* Copyright © 2023 Georgy E. All rights reserved. */

#pragma once


#include <stdint.h>

#include "StorageAT.h"


#define RECORD_BEDUG (true)


class RecordDB
{
public:
    typedef enum _RecordStatus {
        RECORD_OK = 0,
        RECORD_ERROR,
        RECORD_NO_LOG
    } RecordStatus;

    static const uint32_t RECORD_TIME_ARRAY_SIZE  = 6;
    typedef struct __attribute__((packed)) _Record {
    	uint32_t id;                           // Record ID
    	uint8_t  time[RECORD_TIME_ARRAY_SIZE]; // Record time
    	uint32_t cf_id;                        // Configuration version
    	int32_t  level;                        // Liquid level
    	uint16_t press_1;                      // First pressure sensor
//    	uint32_t press_2;                      // Second pressure sensor
    	uint32_t pump_wok_time;                // Log pump downtime sec
    	uint32_t pump_downtime;                // Log pump work sec
    } Record;

    RecordDB(uint32_t recordId);

    RecordStatus load();
    RecordStatus loadNext();
    RecordStatus save();

    Record record = { 0 };

private:
    static const char RECORD_PREFIX[Page::PREFIX_SIZE];
    static const char* TAG;

    static const uint32_t CLUST_SIZE  = ((Page::PAYLOAD_SIZE - sizeof(uint8_t)) / sizeof(struct _Record));
    static const uint32_t CLUST_MAGIC = (sizeof(struct _Record));

    typedef struct __attribute__((packed)) _RecordClust {
        uint8_t record_magic;
        Record  records[CLUST_SIZE];
    } RecordClust;


    uint32_t m_recordId;

    uint32_t m_clustId;
    RecordClust m_clust;

    RecordDB() {}

    RecordStatus loadClust(uint32_t address);
    RecordStatus getNewId(uint32_t *newId);
};
