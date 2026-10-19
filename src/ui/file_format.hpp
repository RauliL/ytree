#pragma once

#include "core/domain.hpp"

#include <memory>
#include <string>
#include <vector>

/** Format one file-list row for the current ViewMode / file metrics (curses). */
std::string FormatFileEntryLine(const std::shared_ptr<FileEntry>& fe_ptr);

/** Horizontal cell pitch for one column in the current file view mode. */
int FileEntryColumnStride();

struct FileTableColumn
{
  std::string id;
  std::string title;
  bool expand = false;
  /** 0 = left, 1 = right */
  float xalign = 0.0f;
};

struct FileTableRow
{
  std::vector<std::string> cells;
};

/** Column headers/layout for a file view mode (GTK table). */
std::vector<FileTableColumn> FileTableColumnsForMode(ViewMode mode);

/** Cell values matching FileTableColumnsForMode(mode). */
FileTableRow FormatFileTableRow(
  const std::shared_ptr<FileEntry>& fe_ptr,
  ViewMode mode
);
