#include <SDL.h>
#include <cmath>
#include <vector>

struct Vec2 {
    float x, y;
    Vec2(float x = 0, float y = 0) : x(x), y(y) {}

    Vec2 operator+(const Vec2& v) const { return Vec2(x + v.x, y + v.y); }
    Vec2 operator-(const Vec2& v) const { return Vec2(x - v.x, y - v.y); }
    Vec2 operator*(float s) const { return Vec2(x * s, y * s); }

    float length() const { return std::sqrt(x * x + y * y); }
    Vec2 normalize() const {
        float len = length();
        return len > 0 ? Vec2(x / len, y / len) : *this;
    }
};

Vec2 lightPos(400, 300);
Vec2 sphereCenter(400, 300);
float sphereRadius = 50.0f;
bool draggingLight = false;

bool intersect(const Vec2& rayOrigin, const Vec2& rayDir, float& t) {
    Vec2 oc = rayOrigin - sphereCenter;
    float a = rayDir.x * rayDir.x + rayDir.y * rayDir.y;
    float b = 2.0f * (oc.x * rayDir.x + oc.y * rayDir.y);
    float c = oc.x * oc.x + oc.y * oc.y - sphereRadius * sphereRadius;
    float discriminant = b * b - 4 * a * c;

    if (discriminant < 0) return false;

    t = (-b - std::sqrt(discriminant)) / (2.0f * a);
    return t > 0.001f;
}

void drawCircle(SDL_Renderer* renderer, Vec2 center, float radius) {
    const int segments = 32;
    for (int i = 0; i < segments; ++i) {
        float angle1 = 2 * M_PI * i / segments;
        float angle2 = 2 * M_PI * (i + 1) / segments;

        SDL_RenderDrawLine(renderer,
            static_cast<int>(center.x + radius * cos(angle1)),
            static_cast<int>(center.y + radius * sin(angle1)),
            static_cast<int>(center.x + radius * cos(angle2)),
            static_cast<int>(center.y + radius * sin(angle2))
        );
    }
}

int main(int argc, char** argv) {
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow("Interactive Raytracer",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 600, SDL_WINDOW_SHOWN);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    bool running = true;

    while (running) {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;

            if (event.type == SDL_MOUSEBUTTONDOWN) {
                Vec2 mouse(event.button.x, event.button.y);
                if ((mouse - lightPos).length() < 20) {
                    draggingLight = true;
                }
            }

            if (event.type == SDL_MOUSEBUTTONUP) {
                draggingLight = false;
            }

            if (event.type == SDL_MOUSEMOTION && draggingLight) {
                lightPos.x = event.motion.x;
                lightPos.y = event.motion.y;
            }
        }

        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 0, 120, 200, 255);
        drawCircle(renderer, sphereCenter, sphereRadius);

        const int numRays = 360;
        for (int i = 0; i < numRays; ++i) {
            float angle = 2 * M_PI * i / numRays;
            Vec2 dir(std::cos(angle), std::sin(angle));

            float t;
            if (intersect(lightPos, dir, t)) {
                Vec2 hitPoint = lightPos + (dir * t);
                SDL_SetRenderDrawColor(renderer, 255, 255, 0, 100);
                SDL_RenderDrawLine(renderer,
                    static_cast<int>(lightPos.x), static_cast<int>(lightPos.y),
                    static_cast<int>(hitPoint.x), static_cast<int>(hitPoint.y)
                );
            }
            else {
                Vec2 end = lightPos + (dir * 1000.0f);
                SDL_SetRenderDrawColor(renderer, 255, 255, 0, 50);
                SDL_RenderDrawLine(renderer,
                    static_cast<int>(lightPos.x), static_cast<int>(lightPos.y),
                    static_cast<int>(end.x), static_cast<int>(end.y)
                );
            }
        }

        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        drawCircle(renderer, lightPos, 20);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
