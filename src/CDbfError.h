//
// EnlyzeDbfLib - Library for reading dBASE .dbf database files
// Copyright (c) 2022 Colin Finck, ENLYZE GmbH <c.finck@enlyze.com>
// SPDX-License-Identifier: MIT
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
