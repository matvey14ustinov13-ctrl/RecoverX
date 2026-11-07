#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>
#include <uxtheme.h>
#include <winhttp.h>
#include <powrprof.h>

#include <algorithm>
#include <cwchar>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#ifdef _MSC_VER
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "powrprof.lib")
#pragma comment(lib, "uxtheme.lib")
#endif

// Constants
constexpr wchar_t kMainClass[] = L"RecoverXMainWindow";
constexpr wchar_t kPageClass[] = L"RecoverXPage";
constexpr wchar_t kBootClass[] = L"RecoverXBootMenu";
constexpr wchar_t kMutexName[] = L"Local\\RecoverXSingleInstance";
constexpr wchar_t kVersion[] = L"v1.0.0";
constexpr wchar_t kUpdatePath[] = L"/repos/matvey14ustinov13-ctrl/RecoverX/releases/latest";
constexpr UINT kTrayMessage = WM_APP + 1;
constexpr UINT_PTR kCountdownTimer = 1;
constexpr UINT_PTR kMonitorTimer = 2;
constexpr int kWindowWidth = 800;
constexpr int kWindowHeight = 620;
constexpr int kNavWidth = 180;

// Pages
enum PageIndex {
    PAGE_POWER = 0,
    PAGE_BOOT,
    PAGE_SYSTEM,
    PAGE_MAINTENANCE,
    PAGE_SCHEDULER,
    PAGE_MONITOR,
    PAGE_RESTORE,
    PAGE_UPDATES,
    PAGE_UWP,
    PAGE_DRIVERS,
    PAGE_SETTINGS,
    PAGE_COUNT
};

// Control IDs
enum ControlId {
    ID_TAB_FIRST = 100,
    ID_TAB_LAST = ID_TAB_FIRST + PAGE_COUNT - 1,

    ID_TIMER_SHUTDOWN = 200,
    ID_TIMER_RESTART,
    ID_MIN_EDIT,
    ID_SEC_EDIT,
    ID_FORCE_CHECK,
    ID_START_TIMER,
    ID_CANCEL_TIMER,
    ID_SLEEP,
    ID_SHUTDOWN,
    ID_FORCE_SHUTDOWN,

    ID_BOOT_BIOS = 300,
    ID_BOOT_SAFE_MIN,
    ID_BOOT_SAFE_NET,
    ID_BOOT_SAFE_RESET,
    ID_BOOT_MENU,

    ID_SYS_INFO = 400,
    ID_DEV_MGR,
    ID_TASK_MGR,
    ID_DISK_MGR,
    ID_LOCAL_USERS,
    ID_REGEDIT,
    ID_CONTROL_PANEL,
    ID_OSK,
    ID_MAGNIFIER,
    ID_MINIMIZE_ALL,
    ID_GPU_RESET,
    ID_APPS_FOLDER,
    ID_POWERSHELL,
    ID_CHANGE_TIME,
    ID_FLUSH_DNS,
    ID_SERVICES,
    ID_BACKUP_REGISTRY,

    ID_CLEAN_TEMP = 500,
    ID_SFC,
    ID_DISM,
    ID_CLEAN_UPDATES,
    ID_MRT,

    ID_SCHED_ADD = 600,
    ID_SCHED_TIME,
    ID_SCHED_SHUTDOWN,
    ID_SCHED_RESTART,
    ID_DAY_FIRST = 610,

    ID_RESTORE_REFRESH = 800,
    ID_RESTORE_CREATE,
    ID_RESTORE_DELETE,
    ID_RESTORE_OPEN,
    ID_RESTORE_LIST = 810,

    ID_UPDATE_CHECK = 900,
    ID_UPDATE_DISABLE,
    ID_DEFENDER_DISABLE,
    ID_DEFENDER_ENABLE,

    ID_UWP_REFRESH = 1000,
    ID_UWP_REMOVE,
    ID_UWP_ALL,
    ID_UWP_LIST = 1010,

    ID_DRIVER_NVIDIA = 1100,
    ID_DRIVER_AMD,
    ID_DRIVER_INTEL,

    ID_TRAY_CHECK = 1200,
    ID_AUTOSTART_CHECK,
    ID_STARTUP_SETTINGS,
    ID_APP_UPDATE_CHECK,
    ID_LANG_EN,
    ID_LANG_RU,

    ID_TRAY_OPEN = 1300,
    ID_TRAY_EXIT
};

using StrMap = std::map<std::wstring, std::wstring>;

// Helpers
static std::wstring GetExePath() {
    std::wstring path(MAX_PATH, L'\0');
    DWORD len = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    path.resize(len);
    return path;
}

static std::wstring GetExeDir() {
    std::wstring path = GetExePath();
    size_t pos = path.find_last_of(L"\\/");
    return pos == std::wstring::npos ? L"." : path.substr(0, pos);
}

static std::wstring ConfigPath() {
    return GetExeDir() + L"\\recoverx_config.json";
}

static std::wstring TempBatPath() {
    wchar_t temp[MAX_PATH]{};
    DWORD len = GetTempPathW(MAX_PATH, temp);
    std::wstring dir(temp, len);
    if (!dir.empty() && dir.back() != L'\\') dir += L'\\';
    return dir + L"recoverx_sched.bat";
}

static std::wstring Utf8ToWide(const std::string& text) {
    if (text.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    std::wstring out(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), out.data(), len);
    return out;
}

static std::wstring GetText(HWND hwnd) {
    int len = GetWindowTextLengthW(hwnd);
    if (len <= 0) return {};
    std::wstring text(static_cast<size_t>(len) + 1, L'\0');
    GetWindowTextW(hwnd, text.data(), len + 1);
    text.resize(len);
    return text;
}

static void SetText(HWND hwnd, const std::wstring& text) {
    if (hwnd) SetWindowTextW(hwnd, text.c_str());
}

static bool IsChecked(HWND hwnd) {
    return hwnd && SendMessageW(hwnd, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

static void ReplaceAll(std::wstring& text, const std::wstring& from, const std::wstring& to) {
    if (from.empty()) return;
    size_t pos = 0;
    while ((pos = text.find(from, pos)) != std::wstring::npos) {
        text.replace(pos, from.size(), to);
        pos += to.size();
    }
}

static std::wstring Trim(std::wstring text) {
    const wchar_t* ws = L" \t\r\n";
    size_t first = text.find_first_not_of(ws);
    if (first == std::wstring::npos) return {};
    size_t last = text.find_last_not_of(ws);
    return text.substr(first, last - first + 1);
}

static std::vector<std::wstring> SplitLines(const std::wstring& text) {
    std::vector<std::wstring> lines;
    std::wstringstream ss(text);
    std::wstring line;
    while (std::getline(ss, line)) {
        line = Trim(line);
        if (!line.empty()) lines.push_back(line);
    }
    return lines;
}

static std::wstring Quote(const std::wstring& text) {
    return L"\"" + text + L"\"";
}

static std::wstring FormatTime(int seconds) {
    std::wstringstream ss;
    ss << std::setfill(L'0') << std::setw(2) << seconds / 60
       << L":" << std::setw(2) << seconds % 60;
    return ss.str();
}

static std::wstring FormatPercent(double value) {
    std::wstringstream ss;
    ss << std::fixed << std::setprecision(1) << value << L"%";
    return ss.str();
}

static std::wstring LastErrorText() {
    DWORD code = GetLastError();
    wchar_t* buffer = nullptr;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, code, 0, reinterpret_cast<wchar_t*>(&buffer), 0, nullptr);
    std::wstring text = buffer ? buffer : L"";
    if (buffer) LocalFree(buffer);
    return text;
}

static std::string ExtractJsonString(const std::string& json, const std::string& key) {
    std::string pattern = "\"" + key + "\"";
    size_t pos = json.find(pattern);
    if (pos == std::string::npos) return {};
    pos = json.find(':', pos);
    if (pos == std::string::npos) return {};
    pos = json.find('"', pos);
    if (pos == std::string::npos) return {};
    ++pos;
    std::string out;
    bool escaped = false;
    for (; pos < json.size(); ++pos) {
        char ch = json[pos];
        if (escaped) {
            if (ch != '/') out.push_back(ch);
            else out.push_back('/');
            escaped = false;
        } else if (ch == '\\') {
            escaped = true;
        } else if (ch == '"') {
            break;
        } else {
            out.push_back(ch);
        }
    }
    return out;
}

// English
static StrMap EnglishStrings() {
    return {
        {L"title", L"RecoverX - System Utility"},
        {L"power", L"Power"},
        {L"boot", L"Boot"},
        {L"system", L"System"},
        {L"maintenance", L"Maintenance"},
        {L"scheduler", L"Scheduler"},
        {L"monitor", L"Monitor"},
        {L"restore", L"Restore Points"},
        {L"windows_update", L"Windows Update"},
        {L"uwp", L"UWP Apps"},
        {L"drivers", L"Drivers"},
        {L"settings", L"Settings"},
        {L"timer_shutdown", L"Shutdown"},
        {L"timer_restart", L"Restart"},
        {L"force_check", L"Force close programs"},
        {L"start_timer", L"Start timer"},
        {L"cancel_timer", L"Cancel"},
        {L"status_idle", L"Timer not running"},
        {L"sleep", L"Sleep"},
        {L"shutdown_direct", L"Shutdown"},
        {L"force_shutdown", L"Force Shutdown"},
        {L"boot_bios", L"BIOS (UEFI)"},
        {L"boot_safe_min", L"Safe Mode (Minimal)"},
        {L"boot_safe_net", L"Safe Mode (Network)"},
        {L"boot_reset", L"Reset Safe Mode"},
        {L"boot_menu", L"Boot Menu"},
        {L"sys_info", L"System Information"},
        {L"dev_mgr", L"Device Manager"},
        {L"task_mgr", L"Task Manager"},
        {L"disk_mgr", L"Disk Management"},
        {L"local_users", L"Local Users"},
        {L"regedit", L"Registry Editor"},
        {L"control_panel", L"Control Panel"},
        {L"osk", L"On-Screen Keyboard"},
        {L"magnifier", L"Magnifier"},
        {L"minimize_all", L"Minimize All"},
        {L"gpu_reset", L"Reset GPU Driver"},
        {L"apps_folder", L"AppsFolder"},
        {L"powershell", L"PowerShell"},
        {L"change_time", L"Change System Time"},
        {L"clean_temp", L"Clean Temp Files"},
        {L"sfc", L"SFC Scan"},
        {L"dism", L"DISM Restore"},
        {L"clean_updates", L"Clean Update Leftovers"},
        {L"mrt", L"Run MRT"},
        {L"flush_dns", L"Flush DNS Cache"},
        {L"services_mgr", L"Services Manager"},
        {L"backup_registry", L"Backup Registry"},
        {L"sched_info", L"Scheduler runs in background. It may consume some resources."},
        {L"sched_action", L"Action:"},
        {L"sched_time", L"Time (HH:MM):"},
        {L"sched_days", L"Days:"},
        {L"sched_add", L"Add Schedule"},
        {L"cpu_label", L"CPU Usage:"},
        {L"ram_label", L"RAM Usage:"},
        {L"refresh_rp", L"Refresh List"},
        {L"create_rp", L"Create Restore Point"},
        {L"delete_rp", L"Delete Selected"},
        {L"open_restore", L"System Restore..."},
        {L"confirm_delete_rp", L"Delete the selected restore point?"},
        {L"error_no_admin", L"Admin rights required for this action."},
        {L"error_rp_list", L"Could not list restore points. Check admin rights or system protection."},
        {L"check_updates", L"Check for Updates"},
        {L"disable_updates", L"Disable Automatic Updates"},
        {L"disable_defender", L"Disable Defender"},
        {L"enable_defender", L"Enable Defender"},
        {L"confirm_disable_updates", L"This will disable automatic updates. Continue?"},
        {L"confirm_disable_defender", L"This will disable Windows Defender (requires restart). Continue?"},
        {L"confirm_enable_defender", L"This will enable Windows Defender (requires restart). Continue?"},
        {L"defender_section", L"Windows Defender"},
        {L"refresh_uwp", L"Refresh List"},
        {L"remove_uwp", L"Remove Selected"},
        {L"uwp_for_all", L"For all users"},
        {L"confirm_uwp_remove", L"Remove selected apps permanently?"},
        {L"uwp_no_apps", L"No UWP apps found or admin rights required."},
        {L"drivers_nvidia", L"NVIDIA Drivers"},
        {L"drivers_amd", L"AMD Drivers"},
        {L"drivers_intel", L"Intel Drivers"},
        {L"tray_on", L"Minimize to tray on close"},
        {L"autostart", L"Start RecoverX with Windows"},
        {L"startup_settings", L"Windows Startup Settings"},
        {L"admin_btn", L"Run as Administrator"},
        {L"check_app_updates", L"Check for RecoverX updates"},
        {L"no_updates", L"You have the latest version."},
        {L"update_available", L"New version available: {version}\nDownload: {url}"},
        {L"update_error", L"Could not check for updates."},
        {L"confirm_shutdown", L"Shut down now?"},
        {L"confirm_force", L"All unsaved data will be lost. Continue?"},
        {L"confirm_bios", L"Restart into UEFI?"},
        {L"confirm_safe", L"Set Safe Mode and restart?"},
        {L"confirm_clean_temp", L"Clean Windows temp files?"},
        {L"confirm_clean_updates", L"Remove update leftovers? Restart may be required."},
        {L"admin_needed", L"Admin rights required. Restart as admin?"},
        {L"done", L"Done"},
        {L"error", L"Error"},
        {L"not_available", L"This snap-in is not available in your Windows edition."},
        {L"until_shutdown", L"Until shutdown:"},
        {L"until_restart", L"Until restart:"},
        {L"executing", L"Executing..."},
        {L"open", L"Open"},
        {L"exit", L"Exit"},
        {L"language", L"Language:"},
        {L"invalid_time", L"Enter a valid time."},
        {L"select_day", L"Select at least one day."},
        {L"task_scheduled", L"Task scheduled."},
        {L"task_failed", L"Failed to create task."},
        {L"select_rp", L"Select a restore point."},
        {L"select_apps", L"Select apps."},
        {L"registry_exported", L"Registry exported to {path}"},
        {L"registry_failed", L"Failed to export registry."},
        {L"dns_question", L"Clear DNS cache?"},
        {L"dns_done", L"DNS cache cleared."},
        {L"autoupdates_disabled", L"Automatic updates disabled."},
        {L"defender_disabled", L"Disabled. Restart required."},
        {L"defender_enabled", L"Enabled. Restart required."},
        {L"admin_failed", L"Failed to get admin rights."},
        {L"uwp_admin", L"UWP Apps (Admin)"}
    };
}

// Russian
static StrMap RussianStrings() {
    return {
        {L"title", L"RecoverX - Системная утилита"},
        {L"power", L"Питание"},
        {L"boot", L"Загрузка"},
        {L"system", L"Система"},
        {L"maintenance", L"Обслуживание"},
        {L"scheduler", L"Планировщик"},
        {L"monitor", L"Мониторинг"},
        {L"restore", L"Точки восстановления"},
        {L"windows_update", L"Windows Update"},
        {L"uwp", L"UWP Приложения"},
        {L"drivers", L"Драйверы"},
        {L"settings", L"Настройки"},
        {L"timer_shutdown", L"Выключение"},
        {L"timer_restart", L"Перезагрузка"},
        {L"force_check", L"Принудительно закрыть программы"},
        {L"start_timer", L"Запустить таймер"},
        {L"cancel_timer", L"Отмена"},
        {L"status_idle", L"Таймер не запущен"},
        {L"sleep", L"Сон"},
        {L"shutdown_direct", L"Выключить"},
        {L"force_shutdown", L"Принудительно"},
        {L"boot_bios", L"BIOS (UEFI)"},
        {L"boot_safe_min", L"Безопасный режим (Мин.)"},
        {L"boot_safe_net", L"Безопасный режим (Сеть)"},
        {L"boot_reset", L"Сброс Safe Mode"},
        {L"boot_menu", L"Boot Menu"},
        {L"sys_info", L"Сведения о системе"},
        {L"dev_mgr", L"Диспетчер устройств"},
        {L"task_mgr", L"Диспетчер задач"},
        {L"disk_mgr", L"Управление дисками"},
        {L"local_users", L"Локальные пользователи"},
        {L"regedit", L"Редактор реестра"},
        {L"control_panel", L"Панель управления"},
        {L"osk", L"Экранная клавиатура"},
        {L"magnifier", L"Лупа"},
        {L"minimize_all", L"Свернуть всё"},
        {L"gpu_reset", L"Перезагрузка видеодрайвера"},
        {L"apps_folder", L"AppsFolder"},
        {L"powershell", L"PowerShell"},
        {L"change_time", L"Изменить системное время"},
        {L"clean_temp", L"Очистка временных файлов"},
        {L"sfc", L"Проверка SFC"},
        {L"dism", L"Восстановление DISM"},
        {L"clean_updates", L"Удаление остатков обновлений"},
        {L"mrt", L"Запустить MRT"},
        {L"flush_dns", L"Очистить DNS-кэш"},
        {L"services_mgr", L"Менеджер служб"},
        {L"backup_registry", L"Резервная копия реестра"},
        {L"sched_info", L"Планировщик работает в фоне и потребляет ресурсы ПК."},
        {L"sched_action", L"Действие:"},
        {L"sched_time", L"Время (ЧЧ:ММ):"},
        {L"sched_days", L"Дни:"},
        {L"sched_add", L"Добавить задание"},
        {L"cpu_label", L"Загрузка ЦП:"},
        {L"ram_label", L"Загрузка ОЗУ:"},
        {L"refresh_rp", L"Обновить список"},
        {L"create_rp", L"Создать точку восстановления"},
        {L"delete_rp", L"Удалить выбранную"},
        {L"open_restore", L"Восстановление системы..."},
        {L"confirm_delete_rp", L"Удалить выбранную точку восстановления?"},
        {L"error_no_admin", L"Для этого действия нужны права администратора."},
        {L"error_rp_list", L"Не удалось получить список точек восстановления. Проверьте права администратора или защиту системы."},
        {L"check_updates", L"Проверить обновления"},
        {L"disable_updates", L"Отключить автообновления"},
        {L"disable_defender", L"Отключить Defender"},
        {L"enable_defender", L"Включить Defender"},
        {L"confirm_disable_updates", L"Будут отключены автоматические обновления. Продолжить?"},
        {L"confirm_disable_defender", L"Будет отключён Windows Defender (потребуется перезагрузка). Продолжить?"},
        {L"confirm_enable_defender", L"Будет включён Windows Defender (потребуется перезагрузка). Продолжить?"},
        {L"defender_section", L"Защитник Windows"},
        {L"refresh_uwp", L"Обновить список"},
        {L"remove_uwp", L"Удалить выбранные"},
        {L"uwp_for_all", L"Для всех пользователей"},
        {L"confirm_uwp_remove", L"Удалить выбранные приложения безвозвратно?"},
        {L"uwp_no_apps", L"Приложения UWP не найдены или требуются права администратора."},
        {L"drivers_nvidia", L"Драйверы NVIDIA"},
        {L"drivers_amd", L"Драйверы AMD"},
        {L"drivers_intel", L"Драйверы Intel"},
        {L"tray_on", L"Сворачивать в трей при закрытии"},
        {L"autostart", L"Запускать RecoverX с Windows"},
        {L"startup_settings", L"Настройки автозагрузок Windows"},
        {L"admin_btn", L"Запустить от имени администратора"},
        {L"check_app_updates", L"Проверить обновления RecoverX"},
        {L"no_updates", L"У вас последняя версия."},
        {L"update_available", L"Доступна новая версия: {version}\nСкачать: {url}"},
        {L"update_error", L"Не удалось проверить обновления."},
        {L"confirm_shutdown", L"Выключить сейчас?"},
        {L"confirm_force", L"Все несохранённые данные будут потеряны. Продолжить?"},
        {L"confirm_bios", L"Перезагрузить в UEFI?"},
        {L"confirm_safe", L"Установить Safe Mode и перезагрузить?"},
        {L"confirm_clean_temp", L"Очистить временные файлы Windows?"},
        {L"confirm_clean_updates", L"Удалить остатки обновлений? Может потребоваться перезагрузка."},
        {L"admin_needed", L"Требуются права администратора. Перезапустить?"},
        {L"done", L"Готово"},
        {L"error", L"Ошибка"},
        {L"not_available", L"Оснастка недоступна в этой редакции Windows."},
        {L"until_shutdown", L"До выключения:"},
        {L"until_restart", L"До перезагрузки:"},
        {L"executing", L"Выполняется..."},
        {L"open", L"Открыть"},
        {L"exit", L"Выход"},
        {L"language", L"Язык:"},
        {L"invalid_time", L"Введите корректное время."},
        {L"select_day", L"Выберите хотя бы один день."},
        {L"task_scheduled", L"Задание создано."},
        {L"task_failed", L"Не удалось создать задание."},
        {L"select_rp", L"Выберите точку восстановления."},
        {L"select_apps", L"Выберите приложения."},
        {L"registry_exported", L"Реестр экспортирован в {path}"},
        {L"registry_failed", L"Не удалось экспортировать реестр."},
        {L"dns_question", L"Очистить DNS-кэш?"},
        {L"dns_done", L"DNS-кэш очищен."},
        {L"autoupdates_disabled", L"Автоматические обновления отключены."},
        {L"defender_disabled", L"Отключено. Требуется перезагрузка."},
        {L"defender_enabled", L"Включено. Требуется перезагрузка."},
        {L"admin_failed", L"Не удалось получить права администратора."},
        {L"uwp_admin", L"UWP Приложения (Админ)"}
    };
}

static StrMap BuildStrings(const std::string& lang) {
    return lang == "ru" ? RussianStrings() : EnglishStrings();
}

// Process helpers
static bool RunProcess(const std::wstring& command, DWORD flags, bool wait, DWORD* exitCode = nullptr) {
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    if ((flags & CREATE_NEW_CONSOLE) == 0) {
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
    }
    PROCESS_INFORMATION pi{};
    std::wstring mutableCommand = command;
    if (!CreateProcessW(nullptr, mutableCommand.data(), nullptr, nullptr, FALSE, flags, nullptr, nullptr, &si, &pi)) {
        return false;
    }
    if (wait) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        if (exitCode) GetExitCodeProcess(pi.hProcess, exitCode);
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return true;
}

static bool RunHidden(const std::wstring& command) {
    return RunProcess(command, CREATE_NO_WINDOW, false);
}

static bool RunHiddenWait(const std::wstring& command, DWORD* exitCode = nullptr) {
    return RunProcess(command, CREATE_NO_WINDOW, true, exitCode);
}

static bool RunNewConsole(const std::wstring& command) {
    return RunProcess(command, CREATE_NEW_CONSOLE, false);
}

static bool RunCapture(const std::wstring& command, std::string& output, DWORD* exitCode = nullptr) {
    output.clear();
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    HANDLE readPipe = nullptr;
    HANDLE writePipe = nullptr;
    if (!CreatePipe(&readPipe, &writePipe, &sa, 0)) return false;
    SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);
    HANDLE nullInput = CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, 0, nullptr);

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = nullInput;
    si.hStdOutput = writePipe;
    si.hStdError = writePipe;
    PROCESS_INFORMATION pi{};
    std::wstring mutableCommand = command;
    BOOL created = CreateProcessW(nullptr, mutableCommand.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW,
                                  nullptr, nullptr, &si, &pi);
    CloseHandle(writePipe);
    if (nullInput) CloseHandle(nullInput);
    if (!created) {
        CloseHandle(readPipe);
        return false;
    }

    char buffer[4096];
    DWORD read = 0;
    while (ReadFile(readPipe, buffer, sizeof(buffer), &read, nullptr) && read > 0) {
        output.append(buffer, read);
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    if (exitCode) GetExitCodeProcess(pi.hProcess, exitCode);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    CloseHandle(readPipe);
    return true;
}

static std::wstring PowerShellCommand(const std::wstring& script) {
    return L"powershell.exe -NoProfile -NonInteractive -ExecutionPolicy Bypass -Command \"[Console]::OutputEncoding=[System.Text.Encoding]::UTF8; " + script + L"\"";
}

static bool RunPowerShellWait(const std::wstring& script, DWORD* exitCode = nullptr) {
    return RunHiddenWait(PowerShellCommand(script), exitCode);
}

static bool RunPowerShellCapture(const std::wstring& script, std::string& output, DWORD* exitCode = nullptr) {
    return RunCapture(PowerShellCommand(script), output, exitCode);
}

// Registry helpers
static bool RegSetDword(HKEY root, const wchar_t* subkey, const wchar_t* name, DWORD value) {
    HKEY key = nullptr;
    if (RegCreateKeyExW(root, subkey, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS) {
        return false;
    }
    LONG result = RegSetValueExW(key, name, 0, REG_DWORD, reinterpret_cast<const BYTE*>(&value), sizeof(value));
    RegCloseKey(key);
    return result == ERROR_SUCCESS;
}

static bool RegDeleteValueSafe(HKEY root, const wchar_t* subkey, const wchar_t* name) {
    HKEY key = nullptr;
    if (RegOpenKeyExW(root, subkey, 0, KEY_SET_VALUE, &key) != ERROR_SUCCESS) return false;
    LONG result = RegDeleteValueW(key, name);
    RegCloseKey(key);
    return result == ERROR_SUCCESS;
}

static bool RegSetString(HKEY root, const wchar_t* subkey, const wchar_t* name, const std::wstring& value) {
    HKEY key = nullptr;
    if (RegCreateKeyExW(root, subkey, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS) {
        return false;
    }
    DWORD size = static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t));
    LONG result = RegSetValueExW(key, name, 0, REG_SZ, reinterpret_cast<const BYTE*>(value.c_str()), size);
    RegCloseKey(key);
    return result == ERROR_SUCCESS;
}

static bool RegGetString(HKEY root, const wchar_t* subkey, const wchar_t* name, std::wstring& value) {
    HKEY key = nullptr;
    if (RegOpenKeyExW(root, subkey, 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS) return false;
    wchar_t buffer[1024]{};
    DWORD size = sizeof(buffer);
    DWORD type = 0;
    LONG result = RegQueryValueExW(key, name, nullptr, &type, reinterpret_cast<BYTE*>(buffer), &size);
    RegCloseKey(key);
    if (result != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ)) return false;
    value = buffer;
    return true;
}

// HTTP helper
static bool HttpGet(const std::wstring& path, std::string& body, DWORD& status) {
    body.clear();
    status = 0;
    HINTERNET session = WinHttpOpen(L"RecoverX/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                    WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) return false;
    WinHttpSetTimeouts(session, 5000, 5000, 5000, 5000);
    HINTERNET connection = WinHttpConnect(session, L"api.github.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!connection) {
        WinHttpCloseHandle(session);
        return false;
    }
    HINTERNET request = WinHttpOpenRequest(connection, L"GET", path.c_str(), nullptr,
                                           WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                           WINHTTP_FLAG_SECURE);
    if (!request) {
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return false;
    }
    WinHttpAddRequestHeaders(request, L"User-Agent: RecoverX\r\nAccept: application/vnd.github+json\r\n",
                             static_cast<DWORD>(-1), WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
    bool ok = WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
              WinHttpReceiveResponse(request, nullptr);
    if (ok) {
        DWORD size = sizeof(status);
        WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                            WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX);
        DWORD available = 0;
        while (WinHttpQueryDataAvailable(request, &available) && available > 0) {
            std::string chunk(available, '\0');
            DWORD read = 0;
            if (!WinHttpReadData(request, chunk.data(), available, &read)) break;
            body.append(chunk.data(), read);
        }
    }
    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);
    return ok;
}

class RecoverXApp {
public:
    explicit RecoverXApp(HINSTANCE instance);
    ~RecoverXApp();
    bool Create();
    HWND hwnd() const { return hwnd_; }

private:
    static LRESULT CALLBACK MainProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK PageProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK BootProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    LRESULT HandleMain(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandlePage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleBoot(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    bool RegisterClasses();
    HICON LoadAppIcon();
    HICON MakeIcon();
    void CreateFonts();
    void CreateNavigation();
    void CreatePages();
    void BuildPage(int index);
    void LayoutPages();
    void RebuildPages();
    void SwitchPage(int index);
    void UpdateNavTexts();

    void BuildPowerPage();
    void BuildBootPage();
    void BuildSystemPage();
    void BuildMaintenancePage();
    void BuildSchedulerPage();
    void BuildMonitorPage();
    void BuildRestorePage();
    void BuildUpdatesPage();
    void BuildUwpPage();
    void BuildDriversPage();
    void BuildSettingsPage();

    HWND CreateButton(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h);
    HWND CreateLabel(HWND parent, const std::wstring& text, int x, int y, int w, int h, HFONT font = nullptr);
    HWND CreateCheck(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h);
    HWND CreateRadio(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h, bool group);
    HWND CreateEdit(HWND parent, int id, int x, int y, int w, int h);
    HWND CreateList(HWND parent, int id, int x, int y, int w, int h, bool multiSelect);

    LRESULT OnCreate();
    void OnDestroy();
    void OnClose();
    void OnCommand(int id);
    void OnDrawItem(const DRAWITEMSTRUCT* item);
    void OnTimer(UINT_PTR id);
    void OnTray(UINT event);
    LRESULT OnCtlColor(UINT msg, WPARAM wParam);

    void DrawButton(const DRAWITEMSTRUCT* item);
    const std::wstring& S(const wchar_t* key) const;
    HWND Page(int index) const;

    void Info(const std::wstring& text, const std::wstring& title = L"RecoverX") const;
    void Error(const std::wstring& text, const std::wstring& title = L"Error") const;
    void Warn(const std::wstring& text, const std::wstring& title = L"RecoverX") const;
    bool Confirm(const std::wstring& text, const std::wstring& title) const;

    std::string LoadLanguage() const;
    void SaveLanguage() const;
    void SwitchLanguage(const std::string& lang);
    void CaptureUI();

    void StartTimer();
    void CancelTimer();
    std::wstring TimerText() const;
    void UpdateTimerDisplay();
    void UpdateMonitor();

    bool EnsureAdmin();
    void RequestAdmin();

    void AddTray();
    void RemoveTray();
    void ShowMainWindow();
    void Quit();

    bool IsAutostartEnabled() const;
    bool SetAutostart(bool enabled);

    void AddSchedule();
    void RefreshRestorePoints();
    void CreateRestorePoint();
    void DeleteRestorePoint();
    void OpenSystemRestore();

    void CheckAppUpdates();
    void RefreshUwpList();
    void RemoveSelectedUwp();

    void ShowBootMenu();
    void RebootToBios();
    void SetSafeMode(const std::wstring& mode);

    bool OpenApp(const std::wstring& file, const std::wstring& params = L"");
    bool OpenUrl(const std::wstring& url);
    void FlushDns();
    void BackupRegistry();
    void CleanTemp();
    void CleanUpdates();
    void MinimizeAll();
    void RestartGpu();
    void OpenPowerShell();
    void DisableAutoUpdates();
    void SetDefender(bool enabled);

    HINSTANCE instance_;
    HWND hwnd_ = nullptr;
    HICON icon_ = nullptr;
    bool iconOwned_ = false;
    HFONT font_ = nullptr;
    HFONT fontBold_ = nullptr;
    HFONT fontLarge_ = nullptr;
    HBRUSH brushBlack_ = nullptr;
    HBRUSH brushWhite_ = nullptr;
    HBRUSH brushDark_ = nullptr;
    HBRUSH brushGray_ = nullptr;
    HBRUSH brushBlue_ = nullptr;

    StrMap strings_;
    std::string lang_ = "en";
    int currentPage_ = 0;
    std::vector<HWND> pages_;
    std::vector<HWND> navButtons_;

    HWND minEdit_ = nullptr;
    HWND secEdit_ = nullptr;
    HWND forceCheck_ = nullptr;
    HWND startButton_ = nullptr;
    HWND cancelButton_ = nullptr;
    HWND statusLabel_ = nullptr;
    HWND timerShutdownRadio_ = nullptr;
    HWND timerRestartRadio_ = nullptr;
    std::wstring minText_ = L"5";
    std::wstring secText_ = L"0";
    std::wstring timerAction_ = L"shutdown";
    bool forceClose_ = false;
    bool timerRunning_ = false;
    int timerSeconds_ = 0;

    HWND schedTimeEdit_ = nullptr;
    HWND schedShutdownRadio_ = nullptr;
    HWND schedRestartRadio_ = nullptr;
    HWND dayChecks_[7]{};
    std::wstring schedTime_ = L"23:00";
    std::wstring schedAction_ = L"shutdown";
    bool dayChecked_[7]{};

    HWND cpuLabel_ = nullptr;
    HWND ramLabel_ = nullptr;
    ULONGLONG prevIdle_ = 0;
    ULONGLONG prevKernel_ = 0;
    ULONGLONG prevUser_ = 0;
    bool cpuReady_ = false;

    HWND restoreList_ = nullptr;
    HWND uwpList_ = nullptr;
    HWND uwpAllCheck_ = nullptr;
    bool uwpAll_ = false;
    std::vector<std::wstring> uwpPackages_;

    HWND trayCheck_ = nullptr;
    HWND autostartCheck_ = nullptr;
    bool trayEnabled_ = true;
    bool trayAdded_ = false;
    bool autostart_ = false;
    bool forceQuit_ = false;

    int bootChoice_ = -1;
};

// Local helpers
constexpr UINT kLanguageMessage = WM_APP + 2;
static HANDLE g_instanceMutex = nullptr;

static bool ParseInt(const std::wstring& text, int& value) {
    std::wstring trimmed = Trim(text);
    if (trimmed.empty()) return false;
    wchar_t* end = nullptr;
    long result = wcstol(trimmed.c_str(), &end, 10);
    if (end == trimmed.c_str()) return false;
    if (end && *end != L'\0') return false;
    value = static_cast<int>(result);
    return true;
}

static std::wstring EscapePowerShell(const std::wstring& text) {
    std::wstring out = text;
    ReplaceAll(out, L"'", L"''");
    return out;
}

static std::wstring ListText(HWND list, int index) {
    if (!list || index < 0) return {};
    int len = static_cast<int>(SendMessageW(list, LB_GETTEXTLEN, index, 0));
    if (len <= 0) return {};
    std::wstring text(static_cast<size_t>(len) + 1, L'\0');
    SendMessageW(list, LB_GETTEXT, index, reinterpret_cast<LPARAM>(text.data()));
    text.resize(len);
    return text;
}

static std::wstring Join(const std::vector<std::wstring>& parts, const std::wstring& sep) {
    std::wstring out;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i) out += sep;
        out += parts[i];
    }
    return out;
}

static void SetCheckState(HWND hwnd, bool checked) {
    if (!hwnd) return;
    SendMessageW(hwnd, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
    InvalidateRect(hwnd, nullptr, TRUE);
}

static const wchar_t* const kDayNamesEn[7] = {L"Mon", L"Tue", L"Wed", L"Thu", L"Fri", L"Sat", L"Sun"};
static const wchar_t* const kDayNamesRu[7] = {L"Пн", L"Вт", L"Ср", L"Чт", L"Пт", L"Сб", L"Вс"};
static const wchar_t* const kDayNamesSched[7] = {L"MON", L"TUE", L"WED", L"THU", L"FRI", L"SAT", L"SUN"};

static ULONGLONG FileTimeToU64(const FILETIME& ft) {
    ULARGE_INTEGER value{};
    value.LowPart = ft.dwLowDateTime;
    value.HighPart = ft.dwHighDateTime;
    return value.QuadPart;
}

RecoverXApp::RecoverXApp(HINSTANCE instance) : instance_(instance) {
    lang_ = LoadLanguage();
    strings_ = BuildStrings(lang_);
}

RecoverXApp::~RecoverXApp() {
    if (trayAdded_) {
        NOTIFYICONDATAW nid{};
        nid.cbSize = sizeof(nid);
        nid.hWnd = hwnd_;
        nid.uID = 1;
        Shell_NotifyIconW(NIM_DELETE, &nid);
    }
    if (iconOwned_ && icon_) DestroyIcon(icon_);
    if (font_) DeleteObject(font_);
    if (fontBold_) DeleteObject(fontBold_);
    if (fontLarge_) DeleteObject(fontLarge_);
    if (brushBlack_) DeleteObject(brushBlack_);
    if (brushWhite_) DeleteObject(brushWhite_);
    if (brushDark_) DeleteObject(brushDark_);
    if (brushGray_) DeleteObject(brushGray_);
    if (brushBlue_) DeleteObject(brushBlue_);
}

bool RecoverXApp::Create() {
    if (!RegisterClasses()) return false;
    RECT rect{0, 0, kWindowWidth, kWindowHeight};
    AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, 0);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int x = (screenW - width) / 2;
    int y = (screenH - height) / 2;
    hwnd_ = CreateWindowExW(WS_EX_APPWINDOW, kMainClass, S(L"title").c_str(),
                            WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, x, y, width, height,
                            nullptr, nullptr, instance_, this);
    if (!hwnd_) return false;
    ShowWindow(hwnd_, SW_SHOW);
    UpdateWindow(hwnd_);
    return true;
}

bool RecoverXApp::RegisterClasses() {
    WNDCLASSEXW mainClass{};
    mainClass.cbSize = sizeof(mainClass);
    mainClass.style = CS_HREDRAW | CS_VREDRAW;
    mainClass.lpfnWndProc = MainProc;
    mainClass.hInstance = instance_;
    mainClass.hIcon = LoadIconW(instance_, MAKEINTRESOURCEW(101));
    mainClass.hIconSm = mainClass.hIcon;
    mainClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    mainClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    mainClass.lpszClassName = kMainClass;
    if (!RegisterClassExW(&mainClass)) return false;

    WNDCLASSEXW pageClass = mainClass;
    pageClass.lpfnWndProc = PageProc;
    pageClass.hIcon = nullptr;
    pageClass.hIconSm = nullptr;
    pageClass.lpszClassName = kPageClass;
    if (!RegisterClassExW(&pageClass)) return false;

    WNDCLASSEXW bootClass = mainClass;
    bootClass.lpfnWndProc = BootProc;
    bootClass.hbrBackground = nullptr;
    bootClass.lpszClassName = kBootClass;
    if (!RegisterClassExW(&bootClass)) return false;
    return true;
}

HICON RecoverXApp::LoadAppIcon() {
    HICON icon = LoadIconW(instance_, MAKEINTRESOURCEW(101));
    if (icon) {
        iconOwned_ = false;
        return icon;
    }
    iconOwned_ = true;
    return MakeIcon();
}

HICON RecoverXApp::MakeIcon() {
    const int size = 64;
    BITMAPV5HEADER header{};
    header.bV5Size = sizeof(header);
    header.bV5Width = size;
    header.bV5Height = -size;
    header.bV5Planes = 1;
    header.bV5BitCount = 32;
    header.bV5Compression = BI_BITFIELDS;
    header.bV5RedMask = 0x00FF0000;
    header.bV5GreenMask = 0x0000FF00;
    header.bV5BlueMask = 0x000000FF;
    header.bV5AlphaMask = 0xFF000000;

    HDC screen = GetDC(nullptr);
    void* bits = nullptr;
    HBITMAP color = CreateDIBSection(screen, reinterpret_cast<BITMAPINFO*>(&header), DIB_RGB_COLORS, &bits, nullptr, 0);
    HBITMAP mask = CreateBitmap(size, size, 1, 1, nullptr);
    HICON icon = nullptr;
    if (color && mask && bits) {
        HDC memory = CreateCompatibleDC(screen);
        HGDIOBJ oldBitmap = SelectObject(memory, color);
        RECT rect{0, 0, size, size};
        HBRUSH black = CreateSolidBrush(RGB(0, 0, 0));
        FillRect(memory, &rect, black);
        DeleteObject(black);

        SetBkMode(memory, TRANSPARENT);
        SetTextColor(memory, RGB(255, 255, 255));
        HFONT font = CreateFontW(-46, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                 OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                 DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        HGDIOBJ oldFont = SelectObject(memory, font);
        DrawTextW(memory, L"R", -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(memory, oldFont);
        DeleteObject(font);
        SelectObject(memory, oldBitmap);
        DeleteDC(memory);

        DWORD* pixels = static_cast<DWORD*>(bits);
        for (int i = 0; i < size * size; ++i) pixels[i] |= 0xFF000000;

        ICONINFO info{};
        info.fIcon = TRUE;
        info.hbmMask = mask;
        info.hbmColor = color;
        icon = CreateIconIndirect(&info);
    }
    if (color) DeleteObject(color);
    if (mask) DeleteObject(mask);
    ReleaseDC(nullptr, screen);
    return icon;
}

void RecoverXApp::CreateFonts() {
    font_ = CreateFontW(-15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                        DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    fontBold_ = CreateFontW(-15, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                            DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    fontLarge_ = CreateFontW(-30, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                             DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
}

void RecoverXApp::CreateNavigation() {
    CreateLabel(hwnd_, L"RecoverX", 18, 10, 144, 40, fontLarge_);
    CreateLabel(hwnd_, kVersion, 20, 50, 120, 20, font_);

    static const wchar_t* keys[PAGE_COUNT] = {
        L"power", L"boot", L"system", L"maintenance", L"scheduler", L"monitor",
        L"restore", L"windows_update", L"uwp", L"drivers", L"settings"
    };
    navButtons_.clear();
    for (int i = 0; i < PAGE_COUNT; ++i) {
        HWND button = CreateButton(hwnd_, ID_TAB_FIRST + i, S(keys[i]), 10, 88 + i * 38, 160, 34);
        navButtons_.push_back(button);
    }
}

void RecoverXApp::CreatePages() {
    RECT rc{};
    GetClientRect(hwnd_, &rc);
    int width = std::max(200, static_cast<int>(rc.right) - kNavWidth);
    int height = std::max(200, static_cast<int>(rc.bottom));
    pages_.assign(PAGE_COUNT, nullptr);
    for (int i = 0; i < PAGE_COUNT; ++i) {
        pages_[i] = CreateWindowExW(0, kPageClass, L"", WS_CHILD | WS_CLIPCHILDREN,
                                    kNavWidth, 0, width, height, hwnd_,
                                    reinterpret_cast<HMENU>(static_cast<INT_PTR>(i)), instance_, this);
        BuildPage(i);
        ShowWindow(pages_[i], i == currentPage_ ? SW_SHOW : SW_HIDE);
    }
}

void RecoverXApp::BuildPage(int index) {
    switch (index) {
        case PAGE_POWER: BuildPowerPage(); break;
        case PAGE_BOOT: BuildBootPage(); break;
        case PAGE_SYSTEM: BuildSystemPage(); break;
        case PAGE_MAINTENANCE: BuildMaintenancePage(); break;
        case PAGE_SCHEDULER: BuildSchedulerPage(); break;
        case PAGE_MONITOR: BuildMonitorPage(); break;
        case PAGE_RESTORE: BuildRestorePage(); break;
        case PAGE_UPDATES: BuildUpdatesPage(); break;
        case PAGE_UWP: BuildUwpPage(); break;
        case PAGE_DRIVERS: BuildDriversPage(); break;
        case PAGE_SETTINGS: BuildSettingsPage(); break;
        default: break;
    }
}

void RecoverXApp::LayoutPages() {
    if (pages_.empty()) return;
    RECT rc{};
    GetClientRect(hwnd_, &rc);
    int width = std::max(200, static_cast<int>(rc.right) - kNavWidth);
    int height = std::max(200, static_cast<int>(rc.bottom));
    for (HWND page : pages_) {
        if (page) MoveWindow(page, kNavWidth, 0, width, height, TRUE);
    }

    HWND restorePage = Page(PAGE_RESTORE);
    if (restorePage && restoreList_) {
        int listW = std::max(120, width - 40);
        int listH = std::max(80, height - 150);
        MoveWindow(restoreList_, 20, 64, listW, listH, TRUE);
        int buttonW = std::max(90, (width - 70) / 4);
        int y = height - 64;
        for (int i = 0; i < 4; ++i) {
            HWND button = GetDlgItem(restorePage, ID_RESTORE_REFRESH + i);
            if (button) MoveWindow(button, 20 + i * (buttonW + 10), y, buttonW, 36, TRUE);
        }
    }

    HWND uwpPage = Page(PAGE_UWP);
    if (uwpPage && uwpList_) {
        int listW = std::max(120, width - 40);
        int listH = std::max(80, height - 190);
        MoveWindow(uwpList_, 20, 92, listW, listH, TRUE);
        int y = height - 72;
        if (HWND button = GetDlgItem(uwpPage, ID_UWP_REFRESH))
            MoveWindow(button, 20, y, 150, 36, TRUE);
        if (uwpAllCheck_)
            MoveWindow(uwpAllCheck_, 184, y + 6, 170, 26, TRUE);
        if (HWND button = GetDlgItem(uwpPage, ID_UWP_REMOVE))
            MoveWindow(button, std::max(360, width - 180), y, 160, 36, TRUE);
    }
}

void RecoverXApp::RebuildPages() {
    for (HWND page : pages_) {
        if (page) DestroyWindow(page);
    }
    pages_.clear();
    minEdit_ = nullptr;
    secEdit_ = nullptr;
    forceCheck_ = nullptr;
    startButton_ = nullptr;
    cancelButton_ = nullptr;
    statusLabel_ = nullptr;
    timerShutdownRadio_ = nullptr;
    timerRestartRadio_ = nullptr;
    schedTimeEdit_ = nullptr;
    schedShutdownRadio_ = nullptr;
    schedRestartRadio_ = nullptr;
    for (auto& day : dayChecks_) day = nullptr;
    cpuLabel_ = nullptr;
    ramLabel_ = nullptr;
    restoreList_ = nullptr;
    uwpList_ = nullptr;
    uwpAllCheck_ = nullptr;
    trayCheck_ = nullptr;
    autostartCheck_ = nullptr;
    CreatePages();
    LayoutPages();
    SwitchPage(currentPage_);
    CaptureUI();
}

void RecoverXApp::SwitchPage(int index) {
    if (index < 0 || index >= PAGE_COUNT) return;
    currentPage_ = index;
    for (int i = 0; i < static_cast<int>(pages_.size()); ++i) {
        if (pages_[i]) ShowWindow(pages_[i], i == index ? SW_SHOW : SW_HIDE);
    }
    for (HWND button : navButtons_) InvalidateRect(button, nullptr, TRUE);
    if (index == PAGE_MONITOR) UpdateMonitor();
    if (pages_[index]) SetFocus(pages_[index]);
}

void RecoverXApp::UpdateNavTexts() {
    static const wchar_t* keys[PAGE_COUNT] = {
        L"power", L"boot", L"system", L"maintenance", L"scheduler", L"monitor",
        L"restore", L"windows_update", L"uwp", L"drivers", L"settings"
    };
    for (int i = 0; i < static_cast<int>(navButtons_.size()); ++i) {
        SetText(navButtons_[i], S(keys[i]));
    }
}

void RecoverXApp::CaptureUI() {
    InvalidateRect(hwnd_, nullptr, TRUE);
    UpdateWindow(hwnd_);
}

HWND RecoverXApp::CreateButton(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
    HWND button = CreateWindowExW(0, L"BUTTON", text.c_str(),
                                  WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                                  x, y, w, h, parent,
                                  reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance_, nullptr);
    SendMessageW(button, WM_SETFONT, reinterpret_cast<WPARAM>(font_), TRUE);
    return button;
}

HWND RecoverXApp::CreateLabel(HWND parent, const std::wstring& text, int x, int y, int w, int h, HFONT font) {
    HWND label = CreateWindowExW(0, L"STATIC", text.c_str(), WS_CHILD | WS_VISIBLE | SS_LEFT,
                                 x, y, w, h, parent, nullptr, instance_, nullptr);
    SendMessageW(label, WM_SETFONT, reinterpret_cast<WPARAM>(font ? font : font_), TRUE);
    return label;
}

HWND RecoverXApp::CreateCheck(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h) {
    HWND check = CreateWindowExW(0, L"BUTTON", text.c_str(),
                                 WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                                 x, y, w, h, parent,
                                 reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance_, nullptr);
    SetPropW(check, L"rxkind", reinterpret_cast<HANDLE>(1));
    SendMessageW(check, WM_SETFONT, reinterpret_cast<WPARAM>(font_), TRUE);
    return check;
}

HWND RecoverXApp::CreateRadio(HWND parent, int id, const std::wstring& text, int x, int y, int w, int h, bool group) {
    DWORD style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW;
    if (group) style |= WS_GROUP;
    HWND radio = CreateWindowExW(0, L"BUTTON", text.c_str(), style,
                                 x, y, w, h, parent,
                                 reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance_, nullptr);
    SetPropW(radio, L"rxkind", reinterpret_cast<HANDLE>(2));
    SendMessageW(radio, WM_SETFONT, reinterpret_cast<WPARAM>(font_), TRUE);
    return radio;
}

HWND RecoverXApp::CreateEdit(HWND parent, int id, int x, int y, int w, int h) {
    HWND edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL | ES_LEFT,
                                x, y, w, h, parent,
                                reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance_, nullptr);
    SendMessageW(edit, WM_SETFONT, reinterpret_cast<WPARAM>(font_), TRUE);
    SendMessageW(edit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(4, 4));
    return edit;
}

HWND RecoverXApp::CreateList(HWND parent, int id, int x, int y, int w, int h, bool multiSelect) {
    DWORD style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | WS_BORDER |
                  LBS_NOINTEGRALHEIGHT | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS;
    style |= multiSelect ? LBS_EXTENDEDSEL : 0;
    HWND list = CreateWindowExW(0, L"LISTBOX", nullptr, style, x, y, w, h, parent,
                                reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance_, nullptr);
    SendMessageW(list, WM_SETFONT, reinterpret_cast<WPARAM>(font_), TRUE);
    SendMessageW(list, LB_SETITEMHEIGHT, 0, 24);
    return list;
}

void RecoverXApp::BuildPowerPage() {
    HWND page = Page(PAGE_POWER);
    if (!page) return;
    CreateLabel(page, S(L"power"), 24, 14, 540, 40, fontLarge_);

    timerShutdownRadio_ = CreateRadio(page, ID_TIMER_SHUTDOWN, S(L"timer_shutdown"), 24, 74, 180, 26, true);
    timerRestartRadio_ = CreateRadio(page, ID_TIMER_RESTART, S(L"timer_restart"), 216, 74, 180, 26, false);
    SetCheckState(timerShutdownRadio_, timerAction_ == L"shutdown");
    SetCheckState(timerRestartRadio_, timerAction_ == L"restart");

    CreateLabel(page, L"Min:", 24, 118, 36, 24);
    minEdit_ = CreateEdit(page, ID_MIN_EDIT, 66, 114, 62, 26);
    SetText(minEdit_, minText_);
    CreateLabel(page, L"Sec:", 146, 118, 36, 24);
    secEdit_ = CreateEdit(page, ID_SEC_EDIT, 188, 114, 62, 26);
    SetText(secEdit_, secText_);

    forceCheck_ = CreateCheck(page, ID_FORCE_CHECK, S(L"force_check"), 24, 154, 440, 26);
    SetCheckState(forceCheck_, forceClose_);

    startButton_ = CreateButton(page, ID_START_TIMER, S(L"start_timer"), 24, 198, 170, 38);
    cancelButton_ = CreateButton(page, ID_CANCEL_TIMER, S(L"cancel_timer"), 208, 198, 170, 38);
    EnableWindow(startButton_, !timerRunning_);
    EnableWindow(cancelButton_, timerRunning_);

    statusLabel_ = CreateLabel(page, L"", 24, 250, 540, 28, fontBold_);
    SetText(statusLabel_, timerRunning_ ? TimerText() : S(L"status_idle"));

    CreateButton(page, ID_SLEEP, S(L"sleep"), 24, 314, 166, 38);
    CreateButton(page, ID_SHUTDOWN, S(L"shutdown_direct"), 202, 314, 166, 38);
    CreateButton(page, ID_FORCE_SHUTDOWN, S(L"force_shutdown"), 380, 314, 166, 38);
}

void RecoverXApp::BuildBootPage() {
    HWND page = Page(PAGE_BOOT);
    if (!page) return;
    CreateLabel(page, S(L"boot"), 24, 14, 540, 40, fontLarge_);
    CreateButton(page, ID_BOOT_BIOS, S(L"boot_bios"), 24, 78, 520, 38);
    CreateButton(page, ID_BOOT_SAFE_MIN, S(L"boot_safe_min"), 24, 126, 520, 38);
    CreateButton(page, ID_BOOT_SAFE_NET, S(L"boot_safe_net"), 24, 174, 520, 38);
    CreateButton(page, ID_BOOT_SAFE_RESET, S(L"boot_reset"), 24, 222, 520, 38);
    CreateButton(page, ID_BOOT_MENU, S(L"boot_menu"), 24, 286, 520, 38);
}

void RecoverXApp::BuildSystemPage() {
    HWND page = Page(PAGE_SYSTEM);
    if (!page) return;
    CreateLabel(page, S(L"system"), 24, 14, 540, 40, fontLarge_);

    struct Entry { int id; const wchar_t* key; };
    static const Entry entries[] = {
        {ID_SYS_INFO, L"sys_info"}, {ID_DEV_MGR, L"dev_mgr"}, {ID_TASK_MGR, L"task_mgr"},
        {ID_DISK_MGR, L"disk_mgr"}, {ID_LOCAL_USERS, L"local_users"}, {ID_REGEDIT, L"regedit"},
        {ID_CONTROL_PANEL, L"control_panel"}, {ID_OSK, L"osk"}, {ID_MAGNIFIER, L"magnifier"},
        {ID_MINIMIZE_ALL, L"minimize_all"}, {ID_GPU_RESET, L"gpu_reset"}, {ID_APPS_FOLDER, L"apps_folder"},
        {ID_POWERSHELL, L"powershell"}, {ID_CHANGE_TIME, L"change_time"}, {ID_FLUSH_DNS, L"flush_dns"},
        {ID_SERVICES, L"services_mgr"}, {ID_BACKUP_REGISTRY, L"backup_registry"}
    };
    const int count = static_cast<int>(sizeof(entries) / sizeof(entries[0]));
    const int half = (count + 1) / 2;
    for (int i = 0; i < count; ++i) {
        int column = i < half ? 0 : 1;
        int row = i < half ? i : i - half;
        CreateButton(page, entries[i].id, S(entries[i].key), 24 + column * 290, 70 + row * 34, 270, 29);
    }
}

void RecoverXApp::BuildMaintenancePage() {
    HWND page = Page(PAGE_MAINTENANCE);
    if (!page) return;
    CreateLabel(page, S(L"maintenance"), 24, 14, 540, 40, fontLarge_);
    CreateButton(page, ID_CLEAN_TEMP, S(L"clean_temp"), 24, 80, 440, 42);
    CreateButton(page, ID_SFC, S(L"sfc"), 24, 136, 440, 42);
    CreateButton(page, ID_DISM, S(L"dism"), 24, 192, 440, 42);
    CreateButton(page, ID_CLEAN_UPDATES, S(L"clean_updates"), 24, 248, 440, 42);
    CreateButton(page, ID_MRT, S(L"mrt"), 24, 304, 440, 42);
}

void RecoverXApp::BuildSchedulerPage() {
    HWND page = Page(PAGE_SCHEDULER);
    if (!page) return;
    CreateLabel(page, S(L"scheduler"), 24, 14, 540, 40, fontLarge_);
    CreateLabel(page, S(L"sched_info"), 24, 62, 540, 40);

    CreateLabel(page, S(L"sched_action"), 24, 116, 76, 24);
    schedShutdownRadio_ = CreateRadio(page, ID_SCHED_SHUTDOWN, S(L"timer_shutdown"), 104, 112, 160, 26, true);
    schedRestartRadio_ = CreateRadio(page, ID_SCHED_RESTART, S(L"timer_restart"), 276, 112, 160, 26, false);
    SetCheckState(schedShutdownRadio_, schedAction_ == L"shutdown");
    SetCheckState(schedRestartRadio_, schedAction_ == L"restart");

    CreateLabel(page, S(L"sched_time"), 24, 158, 132, 24);
    schedTimeEdit_ = CreateEdit(page, ID_SCHED_TIME, 160, 154, 90, 26);
    SetText(schedTimeEdit_, schedTime_);

    CreateLabel(page, S(L"sched_days"), 24, 194, 50, 24);
    for (int i = 0; i < 7; ++i) {
        const wchar_t* name = lang_ == "ru" ? kDayNamesRu[i] : kDayNamesEn[i];
        dayChecks_[i] = CreateCheck(page, ID_DAY_FIRST + i, name, 78 + i * 60, 190, 58, 26);
        SetCheckState(dayChecks_[i], dayChecked_[i]);
    }

    CreateButton(page, ID_SCHED_ADD, S(L"sched_add"), 24, 238, 220, 38);
}

void RecoverXApp::BuildMonitorPage() {
    HWND page = Page(PAGE_MONITOR);
    if (!page) return;
    CreateLabel(page, S(L"monitor"), 24, 14, 540, 40, fontLarge_);
    cpuLabel_ = CreateLabel(page, S(L"cpu_label") + L" --%", 24, 96, 540, 52, fontLarge_);
    ramLabel_ = CreateLabel(page, S(L"ram_label") + L" --%", 24, 176, 540, 52, fontLarge_);
    UpdateMonitor();
}

void RecoverXApp::BuildRestorePage() {
    HWND page = Page(PAGE_RESTORE);
    if (!page) return;
    CreateLabel(page, S(L"restore"), 24, 14, 540, 40, fontLarge_);
    restoreList_ = CreateList(page, ID_RESTORE_LIST, 20, 64, 560, 420, false);
    for (int i = 0; i < 4; ++i) {
        static const wchar_t* keys[4] = {L"refresh_rp", L"create_rp", L"delete_rp", L"open_restore"};
        CreateButton(page, ID_RESTORE_REFRESH + i, S(keys[i]), 20 + i * 145, 520, 135, 36);
    }
}

void RecoverXApp::BuildUpdatesPage() {
    HWND page = Page(PAGE_UPDATES);
    if (!page) return;
    CreateLabel(page, S(L"windows_update"), 24, 14, 540, 40, fontLarge_);
    CreateButton(page, ID_UPDATE_CHECK, S(L"check_updates"), 24, 74, 440, 38);
    CreateButton(page, ID_UPDATE_DISABLE, S(L"disable_updates"), 24, 120, 440, 38);
    CreateLabel(page, S(L"defender_section"), 24, 184, 440, 24, fontBold_);
    CreateButton(page, ID_DEFENDER_DISABLE, S(L"disable_defender"), 24, 218, 440, 38);
    CreateButton(page, ID_DEFENDER_ENABLE, S(L"enable_defender"), 24, 264, 440, 38);
}

void RecoverXApp::BuildUwpPage() {
    HWND page = Page(PAGE_UWP);
    if (!page) return;
    CreateLabel(page, S(L"uwp"), 24, 14, 540, 40, fontLarge_);
    CreateLabel(page, S(L"uwp_admin"), 24, 58, 440, 24, fontBold_);
    uwpList_ = CreateList(page, ID_UWP_LIST, 20, 92, 560, 410, true);
    CreateButton(page, ID_UWP_REFRESH, S(L"refresh_uwp"), 20, 516, 150, 36);
    uwpAllCheck_ = CreateCheck(page, ID_UWP_ALL, S(L"uwp_for_all"), 184, 522, 170, 26);
    SetCheckState(uwpAllCheck_, uwpAll_);
    CreateButton(page, ID_UWP_REMOVE, S(L"remove_uwp"), 400, 516, 160, 36);
}

void RecoverXApp::BuildDriversPage() {
    HWND page = Page(PAGE_DRIVERS);
    if (!page) return;
    CreateLabel(page, S(L"drivers"), 24, 14, 540, 40, fontLarge_);
    CreateButton(page, ID_DRIVER_NVIDIA, S(L"drivers_nvidia"), 24, 84, 460, 42);
    CreateButton(page, ID_DRIVER_AMD, S(L"drivers_amd"), 24, 136, 460, 42);
    CreateButton(page, ID_DRIVER_INTEL, S(L"drivers_intel"), 24, 188, 460, 42);
}

void RecoverXApp::BuildSettingsPage() {
    HWND page = Page(PAGE_SETTINGS);
    if (!page) return;
    CreateLabel(page, S(L"settings"), 24, 14, 540, 40, fontLarge_);
    trayCheck_ = CreateCheck(page, ID_TRAY_CHECK, S(L"tray_on"), 24, 76, 440, 26);
    SetCheckState(trayCheck_, trayEnabled_);
    autostartCheck_ = CreateCheck(page, ID_AUTOSTART_CHECK, S(L"autostart"), 24, 112, 440, 26);
    SetCheckState(autostartCheck_, autostart_);
    CreateButton(page, ID_STARTUP_SETTINGS, S(L"startup_settings"), 24, 154, 440, 38);
    CreateButton(page, ID_APP_UPDATE_CHECK, S(L"check_app_updates"), 24, 202, 440, 38);
    CreateLabel(page, S(L"language"), 24, 258, 160, 24);
    CreateButton(page, ID_LANG_EN, L"English", 24, 286, 160, 38);
    CreateButton(page, ID_LANG_RU, L"Русский", 196, 286, 160, 38);
    CreateLabel(page, std::wstring(L"RecoverX ") + kVersion, 24, 344, 440, 24);
}

// Dialogs
void RecoverXApp::Info(const std::wstring& text, const std::wstring& title) const {
    MessageBoxW(hwnd_, text.c_str(), title.c_str(), MB_OK | MB_ICONINFORMATION);
}

void RecoverXApp::Error(const std::wstring& text, const std::wstring& title) const {
    MessageBoxW(hwnd_, text.c_str(), title.c_str(), MB_OK | MB_ICONERROR);
}

void RecoverXApp::Warn(const std::wstring& text, const std::wstring& title) const {
    MessageBoxW(hwnd_, text.c_str(), title.c_str(), MB_OK | MB_ICONWARNING);
}

bool RecoverXApp::Confirm(const std::wstring& text, const std::wstring& title) const {
    return MessageBoxW(hwnd_, text.c_str(), title.c_str(), MB_YESNO | MB_ICONQUESTION) == IDYES;
}

// Language
std::string RecoverXApp::LoadLanguage() const {
    std::ifstream file(std::filesystem::path(ConfigPath()), std::ios::binary);
    if (!file) return "en";
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string lang = ExtractJsonString(buffer.str(), "language");
    return lang == "ru" ? "ru" : "en";
}

void RecoverXApp::SaveLanguage() const {
    std::ofstream file(std::filesystem::path(ConfigPath()), std::ios::binary | std::ios::trunc);
    if (!file) return;
    file << "{\"language\": \"" << lang_ << "\"}";
}

void RecoverXApp::SwitchLanguage(const std::string& lang) {
    if (lang != "en" && lang != "ru") return;
    lang_ = lang;
    strings_ = BuildStrings(lang_);
    SaveLanguage();
    SetText(hwnd_, S(L"title"));
    UpdateNavTexts();
    RebuildPages();
}

const std::wstring& RecoverXApp::S(const wchar_t* key) const {
    static const std::wstring empty;
    auto it = strings_.find(key);
    return it == strings_.end() ? empty : it->second;
}

HWND RecoverXApp::Page(int index) const {
    if (index < 0 || index >= static_cast<int>(pages_.size())) return nullptr;
    return pages_[index];
}

LRESULT RecoverXApp::OnCreate() {
    CreateFonts();
    brushBlack_ = CreateSolidBrush(RGB(0, 0, 0));
    brushWhite_ = CreateSolidBrush(RGB(255, 255, 255));
    brushDark_ = CreateSolidBrush(RGB(24, 24, 24));
    brushGray_ = CreateSolidBrush(RGB(45, 45, 45));
    brushBlue_ = CreateSolidBrush(RGB(0, 90, 158));

    icon_ = LoadAppIcon();
    SendMessageW(hwnd_, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(icon_));
    SendMessageW(hwnd_, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(icon_));
    SetText(hwnd_, S(L"title"));

    BOOL dark = TRUE;
    if (HMODULE dwm = LoadLibraryW(L"dwmapi.dll")) {
        using DwmSetWindowAttributeFn = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
        auto setAttribute = reinterpret_cast<DwmSetWindowAttributeFn>(
            GetProcAddress(dwm, "DwmSetWindowAttribute"));
        if (setAttribute) {
            if (FAILED(setAttribute(hwnd_, 20, &dark, sizeof(dark))))
                setAttribute(hwnd_, 19, &dark, sizeof(dark));
        }
        FreeLibrary(dwm);
    }

    CreateNavigation();
    autostart_ = IsAutostartEnabled();
    CreatePages();
    LayoutPages();
    if (trayEnabled_) AddTray();
    SetTimer(hwnd_, kMonitorTimer, 2000, nullptr);
    return 0;
}

void RecoverXApp::OnDestroy() {
    KillTimer(hwnd_, kCountdownTimer);
    KillTimer(hwnd_, kMonitorTimer);
    RemoveTray();
    PostQuitMessage(0);
}

void RecoverXApp::OnClose() {
    if (forceQuit_ || !trayEnabled_) {
        Quit();
        return;
    }
    AddTray();
    if (trayAdded_) {
        ShowWindow(hwnd_, SW_HIDE);
    } else {
        ShowWindow(hwnd_, SW_MINIMIZE);
    }
}

void RecoverXApp::OnCommand(int id) {
    if (id >= ID_TAB_FIRST && id <= ID_TAB_LAST) {
        SwitchPage(id - ID_TAB_FIRST);
        return;
    }
    if (id >= ID_DAY_FIRST && id < ID_DAY_FIRST + 7) {
        int index = id - ID_DAY_FIRST;
        dayChecked_[index] = !dayChecked_[index];
        SetCheckState(dayChecks_[index], dayChecked_[index]);
        return;
    }

    switch (id) {
        case ID_TIMER_SHUTDOWN:
        case ID_TIMER_RESTART:
            timerAction_ = id == ID_TIMER_SHUTDOWN ? L"shutdown" : L"restart";
            SetCheckState(timerShutdownRadio_, timerAction_ == L"shutdown");
            SetCheckState(timerRestartRadio_, timerAction_ == L"restart");
            return;
        case ID_FORCE_CHECK:
            forceClose_ = !forceClose_;
            SetCheckState(forceCheck_, forceClose_);
            return;
        case ID_START_TIMER: StartTimer(); return;
        case ID_CANCEL_TIMER: CancelTimer(); return;
        case ID_SLEEP: SetSuspendState(FALSE, TRUE, FALSE); return;
        case ID_SHUTDOWN:
            if (Confirm(S(L"confirm_shutdown"), S(L"timer_shutdown"))) RunHidden(L"shutdown /s /t 0");
            return;
        case ID_FORCE_SHUTDOWN:
            if (Confirm(S(L"confirm_force"), S(L"force_shutdown"))) RunHidden(L"shutdown /s /f /t 0");
            return;

        case ID_BOOT_BIOS: RebootToBios(); return;
        case ID_BOOT_SAFE_MIN: SetSafeMode(L"minimal"); return;
        case ID_BOOT_SAFE_NET: SetSafeMode(L"network"); return;
        case ID_BOOT_SAFE_RESET: SetSafeMode(L"normal"); return;
        case ID_BOOT_MENU: ShowBootMenu(); return;

        case ID_SYS_INFO: OpenApp(L"msinfo32.exe"); return;
        case ID_DEV_MGR: OpenApp(L"devmgmt.msc"); return;
        case ID_TASK_MGR: OpenApp(L"taskmgr.exe"); return;
        case ID_DISK_MGR: OpenApp(L"diskmgmt.msc"); return;
        case ID_LOCAL_USERS:
            if (!OpenApp(L"lusrmgr.msc")) Error(S(L"not_available"), S(L"system"));
            return;
        case ID_REGEDIT: OpenApp(L"regedit.exe"); return;
        case ID_CONTROL_PANEL: OpenApp(L"control.exe"); return;
        case ID_OSK: OpenApp(L"osk.exe"); return;
        case ID_MAGNIFIER: OpenApp(L"magnify.exe"); return;
        case ID_MINIMIZE_ALL: MinimizeAll(); return;
        case ID_GPU_RESET: RestartGpu(); return;
        case ID_APPS_FOLDER: OpenApp(L"shell:AppsFolder"); return;
        case ID_POWERSHELL: OpenPowerShell(); return;
        case ID_CHANGE_TIME: OpenApp(L"control.exe", L"timedate.cpl"); return;
        case ID_FLUSH_DNS: FlushDns(); return;
        case ID_SERVICES: OpenApp(L"services.msc"); return;
        case ID_BACKUP_REGISTRY: BackupRegistry(); return;

        case ID_CLEAN_TEMP: CleanTemp(); return;
        case ID_SFC:
            if (EnsureAdmin()) RunNewConsole(L"cmd.exe /k sfc /scannow");
            return;
        case ID_DISM:
            if (EnsureAdmin()) RunNewConsole(L"cmd.exe /k DISM /Online /Cleanup-Image /RestoreHealth");
            return;
        case ID_CLEAN_UPDATES: CleanUpdates(); return;
        case ID_MRT: OpenApp(L"mrt.exe"); return;

        case ID_SCHED_SHUTDOWN:
        case ID_SCHED_RESTART:
            schedAction_ = id == ID_SCHED_SHUTDOWN ? L"shutdown" : L"restart";
            SetCheckState(schedShutdownRadio_, schedAction_ == L"shutdown");
            SetCheckState(schedRestartRadio_, schedAction_ == L"restart");
            return;
        case ID_SCHED_ADD: AddSchedule(); return;

        case ID_RESTORE_REFRESH: RefreshRestorePoints(); return;
        case ID_RESTORE_CREATE: CreateRestorePoint(); return;
        case ID_RESTORE_DELETE: DeleteRestorePoint(); return;
        case ID_RESTORE_OPEN: OpenSystemRestore(); return;

        case ID_UPDATE_CHECK: CheckAppUpdates(); return;
        case ID_UPDATE_DISABLE: DisableAutoUpdates(); return;
        case ID_DEFENDER_DISABLE: SetDefender(false); return;
        case ID_DEFENDER_ENABLE: SetDefender(true); return;

        case ID_UWP_REFRESH: RefreshUwpList(); return;
        case ID_UWP_ALL:
            uwpAll_ = !uwpAll_;
            SetCheckState(uwpAllCheck_, uwpAll_);
            return;
        case ID_UWP_REMOVE: RemoveSelectedUwp(); return;

        case ID_DRIVER_NVIDIA: OpenUrl(L"https://www.nvidia.com/download/index.aspx"); return;
        case ID_DRIVER_AMD: OpenUrl(L"https://www.amd.com/en/support"); return;
        case ID_DRIVER_INTEL: OpenUrl(L"https://www.intel.com/content/www/us/en/download-center/home.html"); return;

        case ID_TRAY_CHECK:
            trayEnabled_ = !trayEnabled_;
            SetCheckState(trayCheck_, trayEnabled_);
            if (trayEnabled_) {
                AddTray();
            } else {
                RemoveTray();
                if (!IsWindowVisible(hwnd_)) ShowMainWindow();
            }
            return;
        case ID_AUTOSTART_CHECK: {
            autostart_ = !autostart_;
            SetCheckState(autostartCheck_, autostart_);
            if (!SetAutostart(autostart_)) {
                Error(LastErrorText(), S(L"error"));
                autostart_ = !autostart_;
                SetCheckState(autostartCheck_, autostart_);
            }
            return;
        }
        case ID_STARTUP_SETTINGS: OpenUrl(L"ms-settings:startupapps"); return;
        case ID_APP_UPDATE_CHECK: CheckAppUpdates(); return;
        case ID_LANG_EN: PostMessageW(hwnd_, kLanguageMessage, ID_LANG_EN, 0); return;
        case ID_LANG_RU: PostMessageW(hwnd_, kLanguageMessage, ID_LANG_RU, 0); return;

        case ID_TRAY_OPEN: ShowMainWindow(); return;
        case ID_TRAY_EXIT: Quit(); return;
        default: return;
    }
}

void RecoverXApp::OnDrawItem(const DRAWITEMSTRUCT* item) {
    if (!item || !item->hDC) return;
    if (item->CtlID == ID_RESTORE_LIST || item->CtlID == ID_UWP_LIST) {
        HBRUSH background = (item->itemState & ODS_SELECTED) ? brushGray_ : brushBlack_;
        FillRect(item->hDC, &item->rcItem, background);
        if (item->itemID != static_cast<UINT>(-1)) {
            std::wstring text = ListText(item->hwndItem, static_cast<int>(item->itemID));
            RECT rect = item->rcItem;
            rect.left += 8;
            rect.right -= 4;
            SetBkMode(item->hDC, TRANSPARENT);
            SetTextColor(item->hDC, RGB(255, 255, 255));
            HGDIOBJ oldFont = SelectObject(item->hDC, font_);
            DrawTextW(item->hDC, text.c_str(), -1, &rect,
                      DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
            SelectObject(item->hDC, oldFont);
        }
        if (item->itemState & ODS_FOCUS) {
            RECT focus = item->rcItem;
            InflateRect(&focus, -2, -2);
            DrawFocusRect(item->hDC, &focus);
        }
        return;
    }
    DrawButton(item);
}

void RecoverXApp::DrawButton(const DRAWITEMSTRUCT* item) {
    HDC dc = item->hDC;
    RECT rect = item->rcItem;
    int kind = static_cast<int>(reinterpret_cast<INT_PTR>(GetPropW(item->hwndItem, L"rxkind")));
    bool disabled = (item->itemState & ODS_DISABLED) != 0;

    if (kind == 1 || kind == 2) {
        FillRect(dc, &rect, brushBlack_);
        bool checked = SendMessageW(item->hwndItem, BM_GETCHECK, 0, 0) == BST_CHECKED;
        const int box = 16;
        int left = rect.left + 2;
        int top = rect.top + (rect.bottom - rect.top - box) / 2;
        RECT boxRect{left, top, left + box, top + box};
        if (kind == 1) {
            FillRect(dc, &boxRect, brushBlack_);
            FrameRect(dc, &boxRect, brushWhite_);
            if (checked) {
                RECT inner{left + 4, top + 4, left + box - 4, top + box - 4};
                FillRect(dc, &inner, brushWhite_);
            }
        } else {
            HPEN pen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
            HGDIOBJ oldPen = SelectObject(dc, pen);
            HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(NULL_BRUSH));
            Ellipse(dc, left, top, left + box, top + box);
            if (checked) {
                SelectObject(dc, brushWhite_);
                Ellipse(dc, left + 5, top + 5, left + box - 5, top + box - 5);
            }
            SelectObject(dc, oldBrush);
            SelectObject(dc, oldPen);
            DeleteObject(pen);
        }
        RECT textRect = rect;
        textRect.left += 24;
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, disabled ? RGB(120, 120, 120) : RGB(255, 255, 255));
        HGDIOBJ oldFont = SelectObject(dc, font_);
        std::wstring label = GetText(item->hwndItem);
        DrawTextW(dc, label.c_str(), -1, &textRect,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        SelectObject(dc, oldFont);
        return;
    }

    bool navigation = item->CtlID >= ID_TAB_FIRST && item->CtlID <= ID_TAB_LAST;
    bool selected = navigation && (item->CtlID - ID_TAB_FIRST) == static_cast<UINT>(currentPage_);
    bool pressed = (item->itemState & ODS_SELECTED) != 0;

    HBRUSH fill = brushBlack_;
    if (navigation) {
        if (selected) fill = brushGray_;
    } else if (pressed) {
        fill = brushDark_;
    }
    FillRect(dc, &rect, fill);

    HBRUSH border = brushWhite_;
    if (disabled || (navigation && !selected)) border = brushGray_;
    FrameRect(dc, &rect, border);

    if (selected) {
        RECT accent{rect.left, rect.top, rect.left + 3, rect.bottom};
        FillRect(dc, &accent, brushWhite_);
    }

    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, disabled ? RGB(120, 120, 120) : RGB(255, 255, 255));
    HGDIOBJ oldFont = SelectObject(dc, navigation ? fontBold_ : font_);
    std::wstring label = GetText(item->hwndItem);
    RECT textRect = rect;
    UINT format = DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS;
    if (navigation) {
        textRect.left += 18;
        format |= DT_LEFT;
    } else {
        format |= DT_CENTER;
    }
    DrawTextW(dc, label.c_str(), -1, &textRect, format);
    SelectObject(dc, oldFont);
}

LRESULT RecoverXApp::OnCtlColor(UINT msg, WPARAM wParam) {
    HDC dc = reinterpret_cast<HDC>(wParam);
    if (msg == WM_CTLCOLORSTATIC || msg == WM_CTLCOLOREDIT || msg == WM_CTLCOLORLISTBOX) {
        SetTextColor(dc, RGB(255, 255, 255));
        SetBkColor(dc, RGB(0, 0, 0));
        SetBkMode(dc, OPAQUE);
        return reinterpret_cast<LRESULT>(brushBlack_);
    }
    SetTextColor(dc, RGB(255, 255, 255));
    SetBkColor(dc, RGB(0, 0, 0));
    return reinterpret_cast<LRESULT>(brushBlack_);
}

LRESULT CALLBACK RecoverXApp::MainProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCCREATE) {
        auto create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
        return TRUE;
    }
    auto app = reinterpret_cast<RecoverXApp*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (app) return app->HandleMain(hwnd, msg, wParam, lParam);
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK RecoverXApp::PageProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCCREATE) {
        auto create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
        return TRUE;
    }
    auto app = reinterpret_cast<RecoverXApp*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (app) return app->HandlePage(hwnd, msg, wParam, lParam);
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK RecoverXApp::BootProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCCREATE) {
        auto create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
        return TRUE;
    }
    auto app = reinterpret_cast<RecoverXApp*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (app) return app->HandleBoot(hwnd, msg, wParam, lParam);
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT RecoverXApp::HandleMain(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            hwnd_ = hwnd;
            return OnCreate();
        case WM_SIZE:
            LayoutPages();
            return 0;
        case WM_COMMAND:
            OnCommand(LOWORD(wParam));
            return 0;
        case WM_DRAWITEM:
            OnDrawItem(reinterpret_cast<const DRAWITEMSTRUCT*>(lParam));
            return TRUE;
        case WM_MEASUREITEM:
            reinterpret_cast<MEASUREITEMSTRUCT*>(lParam)->itemHeight = 24;
            return TRUE;
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX:
            return OnCtlColor(msg, wParam);
        case WM_TIMER:
            OnTimer(wParam);
            return 0;
        case kTrayMessage:
            OnTray(static_cast<UINT>(lParam));
            return 0;
        case kLanguageMessage:
            SwitchLanguage(wParam == ID_LANG_RU ? "ru" : "en");
            return 0;
        case WM_CLOSE:
            OnClose();
            return 0;
        case WM_DESTROY:
            OnDestroy();
            return 0;
        case WM_ERASEBKGND: {
            if (!brushBlack_) break;
            RECT rect{};
            GetClientRect(hwnd, &rect);
            FillRect(reinterpret_cast<HDC>(wParam), &rect, brushBlack_);
            return 1;
        }
        case WM_PAINT: {
            PAINTSTRUCT paint{};
            HDC dc = BeginPaint(hwnd, &paint);
            RECT divider{kNavWidth - 1, 0, kNavWidth, 4096};
            if (brushGray_) FillRect(dc, &divider, brushGray_);
            RECT line{16, 78, 164, 79};
            if (brushGray_) FillRect(dc, &line, brushGray_);
            EndPaint(hwnd, &paint);
            return 0;
        }
        case WM_GETMINMAXINFO: {
            auto info = reinterpret_cast<MINMAXINFO*>(lParam);
            info->ptMinTrackSize.x = kWindowWidth + 20;
            info->ptMinTrackSize.y = kWindowHeight + 40;
            return 0;
        }
        default:
            break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT RecoverXApp::HandlePage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_COMMAND:
            OnCommand(LOWORD(wParam));
            return 0;
        case WM_DRAWITEM:
            OnDrawItem(reinterpret_cast<const DRAWITEMSTRUCT*>(lParam));
            return TRUE;
        case WM_MEASUREITEM:
            reinterpret_cast<MEASUREITEMSTRUCT*>(lParam)->itemHeight = 24;
            return TRUE;
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX:
            return OnCtlColor(msg, wParam);
        case WM_ERASEBKGND: {
            if (!brushBlack_) break;
            RECT rect{};
            GetClientRect(hwnd, &rect);
            FillRect(reinterpret_cast<HDC>(wParam), &rect, brushBlack_);
            return 1;
        }
        default:
            break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT RecoverXApp::HandleBoot(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT: {
            PAINTSTRUCT paint{};
            HDC dc = BeginPaint(hwnd, &paint);
            RECT rect{};
            GetClientRect(hwnd, &rect);
            HBRUSH background = CreateSolidBrush(RGB(0, 0, 170));
            FillRect(dc, &rect, background);
            DeleteObject(background);

            HFONT font = CreateFontW(-22, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                     OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                     DEFAULT_PITCH | FF_DONTCARE, L"Consolas");
            HGDIOBJ oldFont = SelectObject(dc, font);
            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, RGB(255, 255, 255));

            RECT titleRect{40, 24, rect.right - 40, 84};
            FrameRect(dc, &titleRect, brushWhite_);
            DrawTextW(dc, L"BOOT MENU", -1, &titleRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            static const wchar_t* items[5] = {
                L"Continue", L"Boot from HDD", L"Boot from USB", L"Boot from CD/DVD", L"Enter BIOS Setup"
            };
            int y = 120;
            for (int i = 0; i < 5; ++i) {
                RECT lineRect{60, y, rect.right - 60, y + 32};
                std::wstring line = (i == bootChoice_ ? L"  \u25B6  " : L"     ") + std::wstring(items[i]);
                DrawTextW(dc, line.c_str(), -1, &lineRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
                y += 36;
            }

            RECT footer{60, rect.bottom - 48, rect.right - 60, rect.bottom - 20};
            SetTextColor(dc, RGB(210, 210, 210));
            DrawTextW(dc, L"Up / Down    Enter    Esc", -1, &footer,
                      DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            SelectObject(dc, oldFont);
            DeleteObject(font);
            EndPaint(hwnd, &paint);
            return 0;
        }
        case WM_KEYDOWN: {
            if (wParam == VK_UP) {
                bootChoice_ = (bootChoice_ + 4) % 5;
                InvalidateRect(hwnd, nullptr, TRUE);
            } else if (wParam == VK_DOWN) {
                bootChoice_ = (bootChoice_ + 1) % 5;
                InvalidateRect(hwnd, nullptr, TRUE);
            } else if (wParam == VK_RETURN) {
                if (bootChoice_ == 0) {
                    DestroyWindow(hwnd);
                } else if (bootChoice_ == 4) {
                    DestroyWindow(hwnd);
                    RebootToBios();
                }
            } else if (wParam == VK_ESCAPE) {
                DestroyWindow(hwnd);
            }
            return 0;
        }
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY: {
            HWND owner = GetParent(hwnd);
            if (owner) {
                EnableWindow(owner, TRUE);
                SetForegroundWindow(owner);
            }
            return 0;
        }
        default:
            break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// Timer and monitor
void RecoverXApp::StartTimer() {
    if (timerRunning_) return;
    int minutes = 0;
    int seconds = 0;
    if (!ParseInt(GetText(minEdit_), minutes) || !ParseInt(GetText(secEdit_), seconds) ||
        minutes < 0 || seconds < 0) {
        Error(S(L"invalid_time"), S(L"error"));
        return;
    }
    int total = minutes * 60 + seconds;
    if (total <= 0) {
        Error(S(L"invalid_time"), S(L"error"));
        return;
    }

    minText_ = GetText(minEdit_);
    secText_ = GetText(secEdit_);
    timerAction_ = IsChecked(timerRestartRadio_) ? L"restart" : L"shutdown";
    forceClose_ = IsChecked(forceCheck_);

    std::wstring command = std::wstring(L"shutdown ") +
                           (timerAction_ == L"shutdown" ? L"/s" : L"/r") +
                           (forceClose_ ? L" /f" : L"") + L" /t " + std::to_wstring(total);
    if (!RunHiddenWait(command)) {
        Error(LastErrorText(), S(L"error"));
        return;
    }

    timerSeconds_ = total;
    timerRunning_ = true;
    EnableWindow(startButton_, FALSE);
    EnableWindow(cancelButton_, TRUE);
    UpdateTimerDisplay();
    SetTimer(hwnd_, kCountdownTimer, 1000, nullptr);
}

void RecoverXApp::CancelTimer() {
    RunHiddenWait(L"shutdown /a");
    timerRunning_ = false;
    timerSeconds_ = 0;
    KillTimer(hwnd_, kCountdownTimer);
    EnableWindow(startButton_, TRUE);
    EnableWindow(cancelButton_, FALSE);
    SetText(statusLabel_, S(L"status_idle"));
}

std::wstring RecoverXApp::TimerText() const {
    const std::wstring& prefix = S(timerAction_ == L"shutdown" ? L"until_shutdown" : L"until_restart");
    return prefix + L" " + FormatTime(std::max(0, timerSeconds_));
}

void RecoverXApp::UpdateTimerDisplay() {
    if (!timerRunning_) return;
    if (timerSeconds_ == 10) MessageBeep(MB_ICONEXCLAMATION);
    if (timerSeconds_ > 0) {
        SetText(statusLabel_, TimerText());
        --timerSeconds_;
    } else {
        SetText(statusLabel_, S(L"executing"));
        timerRunning_ = false;
        KillTimer(hwnd_, kCountdownTimer);
        EnableWindow(startButton_, TRUE);
        EnableWindow(cancelButton_, FALSE);
    }
}

void RecoverXApp::OnTimer(UINT_PTR id) {
    if (id == kCountdownTimer) UpdateTimerDisplay();
    else if (id == kMonitorTimer) UpdateMonitor();
}

void RecoverXApp::UpdateMonitor() {
    if (!cpuLabel_ || !ramLabel_) return;
    FILETIME idleTime{};
    FILETIME kernelTime{};
    FILETIME userTime{};
    if (!GetSystemTimes(&idleTime, &kernelTime, &userTime)) return;

    ULONGLONG idle = FileTimeToU64(idleTime);
    ULONGLONG kernel = FileTimeToU64(kernelTime);
    ULONGLONG user = FileTimeToU64(userTime);
    double cpu = 0.0;
    if (cpuReady_) {
        ULONGLONG idleDelta = idle - prevIdle_;
        ULONGLONG totalDelta = (kernel - prevKernel_) + (user - prevUser_);
        if (totalDelta > 0) {
            cpu = (1.0 - static_cast<double>(idleDelta) / static_cast<double>(totalDelta)) * 100.0;
        }
    }
    prevIdle_ = idle;
    prevKernel_ = kernel;
    prevUser_ = user;
    cpuReady_ = true;
    cpu = std::clamp(cpu, 0.0, 100.0);

    MEMORYSTATUSEX memory{};
    memory.dwLength = sizeof(memory);
    GlobalMemoryStatusEx(&memory);
    SetText(cpuLabel_, S(L"cpu_label") + L" " + FormatPercent(cpu));
    SetText(ramLabel_, S(L"ram_label") + L" " + FormatPercent(static_cast<double>(memory.dwMemoryLoad)));
}

// Admin
bool RecoverXApp::EnsureAdmin() {
    if (IsUserAnAdmin()) return true;
    if (Confirm(S(L"admin_needed"), L"Admin")) RequestAdmin();
    return false;
}

void RecoverXApp::RequestAdmin() {
    wchar_t path[MAX_PATH]{};
    if (!GetModuleFileNameW(nullptr, path, MAX_PATH)) {
        Error(S(L"admin_failed"), S(L"error"));
        return;
    }
    if (g_instanceMutex) {
        CloseHandle(g_instanceMutex);
        g_instanceMutex = nullptr;
    }
    HINSTANCE result = ShellExecuteW(hwnd_, L"runas", path, nullptr, nullptr, SW_SHOWNORMAL);
    if (reinterpret_cast<INT_PTR>(result) <= 32) {
        g_instanceMutex = CreateMutexW(nullptr, FALSE, kMutexName);
        Error(S(L"admin_failed"), S(L"error"));
        return;
    }
    forceQuit_ = true;
    PostMessageW(hwnd_, WM_CLOSE, 0, 0);
}

// Tray
void RecoverXApp::AddTray() {
    if (trayAdded_) return;
    NOTIFYICONDATAW data{};
    data.cbSize = sizeof(data);
    data.hWnd = hwnd_;
    data.uID = 1;
    data.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    data.uCallbackMessage = kTrayMessage;
    data.hIcon = icon_ ? icon_ : LoadIconW(nullptr, IDI_APPLICATION);
    lstrcpynW(data.szTip, L"RecoverX", 128);
    trayAdded_ = Shell_NotifyIconW(NIM_ADD, &data) != FALSE;
}

void RecoverXApp::RemoveTray() {
    if (!trayAdded_) return;
    NOTIFYICONDATAW data{};
    data.cbSize = sizeof(data);
    data.hWnd = hwnd_;
    data.uID = 1;
    Shell_NotifyIconW(NIM_DELETE, &data);
    trayAdded_ = false;
}

void RecoverXApp::ShowMainWindow() {
    if (!hwnd_) return;
    if (IsIconic(hwnd_)) ShowWindow(hwnd_, SW_RESTORE);
    ShowWindow(hwnd_, SW_SHOW);
    SetForegroundWindow(hwnd_);
}

void RecoverXApp::Quit() {
    if (timerRunning_) CancelTimer();
    forceQuit_ = true;
    RemoveTray();
    if (hwnd_) DestroyWindow(hwnd_);
}

void RecoverXApp::OnTray(UINT event) {
    if (event == WM_LBUTTONUP || event == WM_LBUTTONDBLCLK) {
        ShowMainWindow();
    } else if (event == WM_RBUTTONUP) {
        POINT point{};
        GetCursorPos(&point);
        HMENU menu = CreatePopupMenu();
        AppendMenuW(menu, MF_STRING, ID_TRAY_OPEN, S(L"open").c_str());
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(menu, MF_STRING, ID_TRAY_EXIT, S(L"exit").c_str());
        SetForegroundWindow(hwnd_);
        int command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY,
                                     point.x, point.y, 0, hwnd_, nullptr);
        DestroyMenu(menu);
        if (command == ID_TRAY_OPEN) ShowMainWindow();
        else if (command == ID_TRAY_EXIT) Quit();
        PostMessageW(hwnd_, WM_NULL, 0, 0);
    }
}

// Autostart
bool RecoverXApp::IsAutostartEnabled() const {
    std::wstring value;
    return RegGetString(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                        L"RecoverX", value) && !value.empty();
}

bool RecoverXApp::SetAutostart(bool enabled) {
    if (enabled) {
        return RegSetString(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                            L"RecoverX", Quote(GetExePath()));
    }
    RegDeleteValueSafe(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", L"RecoverX");
    return true;
}

// Scheduler
void RecoverXApp::AddSchedule() {
    std::wstring time = GetText(schedTimeEdit_);
    int hour = -1;
    int minute = -1;
    if (swscanf_s(time.c_str(), L"%d:%d", &hour, &minute) != 2 ||
        hour < 0 || hour > 23 || minute < 0 || minute > 59) {
        Error(S(L"invalid_time"), S(L"scheduler"));
        return;
    }

    std::vector<std::wstring> days;
    for (int i = 0; i < 7; ++i) {
        if (IsChecked(dayChecks_[i])) days.emplace_back(kDayNamesSched[i]);
    }
    if (days.empty()) {
        Warn(S(L"select_day"), S(L"scheduler"));
        return;
    }

    schedAction_ = IsChecked(schedRestartRadio_) ? L"restart" : L"shutdown";
    schedTime_ = time;
    std::wstring scriptPath = TempBatPath();
    std::ofstream script(std::filesystem::path(scriptPath), std::ios::binary | std::ios::trunc);
    if (!script) {
        Error(S(L"task_failed"), S(L"scheduler"));
        return;
    }
    script << (schedAction_ == L"shutdown" ? "shutdown /s /t 0" : "shutdown /r /t 0") << "\r\n";
    script.close();

    wchar_t timeText[16]{};
    swprintf_s(timeText, L"%02d:%02d", hour, minute);
    std::wstring taskName = schedAction_ == L"shutdown" ? L"RecoverX_shutdown" : L"RecoverX_restart";
    std::wstring command = L"schtasks /create /tn " + Quote(taskName) +
                           L" /tr " + Quote(scriptPath) +
                           L" /sc weekly /d " + Join(days, L",") +
                           L" /st " + timeText + L" /f";
    DWORD code = 0;
    if (RunHiddenWait(command, &code) && code == 0) {
        Info(S(L"task_scheduled"), S(L"scheduler"));
    } else {
        Error(S(L"task_failed") + L" (" + std::to_wstring(code) + L")", S(L"scheduler"));
    }
}

// Restore points
void RecoverXApp::RefreshRestorePoints() {
    if (!restoreList_) return;
    SendMessageW(restoreList_, LB_RESETCONTENT, 0, 0);
    std::string output;
    DWORD code = 0;
    if (RunPowerShellCapture(L"Get-ComputerRestorePoint | Select-Object -ExpandProperty Description | Out-String -Width 4096",
                             output, &code) && code == 0) {
        std::wstring text = Utf8ToWide(output);
        for (const auto& line : SplitLines(text)) {
            SendMessageW(restoreList_, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(line.c_str()));
        }
    }
    if (SendMessageW(restoreList_, LB_GETCOUNT, 0, 0) == 0) {
        std::wstring message = S(L"error_rp_list");
        SendMessageW(restoreList_, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(message.c_str()));
    }
}

void RecoverXApp::CreateRestorePoint() {
    if (!EnsureAdmin()) return;
    DWORD code = 0;
    bool ok = RunPowerShellWait(L"Checkpoint-Computer -Description 'RecoverX' -RestorePointType MODIFY_SETTINGS", &code) && code == 0;
    if (ok) {
        Info(S(L"done"), S(L"restore"));
        RefreshRestorePoints();
    } else {
        Error(std::wstring(S(L"error")) + L" (" + std::to_wstring(code) + L")", S(L"restore"));
    }
}

void RecoverXApp::DeleteRestorePoint() {
    if (!restoreList_) return;
    int selection = static_cast<int>(SendMessageW(restoreList_, LB_GETCURSEL, 0, 0));
    if (selection < 0) {
        Warn(S(L"select_rp"), S(L"restore"));
        return;
    }
    if (!Confirm(S(L"confirm_delete_rp"), S(L"restore"))) return;
    std::wstring description = ListText(restoreList_, selection);
    if (description.empty()) return;
    std::wstring script = L"Get-ComputerRestorePoint | Where-Object {$_.Description -eq '" +
                          EscapePowerShell(description) + L"'} | Remove-ComputerRestorePoint";
    DWORD code = 0;
    if (RunPowerShellWait(script, &code) && code == 0) {
        RefreshRestorePoints();
        Info(S(L"done"), S(L"restore"));
    } else {
        Error(std::wstring(S(L"error")) + L" (" + std::to_wstring(code) + L")", S(L"restore"));
    }
}

void RecoverXApp::OpenSystemRestore() {
    OpenApp(L"rstrui.exe");
}

// Updates
void RecoverXApp::CheckAppUpdates() {
    std::string body;
    DWORD status = 0;
    if (HttpGet(kUpdatePath, body, status) && status == 200) {
        std::wstring latest = Utf8ToWide(ExtractJsonString(body, "tag_name"));
        std::wstring link = Utf8ToWide(ExtractJsonString(body, "html_url"));
        if (latest.empty()) {
            Error(S(L"update_error"), L"RecoverX");
        } else if (latest != kVersion) {
            std::wstring message = S(L"update_available");
            ReplaceAll(message, L"{version}", latest);
            ReplaceAll(message, L"{url}", link);
            Info(message, L"RecoverX");
        } else {
            Info(S(L"no_updates"), L"RecoverX");
        }
    } else {
        std::wstring message = S(L"update_error");
        if (status) message += L"\nHTTP " + std::to_wstring(status);
        Error(message, L"RecoverX");
    }
}

// UWP
void RecoverXApp::RefreshUwpList() {
    if (!uwpList_) return;
    SendMessageW(uwpList_, LB_RESETCONTENT, 0, 0);
    uwpPackages_.clear();
    if (!EnsureAdmin()) {
        if (!uwpList_) return;
        std::wstring message = S(L"uwp_no_apps");
        SendMessageW(uwpList_, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(message.c_str()));
        return;
    }

    std::string output;
    DWORD code = 0;
    if (RunPowerShellCapture(L"Get-AppxPackage | Select-Object -ExpandProperty Name | Out-String -Width 4096",
                             output, &code) && code == 0) {
        uwpPackages_ = SplitLines(Utf8ToWide(output));
    }
    if (uwpPackages_.empty()) {
        std::wstring message = S(L"uwp_no_apps");
        SendMessageW(uwpList_, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(message.c_str()));
        return;
    }
    for (const auto& name : uwpPackages_) {
        SendMessageW(uwpList_, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(name.c_str()));
    }
}

void RecoverXApp::RemoveSelectedUwp() {
    if (!uwpList_ || !EnsureAdmin()) return;
    int count = static_cast<int>(SendMessageW(uwpList_, LB_GETSELCOUNT, 0, 0));
    if (count <= 0) {
        Warn(S(L"select_apps"), S(L"uwp"));
        return;
    }
    if (!Confirm(S(L"confirm_uwp_remove"), S(L"uwp"))) return;

    std::vector<int> items(static_cast<size_t>(count), 0);
    SendMessageW(uwpList_, LB_GETSELITEMS, count, reinterpret_cast<LPARAM>(items.data()));
    uwpAll_ = IsChecked(uwpAllCheck_);
    for (int index : items) {
        std::wstring name = ListText(uwpList_, index);
        if (name.empty() || name == S(L"uwp_no_apps")) continue;
        std::wstring script = L"Get-AppxPackage -Name '" + EscapePowerShell(name) + L"' | Remove-AppxPackage";
        if (uwpAll_) script += L" -AllUsers";
        DWORD code = 0;
        RunPowerShellWait(script, &code);
    }
    RefreshUwpList();
    Info(S(L"done"), S(L"uwp"));
}

// Boot methods
void RecoverXApp::ShowBootMenu() {
    HWND existing = FindWindowW(kBootClass, nullptr);
    if (existing) {
        SetForegroundWindow(existing);
        return;
    }
    RECT mainRect{};
    GetWindowRect(hwnd_, &mainRect);
    int width = 640;
    int height = 440;
    int x = mainRect.left + (mainRect.right - mainRect.left - width) / 2;
    int y = mainRect.top + (mainRect.bottom - mainRect.top - height) / 2;
    EnableWindow(hwnd_, FALSE);
    bootChoice_ = 0;
    HWND window = CreateWindowExW(WS_EX_DLGMODALFRAME, kBootClass, S(L"boot_menu").c_str(),
                                  WS_POPUP | WS_CAPTION | WS_SYSMENU, x, y, width, height,
                                  hwnd_, nullptr, instance_, this);
    if (!window) {
        EnableWindow(hwnd_, TRUE);
        return;
    }
    ShowWindow(window, SW_SHOW);
    SetFocus(window);
}

void RecoverXApp::RebootToBios() {
    if (!EnsureAdmin()) return;
    if (!Confirm(S(L"confirm_bios"), L"BIOS")) return;
    if (!RunHidden(L"shutdown /r /fw /t 0")) {
        Error(LastErrorText(), S(L"error"));
    }
}

void RecoverXApp::SetSafeMode(const std::wstring& mode) {
    if (!EnsureAdmin()) return;
    if (!Confirm(S(L"confirm_safe"), L"Safe Mode")) return;
    std::wstring command;
    if (mode == L"normal") {
        command = L"bcdedit /deletevalue {default} safeboot";
    } else {
        command = L"bcdedit /set {default} safeboot " + mode;
    }
    DWORD code = 0;
    if (!RunHiddenWait(command, &code) || code != 0) {
        Error(std::wstring(S(L"error")) + L" (" + std::to_wstring(code) + L")", L"Safe Mode");
        return;
    }
    RunHidden(L"shutdown /r /t 0");
}

// Shell helpers
bool RecoverXApp::OpenApp(const std::wstring& file, const std::wstring& params) {
    HINSTANCE result = ShellExecuteW(hwnd_, L"open", file.c_str(),
                                     params.empty() ? nullptr : params.c_str(),
                                     nullptr, SW_SHOWNORMAL);
    return reinterpret_cast<INT_PTR>(result) > 32;
}

bool RecoverXApp::OpenUrl(const std::wstring& url) {
    HINSTANCE result = ShellExecuteW(hwnd_, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    return reinterpret_cast<INT_PTR>(result) > 32;
}

void RecoverXApp::MinimizeAll() {
    const BYTE keys[2] = {0x5B, 0x44};
    for (BYTE key : keys) keybd_event(key, 0, 0, 0);
    for (int i = 1; i >= 0; --i) keybd_event(keys[i], 0, KEYEVENTF_KEYUP, 0);
}

void RecoverXApp::RestartGpu() {
    const BYTE keys[4] = {0xA2, 0xA0, 0x5B, 0x42};
    for (BYTE key : keys) keybd_event(key, 0, 0, 0);
    for (int i = 3; i >= 0; --i) keybd_event(keys[i], 0, KEYEVENTF_KEYUP, 0);
}

void RecoverXApp::OpenPowerShell() {
    RunNewConsole(L"powershell.exe");
}

void RecoverXApp::FlushDns() {
    if (!Confirm(S(L"dns_question"), S(L"flush_dns"))) return;
    RunHiddenWait(L"ipconfig /flushdns");
    Info(S(L"dns_done"), L"DNS");
}

void RecoverXApp::BackupRegistry() {
    wchar_t file[MAX_PATH]{};
    lstrcpynW(file, L"RecoverX_backup.reg", MAX_PATH);
    static const wchar_t filter[] =
        L"Registry files (*.reg)\0*.reg\0All files (*.*)\0*.*\0\0";
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = hwnd_;
    dialog.lpstrFilter = filter;
    dialog.lpstrFile = file;
    dialog.nMaxFile = MAX_PATH;
    dialog.lpstrDefExt = L"reg";
    dialog.lpstrTitle = S(L"backup_registry").c_str();
    dialog.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    if (!GetSaveFileNameW(&dialog)) return;

    std::wstring path = file;
    DWORD code = 0;
    if (RunHiddenWait(L"regedit /e " + Quote(path), &code) && code == 0) {
        std::wstring message = S(L"registry_exported");
        ReplaceAll(message, L"{path}", path);
        Info(message, S(L"backup_registry"));
    } else {
        Error(S(L"registry_failed"), S(L"error"));
    }
}

void RecoverXApp::CleanTemp() {
    if (!Confirm(S(L"confirm_clean_temp"), S(L"maintenance"))) return;
    RunHiddenWait(L"cmd.exe /c del /f /s /q \"%TEMP%\\*\" >nul 2>&1");
    RunHiddenWait(L"cmd.exe /c del /f /s /q \"C:\\Windows\\Temp\\*\" >nul 2>&1");
    RunHiddenWait(L"cmd.exe /c del /f /s /q \"C:\\Windows\\Prefetch\\*\" >nul 2>&1");
    Info(S(L"done"), S(L"maintenance"));
}

void RecoverXApp::CleanUpdates() {
    if (!EnsureAdmin()) return;
    if (!Confirm(S(L"confirm_clean_updates"), S(L"windows_update"))) return;
    RunHiddenWait(L"cmd.exe /c del /f /s /q \"C:\\Windows\\SoftwareDistribution\\Download\\*\" >nul 2>&1");
    RunNewConsole(L"cmd.exe /k Dism.exe /online /Cleanup-Image /StartComponentCleanup /ResetBase");
}

void RecoverXApp::DisableAutoUpdates() {
    if (!EnsureAdmin()) return;
    if (!Confirm(S(L"confirm_disable_updates"), S(L"windows_update"))) return;
    if (RegSetDword(HKEY_LOCAL_MACHINE,
                    L"SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate\\AU",
                    L"NoAutoUpdate", 1)) {
        Info(S(L"autoupdates_disabled"), S(L"windows_update"));
    } else {
        Error(S(L"error") + L"\n" + LastErrorText(), S(L"error"));
    }
}

void RecoverXApp::SetDefender(bool enabled) {
    if (!EnsureAdmin()) return;
    if (!Confirm(enabled ? S(L"confirm_enable_defender") : S(L"confirm_disable_defender"),
                 S(L"defender_section"))) return;
    const wchar_t* key = L"SOFTWARE\\Policies\\Microsoft\\Windows Defender";
    bool ok = false;
    if (enabled) {
        ok = RegDeleteValueSafe(HKEY_LOCAL_MACHINE, key, L"DisableAntiSpyware");
        if (!ok) ok = RegSetDword(HKEY_LOCAL_MACHINE, key, L"DisableAntiSpyware", 0);
    } else {
        ok = RegSetDword(HKEY_LOCAL_MACHINE, key, L"DisableAntiSpyware", 1);
    }
    if (ok) {
        Info(enabled ? S(L"defender_enabled") : S(L"defender_disabled"), S(L"defender_section"));
    } else {
        Error(S(L"error") + L"\n" + LastErrorText(), S(L"error"));
    }
}

// Entry point
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    g_instanceMutex = CreateMutexW(nullptr, TRUE, kMutexName);
    if (g_instanceMutex && GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND existing = FindWindowW(kMainClass, nullptr);
        if (existing) {
            if (IsIconic(existing)) ShowWindow(existing, SW_RESTORE);
            ShowWindow(existing, SW_SHOW);
            SetForegroundWindow(existing);
        }
        CloseHandle(g_instanceMutex);
        g_instanceMutex = nullptr;
        return 0;
    }

    INITCOMMONCONTROLSEX controls{};
    controls.dwSize = sizeof(controls);
    controls.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&controls);

    RecoverXApp app(instance);
    if (!app.Create()) {
        if (g_instanceMutex) CloseHandle(g_instanceMutex);
        g_instanceMutex = nullptr;
        return 1;
    }

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    if (g_instanceMutex) CloseHandle(g_instanceMutex);
    g_instanceMutex = nullptr;
    return static_cast<int>(message.wParam);
}
