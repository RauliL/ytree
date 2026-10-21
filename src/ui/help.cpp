#include "ui/help.hpp"

#include "core/api.hpp"
#include "core/domain.hpp"
#include "core/profile.hpp"

static constexpr std::size_t HELP_LABEL_WIDTH = 10;

static const HelpLines disk_dir_help =
{
  "DIR       (A)ttribute (D)elete  (F)ilespec  (G)roup (L)og (M)akedir                 (Q)uit",
  "COMMANDS  (O)wner (R)ename (S)howall (^S)how-tagged (T)ag (U)ntag e(X)ecute   (^F) dirmode"
};

static const HelpLines ll_dir_help =
{
  "DIR       (F)ilespec (L)ogin (S)howall (T)ag (U)ntag e(X)ecute   (^F) dirmode  (Q)uit       ",
  "COMMANDS                                                                    "
};

static const std::string archive_dir_actions =
  "(F)ilespec (L)og (S)howall (T)ag (U)ntag e(X)ecute   (^F) dirmode  (Q)uit         ";
static const std::string archive_dir_commands =
  "COMMANDS                                                                    ";

static const HelpLines disk_file_help =
{
  "FILE      (A)ttribute (C)opy (D)elete (E)dit (F)ilespec (G)roup (H)ex (L)ogin (M)ove    (Q)uit ",
  "COMMANDS  (O)wner (P)ipe (R)ename (S)ort (T)ag (U)ntag (V)iew e(X)ecute pathcop(Y) (^F)ilemode "
};

static const HelpLines ll_file_help =
{
  "FILE      (F)ilespec (L)ogin (S)ort (T)ag (U)ntag e(X)ecute (^F)ilemode      (Q)uit        ",
  "COMMANDS                                                                   "
};

static const std::string archive_file_actions =
  "(C)opy (F)ilespec (H)ex (P)ipe (S)ort (T)ag (U)ntag (V)iew pathcop(Y) (Q)uit         ";
static const std::string archive_file_commands =
  "COMMANDS  (^F)ilemode                                                        ";

namespace {

/* Overridable USER_MODE banners; start as disk-mode defaults. */
HelpLines user_dir_help = disk_dir_help;
HelpLines user_file_help = disk_file_help;

struct HelpContext
{
  const HelpLines& disk;
  HelpLines& user;
  const HelpLines& ll;
  const std::string& archive_actions;
  const std::string& archive_commands;
  const char* archive_suffix;
  const char* profile_key0;
  const char* profile_key1;
};

const HelpContext dir_help_context = {
  disk_dir_help,
  user_dir_help,
  ll_dir_help,
  archive_dir_actions,
  archive_dir_commands,
  "-DIR",
  "DIR1",
  "DIR2"
};

const HelpContext file_help_context = {
  disk_file_help,
  user_file_help,
  ll_file_help,
  archive_file_actions,
  archive_file_commands,
  "-FILE",
  "FILE1",
  "FILE2"
};

std::string PadHelpLabel(std::string label)
{
  if (label.size() < HELP_LABEL_WIDTH)
  {
    label.append(HELP_LABEL_WIDTH - label.size(), ' ');
  }
  return label;
}

HelpLines MakeArchiveHelp(
  std::string_view kind,
  std::string_view suffix,
  const std::string& actions,
  const std::string& commands
)
{
  return {
    PadHelpLabel(std::string(kind) + std::string(suffix)) + actions,
    commands
  };
}

std::optional<std::string_view> ArchiveHelpKind(Mode m)
{
  switch (m)
  {
    using enum Mode;
    case TAR_FILE_MODE:
      return "TAR";
    case ZOO_FILE_MODE:
      return "ZOO";
    case ZIP_FILE_MODE:
      return "ZIP";
    case LHA_FILE_MODE:
      return "LHA";
    case ARC_FILE_MODE:
      return "ARC";
    case RPM_FILE_MODE:
      return "RPM";
    case RAR_FILE_MODE:
      return "RAR";
    case TAPE_MODE:
      return "TAPE";
    case SEVENZIP_FILE_MODE:
      return "7ZIP";
    default:
      return std::nullopt;
  }
}

HelpLines GetHelp(const HelpContext& ctx, Mode m)
{
  if (m == Mode::DISK_MODE)
  {
    return ctx.disk;
  }
  if (m == Mode::USER_MODE)
  {
    return ctx.user;
  }
  if (m == Mode::LL_FILE_MODE)
  {
    return ctx.ll;
  }
  if (const auto kind = ArchiveHelpKind(m))
  {
    return MakeArchiveHelp(
      *kind,
      ctx.archive_suffix,
      ctx.archive_actions,
      ctx.archive_commands
    );
  }
  return ctx.disk;
}

void ApplyUserProfileOverrides(const HelpContext& ctx)
{
  if (mode != Mode::USER_MODE)
  {
    return;
  }

  if (ctx.user[0] == ctx.disk[0])
  {
    if (const auto cptr = GetProfileValue(ctx.profile_key0))
    {
      ctx.user[0] = *cptr;
    }
  }
  if (ctx.user[1] == ctx.disk[1])
  {
    if (const auto cptr = GetProfileValue(ctx.profile_key1))
    {
      ctx.user[1] = *cptr;
    }
  }
}

HelpLines ResolveHelp(const HelpContext& ctx)
{
  ApplyUserProfileOverrides(ctx);
  return GetHelp(ctx, mode);
}

} // namespace

HelpLines DirHelpLines()
{
  return ResolveHelp(dir_help_context);
}

HelpLines FileHelpLines()
{
  return ResolveHelp(file_help_context);
}
