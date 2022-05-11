//
// EnlyzeDbfLib - Library for reading dBASE .dbf database files
// Written by Colin Finck for ENLYZE GmbH
//

#pragma once

#include <string>

class CDbfError
{
public:
    explicit CDbfError() {}
    explicit CDbfError(const std::wstring& wstrMessage) : m_wstrMessage(wstrMessage) {}

    const std::wstring& Message() const { return m_wstrMessage; }

private:
    std::wstring m_wstrMessage;
};
