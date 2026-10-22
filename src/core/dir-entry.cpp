#include "core/dir-entry.hpp"
#include "core/path.hpp"

std::filesystem::path DirEntry::GetPath() const
{
  std::string result;

  // Parents are owned by the tree, so the raw cursor stays valid while the
  // temporary `shared_ptr` returned by `Parent()` goes away.
  for (auto ptr = this; ptr;)
  {
    const auto parent = ptr->Parent();
    const auto is_root = IsRoot(ptr->name);

    if (!is_root)
    {
      result.insert(0, ptr->name);
    }
    // A nameless root (archive placeholder) doesn't contribute a separator.
    if (parent && !parent->name.empty())
    {
      result.insert(result.begin(), std::filesystem::path::preferred_separator);
    }

    ptr = parent.get();
  }

  return result;
}
