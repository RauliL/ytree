#pragma once

#include "core/domain.hpp"
#include "core/ui.hpp"

ui::DirPanelInfo MakeDirPanelInfo(const DirEntry& dir);
ui::FilePanelInfo MakeFilePanelInfo(const FileEntry& file);
ui::FilePanelInfo MakeGlobalFilePanelInfo(const FileEntry& file);
ui::TaggedCounts MakeTaggedCounts(const DirEntry& dir);
