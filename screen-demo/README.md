## Тестовый проект для работы с дисплеем

На базе этого проекта можно делать задачки покруче.

Для редактирования гуя используется SquareLine Studio. Она должна экспортировать файлы в отдельную директорию. После чего:  
1. Все файлы надо перенести в корень проекта
2. Сделать копию файлов ui_ScreenX.* в директории screens

Из зависимостей — LVGL версии 8.3.10  
В его конфигах надо прописать разные размеры шрифтов (там по умолчанию минимум)
И всякое по мелочи, что уже было — не помню.

Чтобы повернуть экран, см комменты вида // todo ROTATE

В планах — поддержка кириллицы. Для этого надо найти легковесный шрифт.

Дока по модулю — https://www.waveshare.com/wiki/ESP32-C6-LCD-1.47. В ней есть драйвер для инициализации экрана и есть данная версия lvgl.

И можно шить.