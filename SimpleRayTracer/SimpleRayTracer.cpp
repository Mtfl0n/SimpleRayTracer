#include <SDL.h>      // Библиотека SDL для работы с графикой и ввода
#include <cmath>      // Математические функции (sin, cos, sqrt и т.д.)
#include <vector>     // Стандартная библиотека для работы с контейнерами (здесь не используется, но оставлена)

// Структура Vec2 — это 2D-вектор с координатами x и y.
// Используется для представления позиций (например, источника света или центра сферы) и направлений.
struct Vec2 {
    float x, y; // Координаты вектора
    // Конструктор: задаёт значения x и y, по умолчанию 0
    Vec2(float x = 0, float y = 0) : x(x), y(y) {}

    // Оператор сложения: складывает два вектора покомпонентно
    Vec2 operator+(const Vec2& v) const { return Vec2(x + v.x, y + v.y); }

    // Оператор вычитания: вычитает один вектор из другого
    Vec2 operator-(const Vec2& v) const { return Vec2(x - v.x, y - v.y); }

    // Оператор умножения на скаляр: масштабирует вектор
    Vec2 operator*(float s) const { return Vec2(x * s, y * s); }

    // Метод length(): вычисляет длину вектора по теореме Пифагора
    float length() const { return std::sqrt(x * x + y * y); }

    // Метод normalize(): возвращает нормализованный вектор (длиной 1).
    // Если длина = 0, возвращает исходный вектор, чтобы избежать деления на 0.
    Vec2 normalize() const {
        float len = length();
        return len > 0 ? Vec2(x / len, y / len) : *this;
    }
};

// Глобальные переменные для управления сценой:
Vec2 lightPos(400, 300);    // Позиция источника света (жёлтый круг), изначально в центре окна
Vec2 sphereCenter(400, 300); // Центр сферы (синий круг), тоже в центре окна
float sphereRadius = 50.0f;  // Радиус сферы в пикселях
bool draggingLight = false;  // Флаг: перетаскивается ли источник света мышью

// Функция intersect: проверяет, пересекает ли луч сферу.
// Это ключевая часть трассировки лучей.
// Параметры:
// - rayOrigin: точка начала луча (позиция источника света)
// - rayDir: направление луча (нормализованный вектор длиной 1)
// - t: выходной параметр — расстояние от начала луча до точки пересечения
bool intersect(const Vec2& rayOrigin, const Vec2& rayDir, float& t) {
    // Вектор от начала луча до центра сферы
    Vec2 oc = rayOrigin - sphereCenter;

    // Коэффициенты квадратного уравнения для пересечения луча и окружности:
    // a = (rayDir.x)^2 + (rayDir.y)^2. Так как rayDir нормализован, a = 1.
    float a = rayDir.x * rayDir.x + rayDir.y * rayDir.y;

    // b = 2 * скалярное произведение oc и rayDir
    float b = 2.0f * (oc.x * rayDir.x + oc.y * rayDir.y);

    // c = (длина oc)^2 - (радиус сферы)^2
    float c = oc.x * oc.x + oc.y * oc.y - sphereRadius * sphereRadius;

    // Дискриминант: b^2 - 4ac. Определяет, есть ли пересечение.
    float discriminant = b * b - 4 * a * c;

    // Если дискриминант < 0, пересечений нет (луч не касается сферы)
    if (discriminant < 0) return false;

    // Вычисляем меньший корень уравнения (ближайшую точку пересечения)
    t = (-b - std::sqrt(discriminant)) / (2.0f * a);

    // Проверяем, что t > 0.001 (луч идёт вперёд и не начинается внутри сферы)
    return t > 0.001f;
}

// Функция drawCircle: рисует окружность на экране.
// Используется для отрисовки сферы и источника света.
// Параметры:
// - renderer: объект SDL для рисования
// - center: центр окружности
// - radius: радиус окружности
void drawCircle(SDL_Renderer* renderer, Vec2 center, float radius) {
    const int segments = 32; // Количество линий для аппроксимации окружности
    for (int i = 0; i < segments; ++i) {
        // Углы для текущего и следующего сегмента (в радианах)
        float angle1 = 2 * M_PI * i / segments;
        float angle2 = 2 * M_PI * (i + 1) / segments;

        // Рисуем линию между двумя точками на окружности, используя тригонометрию
        SDL_RenderDrawLine(renderer,
            static_cast<int>(center.x + radius * cos(angle1)), // x1
            static_cast<int>(center.y + radius * sin(angle1)), // y1
            static_cast<int>(center.x + radius * cos(angle2)), // x2
            static_cast<int>(center.y + radius * sin(angle2))  // y2
        );
    }
}

// Главная функция программы
int main(int argc, char** argv) {
    // Инициализация SDL для работы с видео
    SDL_Init(SDL_INIT_VIDEO);

    // Создание окна размером 800x600 пикселей с заголовком "Interactive Raytracer"
    SDL_Window* window = SDL_CreateWindow("Interactive Raytracer",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, // Центрируем окно
        800, 600, SDL_WINDOW_SHOWN);                    // Размер и видимость

    // Создание рендера с аппаратным ускорением и вертикальной синхронизацией
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    // Включаем режим смешивания цветов (для прозрачности лучей)
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // Флаг для основного цикла программы
    bool running = true;

    // Основной цикл: работает, пока running = true
    while (running) {
        SDL_Event event; // Структура для хранения событий (мышь, клавиатура и т.д.)

        // Обрабатываем все события в очереди
        while (SDL_PollEvent(&event)) {
            // Если пользователь закрыл окно, выходим из цикла
            if (event.type == SDL_QUIT) running = false;

            // Нажатие кнопки мыши
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                // Получаем координаты клика мыши
                Vec2 mouse(event.button.x, event.button.y);

                // Проверяем, попал ли клик в область источника света (радиус 20 пикселей)
                if ((mouse - lightPos).length() < 20) {
                    draggingLight = true; // Включаем режим перетаскивания
                }
            }

            // Отпускание кнопки мыши
            if (event.type == SDL_MOUSEBUTTONUP) {
                draggingLight = false; // Отключаем перетаскивание
            }

            // Движение мыши, если перетаскивание активно
            if (event.type == SDL_MOUSEMOTION && draggingLight) {
                // Обновляем позицию источника света в реальном времени
                lightPos.x = event.motion.x;
                lightPos.y = event.motion.y;
            }
        }

        // Очистка экрана: заливаем его тёмно-серым цветом (RGB: 30, 30, 30, непрозрачный)
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);

        // Рисуем сферу синим цветом (RGB: 0, 120, 200, непрозрачный)
        SDL_SetRenderDrawColor(renderer, 0, 120, 200, 255);
        drawCircle(renderer, sphereCenter, sphereRadius);

        // Генерация лучей из источника света
        const int numRays = 360; // 360 лучей — по одному на каждый градус
        for (int i = 0; i < numRays; ++i) {
            // Угол луча в радианах (от 0 до 2π)
            float angle = 2 * M_PI * i / numRays;

            // Направление луча: единичный вектор, вычисленный через cos и sin
            Vec2 dir(std::cos(angle), std::sin(angle));

            float t; // Расстояние до точки пересечения (будет вычислено в intersect)

            // Проверяем, пересекает ли луч сферу
            if (intersect(lightPos, dir, t)) {
                // Если пересечение есть, вычисляем точку пересечения:
                // позиция света + направление * расстояние
                Vec2 hitPoint = lightPos + (dir * t);

                // Рисуем жёлтую линию (RGB: 255, 255, 0) с прозрачностью 100
                SDL_SetRenderDrawColor(renderer, 255, 255, 0, 100);
                SDL_RenderDrawLine(renderer,
                    static_cast<int>(lightPos.x), static_cast<int>(lightPos.y), // Начало: источник света
                    static_cast<int>(hitPoint.x), static_cast<int>(hitPoint.y)  // Конец: точка пересечения
                );
            }
            else {
                // Если пересечения нет, рисуем луч в направлении до "бесконечности" (1000 пикселей)
                Vec2 end = lightPos + (dir * 1000.0f);

                // Рисуем более прозрачную жёлтую линию (alpha = 50)
                SDL_SetRenderDrawColor(renderer, 255, 255, 0, 50);
                SDL_RenderDrawLine(renderer,
                    static_cast<int>(lightPos.x), static_cast<int>(lightPos.y), // Начало: источник света
                    static_cast<int>(end.x), static_cast<int>(end.y)            // Конец: далеко за пределами экрана
                );
            }
        }

        // Рисуем источник света жёлтым цветом (RGB: 255, 255, 0, непрозрачный)
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        drawCircle(renderer, lightPos, 20); // Радиус источника света = 20 пикселей

        // Отображаем всё нарисованное на экране
        SDL_RenderPresent(renderer);
    }

    // Очистка ресурсов перед завершением программы
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}   