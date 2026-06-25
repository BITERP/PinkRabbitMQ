# Новое в версии 2.6 (fork)

## Фаза 4 — исправления
* Quorum-очереди: `BasicConsume` не вызывает `basic.qos` при `selectSize = 0`; при `selectSize > 0` — per-consumer QoS (`global = false`) ([#90](https://github.com/BITERP/PinkRabbitMQ/issues/90)).
* Убрана искусственная задержка после `BasicAck`/`BasicReject` ([#99](https://github.com/BITERP/PinkRabbitMQ/issues/99)).
* После `AMQP server timeout error` async-колбэки сбрасываются через `LoopCallbackGuard` ([#77](https://github.com/BITERP/PinkRabbitMQ/issues/77)); таймаут операции не помечает TCP-соединение потерянным.
* `BasicConsumeMessage` не бросает исключений: пустая очередь, потеря связи, таймаут ожидания → `Ложь` + `GetLastError` ([#98](https://github.com/BITERP/PinkRabbitMQ/issues/98), [#93](https://github.com/BITERP/PinkRabbitMQ/issues/93)).
* Проверка длины routing key / exchange / queue (лимит AMQP 255 байт) вместо `FRAME_ERROR` ([#82](https://github.com/BITERP/PinkRabbitMQ/issues/82)).
* Linux: `ioMutex` вокруг `TcpConnection::process()` — устраняет `frame size exceeded` при одновременном consume и publish ([#51](https://github.com/BITERP/PinkRabbitMQ/issues/51)).
* CI Linux: сборка в `debian:bullseye-slim` (glibc 2.31) на `ubuntu-22.04` runner — совместимость с Ubuntu 18.04–20.04 ([#100](https://github.com/BITERP/PinkRabbitMQ/issues/100)); образ `ubuntu-20.04` снят GitHub Actions.
* Безопасный повторный `Connect`: корректное закрытие SSL-сокета и остановка I/O-потока перед новым подключением ([#89](https://github.com/BITERP/PinkRabbitMQ/issues/89)).
* `clear()` полностью уничтожает соединение; безопасное переоткрытие каналов после ошибок брокера ([#79](https://github.com/BITERP/PinkRabbitMQ/issues/79), [#65](https://github.com/BITERP/PinkRabbitMQ/issues/65)).
* Повторный `BasicPublish` после `NOT_FOUND`: канал переоткрывается без таймаута (`releaseChannel`, получение канала вне `withIoLock`) ([#12](https://github.com/BITERP/PinkRabbitMQ/issues/12)).

# Новое в версии 2.5 (fork)

## Фаза 3 — обрыв связи и стабильность соединения
* При потере TCP/AMQP-соединения вызывается `notifyLost`: разблокируется `loop()`, `BasicConsumeMessage` получает ошибку через `GetLastError` ([#88](https://github.com/BITERP/PinkRabbitMQ/issues/88)).
* Linux: AMQP heartbeat — `onNegotiate`/`onHeartbeat` и периодическая отправка `heartbeat()` в event-loop ([#49](https://github.com/BITERP/PinkRabbitMQ/issues/49)).
* `Connect` прогревает транзакционный канал до возврата управления — устраняет гонку «Connect → DeclareExchange» на Linux ([#65](https://github.com/BITERP/PinkRabbitMQ/issues/65)).
* Корректное закрытие: `shutdown()` ждёт AMQP close handshake до разрыва TCP ([#26](https://github.com/BITERP/PinkRabbitMQ/issues/26)).
* Windows: однократная инициализация SSL (`std::call_once`) вместо init/uninit на каждое подключение ([#89](https://github.com/BITERP/PinkRabbitMQ/issues/89)).
* Windows: снижена нагрузка на CPU в фоновом цикле чтения сокета (пауза 1 мс при отсутствии данных).
* Windows: обрыв сокета (`ConnectionReset`, `receiveBytes == 0`) и `onClosed`/`onError` AMQP пробрасываются наверх.
* CI Linux: сборка на `ubuntu-22.04` (ранее пробовали `ubuntu-20.04` для GLIBC 2.31 — образ снят GitHub).
* Таймаут открытия канала вместо бесконечного ожидания.

# Новое в версии 2.4 (fork)

## Фаза 2 — стабильность и большие сообщения
* Синхронизация AMQP I/O: mutex между потоком чтения и вызовами publish/consume ([#51](https://github.com/BITERP/PinkRabbitMQ/issues/51), [#103](https://github.com/BITERP/PinkRabbitMQ/issues/103), [#104](https://github.com/BITERP/PinkRabbitMQ/issues/104)).
* Безопасное закрытие соединения и каналов при `clear()`/деструкторе ([#95](https://github.com/BITERP/PinkRabbitMQ/issues/95), [#79](https://github.com/BITERP/PinkRabbitMQ/issues/79), [#94](https://github.com/BITERP/PinkRabbitMQ/issues/94)).
* Linux: `closeChannel` теперь вызывает `channel->close()` как на Windows.
* Валидация типа параметра порта в `Connect` — ошибка вместо падения ([#24](https://github.com/BITERP/PinkRabbitMQ/issues/24)).
* `clear()` отменяет активных consumers на брокере перед закрытием ([#94](https://github.com/BITERP/PinkRabbitMQ/issues/94)).

## Фаза 1 — ошибки без падений
* BasicConsume: проверка существования очереди перед подпиской ([#107](https://github.com/BITERP/PinkRabbitMQ/issues/107)).
* BasicPublish/BatchPublish: проверка существования exchange, флаг mandatory и обработка returned messages ([#12](https://github.com/BITERP/PinkRabbitMQ/issues/12)).
* BasicConsumeMessage: ошибки без исключений — возврат Ложь + GetLastError ([#54](https://github.com/BITERP/PinkRabbitMQ/issues/54), [#98](https://github.com/BITERP/PinkRabbitMQ/issues/98)).

# Новое в версии 2.3 (fork)

* Исправлена высокая загрузка CPU на Windows и Linux при ожидании соединения (busy-wait в event loop).
* Добавлена отправка AMQP heartbeat на Windows.
* BasicCancel теперь отменяет consumer на брокере, а не только очищает локальный кеш.
* BasicPublish и BatchPublish используют publisher confirms вместо транзакций.
* BasicReject: добавлен необязательный параметр requeue.
* Добавлены методы BatchPublish и GetQueueMessageCount.
* Исправлена работа с большими сообщениями (корректная длина тела).
* GetHeaders возвращает вложенные array/table заголовки.
* Таймаут Connect берётся из параметра метода (по умолчанию 5 секунд).
* Добавлен CI workflow с тестами на Linux и Windows.

# Новое в версии 2.2
* Обновлены библиотеки POCO и AMQP-CPP.
* Добавлена поддержка TLS 1.3

# Новое в версии 2.1
* В метод BasicConsume добавлены произвольные свойства.

# Новое в версии 2.0
* Проект компоненты переписан под кроссплатформенный шаблон
* Реализованы флаги exclusive и noConfirm в методах BasicConsume и DeclareQueue
* Добавлены тесты

# Новое в версии 1.11
* Добавлен новый метод GetHeaders()
* Исправлена ошибка "client unexpectedly closed TCP connection" при закрытии соединения
* Добавлен таймаут на ожидание ответа от сервера RabbitMQ

# Новое в версии 1.10
* Добавлена поддержка TLS 1.2
* Устранены утечки памяти
* Скорость отправки сообщений для Windows составляет на отправку - 500 8кб сообщений/сек и на получение - 1500 8кб сообщений/сек
* Добавлены произвольные свойства в методы DeclareExchange, DeclareQueue, BindQueue, BasicPublish
* Добавлен новый метод GetRoutingKey()

# Новое в версии 1.9
* Исправлена ошибка - не работает метод GetPriority
* Исправлена ошибка отправки сообщений > 10 мб методом BasicPublish
* Исправлена ошибка - Десктопная компонента зависает при вводе правильного сервера, но неправильного логина
# Новое в версии 1.8
* Добавлена поддержка 64 битных Linux систем. 
* Внешняя компонента для Linux входит в общий zip макет компоненты, но по сути является отдельный sln проектом. Однако протокол AMQP реализован через ту же самую библиотеку  - AMQP-CPP, как и для Windows. Обмен TCP с сервером RAbbitMQ реализован через новую библиотеку libevent
* Скорость отправки сообщений для Linux компоненты отличается от Windows компоненты и составляет на отправку - 1000 8кб сообщений/сек и на получение - 4000 8кб сообщений/сек (для Windows - 30 и 1000 8кб сообщений/сек соответственно)

# Новое в версии 1.7
* Исправление критичной ошибки [#14](https://github.com/BITERP/PinkRabbitMQ/issues/14), наведенной в версии 1.6.

# Новое в версии 1.6

* Оптимизирован метод basicConsumeMessage. Таким образом, скорость чтения сообщений увеличена до примерно 1000 сообщений в сек размером 8 кб.
* Реализован параметр selectSize в методе basicConsume, который позволяет изменять размер забор сообщений из очереди. Рекомендуемый диапазон 100-1000.
* Добавлены новый параметр messageTag в методы BasicConsumeMessage, BasickAck, BasicNack.
* Исправлена ошибка зависания компоненты, если неправильно указаны параметры авторизации к серверу RabbitMQ (логин, пароль или vhost)
* Добавлена поддержка свойства сообщения  priority (метод SetPriority)
* Реализован параметр persistent для метода basicPublish 

# Новое в версии 1.5

* Добавлены методы SetPriority и GetPriority
* Добавлен параметр priority в метод DeclareQueue

# Новое в версии 1.4

* Добавлены следующие транзитные свойства: AppId, ContentEncoding, ContentType, UserId, ClusterId, Expiration, ReplyTo

# Новое в версии 1.3

* Добавлено транзитное свойство компоненты AppId. В данное свойство можно передавать дополнительную произвольную информацию вместе с сообщением в RabbitMQ.

# Новое в версии 1.2

* Исправлена ошибка падения компоненты при вызове любого метода перед методом Connect()
* Переименован тип компоненты. Вместо NativeRabbitMQ теперь PinkRabbitMQ
* Первый публичный релиз компоненты

# Новое в версии 1.1

* Исправлена ошибка вида channel not Usable

# Новое в версии 1.0

* Первый приватный релиз компоненты 32 и 64 бит.
