//
// EnlyzeDbfLib - Library for reading dBASE .dbf database files
// Copyright (c) 2022 Colin Finck, ENLYZE GmbH <c.finck@enlyze.com>
// SPDX-License-Identifier: MIT
//

#include <EnlyzeWinStringLib.h>

#include "CDbfReader.h"
#include "dbf.h"


template<class T>
std::ifstream& _ReadObject(std::ifstream& File, T& ObjectData)
{
    File.read(reinterpret_cast<char*>(&ObjectData), sizeof(T));
    return File;
}


static std::variant<std::monostate, CDbfError>
_BeginDbt(std::ifstream& DbtFile, uint16_t &BlockLength, const std::wstring& wstrDbfFilePath)
{
    // Open the corresponding memo file (.dbt)
    std::wstring wstrDbtFilePath = wstrDbfFilePath;
    wstrDbtFilePath.pop_back();
    wstrDbtFilePath.push_back(L't');

    DbtFile.open(wstrDbtFilePath.c_str(), std::ios::binary);
    if (!DbtFile)
    {
        return CDbfError(L"Could not open DBT memo file \"" + wstrDbtFilePath + L"\"");
    }

    // Read the header.
    DbtHeader Header;
    if (!_ReadObject(DbtFile, Header))
    {
        return CDbfError(L"Could not read DbtHeader from \"" + wstrDbtFilePath + L"\"");
    }

    // Verify the header.
    if (std::find(std::begin(DBT_BLOCK_LENGTHS), std::end(DBT_BLOCK_LENGTHS), Header.BlockLength) == std::end(DBT_BLOCK_LENGTHS))
    {
        return CDbfError(L"DBT memo file \"" + wstrDbtFilePath + L"\" has unknown block length " + std::to_wstring(Header.BlockLength));
    }

    BlockLength = Header.BlockLength;

    return std::monostate();
}

static std::variant<std::vector<FieldInfo>, CDbfError>
_ReadDbfFieldDescriptors(std::ifstream& DbfFile, uint16_t HeaderSize)
{
    std::vector<FieldInfo> Fields;

    while (DbfFile.tellg() < HeaderSize)
    {
        // Look for the byte that terminates the list of field descriptors.
        char EndByte;
        if (!DbfFile.read(&EndByte, 1))
        {
            return CDbfError(L"Could not read EndByte");
        }

        if (EndByte == FIELD_DESCRIPTOR_END_BYTE)
        {
            return Fields;
        }

        // This wasn't the termination byte, so go back and read this again as a field descriptor.
        DbfFile.seekg(-1, std::ios_base::cur);

        DbfFieldDescriptor FieldDescriptor;
        if (!_ReadObject(DbfFile, FieldDescriptor))
        {
            return CDbfError(L"Could not read FieldDescriptor");
        }

        // Extract the field name from the fixed-size char array (padded with NUL bytes).
        FieldInfo Field;

        Field.strName = std::string(FieldDescriptor.Name, std::size(FieldDescriptor.Name));
        size_t NulPosition = Field.strName.find('\0');
        if (NulPosition != std::string::npos)
        {
            Field.strName.resize(NulPosition);
        }

        Field.cType = FieldDescriptor.Type;
        Field.Length = FieldDescriptor.Length;

        Fields.push_back(Field);
    }

    return CDbfError(L"Did not find the field descriptor end byte");
}


CDbfReader::CDbfReader(std::ifstream&& DbfFile, std::ifstream&& DbtFile, std::vector<FieldInfo> Fields, size_t DataStart, uint16_t BlockLength)
    : m_DataStart(DataStart), m_BlockLength(BlockLength), m_Fields(Fields), m_DbfFile(std::move(DbfFile)), m_DbtFile(std::move(DbtFile))
{
}

std::variant<std::unique_ptr<CDbfReader>, CDbfError>
CDbfReader::ReadDbf(const std::wstring& wstrDbfFilePath)
{
    // Open the .dbf file for reading.
    std::ifstream DbfFile(wstrDbfFilePath.c_str(), std::ios::binary);
    if (!DbfFile)
    {
        return CDbfError(L"Could not open \"" + wstrDbfFilePath + L"\"");
    }

    // Read the header.
    DbfHeader Header;
    if (!_ReadObject(DbfFile, Header))
    {
        return CDbfError(L"Could not read DbfHeader from \"" + wstrDbfFilePath + L"\"");
    }

    // Verify the header.
    std::ifstream DbtFile;
    uint16_t BlockLength = 0;
    if (Header.Version == DBASE_III_PLUS_VERSION)
    {
        // Nothing else to do.
    }
    else if (Header.Version == DBASE_IV_WITH_MEMO_VERSION)
    {
        // Open and verify the corresponding memo file (.dbt)
        auto ReadDbtResult = _BeginDbt(DbtFile, BlockLength, wstrDbfFilePath);
        if (const auto pError = std::get_if<CDbfError>(&ReadDbtResult))
        {
            return *pError;
        }
    }
    else
    {
        return CDbfError(L"Invalid dBASE Version " + std::to_wstring(Header.Version) + L" in file \"" + wstrDbfFilePath + L"\"");
    }

    // Read the field descriptors.
    auto ReadResult = _ReadDbfFieldDescriptors(DbfFile, Header.HeaderSize);
    if (const auto pError = std::get_if<CDbfError>(&ReadResult))
    {
        return CDbfError(pError->Message() + L" in file \"" + wstrDbfFilePath + L"\"");
    }

    auto Fields = std::get<std::vector<FieldInfo>>(std::move(ReadResult));
    size_t DataStart = DbfFile.tellg();

    return std::unique_ptr<CDbfReader>(
        new CDbfReader(
            std::move(DbfFile),
            std::move(DbtFile),
            Fields,
            DataStart,
            BlockLength
        )
    );
}

std::variant<size_t, CDbfError>
CDbfReader::GetFieldIndex(const std::string& strField) const
{
    for (size_t i = 0; i < m_Fields.size(); i++)
    {
        if (m_Fields[i].strName == strField)
        {
            return i;
        }
    }

    return CDbfError(L"Could not find column \"" + StrToWstr(strField) + L"\"");
}

std::variant<std::monostate, CDbfError>
CDbfReader::_ReadMemoColumn(std::string& Column)
{
    // Check the block number.
    auto Option = StrToSizeT(Column);
    if (!Option.has_value())
    {
        return CDbfError(L"Invalid reference to memo block: " + StrToWstr(Column));
    }

    size_t BlockNumber = Option.value();
    if (BlockNumber == 0)
    {
        // A zero block number indicates an empty cell. Return that.
        Column.clear();
        return std::monostate();
    }

    // We should never hit a block number that high, but this is a safe upper bound for the following calculations.
    if (BlockNumber > DBT_MAX_BLOCK_NUMBER)
    {
        return CDbfError(L"Block number " + std::to_wstring(BlockNumber) + L" is too high");
    }

    // Calculate the block address in the .dbt memo file from the block number currently stored in the Column variable.
    size_t BlockBegin = BlockNumber * m_BlockLength;
    m_DbtFile.seekg(BlockBegin);

    // Read the block header.
    DbtBlockHeader Header;
    if (!_ReadObject(m_DbtFile, Header))
    {
        return CDbfError(L"Could not read DbtBlockHeader for memo block " + std::to_wstring(BlockNumber));
    }

    // Verify the block header.
    if (Header.Magic != DBT_BLOCK_MAGIC)
    {
        return CDbfError(L"Invalid magic " + std::to_wstring(Header.Magic) + L" for memo block " + std::to_wstring(BlockNumber));
    }

    if (Header.DataLength > DBT_MAX_BLOCK_LENGTH)
    {
        return CDbfError(L"Memo block " + std::to_wstring(BlockNumber) + L" is too large: " + std::to_wstring(Header.DataLength));
    }

    // Replace the content of the Column variable with the memo block content.
    Column.resize(Header.DataLength);
    if (!m_DbtFile.read(Column.data(), Column.size()))
    {
        return CDbfError(L"Could not read content of memo block " + std::to_wstring(BlockNumber));
    }

    return std::monostate();
}

std::variant<std::vector<std::string>, std::monostate, CDbfError>
CDbfReader::ReadNextRecord()
{
    for (;;)
    {
        char RecordStatus;
        if (!m_DbfFile.read(&RecordStatus, 1))
        {
            return CDbfError(L"Could not read RecordStatus");
        }

        if (RecordStatus == DBASE_FILE_END_BYTE)
        {
            // We have reached end-of-file and no more records follow.
            return std::monostate();
        }

        std::vector<std::string> Columns;
        for (auto it = m_Fields.cbegin(); it != m_Fields.cend(); it++)
        {
            std::string Column;
            Column.resize(it->Length);

            if (!m_DbfFile.read(Column.data(), Column.size()))
            {
                return CDbfError(L"Could not read column");
            }

            StrTrim(Column);

            // We may encounter a dBASE IV Memo field, which should always be 10 bytes long.
            // In that case, it contains the block number of the .dbt file as a string.
            if (it->cType == 'M' && it->Length == 10 && !Column.empty())
            {
                auto ReadMemoResult = _ReadMemoColumn(Column);
                if (const auto pError = std::get_if<CDbfError>(&ReadMemoResult))
                {
                    return *pError;
                }
            }

            Columns.push_back(Column);
        }

        if (RecordStatus == RECORD_NOT_DELETED)
        {
            // Record is not deleted, return it.
            return Columns;
        }
    }
}

void
CDbfReader::JumpToFirstRecord()
{
    m_DbfFile.seekg(m_DataStart);
}

std::variant<std::monostate, CDbfError>
CDbfReader::JumpToLastRecord()
{
    JumpToFirstRecord();
    size_t CurrentPosition = m_DataStart;
    size_t LastRecordPosition = m_DataStart;

    // A record consists of a single status byte followed by all fields in their custom lengths.
    size_t RecordLength = std::accumulate(m_Fields.cbegin(), m_Fields.cend(), 1, [](size_t Sum, const FieldInfo& Info) {
        return Sum + Info.Length;
    });

    for (;;)
    {
        char RecordStatus;
        if (!m_DbfFile.read(&RecordStatus, 1))
        {
            return CDbfError(L"Could not read RecordStatus");
        }

        if (RecordStatus == DBASE_FILE_END_BYTE)
        {
            // We have reached end-of-file and no more records follow.
            m_DbfFile.seekg(LastRecordPosition);
            return std::monostate();
        }

        if(RecordStatus == RECORD_NOT_DELETED)
        {
            LastRecordPosition = CurrentPosition;
        }

        CurrentPosition += RecordLength;
        m_DbfFile.seekg(CurrentPosition);
    }
}
