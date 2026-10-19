#pragma once

#include "core/domain.hpp"

#include <memory>
#include <string>

/** Format one file-list row for the current ViewMode / file metrics. */
std::string FormatFileEntryLine(const std::shared_ptr<FileEntry>& fe_ptr);

/** Horizontal cell pitch for one column in the current file view mode. */
int FileEntryColumnStride();
