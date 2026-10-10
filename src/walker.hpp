#pragma once

#include "./ytree.h"

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
  char* to_file = nullptr;
  char* to_path = nullptr;
  bool path_copy = false;
  bool confirm = false;
};

struct RenameWalkContext : WalkContextBase
{
  char* new_name = nullptr;
  bool confirm = false;
};

struct MoveWalkContext : WalkContextBase
{
  DirEntry* dest_dir_entry = nullptr;
  char* to_file = nullptr;
  char* to_path = nullptr;
  bool confirm = false;
};

struct PipeWalkContext : WalkContextBase
{
  FILE* pipe_file = nullptr;
};

int PipeTaggedFiles(FileEntry* fe_ptr, PipeWalkContext* ctx);
int SetFileModus(FileEntry* fe_ptr, ChangeModusWalkContext* ctx);
int CopyTaggedFiles(FileEntry* fe_ptr, CopyWalkContext* ctx);
int MoveTaggedFiles(FileEntry* fe_ptr, MoveWalkContext* ctx);
int SetFileOwner(FileEntry* fe_ptr, ChangeOwnerWalkContext* ctx);
int SetFileGroup(FileEntry* fe_ptr, ChangeGroupWalkContext* ctx);
int ExecuteCommand(FileEntry* fe_ptr, ExecuteWalkContext* ctx);
int RenameTaggedFiles(FileEntry* fe_ptr, RenameWalkContext* ctx);
