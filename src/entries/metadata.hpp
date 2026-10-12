#pragma once

class Attributes
{
public:
  enum class Type
  {
    Block,
    Char,
    Directory,
    Fifo,
    Regular,
    Socket,
    Symlink,
    Unknown,
  };

  static Attributes FromStat(const struct stat& st);

  bool IsDirectory() const;

  bool IsSymlink() const;
};
