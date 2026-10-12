#pragma once

#include "./metadata.hpp"
#include "./node_kind.hpp"

class Dir : public std::enable_shared_from_this<Dir>
{
public:
  enum class Operation
  {
    Chmod,
    Chown,
    Mkdir,
    Navigate,
    Rename,
    Rmdir,
    Scan,
  };

  std::weak_ptr<Dir> parent;
  std::vector<std::shared_ptr<File>> files;
  std::vector<std::shared_ptr<Dir>> children;

  inline std::shared_ptr<Dir> Parent() const
  {
    return parent.lock();
  }

  virtual NodeKind Kind() const = 0;

  virtual std::shared_ptr<Volume> GetVolume() const = 0;

  virtual Attributes GetAttributes() const;

  virtual std::filesystem::path LogicalPath() const;  // replaces GetPath for this subtree anchor

  /** Disk: ReadTree; Archive dir: usually no-op (children already inserted). */
  virtual int Populate(const std::filesystem::path& scan_root, int depth);

  virtual bool Supports(Operation op) const;

  /** Factory helpers on the directory that owns the child vectors. */
  virtual std::shared_ptr<File> MakeFile();

  virtual std::shared_ptr<Dir> MakeDir();
};
