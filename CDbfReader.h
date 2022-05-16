//
// EnlyzeDbfLib - Library for reading dBASE .dbf database files
// Written by Colin Finck for ENLYZE GmbH
//

#pragma once

#include <cstdint>
#include <fstream>
#include <memory>
#include <numeric>
#include <string>
#include <variant>
#include <vector>

#include "CDbfError.h"

struct FieldInfo
{
    std::string strName;
    char cType;
    uint8_t Length;
};

class CDbfReader
{
public:
    static std::variant<std::unique_ptr<CDbfReader>, CDbfError> ReadDbf(const std::wstring& pwszDbfFilePath);

    std::variant<size_t, CDbfError> GetFieldIndex(const std::string& strField) const;
    const std::vector<FieldInfo>& GetFields() const { return m_Fields; }
    std::variant<std::vector<std::string>, std::monostate, CDbfError> ReadNextRecord();
    void JumpToFirstRecord();
    std::variant<std::monostate, CDbfError> JumpToLastRecord();

private:
    size_t m_DataStart;
    std::vector<FieldInfo> m_Fields;
    std::ifstream m_DbfFile;
    std::ifstream m_DbtFile;

    CDbfReader(std::ifstream&& DbfFile, std::ifstream&& DbtFile, std::vector<FieldInfo> Fields, size_t DataStart);
    std::variant<std::monostate, CDbfError> _ReadMemoColumn(std::string& Column);
};
