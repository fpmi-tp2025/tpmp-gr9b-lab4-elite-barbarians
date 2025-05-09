# Парфюмерный базар

Система управления парфюмерным базаром для оптовых продаж через маклеров.

## Описание

Система позволяет управлять работой парфюмерного базара, включая:
- Управление информацией о маклерах, товарах и сделках
- Отслеживание поставок и продаж товаров
- Ведение статистики по маклерам и сделкам
- Разграничение прав доступа между администраторами и маклерами

## Требования

- GCC/G++ компилятор
- Make
- Google Test Framework
- pthread

## Установка

1. Склонируйте репозиторий:
```bash
git clone https://github.com/[username]/perfume-bazaar.git
```

2. Установите необходимые зависимости (Ubuntu/Debian):
```bash
sudo apt-get update
sudo apt-get install g++ make libgtest-dev
```

3. Соберите проект:
```bash
make all
```

## Использование

1. Запустите основное приложение:
```bash
make run
```

2. Для запуска тестов:
```bash
make test
```

3. При первом запуске зарегистрируйте администратора системы
4. Войдите в систему используя созданные учетные данные
5. Используйте меню для выполнения необходимых операций:
   - Администратор может управлять пользователями, просматривать все данные и добавлять сделки
   - Маклеры могут просматривать свои сделки и статистику

## Дополнительные команды Makefile

- `make clean` - очистка всех собранных файлов
- `make rebuild` - полная пересборка проекта
- `make test` - сборка и запуск тестов
- `make run` - сборка и запуск основного приложения

## Contributing

Горохов Иван
Шамына Алексей
