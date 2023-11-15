/* Copyright © 2023 Georgy E. All rights reserved. */

#include "RecordDB.h"

#include <stdint.h>
#include <string.h>

#include "StorageAT.h"
#include "SettingsDB.h"

#include "utils.h"


extern StorageAT storage;

extern SettingsDB settings;


const char RecordDB::RECORD_PREFIX[Page::PREFIX_SIZE] = "RCR";
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
        LOG_TAG_BEDUG(RecordDB::TAG, "error load: find clust");
#endif
        return (storageStatus == STORAGE_NOT_FOUND) ? RECORD_NO_LOG : RECORD_ERROR;
    }

    RecordStatus recordStatus = this->loadClust(address);
    if (recordStatus != RECORD_OK) {
#if RECORD_BEDUG
        LOG_TAG_BEDUG(RecordDB::TAG, "error load: load clust");
#endif
        return (storageStatus == STORAGE_NOT_FOUND) ? RECORD_NO_LOG : RECORD_ERROR;
    }

    bool recordFound = false;
    unsigned id;
    for (unsigned i = 0; i < sizeof(CLUST_SIZE); i++) {
    	if (this->m_clust.records[i].id == this->m_recordId) {
    		recordFound = true;
    		id = i;
    		break;
    	}
    }
    if (!recordFound) {
#if RECORD_BEDUG
        LOG_TAG_BEDUG(RecordDB::TAG, "error load: find record");
#endif
        return RECORD_NO_LOG;
    }

    memcpy(reinterpret_cast<void*>(&(this->record)), reinterpret_cast<void*>(&(this->m_clust.records[id])), sizeof(this->record));

#if RECORD_BEDUG
    LOG_TAG_BEDUG(RecordDB::TAG, "record loaded from address=%08X", (unsigned int)address);
#endif

    return RECORD_OK;
}

RecordDB::RecordStatus RecordDB::loadNext()
{
    uint32_t address = 0;

    StorageStatus storageStatus = storage.find(FIND_MODE_NEXT, &address, RECORD_PREFIX, this->m_recordId);
    if (storageStatus != STORAGE_OK) {
#if RECORD_BEDUG
        LOG_TAG_BEDUG(RecordDB::TAG, "error load next: find next record");
#endif
        return (storageStatus == STORAGE_NOT_FOUND) ? RECORD_NO_LOG : RECORD_ERROR;
    }

    RecordStatus recordStatus = this->loadClust(address);
    if (recordStatus != RECORD_OK) {
#if RECORD_BEDUG
        LOG_TAG_BEDUG(RecordDB::TAG, "error load next: load clust");
#endif
        return (storageStatus == STORAGE_NOT_FOUND) ? RECORD_NO_LOG : RECORD_ERROR;
    }

    bool recordFound = false;
	unsigned idx;
	uint32_t curId = 0xFFFFFFFF;
	for (unsigned i = 0; i < sizeof(CLUST_SIZE); i++) {
		if (this->m_clust.records[i].id > this->m_recordId && curId > this->m_clust.records[i].id) {
			curId =this->m_clust.records[i].id;
			recordFound = true;
			idx = i;
			break;
		}
	}
    if (!recordFound) {
#if RECORD_BEDUG
        LOG_TAG_BEDUG(RecordDB::TAG, "error load next: find record");
#endif
        return RECORD_NO_LOG;
    }

    memcpy(
		reinterpret_cast<void*>(&(this->record)),
		reinterpret_cast<void*>(&(this->m_clust.records[idx])),
		sizeof(this->record)
	);

#if RECORD_BEDUG
    LOG_TAG_BEDUG(RecordDB::TAG, "next record loaded from address=%08X", (unsigned int)address);
#endif

    return RECORD_OK;
}

RecordDB::RecordStatus RecordDB::save()
{
    uint32_t id = 0;
    RecordStatus recordStatus = getNewId(&id);
    if (recordStatus != RECORD_OK) {
#if RECORD_BEDUG
        LOG_TAG_BEDUG(RecordDB::TAG, "error save: get new id");
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
    if (storageStatus != STORAGE_OK) {
    	findMode = FIND_MODE_EMPTY;
    	storageStatus = storage.find(findMode, &address, RECORD_PREFIX);
    }
    if (storageStatus != STORAGE_OK) {
    	findMode = FIND_MODE_MIN;
    	storageStatus = storage.find(findMode, &address, RECORD_PREFIX);
    }
    if (storageStatus != STORAGE_OK) {
#if RECORD_BEDUG
        LOG_TAG_BEDUG(RecordDB::TAG, "error save: find address for save record");
#endif
        return RECORD_ERROR;
    }

    if (findMode != FIND_MODE_EMPTY) {
    	recordStatus = this->loadClust(address);
    }
    if (recordStatus != RECORD_OK) {
#if RECORD_BEDUG
        LOG_TAG_BEDUG(RecordDB::TAG, "error save: load clust");
#endif
        return RECORD_ERROR;
    }
    if (findMode == FIND_MODE_MIN) {
    	memset(reinterpret_cast<void*>(&(this->m_clust)), 0, sizeof(this->m_clust));
    }

    bool idFound = false;
    uint32_t idx;
    for (unsigned i = 0; i < __arr_len(this->m_clust.records); i++) {
    	if (this->m_clust.records[i].id == 0) {
    		idFound = true;
    		idx = i;
    		break;
    	}
    }
    if (!idFound) {
#if RECORD_BEDUG
        LOG_TAG_BEDUG(RecordDB::TAG, "error save: find record id in clust");
#endif
        return RECORD_ERROR;
    }

    this->m_clust.record_magic = CLUST_MAGIC;
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

    storageStatus = storage.save(
        address,
        RECORD_PREFIX,
        this->record.id,
        reinterpret_cast<uint8_t*>(&this->m_clust),
        sizeof(this->m_clust)
    );
    if (storageStatus != STORAGE_OK) {
#if RECORD_BEDUG
        LOG_TAG_BEDUG(RecordDB::TAG, "error save: save clust");
#endif
        return RECORD_ERROR;
    }

    settings.info.saved_new_log = true;

#if RECORD_BEDUG
    LOG_TAG_BEDUG(RecordDB::TAG, "record saved on address=%08X", (unsigned int)address);
    LOG_TAG_BEDUG(
		RecordDB::TAG,
		"\n"
		"ID:      %lu\n"
		"Time:    20%02u-%02u-%02uT%02u:%02u:%02u\n"
		"Level:   %lu l\n"
		"Press 1: %u.%02u MPa\n",
//		"Press 2: %d.%02d MPa\n",
		record.id,
		record.time[0], record.time[1], record.time[2], record.time[3], record.time[4], record.time[5],
		record.level,
		record.press_1 / 100, record.press_1 % 100
//		record.press_2 / 100, record.press_2 % 100
	);
#endif

    return RECORD_OK;
}

RecordDB::RecordStatus RecordDB::loadClust(uint32_t address)
{
    RecordClust tmpClust;
    StorageStatus status = storage.load(address, reinterpret_cast<uint8_t*>(&tmpClust), sizeof(tmpClust));
    if (status != STORAGE_OK) {
#if RECORD_BEDUG
        LOG_TAG_BEDUG(RecordDB::TAG, "error load clust");
#endif
        return RECORD_ERROR;
    }

    if (tmpClust.record_magic != CLUST_MAGIC) {
#if RECORD_BEDUG
        LOG_TAG_BEDUG(RecordDB::TAG, "error record magic clust");
#endif
        return RECORD_ERROR;
    }

    memcpy(reinterpret_cast<void*>(&this->m_clust), reinterpret_cast<void*>(&tmpClust), sizeof(this->m_clust));

#if RECORD_BEDUG
    LOG_TAG_BEDUG(RecordDB::TAG, "clust loaded from address=%08X", (unsigned int)address);
#endif

    return RECORD_OK;
}

RecordDB::RecordStatus RecordDB::getNewId(uint32_t *newId)
{
    uint32_t address = 0;

    StorageStatus status = storage.find(FIND_MODE_MAX, &address, RECORD_PREFIX);
    if (status == STORAGE_NOT_FOUND) {
        *newId = 1;
#if RECORD_BEDUG
        LOG_TAG_BEDUG(RecordDB::TAG, "max ID not found, reset max ID");
#endif
        return RECORD_OK;
    }
    if (status != STORAGE_OK) {
#if RECORD_BEDUG
        LOG_TAG_BEDUG(RecordDB::TAG, "error get new id");
#endif
        return RECORD_ERROR;
    }

    RecordClust tmpClust;
    status = storage.load(address, reinterpret_cast<uint8_t*>(&tmpClust), sizeof(tmpClust));
    if (status != STORAGE_OK) {
#if RECORD_BEDUG
        LOG_TAG_BEDUG(RecordDB::TAG, "error get new id");
#endif
        return RECORD_ERROR;
    }

    *newId = 0;
    for (unsigned i = 0; i < __arr_len(tmpClust.records); i++) {
    	if (*newId < tmpClust.records[i].id) {
    		*newId = tmpClust.records[i].id;
    	}
    }

    *newId = *newId + 1;

#if RECORD_BEDUG
    LOG_TAG_BEDUG(RecordDB::TAG, "new ID received from address=%08X id=%lu", (unsigned int)address, *newId);
#endif

    return RECORD_OK;
}
