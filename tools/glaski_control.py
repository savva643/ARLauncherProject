#!/usr/bin/env python3
"""GlaskiOS Control — сборка и запуск лаунчера одной кнопкой.

    python3 tools/glaski_control.py              # окно
    python3 tools/glaski_control.py --build      # без окна: собрать с сохранёнными настройками
    python3 tools/glaski_control.py --run        # без окна: собрать и запустить
    python3 tools/glaski_control.py --print-env  # показать переменные запуска

Настройки сохраняются в ~/.config/glaski/control.json. Нужен только Python 3 с Tk
(Ubuntu: sudo apt install python3-tk).
"""

import json
import os
import queue
import shlex
import shutil
import signal
import socket
import subprocess
import sys
import threading
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
LAUNCHER_SRC = ROOT / "ARLauncher"
CONFIG_PATH = Path.home() / ".config" / "glaski" / "control.json"

# ---------------------------------------------------------------------------------------------
# Настройки: (ключ, подпись, тип, по умолчанию, подсказка)
# ---------------------------------------------------------------------------------------------
BUILD_OPTIONS = [
    ("build_dir", "Папка сборки", "str", "ARLauncher/build_vk", "относительно корня проекта"),
    ("build_type", "Тип сборки", ["Release", "RelWithDebInfo", "Debug"], "Release", ""),
    ("openvins", "OpenVINS (6DoF по камере)", "bool", True, "если собран third_party/openvins"),
    ("audio", "Пространственный звук (OpenAL)", "bool", True, "sudo apt install libopenal-dev"),
    ("opencv", "OpenCV (метки ArUco, руки)", "bool", True, ""),
    ("tools", "Симулятор и stream_probe", "bool", True, ""),
    ("jobs", "Потоков компиляции", "int", os.cpu_count() or 4, ""),
]

RUN_SECTIONS = [
    ("Дисплей", [
        ("ARL_FULLSCREEN", "Полный экран (дисплей шлема)", "bool", False, "макс. частота в родном разрешении"),
        ("ARL_MONITOR", "Монитор №", "int", 0, "0 — основной"),
        ("ARL_STEREO", "Стерео (side-by-side)", "bool", False, "для шлема с одним экраном"),
        ("ARL_IPD_MM", "Межзрачковое, мм", "float", 63.0, ""),
        ("ARL_FOV", "Поле зрения глаза, °", "float", 0.0, "0 — как у камеры iPhone"),
        ("ARL_PRESENT_MODE", "Вывод кадров", ["mailbox", "fifo", "immediate"], "mailbox", "mailbox — меньше задержка"),
    ]),
    ("Трекинг", [
        ("ARL_MOUNT", "Крепление телефона", ["auto", "0", "90", "180", "270"], "auto",
         "0 — ландшафт, верх слева; 180 — верх справа; 90/270 — портрет. Клавиша M"),
        ("ARL_LIDAR_TRACKING", "Позиция по LiDAR (без дрейфа)", "bool", True, ""),
        ("ARL_VIO", "VIO OpenVINS, если нет LiDAR", "bool", True, ""),
        ("ARL_PREDICT_MS", "Предсказание, мс", "float", -1.0, "-1 — авто по частоте дисплея"),
    ]),
    ("Реализм", [
        ("ARL_OCCLUSION", "Окклюзия: стены закрывают объекты", "bool", True, "клавиша K"),
        ("ARL_SHADOWS", "Тени на полу", "bool", True, "клавиша H"),
        ("ARL_LIGHTING", "Свет и отражения комнаты", "bool", True, "клавиша E"),
        ("ARL_LIDAR_OVERLAY", "Показать LiDAR цветом", "bool", False, "клавиша L"),
    ]),
    ("Звук и метки", [
        ("ARL_AUDIO", "Пространственный звук", "bool", True, "клавиша U"),
        ("ARL_HRTF", "HRTF (объёмно в наушниках)", "bool", True, ""),
        ("ARL_MARKERS", "Метки ArUco (4x4)", "bool", True, ""),
        ("ARL_MARKER_SIZE_CM", "Размер метки, см", "float", 10.0, "сторона чёрного квадрата"),
    ]),
    ("Руки и пространство", [
        ("ARL_HANDS", "Руки (щипок — взять объект)", "bool", True, "модели в models/"),
        ("ARL_SHOW_ROOM", "Показать меш комнаты и плоскости", "bool", False, "клавиша V"),
        ("ARL_WORLD_MAP_ON", "Запоминать комнату и объекты", "bool", True, "~/.local/share/glaski/world.map, F5"),
    ]),
    ("Линзы шлема", [
        ("ARL_LENS", "Коррекция линз", "bool", False, "бочка + хроматическая аберрация"),
        ("ARL_LENS_K1", "k1", "float", 0.22, ""),
        ("ARL_LENS_K2", "k2", "float", 0.24, ""),
        ("ARL_LENS_CA", "Хроматическая аберрация", "float", 0.012, "R/B относительно G"),
        ("ARL_LENS_SCALE", "Масштаб", "float", 1.0, "<1 — меньше чёрных углов"),
        ("ARL_LENS_SHIFT", "Сдвиг центра к носу", "float", 0.0, "доля полуширины глаза"),
    ]),
    ("Модули шлема", [
        ("ARL_MIC_MONITOR", "Микрофон в наушники", "bool", False, "F7; F6 — запись WAV"),
        ("SIM_EYES", "Симулятор: камеры глаз", "bool", True, "G — калибровка взгляда"),
        ("SIM_AUDIO", "Симулятор: микрофон", "bool", True, ""),
    ]),
    ("Отладка", [
        ("ARL_VK_VALIDATION", "Проверка Vulkan (validation)", "bool", False, "медленнее"),
        ("ARL_GPU", "Видеокарта №", "int", -1, "-1 — авто (дискретная)"),
        ("SIM_ROOM", "Симулятор: комната (иначе сфера)", "bool", True, ""),
    ]),
]


def default_config():
    cfg = {key: default for key, _, _, default, _ in BUILD_OPTIONS}
    for _, items in RUN_SECTIONS:
        for key, _, _, default, _ in items:
            cfg[key] = default
    return cfg


def load_config():
    cfg = default_config()
    try:
        cfg.update(json.loads(CONFIG_PATH.read_text()))
    except (OSError, ValueError):
        pass
    return cfg


def save_config(cfg):
    CONFIG_PATH.parent.mkdir(parents=True, exist_ok=True)
    CONFIG_PATH.write_text(json.dumps(cfg, indent=2, ensure_ascii=False))


def build_dir(cfg):
    p = Path(cfg["build_dir"]).expanduser()
    return p if p.is_absolute() else ROOT / p


def configure_command(cfg):
    on = lambda v: "ON" if v else "OFF"
    return ["cmake", "-S", str(LAUNCHER_SRC), "-B", str(build_dir(cfg)),
            f"-DCMAKE_BUILD_TYPE={cfg['build_type']}", "-DUSE_VULKAN=ON", "-DUSE_OPENGL=OFF",
            f"-DLENSENGINE_WITH_OPENVINS={on(cfg['openvins'])}", f"-DARL_WITH_AUDIO={on(cfg['audio'])}",
            f"-DLENSENGINE_USE_OPENCV={on(cfg['opencv'])}", f"-DSENSORSTREAM_BUILD_TOOLS={on(cfg['tools'])}"]


def build_command(cfg):
    return ["cmake", "--build", str(build_dir(cfg)), "-j", str(int(cfg["jobs"]))]


def stale_cache(cfg):
    """CMakeCache из другой папки (скопировали проект / другой пользователь) — cmake откажется работать."""
    cache = build_dir(cfg) / "CMakeCache.txt"
    if not cache.exists():
        return False
    for line in cache.read_text(errors="ignore").splitlines():
        if line.startswith("CMAKE_HOME_DIRECTORY:INTERNAL="):
            return Path(line.split("=", 1)[1]).resolve() != LAUNCHER_SRC.resolve()
    return False


def launcher_env(cfg):
    """Переменные окружения для ARLauncher (только отличающиеся от умолчаний лаунчера)."""
    env = {}
    for _, items in RUN_SECTIONS:
        for key, _, kind, default, _ in items:
            if key.startswith("SIM_"):
                continue
            value = cfg.get(key, default)
            if key == "ARL_VIO":
                if not value:
                    env["ARL_NO_VIO"] = "1"
                continue
            if key == "ARL_WORLD_MAP_ON":
                if not value:
                    env["ARL_WORLD_MAP"] = "0"
                continue
            if kind == "bool":
                env[key] = "1" if value else "0"
            elif key == "ARL_GPU" and int(value) < 0:
                continue
            elif key == "ARL_PREDICT_MS" and float(value) < 0:
                continue
            elif key == "ARL_FOV" and float(value) <= 0:
                continue
            else:
                env[key] = str(value)
    return env


def local_addresses():
    """IP этого ПК — их вводят в iPhone (Wi-Fi: порт 9000; USB-модем iPhone: 9001)."""
    addrs = set()
    try:
        out = subprocess.run(["hostname", "-I"], capture_output=True, text=True, timeout=2).stdout
        addrs.update(a for a in out.split() if ":" not in a)
    except (OSError, subprocess.SubprocessError):
        pass
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("10.255.255.255", 1))
        addrs.add(s.getsockname()[0])
        s.close()
    except OSError:
        pass
    addrs.discard("127.0.0.1")
    return sorted(addrs)


def missing_tools():
    need = ["cmake", "glslangValidator", "c++"]
    return [t for t in need if shutil.which(t) is None]


# ---------------------------------------------------------------------------------------------
# Без окна
# ---------------------------------------------------------------------------------------------
def run_headless(args):
    cfg = load_config()
    if "--print-env" in args:
        for k, v in launcher_env(cfg).items():
            print(f"{k}={v}")
        return 0
    if stale_cache(cfg):
        print(f"[glaski] old CMakeCache from another folder — removing {build_dir(cfg) / 'CMakeCache.txt'}")
        (build_dir(cfg) / "CMakeCache.txt").unlink()
    for cmd in (configure_command(cfg), build_command(cfg)):
        print("$ " + " ".join(shlex.quote(c) for c in cmd), flush=True)
        if subprocess.call(cmd) != 0:
            return 1
    if "--run" in args:
        env = dict(os.environ, **launcher_env(cfg))
        return subprocess.call([str(build_dir(cfg) / "bin" / "ARLauncher")], cwd=build_dir(cfg), env=env)
    return 0


# ---------------------------------------------------------------------------------------------
# Окно
# ---------------------------------------------------------------------------------------------
def run_gui():
    try:
        import tkinter as tk
        from tkinter import ttk, messagebox
    except ImportError:
        print("Нужен Tk для окна: sudo apt install python3-tk\n"
              "Без окна: python3 tools/glaski_control.py --run")
        sys.exit(1)

    cfg = load_config()
    root = tk.Tk()
    root.title("GlaskiOS Control")
    root.geometry("1500x780")
    root.minsize(1000, 600)

    style = ttk.Style(root)
    if "clam" in style.theme_names():
        style.theme_use("clam")
    style.configure("Accent.TButton", font=("TkDefaultFont", 10, "bold"))
    style.configure("Status.TLabel", foreground="#2a6")
    style.configure("Error.TLabel", foreground="#c33")
    style.configure("Hint.TLabel", foreground="#777", font=("TkDefaultFont", 8))

    variables = {}

    def make_var(key, kind, default):
        value = cfg.get(key, default)
        if kind == "bool":
            var = tk.BooleanVar(value=bool(value))
        elif kind == "int":
            var = tk.IntVar(value=int(value))
        elif kind == "float":
            var = tk.DoubleVar(value=float(value))
        else:
            var = tk.StringVar(value=str(value))
        variables[key] = (var, kind)
        return var

    def collect():
        for key, (var, kind) in variables.items():
            try:
                cfg[key] = var.get()
            except tk.TclError:
                pass
        save_config(cfg)
        return cfg

    def add_row(parent, row, key, label, kind, default, hint):
        var = make_var(key, kind, default)
        if kind == "bool":
            w = ttk.Checkbutton(parent, text=label, variable=var)
            w.grid(row=row, column=0, columnspan=2, sticky="w", padx=6, pady=2)
        else:
            ttk.Label(parent, text=label).grid(row=row, column=0, sticky="w", padx=6, pady=2)
            if isinstance(kind, list):
                w = ttk.Combobox(parent, textvariable=var, values=kind, state="readonly", width=12)
            else:
                w = ttk.Entry(parent, textvariable=var, width=14 if kind != "str" else 26)
            w.grid(row=row, column=1, sticky="w", padx=6, pady=2)
        if hint:
            ttk.Label(parent, text=hint, style="Hint.TLabel", wraplength=170, justify="left").grid(
                row=row, column=2, sticky="w", padx=4)

    # --- левая колонка: вкладки настроек
    paned = ttk.PanedWindow(root, orient="vertical")
    paned.pack(fill="both", expand=True, padx=8, pady=8)
    left = ttk.Frame(paned)
    right = ttk.Frame(paned)
    paned.add(left, weight=3)
    paned.add(right, weight=2)

    notebook = ttk.Notebook(left)
    notebook.pack(fill="both", expand=True)

    run_tab = ttk.Frame(notebook)
    notebook.add(run_tab, text="Запуск")
    # две колонки: дисплей + трекинг | реализм + звук + отладка
    columns = [ttk.Frame(run_tab), ttk.Frame(run_tab), ttk.Frame(run_tab)]
    for c, col in enumerate(columns):
        col.grid(row=0, column=c, sticky="n")
    placement = {"Дисплей": 0, "Трекинг": 0, "Реализм": 1, "Звук и метки": 1, "Руки и пространство": 1,
                 "Линзы шлема": 2, "Модули шлема": 2, "Отладка": 2}
    for index, (title, items) in enumerate(RUN_SECTIONS):
        box = ttk.LabelFrame(columns[placement.get(title, 2)], text=title)
        box.pack(fill="x", padx=6, pady=4)
        for i, item in enumerate(items):
            add_row(box, i, *item)

    build_tab = ttk.Frame(notebook)
    notebook.add(build_tab, text="Сборка")
    box = ttk.LabelFrame(build_tab, text="CMake")
    box.pack(fill="x", padx=6, pady=4)
    for i, item in enumerate(BUILD_OPTIONS):
        add_row(box, i, *item)
    ttk.Label(build_tab, text="Зависимости Ubuntu — в README.md (раздел «Сборка»).\n"
                              "OpenVINS (6DoF по камере) собирается отдельно: кнопка ниже, ~10 мин.",
              style="Hint.TLabel", justify="left").pack(anchor="w", padx=10, pady=6)

    info_tab = ttk.Frame(notebook)
    notebook.add(info_tab, text="iPhone")
    addrs = local_addresses()
    text = ("Подключение в ARStreamer на iPhone:\n\n"
            "• USB (режим модема iPhone): адрес ПК в сети iPhone, обычно 172.20.10.2, порт 9001\n"
            "• Wi-Fi: один из адресов этого ПК, порт 9000\n\n"
            "Адреса этого ПК: " + (", ".join(addrs) if addrs else "не найдены") + "\n\n"
            "Клавиши в лаунчере:\n"
            "  R — центрировать взгляд,  M — крепление телефона (авто/0/90/180/270)\n"
            "  K — окклюзия,  H — тени,  E — свет/отражения,  U — звук\n"
            "  V — меш комнаты,  F5 — сохранить карту,  G — калибровка взгляда\n"
            "  F6 — запись микрофона,  F7 — микрофон в наушники\n"
            "  L — LiDAR цветом,  P — репроекция,  O — предсказание,  Esc — выход\n"
            "  Щипок (большой + указательный) — взять объект, отпустить — оставить (запомнится)")
    ttk.Label(info_tab, text=text, justify="left").pack(anchor="w", padx=10, pady=10)

    # --- правая колонка: журнал
    status = tk.StringVar(value="Готово")
    status_label = ttk.Label(right, textvariable=status, style="Status.TLabel")
    status_label.pack(anchor="w", padx=6)

    def set_status(text, error=False):
        status.set(text)
        status_label.configure(style="Error.TLabel" if error else "Status.TLabel")
    log_frame = ttk.Frame(right)
    log_frame.pack(fill="both", expand=True, padx=6, pady=4)
    log = tk.Text(log_frame, wrap="none", height=10, bg="#111418", fg="#d8dee9", insertbackground="#d8dee9",
                  font=("DejaVu Sans Mono", 9), state="disabled")
    sb = ttk.Scrollbar(log_frame, command=log.yview)
    log.configure(yscrollcommand=sb.set)
    log.pack(side="left", fill="both", expand=True)
    sb.pack(side="right", fill="y")
    log.tag_configure("err", foreground="#ff6b6b")
    log.tag_configure("ok", foreground="#7ee787")
    log.tag_configure("cmd", foreground="#79c0ff")

    lines = queue.Queue()
    procs = {}
    stopped = set()   # остановлено кнопкой «Стоп» — это не ошибка

    def append(text, tag=None):
        log.configure(state="normal")
        log.insert("end", text, tag)
        if int(log.index("end-1c").split(".")[0]) > 6000:
            log.delete("1.0", "1000.0")
        log.see("end")
        log.configure(state="disabled")

    def pump():
        try:
            while True:
                text, tag = lines.get_nowait()
                append(text, tag)
        except queue.Empty:
            pass
        root.after(60, pump)

    def classify(line):
        low = line.lower()
        if "error" in low or "fail" in low or "ошибк" in low or "vuid" in low:
            return "err"
        if line.startswith("[OK]") or "pass" in low or "✅" in line or "built target" in low:
            return "ok"
        return None

    def run_steps(name, steps, on_done=None):
        """steps: список (cmd, cwd, env). Выполняются по очереди в фоне, вывод — в журнал."""
        if name in procs:
            messagebox.showinfo("GlaskiOS", f"«{name}» уже выполняется")
            return

        def worker():
            code = 0
            for cmd, cwd, env in steps:
                lines.put(("$ " + " ".join(shlex.quote(c) for c in cmd) + "\n", "cmd"))
                try:
                    p = subprocess.Popen(cmd, cwd=cwd, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                         text=True, bufsize=1, start_new_session=True)
                except OSError as e:
                    lines.put((f"не удалось запустить: {e}\n", "err"))
                    code = 1
                    break
                procs[name] = p
                for line in p.stdout:
                    lines.put((line, classify(line)))
                code = p.wait()
                if name in stopped:
                    break
                if code != 0:
                    lines.put((f"[{name}] завершилось с кодом {code}\n", "err"))
                    break
            procs.pop(name, None)
            if name in stopped:
                stopped.discard(name)
                root.after(0, lambda: set_status(f"{name}: остановлено"))
                return
            root.after(0, lambda: set_status(f"{name}: {'готово' if code == 0 else 'ошибка'}", code != 0))
            if code == 0:
                lines.put((f"[{name}] готово\n", "ok"))
                if on_done:
                    root.after(0, on_done)

        set_status(f"{name}…")
        threading.Thread(target=worker, daemon=True).start()

    def build_steps():
        c = collect()
        if stale_cache(c):
            (build_dir(c) / "CMakeCache.txt").unlink()
            lines.put(("Старый CMakeCache из другой папки удалён\n", None))
        build_dir(c).mkdir(parents=True, exist_ok=True)
        return [(configure_command(c), str(ROOT), None), (build_command(c), str(ROOT), None)]

    def do_build():
        missing = missing_tools()
        if missing:
            messagebox.showerror("GlaskiOS", "Не найдено: " + ", ".join(missing) +
                                 "\nsudo apt install cmake build-essential glslang-tools")
            return
        run_steps("Сборка", build_steps())

    def do_clean_build():
        c = collect()
        d = build_dir(c)
        if d.exists() and messagebox.askyesno("GlaskiOS", f"Удалить {d} и собрать заново?"):
            shutil.rmtree(d)
        do_build()

    def do_run():
        c = collect()
        exe = build_dir(c) / "bin" / "ARLauncher"
        if not exe.exists():
            messagebox.showwarning("GlaskiOS", "Лаунчер ещё не собран — нажмите «Собрать и запустить»")
            return
        env = dict(os.environ, **launcher_env(c))
        lines.put(("env: " + " ".join(f"{k}={v}" for k, v in launcher_env(c).items()) + "\n", "cmd"))
        run_steps("Лаунчер", [([str(exe)], str(build_dir(c)), env)])

    def do_build_and_run():
        missing = missing_tools()
        if missing:
            messagebox.showerror("GlaskiOS", "Не найдено: " + ", ".join(missing))
            return
        run_steps("Сборка", build_steps(), on_done=do_run)

    def do_simulator():
        c = collect()
        sim = build_dir(c) / "SensorStream" / "stream_simulator"
        if not sim.exists():
            messagebox.showwarning("GlaskiOS", "Симулятор не собран (вкладка «Сборка» → «Симулятор»)")
            return
        cmd = [str(sim), "127.0.0.1", "9001", "--seconds", "3600", "--size", "960x720", "--fps", "30", "--depth"]
        if c.get("SIM_ROOM", True):
            cmd.append("--room")
        if c.get("SIM_EYES", True):
            cmd.append("--eyes")
        if c.get("SIM_AUDIO", True):
            cmd.append("--audio")
        run_steps("Симулятор", [(cmd, str(build_dir(c)), None)])

    def do_openvins():
        script = ROOT / "third_party" / "build_openvins.sh"
        if not script.exists():
            messagebox.showerror("GlaskiOS", f"Нет {script}")
            return
        run_steps("OpenVINS", [(["bash", str(script)], str(ROOT), None)])

    def do_stop():
        for name, p in list(procs.items()):
            stopped.add(name)
            try:
                os.killpg(p.pid, signal.SIGINT if name == "Лаунчер" else signal.SIGTERM)
            except OSError:
                pass
            lines.put((f"[{name}] остановлено\n", None))

    def do_reset():
        if messagebox.askyesno("GlaskiOS", "Вернуть все настройки по умолчанию?"):
            for key, value in default_config().items():
                if key in variables:
                    variables[key][0].set(value)
            collect()

    # --- кнопки
    bar = ttk.Frame(root)
    bar.pack(fill="x", padx=8, pady=(8, 0), before=paned)
    ttk.Button(bar, text="▶ Собрать и запустить", style="Accent.TButton", command=do_build_and_run).pack(side="left", padx=3)
    ttk.Button(bar, text="Собрать", command=do_build).pack(side="left", padx=3)
    ttk.Button(bar, text="Запустить", command=do_run).pack(side="left", padx=3)
    ttk.Button(bar, text="■ Стоп", command=do_stop).pack(side="left", padx=3)
    ttk.Separator(bar, orient="vertical").pack(side="left", fill="y", padx=8)
    ttk.Button(bar, text="Симулятор iPhone", command=do_simulator).pack(side="left", padx=3)
    ttk.Button(bar, text="Собрать OpenVINS", command=do_openvins).pack(side="left", padx=3)
    ttk.Button(bar, text="Сборка с нуля", command=do_clean_build).pack(side="left", padx=3)
    ttk.Button(bar, text="По умолчанию", command=do_reset).pack(side="right", padx=3)

    def on_close():
        collect()
        do_stop()
        root.destroy()

    root.protocol("WM_DELETE_WINDOW", on_close)
    append(f"Проект: {ROOT}\nСборка: {build_dir(cfg)}\n")
    if stale_cache(cfg):
        append("В папке сборки CMakeCache от другого пути — при сборке он будет пересоздан\n", "err")
    missing = missing_tools()
    if missing:
        append("Не найдено: " + ", ".join(missing) + " (sudo apt install cmake build-essential glslang-tools)\n", "err")
    pump()
    root.mainloop()


if __name__ == "__main__":
    args = sys.argv[1:]
    if any(a in args for a in ("--build", "--run", "--print-env")):
        sys.exit(run_headless(args))
    run_gui()
