#pragma once

#include "core/domain.hpp"
#include "core/ui.hpp"

#include <cctype>
#include <optional>

/**
 * Semantic commands for the directory-tree window.
 * Mapped from ui::InputEvent so the loop does not switch on backend key codes.
 */
enum class DirCommand
{
  None,
  Resize,
  Mouse,
  MoveDown,
  MoveUp,
  PageDown,
  PageUp,
  Home,
  End,
  Expand,
  Collapse,
  ReadSubtree,
  FileSpec,
  Tag,
  Untag,
  TagAll,
  UntagAll,
  RotateMode,
  ShowTagged,
  ShowAll,
  EnterFiles,
  Execute,
  MakeDirectory,
  Delete,
  Rename,
  ChangeGroup,
  ChangeOwner,
  ChangeMode,
  QuitTo,
  Quit,
  ChangeLogin,
  Redraw,
  LogParent,
  Unknown,
};

inline DirCommand MapDirCommand(const ui::InputEvent& ev)
{
  using ui::Key;

  switch (ev.key)
  {
    case Key::Resize:
      return DirCommand::Resize;
    case Key::Mouse:
      return DirCommand::Mouse;
    case Key::Down:
    case Key::Space:
      return DirCommand::MoveDown;
    case Key::Up:
      return DirCommand::MoveUp;
    case Key::PageDown:
      return DirCommand::PageDown;
    case Key::PageUp:
      return DirCommand::PageUp;
    case Key::Home:
      return DirCommand::Home;
    case Key::End:
      return DirCommand::End;
    case Key::Right:
      return DirCommand::Expand;
    case Key::Left:
    case Key::BackTab:
      return DirCommand::Collapse;
    case Key::Tab:
      return DirCommand::ReadSubtree;
    case Key::Enter:
      return DirCommand::EnterFiles;
    case Key::Escape:
      return DirCommand::None;
    case Key::Char:
    {
      const int ch = ev.code;
      switch (ch)
      {
        case '+':
          return DirCommand::Expand;
        case '-':
          return DirCommand::Collapse;
        case '*':
          return DirCommand::ReadSubtree;
        case 'F':
        case 'f':
          return DirCommand::FileSpec;
        case 'T':
        case 't':
          return DirCommand::Tag;
        case 'U':
        case 'u':
          return DirCommand::Untag;
        case 'T' & 0x1f:
          return DirCommand::TagAll;
        case 'U' & 0x1f:
          return DirCommand::UntagAll;
        case 'F' & 0x1f:
          return DirCommand::RotateMode;
        case 'S' & 0x1f:
          return DirCommand::ShowTagged;
        case 'S':
        case 's':
          return DirCommand::ShowAll;
        case 'X':
        case 'x':
          return DirCommand::Execute;
        case 'M':
        case 'm':
          return DirCommand::MakeDirectory;
        case 'D':
        case 'd':
          return DirCommand::Delete;
        case 'R':
        case 'r':
          return DirCommand::Rename;
        case 'G':
        case 'g':
          return DirCommand::ChangeGroup;
        case 'O':
        case 'o':
          return DirCommand::ChangeOwner;
        case 'A':
        case 'a':
          return DirCommand::ChangeMode;
        case 'Q' & 0x1f:
          return DirCommand::QuitTo;
        case 'Q':
        case 'q':
          return DirCommand::Quit;
#if !defined(VI_KEYS)
        case 'l':
#endif
        case 'L':
          return DirCommand::ChangeLogin;
        case 'L' & 0x1f:
          return DirCommand::Redraw;
        case 'P':
        case 'p':
          return DirCommand::LogParent;
        default:
          return DirCommand::Unknown;
      }
    }
    default:
      return DirCommand::Unknown;
  }
}

/**
 * Semantic commands for the file-list window.
 */
enum class FileCommand
{
  None,
  Resize,
  Mouse,
  MoveDown,
  MoveUp,
  MoveLeft,
  MoveRight,
  PageDown,
  PageUp,
  Home,
  End,
  ListJump,
  ChangeMode,
  ChangeModeTagged,
  ChangeOwner,
  ChangeOwnerTagged,
  ChangeGroup,
  ChangeGroupTagged,
  Tag,
  Untag,
  TagAll,
  UntagAll,
  TagFromHere,
  UntagFromHere,
  RotateMode,
  View,
  HexView,
  Edit,
  Copy,
  CopyTagged,
  Move,
  MoveTagged,
  Rename,
  RenameTagged,
  Delete,
  DeleteTagged,
  Sort,
  FileSpec,
  ChangeLogin,
  Pipe,
  PipeTagged,
  Execute,
  ExecuteTagged,
  SearchTagged,
  QuitTo,
  Quit,
  Redraw,
  Leave,
  LoginEsc,
  Unknown,
};

inline FileCommand MapFileCommand(const ui::InputEvent& ev)
{
  using ui::Key;

  switch (ev.key)
  {
    case Key::Resize:
      return FileCommand::Resize;
    case Key::Mouse:
      return FileCommand::Mouse;
    case Key::Down:
    case Key::Space:
      return FileCommand::MoveDown;
    case Key::Up:
      return FileCommand::MoveUp;
    case Key::Left:
      return FileCommand::MoveLeft;
    case Key::Right:
      return FileCommand::MoveRight;
    case Key::PageDown:
      return FileCommand::PageDown;
    case Key::PageUp:
      return FileCommand::PageUp;
    case Key::Home:
      return FileCommand::Home;
    case Key::End:
      return FileCommand::End;
    case Key::Enter:
      return FileCommand::Leave;
    case Key::Escape:
      return FileCommand::Leave;
    case Key::Function:
      return ev.code == 12 ? FileCommand::ListJump : FileCommand::Unknown;
    case Key::Char:
    {
      const int ch = ev.code;
      switch (ch)
      {
        case 'A':
        case 'a':
          return FileCommand::ChangeMode;
        case 'A' & 0x1f:
          return FileCommand::ChangeModeTagged;
        case 'O':
        case 'o':
          return FileCommand::ChangeOwner;
        case 'O' & 0x1f:
          return FileCommand::ChangeOwnerTagged;
        case 'G':
        case 'g':
          return FileCommand::ChangeGroup;
        case 'G' & 0x1f:
          return FileCommand::ChangeGroupTagged;
        case 'T':
        case 't':
          return FileCommand::Tag;
        case 'U':
        case 'u':
          return FileCommand::Untag;
        case 'T' & 0x1f:
          return FileCommand::TagAll;
        case 'U' & 0x1f:
          return FileCommand::UntagAll;
        case ';':
        case 't' | 0x80:
          return FileCommand::TagFromHere;
        case ':':
        case 'u' | 0x80:
          return FileCommand::UntagFromHere;
        case 'F' & 0x1f:
          return FileCommand::RotateMode;
        case 'V':
        case 'v':
          return FileCommand::View;
        case 'H':
        case 'h':
          return FileCommand::HexView;
        case 'E':
        case 'e':
          return FileCommand::Edit;
        case 'Y':
        case 'y':
        case 'C':
        case 'c':
          return FileCommand::Copy;
        case 'Y' & 0x1f:
        case 'K' & 0x1f:
        case 'C' & 0x1f:
          return FileCommand::CopyTagged;
        case 'M':
        case 'm':
          return FileCommand::Move;
        case 'N' & 0x1f:
          return FileCommand::MoveTagged;
        case 'R':
        case 'r':
          return FileCommand::Rename;
        case 'R' & 0x1f:
          return FileCommand::RenameTagged;
        case 'D':
        case 'd':
          return FileCommand::Delete;
        case 'D' & 0x1f:
          return FileCommand::DeleteTagged;
        case 'S':
        case 's':
          return FileCommand::Sort;
        case 'F':
        case 'f':
          return FileCommand::FileSpec;
#if !defined(VI_KEYS)
        case 'l':
#endif
        case 'L':
          return FileCommand::ChangeLogin;
        case 'P':
        case 'p':
          return FileCommand::Pipe;
        case 'P' & 0x1f:
          return FileCommand::PipeTagged;
        case 'X':
        case 'x':
          return FileCommand::Execute;
        case 'X' & 0x1f:
          return FileCommand::ExecuteTagged;
        case 'S' & 0x1f:
          return FileCommand::SearchTagged;
        case 'Q' & 0x1f:
          return FileCommand::QuitTo;
        case 'Q':
        case 'q':
          return FileCommand::Quit;
        case 'L' & 0x1f:
          return FileCommand::Redraw;
        case LOGIN_ESC:
          return FileCommand::LoginEsc;
        default:
          return FileCommand::Unknown;
      }
    }
    default:
      return FileCommand::Unknown;
  }
}

/** Legacy int form for user-mode remapping and continue checks. */
inline int LegacyCode(const ui::InputEvent& ev)
{
  if (ev.raw != 0 || ev.key == ui::Key::Char || ev.key == ui::Key::Function)
  {
    return ev.raw != 0 ? ev.raw : ev.code;
  }

  using ui::Key;
  switch (ev.key)
  {
    case Key::Enter:
      return '\r';
    case Key::Escape:
      return 27;
    case Key::Tab:
      return '\t';
    case Key::Space:
      return ' ';
    case Key::Resize:
    case Key::None:
      return -1;
    default:
      return ev.code;
  }
}

inline ui::InputEvent EventFromLegacyChar(int ch)
{
  if (ch == '\r' || ch == '\n')
  {
    return ui::InputEvent::Special(ui::Key::Enter, ch);
  }
  if (ch == 27)
  {
    return ui::InputEvent::Special(ui::Key::Escape, ch);
  }
  if (ch == '\t')
  {
    return ui::InputEvent::Special(ui::Key::Tab, ch);
  }
  if (ch == ' ')
  {
    return ui::InputEvent::Special(ui::Key::Space, ch);
  }
  if (ch == -1)
  {
    return ui::InputEvent::Special(ui::Key::Resize, -1);
  }
  return ui::InputEvent::CharKey(ch);
}
