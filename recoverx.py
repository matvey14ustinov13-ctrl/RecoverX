import ctypes, os, subprocess, sys, winsound, json, time, urllib.request, tempfile
import tkinter as tk
from tkinter import ttk, messagebox, filedialog
import winreg, threading
import pystray
from PIL import Image, ImageDraw, ImageFont

# ---------- Проверка единственного запуска ----------
def ensure_single_instance():
    target = "RecoverX - System Utility"
    hwnd = ctypes.windll.user32.FindWindowW(None, None)
    while hwnd:
        length = ctypes.windll.user32.GetWindowTextLengthW(hwnd)
        if length > 0:
            buf = ctypes.create_unicode_buffer(length + 1)
            ctypes.windll.user32.GetWindowTextW(hwnd, buf, length + 1)
            if target in buf.value:
                ctypes.windll.user32.ShowWindow(hwnd, 9)
                ctypes.windll.user32.SetForegroundWindow(hwnd)
                return True
        hwnd = ctypes.windll.user32.FindWindowExW(None, hwnd, None, None)
    return False

# ---------- Генерация иконки ----------
def create_icon():
    icon_path = os.path.join(tempfile.gettempdir(), "recoverx_icon.ico")
    if not os.path.exists(icon_path):
        img = Image.new("RGB", (64, 64), color="#000000")
        draw = ImageDraw.Draw(img)
        try:
            font = ImageFont.truetype("segoeui.ttf", 40)
        except:
            font = ImageFont.load_default()
        draw.text((14, 10), "R", fill="#FFFFFF", font=font)   # буква R
        img.save(icon_path, format="ICO", sizes=[(64, 64)])
    return icon_path

# ---------- i18n ----------
STRINGS = {
    "en": {
        "title": "RecoverX - System Utility",
        "power": "Power",
        "boot": "Boot",
        "system": "System",
        "maintenance": "Maintenance",
        "scheduler": "Scheduler",
        "monitor": "Monitor",
        "restore": "Restore Points",
        "windows_update": "Windows Update",
        "uwp": "UWP Apps",
        "drivers": "Drivers",
        "settings": "Settings",
        "timer_shutdown": "Shutdown",
        "timer_restart": "Restart",
        "force_check": "Force close programs",
        "start_timer": "Start timer",
        "cancel_timer": "Cancel",
        "status_idle": "Timer not running",
        "sleep": "Sleep",
        "shutdown_direct": "Shutdown",
        "force_shutdown": "Force Shutdown",
        "boot_bios": "BIOS (UEFI)",
        "boot_safe_min": "Safe Mode (Minimal)",
        "boot_safe_net": "Safe Mode (Network)",
        "boot_reset": "Reset Safe Mode",
        "boot_menu": "Boot Menu",
        "sys_info": "System Information",
        "dev_mgr": "Device Manager",
        "task_mgr": "Task Manager",
        "disk_mgr": "Disk Management",
        "local_users": "Local Users",
        "regedit": "Registry Editor",
        "control_panel": "Control Panel",
        "osk": "On-Screen Keyboard",
        "magnifier": "Magnifier",
        "minimize_all": "Minimize All",
        "gpu_reset": "Reset GPU Driver",
        "apps_folder": "AppsFolder",
        "powershell": "PowerShell",
        "change_time": "Change System Time",
        "clean_temp": "Clean Temp Files",
        "sfc": "SFC Scan",
        "dism": "DISM Restore",
        "clean_updates": "Clean Update Leftovers",
        "mrt": "Run MRT",
        "flush_dns": "Flush DNS Cache",
        "services_mgr": "Services Manager",
        "backup_registry": "Backup Registry",
        "sched_info": "Scheduler runs in background. It may consume some resources.",
        "sched_action": "Action:",
        "sched_time": "Time (HH:MM):",
        "sched_days": "Days:",
        "sched_add": "Add Schedule",
        "cpu_label": "CPU Usage:",
        "ram_label": "RAM Usage:",
        "refresh_rp": "Refresh List",
        "create_rp": "Create Restore Point",
        "delete_rp": "Delete Selected",
        "open_restore": "System Restore...",
        "confirm_delete_rp": "Delete the selected restore point?",
        "error_no_admin": "Admin rights required for this action.",
        "error_rp_list": "Could not list restore points. Check admin rights or system protection.",
        "check_updates": "Check for Updates",
        "disable_updates": "Disable Automatic Updates",
        "disable_defender": "Disable Defender",
        "enable_defender": "Enable Defender",
        "confirm_disable_updates": "This will disable automatic updates. Continue?",
        "confirm_disable_defender": "This will disable Windows Defender (requires restart). Continue?",
        "confirm_enable_defender": "This will enable Windows Defender (requires restart). Continue?",
        "defender_section": "Windows Defender",
        "refresh_uwp": "Refresh List",
        "remove_uwp": "Remove Selected",
        "uwp_for_all": "For all users",
        "confirm_uwp_remove": "Remove selected apps permanently?",
        "uwp_no_apps": "No UWP apps found or admin rights required.",
        "drivers_nvidia": "NVIDIA Drivers",
        "drivers_amd": "AMD Drivers",
        "drivers_intel": "Intel Drivers",
        "tray_on": "Minimize to tray on close",
        "autostart": "Start RecoverX with Windows",
        "startup_settings": "Windows Startup Settings",
        "admin_btn": "Run as Administrator",
        "check_app_updates": "Check for RecoverX updates",
        "no_updates": "You have the latest version.",
        "update_available": "New version available: {version}\nDownload: {url}",
        "update_error": "Could not check for updates.",
        "confirm_shutdown": "Shut down now?",
        "confirm_force": "All unsaved data will be lost. Continue?",
        "confirm_bios": "Restart into UEFI?",
        "confirm_safe": "Set Safe Mode and restart?",
        "confirm_clean_temp": "Clean Windows temp files?",
        "confirm_clean_updates": "Remove update leftovers? Restart may be required.",
        "admin_needed": "Admin rights required. Restart as admin?",
        "done": "Done",
        "error": "Error",
        "not_available": "This snap-in is not available in your Windows edition.",
    },
    "ru": {
        "title": "RecoverX - Системная утилита",
        "power": "Питание",
        "boot": "Загрузка",
        "system": "Система",
        "maintenance": "Обслуживание",
        "scheduler": "Планировщик",
        "monitor": "Мониторинг",
        "restore": "Точки восстановления",
        "windows_update": "Windows Update",
        "uwp": "UWP Приложения",
        "drivers": "Драйверы",
        "settings": "Настройки",
        "timer_shutdown": "Выключение",
        "timer_restart": "Перезагрузка",
        "force_check": "Принудительно закрыть программы",
        "start_timer": "Запустить таймер",
        "cancel_timer": "Отмена",
        "status_idle": "Таймер не запущен",
        "sleep": "Сон",
        "shutdown_direct": "Выключить",
        "force_shutdown": "Принудительно",
        "boot_bios": "BIOS (UEFI)",
        "boot_safe_min": "Безопасный режим (Мин.)",
        "boot_safe_net": "Безопасный режим (Сеть)",
        "boot_reset": "Сброс Safe Mode",
        "boot_menu": "Boot Menu",
        "sys_info": "Сведения о системе",
        "dev_mgr": "Диспетчер устройств",
        "task_mgr": "Диспетчер задач",
        "disk_mgr": "Управление дисками",
        "local_users": "Локальные пользователи",
        "regedit": "Редактор реестра",
        "control_panel": "Панель управления",
        "osk": "Экранная клавиатура",
        "magnifier": "Лупа",
        "minimize_all": "Свернуть всё",
        "gpu_reset": "Перезагрузка видеодрайвера",
        "apps_folder": "AppsFolder",
        "powershell": "PowerShell",
        "change_time": "Изменить системное время",
        "clean_temp": "Очистка временных файлов",
        "sfc": "Проверка SFC",
        "dism": "Восстановление DISM",
        "clean_updates": "Удаление остатков обновлений",
        "mrt": "Запустить MRT",
        "flush_dns": "Очистить DNS-кэш",
        "services_mgr": "Менеджер служб",
        "backup_registry": "Резервная копия реестра",
        "sched_info": "Планировщик работает в фоне и потребляет ресурсы ПК.",
        "sched_action": "Действие:",
        "sched_time": "Время (ЧЧ:ММ):",
        "sched_days": "Дни:",
        "sched_add": "Добавить задание",
        "cpu_label": "Загрузка ЦП:",
        "ram_label": "Загрузка ОЗУ:",
        "refresh_rp": "Обновить список",
        "create_rp": "Создать точку восстановления",
        "delete_rp": "Удалить выбранную",
        "open_restore": "Восстановление системы...",
        "confirm_delete_rp": "Удалить выбранную точку восстановления?",
        "error_no_admin": "Для этого действия нужны права администратора.",
        "error_rp_list": "Не удалось получить список точек восстановления. Проверьте права администратора или защиту системы.",
        "check_updates": "Проверить обновления",
        "disable_updates": "Отключить автообновления",
        "disable_defender": "Отключить Defender",
        "enable_defender": "Включить Defender",
        "confirm_disable_updates": "Будут отключены автоматические обновления. Продолжить?",
        "confirm_disable_defender": "Будет отключён Windows Defender (потребуется перезагрузка). Продолжить?",
        "confirm_enable_defender": "Будет включён Windows Defender (потребуется перезагрузка). Продолжить?",
        "defender_section": "Защитник Windows",
        "refresh_uwp": "Обновить список",
        "remove_uwp": "Удалить выбранные",
        "uwp_for_all": "Для всех пользователей",
        "confirm_uwp_remove": "Удалить выбранные приложения безвозвратно?",
        "uwp_no_apps": "Приложения UWP не найдены или требуются права администратора.",
        "drivers_nvidia": "Драйверы NVIDIA",
        "drivers_amd": "Драйверы AMD",
        "drivers_intel": "Драйверы Intel",
        "tray_on": "Сворачивать в трей при закрытии",
        "autostart": "Запускать RecoverX с Windows",
        "startup_settings": "Настройки автозагрузок Windows",
        "admin_btn": "Запустить от имени администратора",
        "check_app_updates": "Проверить обновления RecoverX",
        "no_updates": "У вас последняя версия.",
        "update_available": "Доступна новая версия: {version}\nСкачать: {url}",
        "update_error": "Не удалось проверить обновления.",
        "confirm_shutdown": "Выключить сейчас?",
        "confirm_force": "Все несохранённые данные будут потеряны. Продолжить?",
        "confirm_bios": "Перезагрузить в UEFI?",
        "confirm_safe": "Установить Safe Mode и перезагрузить?",
        "confirm_clean_temp": "Очистить временные файлы Windows?",
        "confirm_clean_updates": "Удалить остатки обновлений? Может потребоваться перезагрузка.",
        "admin_needed": "Требуются права администратора. Перезапустить?",
        "done": "Готово",
        "error": "Ошибка",
        "not_available": "Оснастка недоступна в этой редакции Windows.",
    }
}

def load_language():
    try:
        with open("recoverx_config.json", "r") as f:
            config = json.load(f)
            return config.get("language", "en")
    except:
        return "en"

def save_language(lang):
    with open("recoverx_config.json", "w") as f:
        json.dump({"language": lang}, f)

def is_admin():
    try:
        return ctypes.windll.shell32.IsUserAnAdmin()
    except:
        return False

def request_admin_rights():
    if not is_admin():
        try:
            params = subprocess.list2cmdline(sys.argv)
            ctypes.windll.shell32.ShellExecuteW(None, "runas", sys.executable, params, None, 1)
            sys.exit()
        except Exception as e:
            messagebox.showerror("Error", f"Failed to get admin rights: {e}")

class RecoverXApp:
    def __init__(self, root):
        self.root = root
        self.root.option_add("*Font", "TkDefaultFont 10")
        self.lang = load_language()
        self.S = STRINGS[self.lang]

        # Иконка
        self.icon_path = create_icon()
        self.root.iconbitmap(self.icon_path)

        self.root.title(self.S["title"])
        self.root.geometry("600x550")
        self.root.minsize(500, 420)

        # Переменные
        self.timer_running = False
        self.seconds_left = 0
        self.autostart_var = tk.BooleanVar()
        self.tray_enabled_var = tk.BooleanVar(value=True)
        self.timer_action = tk.StringVar(value="shutdown")
        self.force_close = tk.BooleanVar(value=False)
        self.uwp_for_all = tk.BooleanVar(value=False)
        self.registry_key_name = "RecoverX"

        self.style = ttk.Style()
        self.apply_theme()

        self.notebook = ttk.Notebook(root)
        self.notebook.pack(fill="both", expand=True, padx=5, pady=5)

        self.tabs = {}
        tab_keys = ["power", "boot", "system", "maintenance", "scheduler", "monitor",
                    "restore", "windows_update", "uwp", "drivers", "settings"]
        for key in tab_keys:
            self.tabs[key] = ttk.Frame(self.notebook)
            self.notebook.add(self.tabs[key], text=self.S.get(key, key.capitalize()))

        self.rebuild_ui()

        # Трей
        self.tray_icon = None
        if self.tray_enabled_var.get():
            self.create_tray_icon()

        self.root.protocol("WM_DELETE_WINDOW", self.on_close)
        self.init_autostart_state()

    def apply_theme(self):
        bg = "#000000"
        fg = "#FFFFFF"
        btn_bg = "#000000"
        btn_fg = "#FFFFFF"
        border_color = "#FFFFFF"
        active_bg = "#000000"

        self.style.theme_use("clam")
        self.style.configure(".", background=bg, foreground=fg, fieldbackground=bg)
        self.style.configure("TNotebook", background=bg, borderwidth=0)
        self.style.configure("TNotebook.Tab", background=bg, foreground=fg, borderwidth=1, padding=[10,5])
        self.style.map("TNotebook.Tab", background=[("selected", bg)])
        self.style.configure("Outline.TButton", background=btn_bg, foreground=btn_fg,
                             borderwidth=1, relief="solid", bordercolor=border_color,
                             lightcolor=btn_bg, darkcolor=btn_bg,
                             focusthickness=0, padding=8)
        self.style.map("Outline.TButton",
                       background=[("active", active_bg), ("pressed", active_bg)],
                       foreground=[("active", btn_fg)])
        self.style.configure("TEntry", fieldbackground=bg, foreground=fg)
        self.style.configure("TCheckbutton", background=bg, foreground=fg)
        self.style.configure("TRadiobutton", background=bg, foreground=fg)
        self.style.configure("TLabel", background=bg, foreground=fg)
        self.root.configure(bg=bg)

    def _outline_button(self, parent, text, command, **kwargs):
        return ttk.Button(parent, text=text, command=command, style="Outline.TButton", **kwargs)

    def rebuild_ui(self):
        for key in self.tabs:
            for widget in self.tabs[key].winfo_children():
                widget.destroy()
        self.build_power_tab()
        self.build_boot_tab()
        self.build_system_tab()
        self.build_maintenance_tab()
        self.build_scheduler_tab()
        self.build_monitor_tab()
        try:
            self.build_restore_tab()
        except:
            ttk.Label(self.tabs["restore"], text=self.S.get("error_rp_list", "Restore points unavailable"), wraplength=300).pack(pady=20)
        try:
            self.build_windows_update_tab()
        except:
            ttk.Label(self.tabs["windows_update"], text="Update settings unavailable", wraplength=300).pack(pady=20)
        try:
            self.build_uwp_tab()
        except:
            ttk.Label(self.tabs["uwp"], text="UWP management unavailable", wraplength=300).pack(pady=20)
        self.build_drivers_tab()
        self.build_settings_tab()

    # ---------- Power Tab ----------
    def build_power_tab(self):
        f = self.tabs["power"]; s = self.S
        frame_action = ttk.Frame(f); frame_action.pack(pady=10)
        ttk.Radiobutton(frame_action, text=s["timer_shutdown"], variable=self.timer_action, value="shutdown").pack(side="left", padx=10)
        ttk.Radiobutton(frame_action, text=s["timer_restart"], variable=self.timer_action, value="restart").pack(side="left", padx=10)
        frame_time = ttk.Frame(f); frame_time.pack(pady=5)
        ttk.Label(frame_time, text="Min:").pack(side="left")
        self.entry_min = ttk.Entry(frame_time, width=5); self.entry_min.pack(side="left", padx=5); self.entry_min.insert(0, "5")
        ttk.Label(frame_time, text="Sec:").pack(side="left")
        self.entry_sec = ttk.Entry(frame_time, width=5); self.entry_sec.pack(side="left", padx=5); self.entry_sec.insert(0, "0")
        ttk.Checkbutton(f, text=s["force_check"], variable=self.force_close).pack(pady=5)
        frame_btn = ttk.Frame(f); frame_btn.pack(pady=10)
        self.btn_start = self._outline_button(frame_btn, s["start_timer"], self.start_timer); self.btn_start.pack(side="left", padx=10)
        self.btn_cancel = self._outline_button(frame_btn, s["cancel_timer"], self.cancel_timer); self.btn_cancel.pack(side="left", padx=10)
        self.btn_cancel.state(["disabled"])
        self.lbl_status = ttk.Label(f, text=s["status_idle"]); self.lbl_status.pack(pady=10)
        frame_direct = ttk.Frame(f); frame_direct.pack(pady=10)
        self._outline_button(frame_direct, s["sleep"], self.sleep_now).pack(side="left", padx=5)
        self._outline_button(frame_direct, s["shutdown_direct"], self.shutdown_direct).pack(side="left", padx=5)
        self._outline_button(frame_direct, s["force_shutdown"], self.force_shutdown_direct).pack(side="left", padx=5)

    # ---------- Boot Tab ----------
    def build_boot_tab(self):
        f = self.tabs["boot"]; s = self.S
        self._outline_button(f, s["boot_bios"], self.reboot_to_bios).pack(pady=5, padx=20, fill="x")
        self._outline_button(f, s["boot_safe_min"], lambda: self.set_boot_mode("minimal")).pack(pady=2, padx=20, fill="x")
        self._outline_button(f, s["boot_safe_net"], lambda: self.set_boot_mode("network")).pack(pady=2, padx=20, fill="x")
        self._outline_button(f, s["boot_reset"], lambda: self.set_boot_mode("normal")).pack(pady=2, padx=20, fill="x")
        self._outline_button(f, s["boot_menu"], self.open_boot_menu_simulator).pack(pady=5, padx=20, fill="x")

    # ---------- System Tab ----------
    def build_system_tab(self):
        f = self.tabs["system"]; s = self.S
        left = ttk.Frame(f); left.pack(side="left", fill="both", expand=True, padx=5, pady=5)
        right = ttk.Frame(f); right.pack(side="right", fill="both", expand=True, padx=5, pady=5)
        buttons = [
            (s["sys_info"], self.run_msinfo32), (s["dev_mgr"], self.run_devmgmt), (s["task_mgr"], self.run_taskmgr),
            (s["disk_mgr"], self.run_diskmgmt), (s["local_users"], self.run_lusrmgr), (s["regedit"], self.run_regedit),
            (s["control_panel"], self.run_control), (s["osk"], self.run_osk), (s["magnifier"], self.run_magnify),
            (s["minimize_all"], self.minimize_all), (s["gpu_reset"], self.restart_gpu), (s["apps_folder"], self.open_appsfolder),
            (s["powershell"], self.run_powershell), (s["change_time"], self.change_system_time),
            (s["flush_dns"], self.flush_dns), (s["services_mgr"], self.open_services_mgr),
            (s["backup_registry"], self.backup_registry)
        ]
        half = (len(buttons)+1)//2
        for i,(text,cmd) in enumerate(buttons):
            parent = left if i<half else right
            self._outline_button(parent, text, cmd).pack(pady=2, fill="x")

    # ---------- Maintenance Tab ----------
    def build_maintenance_tab(self):
        f = self.tabs["maintenance"]; s = self.S
        actions = [(s["clean_temp"], self.clean_temp), (s["sfc"], self.run_sfc), (s["dism"], self.run_dism),
                   (s["clean_updates"], self.clean_updates), (s["mrt"], self.run_mrt)]
        for text,cmd in actions:
            self._outline_button(f, text, cmd).pack(pady=3, padx=20, fill="x")

    # ---------- Scheduler Tab ----------
    def build_scheduler_tab(self):
        f = self.tabs["scheduler"]; s = self.S
        ttk.Label(f, text=s["sched_info"], wraplength=400).pack(pady=10)
        frame_top = ttk.Frame(f); frame_top.pack(pady=5)
        ttk.Label(frame_top, text=s["sched_action"]).pack(side="left", padx=5)
        self.sched_action_var = tk.StringVar(value="shutdown")
        ttk.Radiobutton(frame_top, text="Shutdown", variable=self.sched_action_var, value="shutdown").pack(side="left", padx=2)
        ttk.Radiobutton(frame_top, text="Restart", variable=self.sched_action_var, value="restart").pack(side="left", padx=2)
        frame_time = ttk.Frame(f); frame_time.pack(pady=5)
        ttk.Label(frame_time, text=s["sched_time"]).pack(side="left", padx=5)
        self.sched_time_entry = ttk.Entry(frame_time, width=7); self.sched_time_entry.pack(side="left", padx=5); self.sched_time_entry.insert(0, "23:00")
        frame_days = ttk.Frame(f); frame_days.pack(pady=5)
        ttk.Label(frame_days, text=s["sched_days"]).pack(side="left", padx=5)
        self.day_vars = {}
        for day in ["Mon","Tue","Wed","Thu","Fri","Sat","Sun"]:
            var = tk.BooleanVar(); ttk.Checkbutton(frame_days, text=day, variable=var).pack(side="left", padx=2)
            self.day_vars[day] = var
        self._outline_button(f, s["sched_add"], self.add_schedule).pack(pady=10)

    def add_schedule(self):
        time_str = self.sched_time_entry.get()
        try:
            h,m = map(int, time_str.split(':'))
            if not (0<=h<=23 and 0<=m<=59): raise ValueError
        except: messagebox.showerror("Error", "Invalid time"); return
        selected_days = [d for d,var in self.day_vars.items() if var.get()]
        if not selected_days: messagebox.showwarning("Scheduler", "Select at least one day"); return
        action = "/s" if self.sched_action_var.get()=="shutdown" else "/r"
        cmd = f"shutdown {action} /t 0"
        script_path = os.path.join(os.getenv("TEMP"), "recoverx_sched.bat")
        with open(script_path,"w") as f: f.write(cmd)
        days_str = ",".join(selected_days).upper()
        schtasks_cmd = f'schtasks /create /tn "RecoverX_{action}" /tr "{script_path}" /sc weekly /d {days_str} /st {h:02d}:{m:02d} /f'
        try:
            subprocess.run(schtasks_cmd, shell=True, check=True, creationflags=subprocess.CREATE_NO_WINDOW)
            messagebox.showinfo("Scheduler", "Task scheduled")
        except Exception as e: messagebox.showerror("Error", str(e))

    # ---------- Monitor Tab ----------
    def build_monitor_tab(self):
        f = self.tabs["monitor"]; s = self.S
        self.cpu_label = ttk.Label(f, text=s["cpu_label"]+" --%"); self.cpu_label.pack(pady=10)
        self.ram_label = ttk.Label(f, text=s["ram_label"]+" --%"); self.ram_label.pack(pady=5)
        try:
            import psutil; self.update_monitor()
        except ImportError:
            self.cpu_label.config(text="psutil not installed")
            self.ram_label.config(text="Please install psutil to monitor resources")

    def update_monitor(self):
        try:
            import psutil
            cpu = psutil.cpu_percent(interval=0.5); ram = psutil.virtual_memory().percent
            self.cpu_label.config(text=f"{self.S['cpu_label']} {cpu:.1f}%"); self.ram_label.config(text=f"{self.S['ram_label']} {ram:.1f}%")
        except: pass
        self.root.after(2000, self.update_monitor)

    # ---------- Restore Points ----------
    def build_restore_tab(self):
        f = self.tabs["restore"]; s = self.S
        self.rp_listbox = tk.Listbox(f, selectmode="single", height=8, bg="#000000", fg="#FFFFFF", relief="flat")
        self.rp_listbox.pack(pady=5, padx=10, fill="both", expand=True)
        btn_frame = ttk.Frame(f); btn_frame.pack(pady=5)
        self._outline_button(btn_frame, s["refresh_rp"], self.refresh_restore_points).pack(side="left", padx=5)
        self._outline_button(btn_frame, s["create_rp"], self.create_restore_point).pack(side="left", padx=5)
        self._outline_button(btn_frame, s["delete_rp"], self.delete_restore_point).pack(side="left", padx=5)
        self._outline_button(btn_frame, s["open_restore"], self.open_system_restore).pack(side="left", padx=5)
        try: self.refresh_restore_points()
        except: self.rp_listbox.insert("end", "Error loading restore points")

    def refresh_restore_points(self):
        try:
            output = subprocess.check_output(["powershell", "-Command", "Get-ComputerRestorePoint | Select-Object -ExpandProperty Description"], text=True, creationflags=subprocess.CREATE_NO_WINDOW)
            self.rp_listbox.delete(0,"end")
            for line in output.strip().splitlines():
                if line.strip(): self.rp_listbox.insert("end", line.strip())
        except Exception as e:
            self.rp_listbox.delete(0,"end")
            self.rp_listbox.insert("end", "Failed to list restore points")

    def delete_restore_point(self):
        sel = self.rp_listbox.curselection()
        if not sel: messagebox.showwarning("Restore Points", "Select a restore point"); return
        if messagebox.askyesno("Delete", self.S["confirm_delete_rp"]):
            desc = self.rp_listbox.get(sel[0])
            try:
                subprocess.run(["powershell", "-Command", f'Get-ComputerRestorePoint | Where-Object {{$_.Description -eq "{desc}"}} | Remove-ComputerRestorePoint'], check=True, creationflags=subprocess.CREATE_NO_WINDOW)
                self.refresh_restore_points()
                messagebox.showinfo("Restore Points", self.S["done"])
            except Exception as e: messagebox.showerror("Error", str(e))

    def open_system_restore(self): subprocess.Popen("rstrui.exe", shell=True, creationflags=subprocess.CREATE_NO_WINDOW)

    def create_restore_point(self):
        if not self.ensure_admin(): return
        try:
            subprocess.run(["powershell", "-Command", 'Checkpoint-Computer -Description "RecoverX" -RestorePointType MODIFY_SETTINGS'], check=True, creationflags=subprocess.CREATE_NO_WINDOW)
            messagebox.showinfo("Restore Point", self.S["done"]); self.refresh_restore_points()
        except Exception as e: messagebox.showerror("Error", str(e))

    # ---------- Windows Update ----------
    def build_windows_update_tab(self):
        f = self.tabs["windows_update"]; s = self.S
        self._outline_button(f, s["check_updates"], self.check_windows_updates).pack(pady=10, padx=20, fill="x")
        self._outline_button(f, s["disable_updates"], self.disable_auto_updates).pack(pady=5, padx=20, fill="x")
        ttk.Separator(f, orient="horizontal").pack(fill="x", padx=10, pady=10)
        ttk.Label(f, text=s["defender_section"], font=("Segoe UI", 10, "bold")).pack(pady=5)
        self._outline_button(f, s["disable_defender"], self.disable_defender).pack(pady=5, padx=20, fill="x")
        self._outline_button(f, s["enable_defender"], self.enable_defender).pack(pady=5, padx=20, fill="x")

    def check_windows_updates(self): subprocess.Popen("start ms-settings:windowsupdate", shell=True, creationflags=subprocess.CREATE_NO_WINDOW)

    def disable_auto_updates(self):
        if not self.ensure_admin(): return
        if messagebox.askyesno("Windows Update", self.S["confirm_disable_updates"]):
            try:
                key = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, r"SOFTWARE\Policies\Microsoft\Windows\WindowsUpdate\AU", 0, winreg.KEY_SET_VALUE)
                winreg.SetValueEx(key, "NoAutoUpdate", 0, winreg.REG_DWORD, 1); winreg.CloseKey(key)
                messagebox.showinfo("Windows Update", "Automatic updates disabled.")
            except Exception as e: messagebox.showerror("Error", str(e))

    def disable_defender(self):
        if not self.ensure_admin(): return
        if messagebox.askyesno("Defender", self.S["confirm_disable_defender"]):
            try:
                key = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, r"SOFTWARE\Policies\Microsoft\Windows Defender", 0, winreg.KEY_SET_VALUE)
                winreg.SetValueEx(key, "DisableAntiSpyware", 0, winreg.REG_DWORD, 1); winreg.CloseKey(key)
                messagebox.showinfo("Defender", "Disabled. Restart required.")
            except Exception as e: messagebox.showerror("Error", str(e))

    def enable_defender(self):
        if not self.ensure_admin(): return
        if messagebox.askyesno("Defender", self.S["confirm_enable_defender"]):
            try:
                key = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, r"SOFTWARE\Policies\Microsoft\Windows Defender", 0, winreg.KEY_SET_VALUE)
                try: winreg.DeleteValue(key, "DisableAntiSpyware")
                except: winreg.SetValueEx(key, "DisableAntiSpyware", 0, winreg.REG_DWORD, 0)
                winreg.CloseKey(key); messagebox.showinfo("Defender", "Enabled. Restart required.")
            except Exception as e: messagebox.showerror("Error", str(e))

    # ---------- UWP ----------
    def build_uwp_tab(self):
        f = self.tabs["uwp"]; s = self.S
        ttk.Label(f, text="UWP Apps (Admin)").pack(pady=5)
        self.uwp_listbox = tk.Listbox(f, selectmode="extended", height=10, bg="#000000", fg="#FFFFFF", relief="flat")
        self.uwp_listbox.pack(pady=5, padx=10, fill="both", expand=True)
        frame_btn = ttk.Frame(f); frame_btn.pack(pady=5)
        self._outline_button(frame_btn, s["refresh_uwp"], self.refresh_uwp_list).pack(side="left", padx=5)
        ttk.Checkbutton(frame_btn, text=s["uwp_for_all"], variable=self.uwp_for_all).pack(side="left", padx=5)
        self._outline_button(frame_btn, s["remove_uwp"], self.remove_selected_uwp).pack(side="left", padx=5)

    def refresh_uwp_list(self):
        if not self.ensure_admin():
            self.uwp_listbox.delete(0, "end")
            self.uwp_listbox.insert("end", self.S["uwp_no_apps"])
            return
        try:
            output = subprocess.check_output(
                'powershell -Command "Get-AppxPackage | Select-Object -ExpandProperty Name"',
                shell=True,
                creationflags=subprocess.CREATE_NO_WINDOW
            ).decode('utf-8', errors='ignore')
            self.uwp_listbox.delete(0, "end")
            names = [line.strip() for line in output.splitlines() if line.strip()]
            if not names:
                self.uwp_listbox.insert("end", self.S["uwp_no_apps"])
            else:
                for name in names:
                    self.uwp_listbox.insert("end", name)
        except Exception as e:
            self.uwp_listbox.delete(0, "end")
            self.uwp_listbox.insert("end", f"Error: {str(e)}")

    def remove_selected_uwp(self):
        if not self.ensure_admin(): return
        sel = self.uwp_listbox.curselection()
        if not sel: messagebox.showwarning("UWP", "Select apps"); return
        names = [self.uwp_listbox.get(i) for i in sel]
        if messagebox.askyesno("UWP", self.S["confirm_uwp_remove"]):
            all_flag = " -AllUsers" if self.uwp_for_all.get() else ""
            for name in names:
                subprocess.run(["powershell", "-Command", f"Get-AppxPackage -Name {name} | Remove-AppxPackage{all_flag}"], capture_output=True, creationflags=subprocess.CREATE_NO_WINDOW)
            self.refresh_uwp_list(); messagebox.showinfo("UWP", self.S["done"])

    # ---------- Drivers ----------
    def build_drivers_tab(self):
        f = self.tabs["drivers"]; s = self.S
        self._outline_button(f, s["drivers_nvidia"], lambda: os.startfile("https://www.nvidia.com/download/index.aspx")).pack(pady=10, padx=20, fill="x")
        self._outline_button(f, s["drivers_amd"], lambda: os.startfile("https://www.amd.com/en/support")).pack(pady=5, padx=20, fill="x")
        self._outline_button(f, s["drivers_intel"], lambda: os.startfile("https://www.intel.com/content/www/us/en/download-center/home.html")).pack(pady=5, padx=20, fill="x")

    # ---------- Settings ----------
    def build_settings_tab(self):
        f = self.tabs["settings"]; s = self.S
        ttk.Checkbutton(f, text=s["tray_on"], variable=self.tray_enabled_var, command=self.toggle_tray).pack(pady=10, anchor="w", padx=20)
        ttk.Checkbutton(f, text=s["autostart"], variable=self.autostart_var, command=self.toggle_autostart).pack(pady=5, anchor="w", padx=20)
        self._outline_button(f, s["startup_settings"], self.open_startup_settings).pack(pady=20, padx=20, fill="x")
        self._outline_button(f, s["check_app_updates"], self.check_recoverx_updates).pack(pady=10, padx=20, fill="x")
        ttk.Label(f, text="Language:").pack(pady=(20,0))
        btn_frame = ttk.Frame(f); btn_frame.pack(pady=10)
        self._outline_button(btn_frame, "English", lambda: self.switch_language("en")).pack(side="left", padx=10)
        self._outline_button(btn_frame, "Русский", lambda: self.switch_language("ru")).pack(side="left", padx=10)

    def switch_language(self, lang):
        self.lang = lang; self.S = STRINGS[lang]
        self.root.title(self.S["title"])
        for key, tab in self.tabs.items():
            self.notebook.tab(tab, text=self.S[key])
        save_language(lang)
        self.rebuild_ui()

    # ---------- Проверка обновлений RecoverX ----------
    def check_recoverx_updates(self):
        try:
            url = "https://api.github.com/repos/BackupTeam/RecoverX/releases/latest"
            with urllib.request.urlopen(url, timeout=5) as resp:
                data = json.loads(resp.read().decode('utf-8'))
                latest_version = data["tag_name"]
                download_url = data["html_url"]
                current_version = "v1.0.0"
                if latest_version != current_version:
                    messagebox.showinfo("Update Available", self.S["update_available"].format(version=latest_version, url=download_url))
                else:
                    messagebox.showinfo("Up to date", self.S["no_updates"])
        except Exception as e:
            messagebox.showerror("Update Error", f"{self.S['update_error']}\n{str(e)}")

    # ---------- Tray ----------
    def create_tray_icon(self):
        if self.tray_icon: return
        img = Image.open(self.icon_path)
        menu = pystray.Menu(
            pystray.MenuItem("Open", self.restore_window),
            pystray.MenuItem("Exit", self.quit_app)
        )
        self.tray_icon = pystray.Icon("RecoverX", img, "RecoverX", menu)
        threading.Thread(target=self.tray_icon.run, daemon=True).start()

    def remove_tray_icon(self):
        if self.tray_icon:
            self.tray_icon.stop()
            self.tray_icon = None

    def toggle_tray(self):
        if self.tray_enabled_var.get(): self.create_tray_icon()
        else: self.remove_tray_icon()

    def on_close(self):
        if self.tray_enabled_var.get():
            self.root.withdraw()
        else:
            self.quit_app()

    def restore_window(self):
        self.root.deiconify()
        self.root.lift()

    def quit_app(self):
        self.remove_tray_icon()
        if self.timer_running:
            self.cancel_timer()
        self.root.destroy()
        sys.exit(0)

    # ---------- Autostart ----------
    def is_in_autostart(self):
        try:
            key = winreg.OpenKey(winreg.HKEY_CURRENT_USER, r"Software\Microsoft\Windows\CurrentVersion\Run", 0, winreg.KEY_READ)
            winreg.QueryValueEx(key, self.registry_key_name); winreg.CloseKey(key); return True
        except FileNotFoundError: return False

    def toggle_autostart(self):
        try:
            key = winreg.OpenKey(winreg.HKEY_CURRENT_USER, r"Software\Microsoft\Windows\CurrentVersion\Run", 0, winreg.KEY_SET_VALUE)
            if self.autostart_var.get():
                app_path = f'"{sys.executable}" "{os.path.abspath(sys.argv[0])}"'
                winreg.SetValueEx(key, self.registry_key_name, 0, winreg.REG_SZ, app_path)
            else:
                try: winreg.DeleteValue(key, self.registry_key_name)
                except: pass
            winreg.CloseKey(key)
        except Exception as e: messagebox.showerror("Error", str(e))
    def init_autostart_state(self): self.autostart_var.set(self.is_in_autostart())

    # ---------- Timer ----------
    def start_timer(self):
        if self.timer_running: return
        try:
            mins = int(self.entry_min.get() or 0); secs = int(self.entry_sec.get() or 0)
            total = mins*60+secs
            if total<=0: raise ValueError
        except ValueError:
            messagebox.showerror("Error", "Enter time > 0"); return
        self.seconds_left = total
        action = "/s" if self.timer_action.get()=="shutdown" else "/r"
        force = " /f" if self.force_close.get() else ""
        subprocess.run(f"shutdown {action}{force} /t {self.seconds_left}", shell=True, creationflags=subprocess.CREATE_NO_WINDOW)
        self.timer_running = True
        self.btn_start.state(["disabled"]); self.btn_cancel.state(["!disabled"])
        self.update_timer()

    def update_timer(self):
        if not self.timer_running: return
        if self.seconds_left==10: winsound.MessageBeep(winsound.MB_ICONEXCLAMATION)
        if self.seconds_left>0:
            mins, secs = divmod(self.seconds_left,60)
            self.lbl_status.config(text=f"Until {'shutdown' if self.timer_action.get()=='shutdown' else 'restart'}: {mins:02d}:{secs:02d}")
            self.seconds_left-=1; self.root.after(1000, self.update_timer)
        else:
            self.lbl_status.config(text="Executing..."); self.timer_running=False
            self.btn_start.state(["!disabled"]); self.btn_cancel.state(["disabled"])

    def cancel_timer(self):
        subprocess.run("shutdown /a", shell=True, creationflags=subprocess.CREATE_NO_WINDOW)
        self.timer_running=False; self.btn_start.state(["!disabled"]); self.btn_cancel.state(["disabled"])
        self.lbl_status.config(text=self.S["status_idle"])

    # ---------- Power actions ----------
    def sleep_now(self): ctypes.windll.powrprof.SetSuspendState(0,1,0)
    def shutdown_direct(self):
        if messagebox.askyesno("Shutdown", self.S["confirm_shutdown"]): subprocess.run("shutdown /s /t 0", shell=True, creationflags=subprocess.CREATE_NO_WINDOW)
    def force_shutdown_direct(self):
        if messagebox.askyesno("Force Shutdown", self.S["confirm_force"]): subprocess.run("shutdown /s /f /t 0", shell=True, creationflags=subprocess.CREATE_NO_WINDOW)

    # ---------- Boot ----------
    def reboot_to_bios(self):
        if not self.ensure_admin(): return
        if messagebox.askyesno("BIOS", self.S["confirm_bios"]): subprocess.run("shutdown /r /fw /t 0", shell=True, creationflags=subprocess.CREATE_NO_WINDOW)

    def set_boot_mode(self, mode):
        if not self.ensure_admin(): return
        if messagebox.askyesno("Safe Mode", self.S["confirm_safe"]):
            if mode=="normal": subprocess.run("bcdedit /deletevalue {default} safeboot", shell=True, creationflags=subprocess.CREATE_NO_WINDOW)
            else: subprocess.run(f"bcdedit /set {{default}} safeboot {mode}", shell=True, creationflags=subprocess.CREATE_NO_WINDOW)
            subprocess.run("shutdown /r /t 0", shell=True, creationflags=subprocess.CREATE_NO_WINDOW)

    def open_boot_menu_simulator(self):
        win = tk.Toplevel(self.root); win.title("Boot Menu"); win.geometry("600x400"); win.configure(bg="#0000AA"); win.grab_set()
        items = [("Continue", win.destroy), ("Boot from HDD", lambda:0), ("Boot from USB", lambda:0), ("Boot from CD/DVD", lambda:0), ("Enter BIOS Setup", lambda:[win.destroy(), self.reboot_to_bios()])]
        self.bios_sel=0; self.bios_items=items
        self.bios_text = tk.Text(win, bg="#0000AA", fg="white", font=("Courier New",12), bd=0); self.bios_text.pack(expand=True, fill="both", padx=20, pady=20)
        win.bind("<Up>", self._bios_nav); win.bind("<Down>", self._bios_nav); win.bind("<Return>", self._bios_exec); win.bind("<Escape>", lambda e: win.destroy())
        self._bios_update(win)

    def _bios_update(self, win):
        self.bios_text.config(state="normal"); self.bios_text.delete("1.0","end")
        self.bios_text.insert("end", "╔══════════════════════╗\n║     BOOT MENU       ║\n╚══════════════════════╝\n\n")
        for i,(text,_) in enumerate(self.bios_items):
            prefix = " ▶ " if i==self.bios_sel else "   "; self.bios_text.insert("end", f"{prefix}{text}\n")
        self.bios_text.config(state="disabled")
    def _bios_nav(self, event):
        if event.keysym=="Up": self.bios_sel = (self.bios_sel-1)%len(self.bios_items)
        elif event.keysym=="Down": self.bios_sel = (self.bios_sel+1)%len(self.bios_items)
        self._bios_update(event.widget.winfo_toplevel())
    def _bios_exec(self, event): self.bios_items[self.bios_sel][1]()

    # ---------- System functions ----------
    def run_msinfo32(self): subprocess.Popen("msinfo32.exe", shell=True, creationflags=subprocess.CREATE_NO_WINDOW)
    def run_devmgmt(self): subprocess.Popen("devmgmt.msc", shell=True, creationflags=subprocess.CREATE_NO_WINDOW)
    def run_taskmgr(self): subprocess.Popen("taskmgr.exe", shell=True, creationflags=subprocess.CREATE_NO_WINDOW)
    def run_diskmgmt(self): subprocess.Popen("diskmgmt.msc", shell=True, creationflags=subprocess.CREATE_NO_WINDOW)
    def run_lusrmgr(self):
        try: subprocess.Popen("lusrmgr.msc", shell=True, creationflags=subprocess.CREATE_NO_WINDOW)
        except FileNotFoundError: messagebox.showerror("Error", self.S["not_available"])
    def run_regedit(self): subprocess.Popen("regedit.exe", shell=True, creationflags=subprocess.CREATE_NO_WINDOW)
    def run_control(self): subprocess.Popen("control.exe", shell=True, creationflags=subprocess.CREATE_NO_WINDOW)
    def run_osk(self): subprocess.Popen("osk.exe", shell=True, creationflags=subprocess.CREATE_NO_WINDOW)
    def run_magnify(self): subprocess.Popen("magnify.exe", shell=True, creationflags=subprocess.CREATE_NO_WINDOW)
    def minimize_all(self):
        for vk in [0x5B,0x44]: ctypes.windll.user32.keybd_event(vk,0,0,0)
        for vk in [0x44,0x5B]: ctypes.windll.user32.keybd_event(vk,0,2,0)
    def restart_gpu(self):
        for vk in [0xA2,0xA0,0x5B,0x42]: ctypes.windll.user32.keybd_event(vk,0,0,0)
        for vk in [0x42,0x5B,0xA0,0xA2]: ctypes.windll.user32.keybd_event(vk,0,2,0)
    def open_appsfolder(self): os.startfile("shell:AppsFolder")
    def run_powershell(self): subprocess.Popen(["powershell.exe"], creationflags=subprocess.CREATE_NEW_CONSOLE)
    def open_startup_settings(self): subprocess.Popen("start ms-settings:startupapps", shell=True, creationflags=subprocess.CREATE_NO_WINDOW)
    def change_system_time(self): subprocess.Popen("control timedate.cpl", shell=True, creationflags=subprocess.CREATE_NO_WINDOW)
    def flush_dns(self):
        if messagebox.askyesno("Flush DNS", "Clear DNS cache?"):
            subprocess.run("ipconfig /flushdns", shell=True, creationflags=subprocess.CREATE_NO_WINDOW)
            messagebox.showinfo("DNS", "DNS cache cleared.")
    def open_services_mgr(self): subprocess.Popen("services.msc", shell=True, creationflags=subprocess.CREATE_NO_WINDOW)
    def backup_registry(self):
        file_path = filedialog.asksaveasfilename(defaultextension=".reg", filetypes=[("Registry files", "*.reg")], title="Save Registry Backup As")
        if file_path:
            try:
                subprocess.run(f'regedit /e "{file_path}"', shell=True, creationflags=subprocess.CREATE_NO_WINDOW, check=True)
                messagebox.showinfo("Registry Backup", f"Registry exported to {file_path}")
            except Exception as e:
                messagebox.showerror("Error", f"Failed to export registry: {e}")

    def clean_temp(self):
        if messagebox.askyesno("Clean", self.S["confirm_clean_temp"]):
            subprocess.run("del /f /s /q %temp%\\*", shell=True, creationflags=subprocess.CREATE_NO_WINDOW)
            subprocess.run("del /f /s /q C:\\Windows\\Temp\\*", shell=True, creationflags=subprocess.CREATE_NO_WINDOW)
            subprocess.run("del /f /s /q C:\\Windows\\Prefetch\\*", shell=True, creationflags=subprocess.CREATE_NO_WINDOW)
            messagebox.showinfo("Clean", self.S["done"])
    def run_sfc(self):
        if self.ensure_admin(): subprocess.Popen("sfc /scannow", shell=True, creationflags=subprocess.CREATE_NO_WINDOW)
    def run_dism(self):
        if self.ensure_admin(): subprocess.Popen("DISM /Online /Cleanup-Image /RestoreHealth", shell=True, creationflags=subprocess.CREATE_NO_WINDOW)
    def clean_updates(self):
        if not self.ensure_admin(): return
        if messagebox.askyesno("Updates", self.S["confirm_clean_updates"]):
            subprocess.run("del /f /s /q C:\\Windows\\SoftwareDistribution\\Download\\*", shell=True, creationflags=subprocess.CREATE_NO_WINDOW)
            subprocess.run("Dism.exe /online /Cleanup-Image /StartComponentCleanup /ResetBase", shell=True, creationflags=subprocess.CREATE_NO_WINDOW)
            messagebox.showinfo("Updates", self.S["done"])
    def run_mrt(self): subprocess.Popen("mrt.exe", shell=True, creationflags=subprocess.CREATE_NO_WINDOW)

    def ensure_admin(self):
        if not is_admin():
            if messagebox.askyesno("Admin", self.S["admin_needed"]): request_admin_rights()
            return False
        return True

if __name__ == "__main__":
    if ensure_single_instance():
        sys.exit(0)
    root = tk.Tk()
    app = RecoverXApp(root)
    root.mainloop()