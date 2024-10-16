/* Copyright © 2023 Georgy E. All rights reserved. */

#include "RecordDB.h"

#include <stdint.h>
#include <string.h>

#include "StorageAT.h"

#include "glog.h"
#include "soul.h"
#include "clock.h"
#include "gutils.h"
#include "settings.h"
#include "liquid_sensor.h"


extern StorageAT storage;

extern settings_t settings;


const char* RecordDB::RECORD_PREFIX = "RCR";
const char* RecordDB::TAG = "RCR";


RecordDB::RecordDB(uint32_t recordId): m_recordId(recordId) { }

RecordDB::RecordStatus RecordDB::load()
{
    uint32_t address = 0;

    StorageStatus storageStatus = storage.find(FIND_MODE_EQUAL, &address, RECORD_PREFIX, this->record.id);
    if (storageStatus == STORAGE_BUSY) {
    	return RECORD_ERROR;
    }
    if (storageStatus != STORAGE_OK) {
    	storageStatus = storage.find(FIND_MODE_NEXT, &address, RECORD_PREFIX, this->record.id);
    }
    if (storageStatus != STORAGE_OK) {
#if RECORD_BEDUG
        printTagLog(RecordDB::TAG, "error load: find clust");
#endif
        return (storageStatus == STORAGE_NOT_FOUND) ? RECORD_NO_LOG : RECORD_ERROR;
    }

    RecordStatus recordStatus = this->loadClust(address);
    if (recordStatus != RECORD_OK) {
#if RECORD_BEDUG
        printTagLog(RecordDB::TAG, "error load: load clust");
#endif
        return (storageStatus == STORAGE_NOT_FOUND) ? RECORD_NO_LOG : RECORD_ERROR;
    }

    bool recordFound = false;
    unsigned id;
    for (unsigned i = 0; i < CLUST_SIZE; i++) {
    	if (this->m_clust.records[i].id == this->m_recordId) {
    		recordFound = true;
    		id = i;
    		break;
    	}
    }
    if (!recordFound) {
#if RECORD_BEDUG
        printTagLog(RecordDB::TAG, "error load: find record");
#endif
        return RECORD_NO_LOG;
    }

    memcpy(reinterpret_cast<void*>(&(this->record)), reinterpret_cast<void*>(&(this->m_clust.records[id])), sizeof(this->record));

#if RECORD_BEDUG
    printTagLog(RecordDB::TAG, "record loaded from address=%08X", (unsigned int)address);
#endif

    return RECORD_OK;
}

RecordDB::RecordStatus RecordDB::loadNext()
{
    uint32_t address = 0;

    StorageStatus storageStatus = storage.find(FIND_MODE_NEXT, &address, RECORD_PREFIX, this->m_recordId);
    if (storageStatus != STORAGE_OK) {
#if RECORD_BEDUG
        printTagLog(RecordDB::TAG, "error load next: find next record");
#endif
        return (storageStatus == STORAGE_NOT_FOUND) ? RECORD_NO_LOG : RECORD_ERROR;
    }

    RecordStatus recordStatus = this->loadClust(address);
    if (recordStatus != RECORD_OK) {
#if RECORD_BEDUG
        printTagLog(RecordDB::TAG, "error load next: load clust");
#endif
        return (storageStatus == STORAGE_NOT_FOUND) ? RECORD_NO_LOG : RECORD_ERROR;
    }

    bool recordFound = false;
	unsigned idx;
	uint32_t curId = 0xFFFFFFFF;
	for (unsigned i = 0; i < CLUST_SIZE; i++) {
		if (this->m_clust.records[i].id > this->m_recordId && curId > this->m_clust.records[i].id) {
			curId =this->m_clust.records[i].id;
			recordFound = true;
			idx = i;
			break;
		}
	}
    if (!recordFound) {
#if RECORD_BEDUG
        printTagLog(RecordDB::TAG, "error load next: find record");
#endif
        return RECORD_NO_LOG;
    }

    memcpy(
		reinterpret_cast<void*>(&(this->record)),
		reinterpret_cast<void*>(&(this->m_clust.records[idx])),
		sizeof(this->record)
	);

#if RECORD_BEDUG
    printTagLog(RecordDB::TAG, "next record loaded from address=%08X", (unsigned int)address);
#endif

    return RECORD_OK;
}

RecordDB::RecordStatus RecordDB::save()
{
    uint32_t id = 0;
    RecordStatus recordStatus = getNewId(&id);
    if (recordStatus != RECORD_OK) {
#if RECORD_BEDUG
        printTagLog(RecordDB::TAG, "error save: get new id");
#endif
        return RECORD_ERROR;
    }

    this->record.id = id;

    uint32_t address = 0;
    StorageFindMode findMode = FIND_MODE_MAX;
    StorageStatus storageStatus = storage.find(findMode, &address, RECORD_PREFIX);
    if (storageStatus == STORAGE_BUSY) {
    	return RECORD_ERROR;
    }

	bool idFound = false;
	uint32_t idx;
    while (storageStatus != STORAGE_OOM) {
		if (storageStatus != STORAGE_OK) {
			findMode = FIND_MODE_EMPTY;
			storageStatus = storage.find(findMode, &address, RECORD_PREFIX);
		}
		if (storageStatus != STORAGE_OK) {
			findMode = FIND_MODE_MIN;
			storageStatus = storage.find(findMode, &address, RECORD_PREFIX);
		}
		if (storageStatus == STORAGE_BUSY) {
#if RECORD_BEDUG
			printTagLog(RecordDB::TAG, "error save: find address for save record (storage busy)");
#endif
			return RECORD_ERROR;
		}
		if (storageStatus != STORAGE_OK) {
#if RECORD_BEDUG
			printTagLog(RecordDB::TAG, "error save: find address for save record");
#endif
			return RECORD_ERROR;
		}

		if (findMode != FIND_MODE_EMPTY) {
			recordStatus = this->loadClust(address);
		}
		if (recordStatus != RECORD_OK) {
#if RECORD_BEDUG
			printTagLog(RecordDB::TAG, "error save: load clust");
#endif
			return RECORD_ERROR;
		}
		if (findMode == FIND_MODE_MIN || findMode == FIND_MODE_EMPTY) {
			memset(reinterpret_cast<void*>(&(this->m_clust)), 0, sizeof(this->m_clust));
		}

		idFound = false;
		for (unsigned i = 0; i < __arr_len(this->m_clust.records); i++) {
			if (this->m_clust.records[i].id == 0) {
				idFound = true;
				idx = i;
				break;
			}
		}
		if (idFound) {
			break;
		}

		storageStatus = STORAGE_ERROR;
    }

    if (!idFound) {
#if RECORD_BEDUG
        printTagLog(RecordDB::TAG, "error save: find record id in clust");
#endif
        return RECORD_ERROR;
    }

    this->m_clust.rcrd_magic = CLUST_MAGIC;
    this->m_clust.rcrd_ver = CLUST_VERSION;
    memcpy(
		reinterpret_cast<void*>(&(this->m_clust.records[idx])),
		reinterpret_cast<void*>(&(this->record)),
		sizeof(this->record)
	);
    if (idx < CLUST_SIZE) {
    	memset(
			reinterpret_cast<void*>(&(this->m_clust.records[idx + 1])),
			0,
			((CLUST_SIZE - idx - 1) * sizeof(struct _Record))
		);
    }

    storageStatus = storage.rewrite(
        address,
        RECORD_PREFIX,
        this->record.id,
        reinterpret_cast<uint8_t*>(&this->m_clust),
        sizeof(this->m_clust)
    );
    if (storageStatus != STORAGE_OK) {
#if RECORD_BEDUG
        printTagLog(RecordDB::TAG, "error save: save clust");
#endif
        return RECORD_ERROR;
    }

    set_status(HAS_NEW_RECORD);

    printTagLog(
		RecordDB::TAG,
		"record saved on address=%08X",
		(unsigned int)address
	);
    gprint("ID:      %lu\n",         record.id);
	gprint("Time:    %s\n",          get_clock_time_format_by_sec(record.time));
	gprint("Level:   %ld %s\n",      record.level / 1000, (record.level == LEVEL_ERROR ? "" : "l"));
	gprint("Press 1: %u.%02u MPa\n", record.press_1 / 100, record.press_1 % 100);
//	gprint("Press 2: %d.%02d MPa\n",                     record.press_2 / 100, record.press_2 % 100);

    return RECORD_OK;
}

RecordDB::RecordStatus RecordDB::loadClust(uint32_t address)
{
    RecordClust tmpClust;
    StorageStatus status = storage.load(address, reinterpret_cast<uint8_t*>(&tmpClust), sizeof(tmpClust));
    if (status != STORAGE_OK) {
#if RECORD_BEDUG
        printTagLog(RecordDB::TAG, "error load clust");
#endif
        return RECORD_ERROR;
    }

    if (tmpClust.rcrd_magic != CLUST_MAGIC) {
#if RECORD_BEDUG
        printTagLog(RecordDB::TAG, "error record clust magic");
#endif
        storage.clearAddress(address);
        return RECORD_ERROR;
    }

    if (tmpClust.rcrd_ver != CLUST_VERSION) {
#if RECORD_BEDUG
        printTagLog(RecordDB::TAG, "error record clust version");
#endif
        storage.clearAddress(address);
        return RECORD_ERROR;
    }

    memcpy(reinterpret_cast<void*>(&this->m_clust), reinterpret_cast<void*>(&tmpClust), sizeof(this->m_clust));

#if RECORD_BEDUG
    printTagLog(RecordDB::TAG, "clust loaded from address=%08X", (unsigned int)address);
#endif

    return RECORD_OK;
}

RecordDB::RecordStatus RecordDB::getNewId(uint32_t *newId)
{
    uint32_t address = 0;

    StorageStatus status = storage.find(FIND_MODE_MAX, &address, RECORD_PREFIX);
    if (status == STORAGE_NOT_FOUND) {
        *newId = settings.server_log_id + 1;
#if RECORD_BEDUG
        printTagLog(RecordDB::TAG, "max ID not found, reset max ID");
#endif
        return RECORD_OK;
    }
    if (status != STORAGE_OK) {
#if RECORD_BEDUG
        printTagLog(RecordDB::TAG, "error get new id");
#endif
        return RECORD_ERROR;
    }

    RecordClust tmpClust;
    status = storage.load(address, reinterpret_cast<uint8_t*>(&tmpClust), sizeof(tmpClust));
    if (status != STORAGE_OK) {
#if RECORD_BEDUG
        printTagLog(RecordDB::TAG, "error get new id");
#endif
        return RECORD_ERROR;
    }

    *newId = 0;
    for (unsigned i = 0; i < __arr_len(tmpClust.records); i++) {
    	if (*newId < tmpClust.records[i].id) {
    		*newId = tmpClust.records[i].id;
    	}
    }

    if (*newId + 1 <= settings.server_log_id) {
    	*newId = settings.server_log_id + 1;
    } else {
        *newId = *newId + 1;
    }

#if RECORD_BEDUG
    printTagLog(RecordDB::TAG, "new ID received from address=%08X id=%lu", (unsigned int)address, *newId);
#endif

    return RECORD_OK;
}
