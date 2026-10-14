# activity-monitor-agent

Кроссплатформенный консольный агент мониторинга активности пользователя на C++20
для Windows и Linux.

В фоновом режиме собирает метрики активного окна и ввода пользователя, буферизует
их в потокобезопасной очереди, отправляет пакетами по HTTP и сохраняет неотправленные
данные при завершении.

## Возможности

- Сбор метрик каждые 5 секунд:
  - имя процесса активного окна;
  - заголовок окна;
  - факт активности пользователя за последние 5 секунд.
- Потокобезопасная очередь с ограничением на 100 записей.
- Батчинг: отправка при накоплении 10 записей или раз в 30 секунд - что раньше.
- HTTP `POST` с `Content-Type: application/json` через `cpp-httplib`.
- Retry при недоступности сервера без потери данных.
- Graceful shutdown `SIGINT` (`Ctrl+C`), `SIGTERM` (`kill`), `SIGHUP` (закрытие терминала) с сохранением остатка в `backup.json`.
- Восстановление неотправленных метрик из `backup.json` при следующем запуске.

## Поддержка платформ

| | Windows | Linux |
|---|---|---|
| Платформа | Windows 10 | Ubuntu 24.04 |
| Сборка |  MSVC 19.3x |  GCC 13.x |
| Метрики |  WinAPI |  X11 + XScreenSaver |


**Linux реализация требует сессии X11.** Wayland не поддерживается. На Wayland-сессии агент запустится, но поля `process_name`,
`window_title`, `user_active` будут пустыми.

## Требования

- **C++20**: протестировано на **MSVC 19.3x** (Windows), **GCC 13.3** (Linux (Ubuntu 24.04)).  
- **CMake** >= 3.28.
- **Ninja** - поставляется с Visual Studio или ставится через `apt`.
- **Python 3** - для демонстрационного сервера.

Инструменты разработчика:
- **clang-format** - для форматирования кода.

Зависимости подтягиваются автоматически через CMake FetchContent:
- `nlohmann/json` - сериализация.
- `cpp-httplib` - HTTP-клиент.
- `googletest` - unit-тесты.

Системные зависимости Linux (Debian/Ubuntu):
 - `libx11-dev` - X11 API (окна, свойства).
 - `libxss-dev` - XScreenSaver extension (idle time).
 - `libxext-dev` - X11 extensions.

## Сборка

Клонировать репозиторий github
```bash
git clone https://github.com/voks-dev/activity-monitor-agent.git
cd activity-monitor-agent
```
### Windows

Сборка предполагает окружение Developer Command Prompt for Visual Studio. 
Откройте его через меню Пуск -> Visual Studio -> Developer Command Prompt. В нём доступны ninja и cl.exe.

```bash
cmake --preset debug
cmake --build --preset debug
```

Release:

```bash
cmake --preset release
cmake --build --preset release
```
Альтернативные варианты: 

Сборка через **CMD** 

**MSVC** компилятор (в зависимости от установленного генератора 
 "Visual Studio 18 2026" или "Visual Studio 17 2022" )

```bash
cmake -S . -B build -G "Visual Studio 18 2026" 
cmake --build build --config Debug
```

### Linux

```bash
sudo apt install libx11-dev libxss-dev libxext-dev
```

**GCC** компилятор

```bash
cmake --preset debug
cmake --build --preset debug
```

Требуется **X11-сессия**. Проверка:

```bash
echo $XDG_SESSION_TYPE    # должно быть x11
```

Если `wayland` - выйдите из сессии, на экране логина выберите **Ubuntu on Xorg**
(в Ubuntu 24.04 и ниже) или **Xfce Session** (если установлена Xfce).

## Запуск

1. Запустите демонстрационный сервер в отдельном терминале:

   ```python
   # test_server.py
   from http.server import HTTPServer, BaseHTTPRequestHandler

   class Handler(BaseHTTPRequestHandler):
       def do_POST(self):
           length = int(self.headers['Content-Length'])
           body = self.rfile.read(length)
           print("=== received ===")
           print(self.headers.get('Content-Type'))
           print(body.decode('utf-8'))
           self.send_response(200)
           self.end_headers()

   HTTPServer(('127.0.0.1', 8080), Handler).serve_forever()
   ```

   ```bash
   python test_server.py
   ```

2. Запустите агент:

  ```bash
   # Windows
   ./build/debug/activity-monitor-agent.exe

   # Linux
   ./build/debug/activity-monitor-agent
   ```

3. Для завершения нажмите `Ctrl+C` или завершите процесс activity-monitor-agent.
   Неотправленные метрики сохранятся в `backup.json`
   в **текущей рабочей директории**.


## Формат данных

POST на `http://127.0.0.1:8080/` с телом:

```json
{
  "agent_id": "DESKTOP-XXXXXXX",
  "timestamp": 1789921904,
  "payload": [
    {
      "time": "2026-09-20 21:31:14",
      "process_name": "chrome.exe",
      "window_title": "GitHub",
      "user_active": true
    }
  ]
}
```

- `agent_id` - имя компьютера, фиксировано на время работы.
- `timestamp` - Unix-время формирования пакета.
- `payload` - массив метрик.

## Архитектура

```
src/
├── core/                - платформо-независимая логика
│   ├── interface/       - абстракции
│   │   ├── metrics_collector.h
│   │   ├── transport.h
│   │   ├── system_info.h
│   │   └── stop_signal.h
│   ├── metrics.h        - структура одной метрики
│   ├── metrics_json.h   - to_json/from_json для Metrics
│   ├── thread_safe_queue.h
│   ├── packet_builder.* - формирование JSON-пакета
│   ├── backup.*         - сохранение/загрузка backup.json
│   ├── agent.*          - потоки сбора и отправки
│   └── application.*    - composition root: сборка компонентов
├── platform/
│   ├── platform.h       - выбор платформы (type aliases)
│   ├── windows/         - реализация под Windows (WinAPI)
│   │   ├── wide_to_utf8.*
│   │   ├── windows_metrics_collector.*
│   │   ├── windows_stop_signal.cpp
│   │   └── windows_system_info.*
│   └── linux/           - реализация под Linux (POSIX + X11)
│       ├── linux_metrics_collector.*
│       ├── linux_stop_signal.cpp
│       └── linux_system_info.*
├── transport/
│   ├── console/         - ConsoleTransport для тестов
│   └── http/            - HttpTransport (cpp-httplib)
├── main.cpp             - установка stop handler, запуск Application

tests/                   - unit-тесты GTest
├── core/
│   ├── test_backup.cpp
│   ├── test_metrics_json.cpp
│   ├── test_packet_builder.cpp
│   └── test_thread_safe_queue.cpp
└── CMakeLists.txt
```
## Ключевые решения

**Разделение `core` / `platform` / `transport`.**
`core/` не знает про ОС. Платформенные реализации лежат в `platform/windows/`
и `platform/linux/`. `core/interface/` содержит абстракции (`MetricsCollector`,
`Transport`, `SystemInfo`, `StopSignal`). Выбор конкретной реализации -
через `platform/platform.h` (`#if defined(_WIN32) / #elif defined(__linux__)`+ 
`using` aliases). `main` и `core/` не меняются при добавлении платформы.

**`cpp-httplib` вместо libcurl или Boost.Asio.**
Задача - POST JSON на localhost. `cpp-httplib` - header-only, кроссплатформенный,
минимальная зависимость. 

**Очередь на `std::mutex` + `std::condition_variable`.**
Для двух потоков (producer-consumer) мьютекс эффективнее lock-free. `pop`
использует предикат в `wait` - защита от ложных пробуждений и от гонки, когда `stop`
вызван между проверкой флага и входом в ожидание. Поток-отправитель использует
неблокирующие `peekAll` / `removeFront` - сетевые задержки не блокируют сбор.

**Retry с прерываемым ожиданием.**
При недоступности сервера данные не удаляются из очереди. Отправитель ждёт 10 секунд
через `cv_.wait_for` с предикатом - при `stop` просыпается мгновенно, не ждёт полный
интервал.

**Backup при shutdown.**
Обработчик сигналов только выставляет флаг и уведомляет `main`. Вся работа - в `main`:
`agent.stop()` останавливает потоки, затем `queue.extractAll()` забирает остаток,
`Backup::save` пишет `backup.json`.

- **Windows**: `SetConsoleCtrlHandler` + `CreateEvent`. Windows убивает процесс
  сразу после возврата из обработчика (в отличие от POSIX, где сигнал - асинхронный
  и процесс продолжает работу). Поэтому обработчик ждёт через
  `WaitForSingleObject` событие `shutdownComplete`, которое выставляет
  `main` после сохранения backup.
- **Linux**: сигналы блокируются через `pthread_sigmask`, ожидаются синхронно
  через `sigwait`. Никаких async-signal-safe ограничений - обработчика нет,
  код ожидания обычный.

**Мгновенная остановка потоков.**
Оба рабочих потока (`collectorLoop`, `transportLoop`) используют `cv_.wait_for`
с предикатом вместо `sleep_for`. Без этого `agent.stop()` ждал бы до 5 секунд, пока
поток проснётся из сна, и при закрытии через крестик cleanup не укладывался бы
в лимит Windows (~5 секунд). С `condition_variable` остановка занимает миллисекунды.

**`stop_signal` - свободные функции, а не класс.**
Обработчик сигнала регистрируется один на процесс, состояние глобальное. Оборачивать
в класс с экземплярами смысла нет - это соответствует природе WinAPI и POSIX.

**`agent_id` получается один раз.**
`GetComputerName` вызывается в Application и передаётся в `PacketBuilder`.
`core/` не зависит от `platform/`.

**Агент использует at-least-once семантику доставки**.
Батч удаляется из очереди только после успешной отправки (`send() == true`).
При сбое между успешным `send()` и `removeFront()` возможна повторная отправка того же батча.
Приёмник должен быть идемпотентным: дедупликация по `agent_id` + `timestamp` + `process_name` + `window_title`. 
Альтернатива - at-most-once (удаление до отправки) - отклонена, потому что потеря метрик для задачи мониторинга хуже дублирования.

## Обработка ошибок


- Catch на границах + signal-only abort из тела потока (без `join`).
- Сетевые ошибки (`send() == false`) -> retry через 10 секунд.
- Битый `backup.json` -> старт с пустой очередью.
- Битые записи в `backup.json` -> skip с логом, остальные грузятся.
- Ошибки WinAPI / POSIX -> пустые значения, не падение.

## Тесты

Используется **GoogleTest** (подтягивается через FetchContent).

Покрытие:

| Модуль | Что покрыто |
|---|---|
| `ThreadSafeQueue` | контракты методов, FIFO, вытеснение, `stop`/`pop` |
| `Backup` | save/load round-trip, битый JSON, lenient-политика |
| `PacketBuilder` | структура JSON, поля, значения |
| `MetricsJson` | round-trip `to_json` / `from_json` |

Запуск:

```bash
ctest --preset debug --output-on-failure
```

Фикстуры используют temp-файлы с уникальными именами (atomic counter) -
безопасно для параллельного запуска.

## Форматирование кода

Проект использует `clang-format` со стилем Google (`.clang-format` в корне).
Цель CMake:

```bash
cmake --build build --target format        # форматировать
```

## Ограничения и возможные улучшения

- **Wayland не поддерживается.** Требуется X11-сессия.
- **`backup.json` пишется в текущую рабочую директорию.** Для демона может
  потребоваться XDG-путь (`~/.config/activity-monitor/`).
- **Concurrent-тесты** покрывают только `ThreadSafeQueue` (два сценария).
  `Agent` и его взаимодействие с transport/mock - не покрыто.
- **Нет интеграционных тестов.**
- **Конфигурация** (URL сервера, размер очереди, путь к backup) задаётся
  константами в `application.cpp`.
- **`std::cerr` может бросить** при включённых исключениях потока или провале
  нижележащего write. Возможна реализация логгера с записью в файл.