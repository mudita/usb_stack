/*
 * Copyright  Onplick <info@onplick.com> - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 */
#include "FreeRTOS.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

#include <klib/khash.h>

#if DEBUG_DB
#   include <fsl_debug_console.h>
#   define LOG(format...) PRINTF("[MTPDB]: "format)
#else
#   define LOG(...) LOG_INFO(__VA_ARGS__)
#endif

#define NO_HANDLE (0)
typedef uint32_t handle_t;

KHASH_MAP_INIT_INT(MTPF, char*)
KHASH_MAP_INIT_STR(MTPR, handle_t)

struct mtp_db {
    khash_t(MTPF) *fmap;
    khash_t(MTPR) *rmap;
    handle_t lastHandle;
};

static handle_t db_alloc(struct mtp_db *db, const char *key)
{
    if (db->lastHandle==INT32_MAX) {
        LOG("MTP DB max size reached!");
        return NO_HANDLE;
    }

    char* myVal = (char*)malloc(strlen(key)+1);
    if (myVal == NULL) {
        LOG("Not enough memory to allocate MTP DB string!\n");
        return NO_HANDLE;
    }
    memcpy(myVal, key, strlen(key)+1);

    int ret = 0;
    khiter_t fiter = kh_put(MTPF, db->fmap, db->lastHandle+1, &ret);
    if (ret==-1) {
        LOG("Error while inserting MTPF DB key!\n");
        free(myVal);
        return NO_HANDLE;
    }
    kh_val(db->fmap, fiter) = myVal;

    khiter_t riter = kh_put(MTPR, db->rmap, myVal, &ret);
    if (ret==-1) {
        LOG("Error while inserting MTPR DB key!\n");
        free(myVal);
        kh_del(MTPF, db->fmap, fiter);
        return NO_HANDLE;
    }
    kh_val(db->rmap, riter) = db->lastHandle+1;

    return ++db->lastHandle;
}

static void db_update(struct mtp_db *db, uint32_t handle, const char *key)
{
    if (kh_size(db->fmap)==0) {
        LOG("Trying to update element from empty MTP DB\n");
        return;
    }

    char* myVal = (char*)malloc(strlen(key)+1);
    if (myVal == NULL) {
        LOG("Not enough memory to allocate MTP DB updated string!\n");
        return;
    }
    memcpy(myVal, key, strlen(key)+1);

    khiter_t fiter = kh_get(MTPF, db->fmap, handle);
    if (fiter==kh_end(db->fmap)) {
        free(myVal);
        LOG("Cannot update - missing MTPF element in DB\n");
        return;
    }

    int ret = 0;
    khiter_t riter = kh_put(MTPR, db->rmap, myVal, &ret);
    if (ret==-1) {
        LOG("Error while updating MTPR DB key!\n");
        free(myVal);
        return;
    }

    khiter_t riterOld = kh_get(MTPR, db->rmap, kh_val(db->fmap, fiter));
    if (riterOld==kh_end(db->rmap)) {
        LOG("Internal MTPR DB error while update\n");
    }
    else {
        kh_del(MTPR, db->rmap, riterOld);
    }

    free(kh_val(db->fmap, fiter));
    kh_val(db->fmap, fiter) = myVal;
    kh_val(db->rmap, riter) = handle;
}

static void db_free(struct mtp_db *db, handle_t handle)
{
    if (kh_size(db->fmap)==0) {
        LOG("Trying to remove element from empty MTP DB\n");
        return;
    }

    khiter_t fiter = kh_get(MTPF, db->fmap, handle);
    if (fiter==kh_end(db->fmap)) {
        LOG("Cannot remove - missing MTP element in DB\n");
        return;
    }

    khiter_t riter = kh_get(MTPR, db->rmap, kh_val(db->fmap, fiter));
    if (riter==kh_end(db->rmap)) {
        LOG("Internal MTPR DB error while removing element\n");
    }
    else {
        kh_del(MTPR, db->rmap, riter);
    }

    kh_del(MTPF, db->fmap, fiter);
    free(kh_val(db->fmap, fiter));
}

static handle_t db_getr(struct mtp_db *db, const char *key)
{
    khiter_t iter = kh_get(MTPR, db->rmap, key);
    if (iter==kh_end(db->rmap)) {
        LOG("Cannot get MTPR element in DB\n");
        return NO_HANDLE;
    }

    return kh_val(db->rmap, iter);
}

const char* db_getf(struct mtp_db *db, const handle_t handle)
{
    khiter_t iter = kh_get(MTPF, db->fmap, handle);
    if (iter==kh_end(db->fmap)) {
        LOG("Cannot get MTPF element in DB\n");
        return NULL;
    }

    return kh_val(db->fmap, iter);
}

struct mtp_db* mtp_db_alloc(void)
{
    struct mtp_db *db = (struct mtp_db*)malloc(sizeof(struct mtp_db));
    if (!db) {
        LOG("Not enough memory to allocate MTP DB!\n");
        return NULL;
    }
    memset(db, 0, sizeof(struct mtp_db));
    db->fmap = kh_init(MTPF);
    db->rmap = kh_init(MTPR);

    return db;
}

void mtp_db_free(struct mtp_db* db)
{
    for (khiter_t iter = kh_begin(db->fmap); iter != kh_end(db->fmap); ++iter) {
        if (kh_exist(db->fmap, iter)) {
            free(kh_val(db->fmap, iter));
        }
    }

    kh_destroy(MTPF, db->fmap);
    kh_destroy(MTPR, db->rmap);
    free(db);
}

uint32_t mtp_db_add(struct mtp_db *db, const char *key)
{
    assert(db && key);

    uint32_t handle = db_getr(db, key);
    if (handle)
        return handle;

    handle = db_alloc(db, key);
    if (handle) {
        LOG("add [%u]: %s\n", (unsigned int)handle, key);
        return handle;
    }
    return NO_HANDLE;
}

void mtp_db_update(struct mtp_db *db, uint32_t handle, const char *key)
{
    assert(db);
    if (handle > 0 && handle <= db->lastHandle) {
        LOG("update [%u]: %s\n", (unsigned int)handle, key);
        db_update(db, handle, key);
    }
}

const char *mtp_db_get(struct mtp_db *db, uint32_t handle)
{
    assert(db);

    if (handle > 0 && handle <= db->lastHandle) {
        return db_getf(db, handle);
    }
    return NULL;
}

void mtp_db_del(struct mtp_db *db, uint32_t handle)
{
    assert(db);

    if (handle > 0 && handle <= db->lastHandle) {
        LOG("drop [%u]\n", (unsigned int)handle);
        db_free(db, handle);
    }
}
