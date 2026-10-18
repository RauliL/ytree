#pragma once

#include "./ytree.hpp"

struct WalkContextBase
{
  FileEntry* new_fe_ptr = nullptr;
};

struct ChangeModusWalkContext : WalkContextBase
{
  std::string new_modus;
};

struct ChangeOwnerWalkContext : WalkContextBase
{
  unsigned new_owner_id = 0;
};

struct ChangeGroupWalkContext : WalkContextBase
{
  unsigned new_group_id = 0;
};

struct ExecuteWalkContext : WalkContextBase
{
  std::string command;
};

struct CopyWalkContext : WalkContextBase
{
  Statistic* statistic_ptr = nullptr;
  DirEntry* dest_dir_entry = nullptr;
  std::string to_file;
  std::string to_path;
  bool path_copy = false;
  bool confirm = false;
};

struct RenameWalkContext : WalkContextBase
{
  std::string new_name;
  bool confirm = false;
};

struct MoveWalkContext : WalkContextBase
{
  DirEntry* dest_dir_entry = nullptr;
  std::string to_file;
  std::string to_path;
  bool confirm = false;
};

struct PipeWalkContext : WalkContextBase
{
  FILE* pipe_file = nullptr;
};

bool PipeTaggedFiles(FileEntry* fe_ptr, PipeWalkContext* ctx);
bool SetFileModus(FileEntry* fe_ptr, ChangeModusWalkContext* ctx);
bool CopyTaggedFiles(FileEntry* fe_ptr, CopyWalkContext* ctx);
bool MoveTaggedFiles(FileEntry* fe_ptr, MoveWalkContext* ctx);
bool SetFileOwner(FileEntry* fe_ptr, ChangeOwnerWalkContext* ctx);
bool SetFileGroup(FileEntry* fe_ptr, ChangeGroupWalkContext* ctx);
bool ExecuteCommand(FileEntry* fe_ptr, ExecuteWalkContext* ctx);
bool RenameTaggedFiles(FileEntry* fe_ptr, RenameWalkContext* ctx);
