#include <SDL2/SDL.h>
#include <SDL2/SDL_log.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_video.h>
#include <stdbool.h>
#include <stdio.h>

SDL_Color bgColor = {.r = 0, .g = 0, .b = 0};
SDL_Color fgColor = {.r = 255, .g = 255, .b = 255, .a = SDL_ALPHA_OPAQUE};

typedef struct {
  bool running;
  int initialWidth;
  int initialHeight;
  SDL_Renderer *renderer;
  SDL_Window *window;
} AppState;

void CloseApp(AppState *state) { state->running = false; }

void DrawGrid(AppState *app) {
  int w, h;
  SDL_GetWindowSize(app->window, &w, &h);
  float blockWidth = (float)w / 7.0f;
  float blockHeight = (float)h / 5.0f;

  /* SDL_RenderSetScale(app->renderer, 2.0f, 2.0f); */
  SDL_SetRenderDrawColor(app->renderer, fgColor.r, fgColor.g, fgColor.b,
                         fgColor.a);

  for (int i = 1; i < 7; i++) {
    int offsetX = (int)blockWidth * i + 1;
    SDL_RenderDrawLine(app->renderer, offsetX, 0, offsetX, h);
  }

  for (int i = 1; i < 5; i++) {
    int offsetY = (int)blockHeight * i + 1;
    SDL_RenderDrawLine(app->renderer, 0, offsetY, w, offsetY);
  }
}

int main() {
  printf("Initializing calendar\n");

  AppState state = {.running = true, .initialWidth = 600, .initialHeight = 600};

  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    SDL_Log("Failed to initialize SDL: %s\n", SDL_GetError());
    return 1;
  }

  state.window = SDL_CreateWindow("Calendar", SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED, state.initialWidth,
                                  state.initialHeight, SDL_WINDOW_RESIZABLE);
  if (state.window == NULL) {
    SDL_Log("Failed to create window: %s\n", SDL_GetError());
    return 1;
  }

  state.renderer =
      SDL_CreateRenderer(state.window, -1, SDL_RENDERER_ACCELERATED);
  if (state.renderer == NULL) {
    SDL_Log("Failed to create renderer: %s\n", SDL_GetError());
    return 1;
  }

  while (state.running) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      SDL_Log("event: %d\n", e.type);

      switch (e.type) {
      case SDL_QUIT:
        CloseApp(&state);
        break;

      case SDL_KEYDOWN:
        if (e.key.keysym.sym == SDLK_ESCAPE || e.key.keysym.sym == SDLK_q) {
          CloseApp(&state);
          break;
        }
      }
    }

    SDL_RenderSetScale(state.renderer, 1.0f, 1.0f);
    SDL_SetRenderDrawColor(state.renderer, bgColor.r, bgColor.g, bgColor.b,
                           bgColor.a);
    SDL_RenderClear(state.renderer);

    DrawGrid(&state);

    SDL_RenderPresent(state.renderer);
    SDL_Delay(10);
  }

  SDL_Log("Exit calendar\n");
  SDL_DestroyRenderer(state.renderer);
  SDL_DestroyWindow(state.window);
  SDL_Quit();

  return 0;
}
