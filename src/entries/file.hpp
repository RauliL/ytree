#pragma once

#include "./metadata.hpp"
#include "./node_kind.hpp"

class File
{
public:
  enum class Operation
  {
    Chmod,
    Chown,
    CopyIn,
    CopyOut,
    Delete,
    Edit,
    Execute,
    Pipe,
    Rename,
    Tag,
    View,
  };

  std::weak_ptr<Dir> dir;
  bool tagged = false;
  bool matching = false;
  std::string name;
  std::string symlink_target;
  struct stat stat_struct{};  // deprecate gradually

  virtual NodeKind Kind() const = 0;

  virtual std::shared_ptr<Volume> GetVolume() const = 0;

  virtual Attributes GetAttributes() const;

  virtual std::filesystem::path MemberPath() const;   // path inside volume namespace

  virtual std::filesystem::path ResolvePath() const;    // replaces GetRealFileNamePath for this entry

  virtual bool Supports(Operation op) const;
};
