# activity-monitor-agent

Консольный агент мониторинга активности пользователя на C++20 для Windows.

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
- Graceful shutdown по `Ctrl+C` и закрытию консоли с сохранением остатка в `backup.json`.
- Восстановление неотправленных метрик из `backup.json` при следующем запуске.

## Требования

- **C++20**: Протестировано на **MSVC 19.3x** и **Clang 22** (MSVC target). 
Clang на Windows требует установленного MSVC - он использует его STL, Windows SDK и линкер. 
**MinGW-w64** не поддерживается из-за ограничений cpp-httplib.
- **CMake** ≥ 3.28.
- **Ninja** (поставляется с Visual Studio).
- **Windows** - единственная реализованная платформа.
- **Python 3** - для демонстрационного сервера.

Инструменты:
- **clang-format** - для форматирования кода.

Зависимости подтягиваются автоматически через CMake FetchContent:
- `nlohmann/json` - сериализация;
- `cpp-httplib` - HTTP-клиент.

## Сборка
Клонировать репозиторий github
```bash
git clone https://github.com/tworets/activity-monitor-agent.git
cd activity-monitor-agent
```
Сборка предполагает окружение Developer Command Prompt for Visual Studio. 
Откройте его через меню Пуск → Visual Studio → Developer Command Prompt. В нём доступны ninja и cl.exe.

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
**Clang** компилятор (MSVC target). 

```bash
cmake -S . -B build-clang -DCMAKE_CXX_COMPILER=clang++
cmake --build build-clang
```
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
   ./build/debug/activity-monitor-agent
   ```

3. Для завершения нажмите `Ctrl+C` или закройте окно. 
   Неотправленные метрики сохранятся в `backup.json` рядом с исполняемым файлом.


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

- `agent_id` - имя компьютера (`GetComputerNameW`), фиксировано на время работы.
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
│   ├── windows/         - реализация под Windows (WinAPI)
│   └── linux/           - зарезервировано
├── transport/
│   ├── console/         - ConsoleTransport для тестов
│   └── http/            - HttpTransport (cpp-httplib)
└── main.cpp             - установка stop handler, запуск Application
```

## Ключевые решения

**Разделение `core` / `platform` / `transport`.**
`core/` не знает про ОС. Реализации для Windows лежат в `platform/windows/`.
`core/interface/` содержит абстракции (`MetricsCollector`, `Transport`, `SystemInfo`,
`StopSignal`). При добавлении Linux достаточно реализовать их в `platform/linux/` -
`main` и `core/` не меняются. Платформенная зависимость в коде сосредоточена
в конструкторе `Application`.

**`cpp-httplib` вместо libcurl или Boost.Asio.**
Задача - POST JSON на localhost. `cpp-httplib` - header-only, кроссплатформенный,
минимальная зависимость. 

**Очередь на `std::mutex` + `std::condition_variable`.**
Для двух потоков (producer-consumer) мьютекс эффективнее lock-free. `pop` и `peek`
используют предикат в `wait` - защита от ложных пробуждений и от гонки, когда `stop`
вызван между проверкой флага и входом в ожидание. Поток-отправитель использует
неблокирующие `peekAll` / `removeFront` - сетевые задержки не блокируют сбор.

**Retry с прерываемым ожиданием.**
При недоступности сервера данные не удаляются из очереди. Отправитель ждёт 10 секунд
через `cv_.wait_for` с предикатом - при `stop` просыпается мгновенно, не ждёт полный
интервал.

**Backup при shutdown.**
Обработчик сигналов (`SetConsoleCtrlHandler`) только выставляет флаг и уведомляет
`main`. Вся работа - в `main`: `agent.stop()` останавливает потоки, затем
`queue.extractAll()` забирает остаток, `Backup::save` пишет `backup.json`. Для
`CTRL_CLOSE_EVENT` используется дополнительное событие `shutdownComplete` - Windows
убивает процесс сразу после возврата из обработчика, поэтому обработчик ждёт, пока
`main` сохранит данные.

**Мгновенная остановка потоков.**
Оба рабочих потока (`collectorLoop`, `transportLoop`) используют `cv_.wait_for`
с предикатом вместо `sleep_for`. Без этого `agent.stop()` ждал бы до 5 секунд, пока
поток проснётся из сна, и при закрытии через крестик cleanup не укладывался бы
в лимит Windows (~5 секунд). С `condition_variable` остановка занимает миллисекунды.

**`stop_signal` - свободные функции, а не класс.**
Обработчик сигнала регистрируется один на процесс, состояние глобальное. Оборачивать
в класс с экземплярами смысла нет - это соответствует природе WinAPI.

**`agent_id` получается один раз.**
`GetComputerNameW` вызывается в Application и передаётся в `PacketBuilder`.
`core/` не зависит от `platform/`.

**Агент использует at-least-once семантику доставки**.
Батч удаляется из очереди только после успешной отправки (`send() == true`).
При сбое между успешным `send()` и `removeFront()` возможна повторная отправка того же батча.
Приёмник должен быть идемпотентным: дедупликация по `agent_id` + `timestamp` + `process_name` + `window_title`. 
Альтернатива — at-most-once (удаление до отправки) — отклонена, потому что потеря метрик для задачи мониторинга хуже дублирования.

## Обработка ошибок


- Catch на границах + signal-only abort из тела потока (без `join`).
- Сетевые ошибки (`send() == false`) -> retry через 10 секунд.
- Битый `backup.json` -> старт с пустой очередью.
- Битые записи в `backup.json` -> skip с логом, остальные грузятся.
- Ошибки WinAPI -> пустые значения, не падение.

## Форматирование кода

Проект использует `clang-format` со стилем Google (`.clang-format` в корне).
Цель CMake:

```bash
cmake --build build --target format        # форматировать
```

## Кроссплатформенность

Вся логика в `core/` не зависит от платформы. Платформенная специфика изолирована
в `platform/windows/`. Чтобы добавить поддержку Linux:

1. Реализовать `MetricsCollector`, `SystemInfo`, `StopSignal` в `platform/linux/`.
2. Подключить соответствующие `.cpp` в CMake при сборке под Linux.
3. Заменить платформенные типы в конструкторе `Application`.

Интерфейсы в `core/interface/` уже описывают контракты - реализации под Linux должны
им соответствовать.

## Ограничения и возможные улучшения

- Реализована только Windows-версия. `platform/linux/` зарезервирована.
- Нет юнит-тестов (кроме ручной проверки очереди).
- Конфигурация (URL сервера, размер очереди, путь к backup) задаётся константами
  в `application.cpp`, а не через аргументы командной строки или конфиг-файл.
- Теоретически cerr может бросить, если у потока включены исключения (cerr.exceptions(...)) 
или если нижележащий write провалился. Возможна реалазиция логгера созданием журнала лога.