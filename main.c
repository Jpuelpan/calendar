#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <fontconfig/fontconfig.h>
#include <stdbool.h>
#include <stdio.h>

SDL_Color bgColor = {.r = 0, .g = 0, .b = 0, .a = SDL_ALPHA_OPAQUE};
SDL_Color fgColor = {.r = 255, .g = 255, .b = 255, .a = SDL_ALPHA_OPAQUE};
SDL_Texture *textures[31];

typedef struct {
  bool running;
  int w;
  int h;
  SDL_Renderer *renderer;
  SDL_Window *window;
} AppState;

void CloseApp(AppState *state) { state->running = false; }

bool LoadFont(void *buff, char *fontName, size_t buffSize) {
  FcConfig *config = NULL;

  if (!FcInit()) {
    SDL_Log("Failed to initialize FontConfig\n");
    return false;
  }

  config = FcInitLoadConfigAndFonts();
  if (config == NULL) {
    SDL_Log("Failed to load FontConfig configuration\n");
    return false;
  }

  if (fontName == NULL || strlen(fontName) == 0) {
    fontName = "Mono:style=Bold";
  }

  FcResult result;
  FcPattern *pat = FcNameParse((const FcChar8 *)fontName);

  FcConfigSubstitute(config, pat, FcMatchKindBegin);
  FcDefaultSubstitute(pat);
  SDL_Log("Looking for font: %s\n", fontName);

  FcPattern *font = FcFontMatch(config, pat, &result);
  if (font) {
    FcChar8 *file = NULL;
    if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch) {
      memcpy(buff, file, buffSize);
    }
  }

  FcPatternDestroy(font);
  FcPatternDestroy(pat);
  FcConfigDestroy(config);
  FcFini();

  SDL_Log("Font found: %s\n", (char *)buff);
  return true;
}

int LoadTextures(SDL_Renderer *renderer) {
  char fontPath[500];
  if (!LoadFont(&fontPath, NULL, sizeof(fontPath))) {
    SDL_Log("Failed to load font\n");
    return 1;
  }

  if (TTF_Init() < 0) {
    SDL_Log("Failed to initialize SDL_ttf: %s\n", TTF_GetError());
    return 1;
  }

  TTF_Font *font = TTF_OpenFont(fontPath, 1000);
  if (font == NULL) {
    SDL_Log("Failed to open font: %s\n", TTF_GetError());
    return 1;
  }

  for (int i = 0; i < 31; i++) {
    char content[10];
    sprintf(content, "%0d", i);

    SDL_Surface *text = TTF_RenderText_Blended(font, content, fgColor);
    if (!text) {
      SDL_Log("Failed to texturize number: %s\n", TTF_GetError());
      return 1;
    }

    textures[i] = SDL_CreateTextureFromSurface(renderer, text);
    SDL_FreeSurface(text);
  }

  return 0;
}

void RenderGrid(AppState *state) {
  float blockWidth = (float)state->w / 7.0f;
  float blockHeight = (float)state->h / 5.0f;

  /* SDL_RenderSetScale(app->renderer, 2.0f, 2.0f); */
  SDL_SetRenderDrawColor(state->renderer, fgColor.r, fgColor.g, fgColor.b,
                         fgColor.a);

  for (int i = 0; i <= 7; i++) {
    int offsetX = (int)blockWidth * i + 1;
    SDL_RenderDrawLine(state->renderer, offsetX, 0, offsetX, state->h);
  }

  for (int i = 0; i <= 5; i++) {
    int offsetY = (int)blockHeight * i + 1;
    SDL_RenderDrawLine(state->renderer, 0, offsetY, state->w, offsetY);
  }
}

void RenderDays(AppState *state) {
  float blockWidth = (float)state->w / 7.0f;
  float blockHeight = (float)state->h / 5.0f;

  SDL_Rect r = {.x = 0, .y = 0, .w = blockWidth, .h = blockHeight};

  SDL_RenderCopy(state->renderer, textures[10], NULL, &r);
}

int main() {
  printf("Initializing calendar\n");

  AppState state = {.running = true, .w = 600, .h = 600};

  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    SDL_Log("Failed to initialize SDL: %s\n", SDL_GetError());
    return 1;
  }

  state.window = SDL_CreateWindow("Calendar", SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED, state.w, state.h,
                                  SDL_WINDOW_RESIZABLE);
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

  if (LoadTextures(state.renderer) != 0) {
    SDL_Log("Failed to load textures\n");
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
    SDL_GetWindowSize(state.window, &state.w, &state.h);

    RenderGrid(&state);
    RenderDays(&state);

    SDL_RenderPresent(state.renderer);
    SDL_Delay(10);
  }

  SDL_Log("Exit calendar\n");
  SDL_DestroyRenderer(state.renderer);
  SDL_DestroyWindow(state.window);
  SDL_Quit();

  return 0;
}
