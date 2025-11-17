#pragma once

/**
 * @brief 统一维护所有语言条目编号。
 */
namespace LangKey {
namespace Chat {
constexpr int SelectContact = 10001;
constexpr int EmptyHint = 10002;
constexpr int EmojiTooltip = 10003;
constexpr int ScreenshotTooltip = 10004;
constexpr int MoreFeatureTooltip = 10005;
constexpr int FileTooltip = 10006;
constexpr int ShareTooltip = 10007;
constexpr int InputPlaceholder = 10008;
constexpr int SendButton = 10009;
constexpr int StatusDisconnected = 10010;
constexpr int StatusReady = 10011;
} // namespace Chat

namespace MainWindow {
constexpr int Title = 20001;
constexpr int SelectContactHint = 20002;
constexpr int DefaultRoleName = 20003;
constexpr int FileDialogTitle = 20004;
constexpr int FileSelected = 20005;
constexpr int FileSaved = 20006;
constexpr int FileTag = 20007;
} // namespace MainWindow

namespace ProfileCard {
constexpr int DefaultName = 30001;
constexpr int DefaultInitial = 30002;
constexpr int EditSignature = 30003;
constexpr int RefreshStatus = 30004;
constexpr int OpenSettings = 30005;
constexpr int StatusOnline = 30006;
constexpr int StatusRefreshing = 30007;
constexpr int SearchPlaceholder = 30008;
constexpr int Contacts = 30009;
constexpr int Groups = 30010;
constexpr int Org = 30011;
constexpr int Collaboration = 30012;
constexpr int EmptyIcon = 30013;
constexpr int EmptyText = 30014;
} // namespace ProfileCard

namespace Controller {
constexpr int RouterFailed = 31001;
constexpr int StartupReady = 31002;
constexpr int PeerMissing = 31003;
constexpr int CannotReadFile = 31004;
constexpr int FileSent = 31005;
constexpr int CannotWriteConfig = 31006;
constexpr int CannotSaveFile = 31007;
constexpr int FileSaved = 31008;
constexpr int ShareMissing = 31009;
} // namespace Controller
} // namespace LangKey
