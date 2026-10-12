#pragma once

class Volume
{
public:
  enum class Kind
  {
    Arc,
    Disk,
    Lha,
    Rar,
    Rpm,
    SevenZip,
    Tape,
    Tar,
    User,
    Zip,
    Zoo,
  };
};
