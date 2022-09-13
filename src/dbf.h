//
// EnlyzeDbfLib - Library for reading dBASE .dbf database files
// Copyright (c) 2022 Colin Finck, ENLYZE GmbH <c.finck@enlyze.com>
// SPDX-License-Identifier: MIT
//

#pragma once

#include <cstdint>

// From https://en.wikipedia.org/wiki/.dbf
#pragma pack(push, 1)
struct DbfHeader
{
    uint8_t Version;
    struct
    {
        uint8_t Year;
        uint8_t Month;
        uint8_t Day;
    }
    LastUpdate;
    uint32_t RecordCount;
    uint16_t HeaderSize;
    uint16_t RecordSize;
    uint8_t Reserved1[2];
    uint8_t IncompleteTransactionFlag;
    uint8_t EncryptionFlag;
    uint8_t Reserved2[12];
    uint8_t ProductionMdxFlag;
    uint8_t LanguageDriverId;
    uint8_t Reserved3[2];
};
#pragma pack(pop)

#pragma pack(push, 1)
struct DbfFieldDescriptor
{
    char Name[11];
    char Type;
    uint8_t Reserved1[4];
    uint8_t Length;
    uint8_t DecimalCount;
    uint16_t WorkAreaID;
    uint8_t Example;
    uint8_t Reserved2[10];
    uint8_t ProductionMdxFlag;
};
#pragma pack(pop)

// dBASE IV Format from http://www.manmrk.net/tutorials/database/xbase/dbt.html
#pragma pack(push, 1)
struct DbtHeader
{
    uint32_t NextAvailableBlock;
    uint32_t Reserved1;
    char szFileName[9];
    uint8_t Reserved2[3];
    uint16_t BlockLength;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct DbtBlockHeader
{
    uint32_t Magic;
    uint32_t DataLength;
};
#pragma pack(pop)


#define DBASE_III_PLUS_VERSION      0x03
#define DBASE_IV_WITH_MEMO_VERSION  0x8B
#define FIELD_DESCRIPTOR_END_BYTE   0x0D
#define RECORD_DELETED              '*'
#define RECORD_NOT_DELETED          ' '
#define DBASE_FILE_END_BYTE         0x1A

#define DBT_DEFAULT_BLOCK_LENGTH    512
#define DBT_BLOCK_MAGIC             0x0008FFFF


// Arbitrary limits set by us to be on the safe side regarding memory allocations
#define DBT_MAX_BLOCK_NUMBER        1048576
#define DBT_MAX_BLOCK_LENGTH        2097152
