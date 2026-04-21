#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <fontconfig/fontconfig.h>
#include <stdbool.h>
#include <stdio.h>

SDL_Color BG_COLOR = {.r = 0, .g = 0, .b = 0, .a = SDL_ALPHA_OPAQUE};
SDL_Color FG_COLOR = {.r = 255, .g = 255, .b = 255, .a = SDL_ALPHA_OPAQUE};
const char *WEEKDAYS[] = {"Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"};

int FONT_HEIGHT = 0;
int FONT_DESCENT = 0;
SDL_Texture *NUMBER_TEXTURES[31];
SDL_Texture *WEEKDAYS_TEXTURES[7];

typedef struct {
  bool running;
  int w;
  int h;
  SDL_Renderer *renderer;
  SDL_Window *window;
} AppState;

void CloseApp(AppState *state) { state->running = false; }

bool LoadFont(void *buff, char *font_name, size_t buff_size) {
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

  if (font_name == NULL || strlen(font_name) == 0) {
    font_name = "Mono:style=Regular";
  }

  FcResult result;
  FcPattern *pat = FcNameParse((const FcChar8 *)font_name);

  FcConfigSubstitute(config, pat, FcMatchKindBegin);
  FcDefaultSubstitute(pat);
  SDL_Log("Looking for font: %s\n", font_name);

  FcPattern *font = FcFontMatch(config, pat, &result);
  if (font) {
    FcChar8 *file = NULL;
    if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch) {
      memcpy(buff, file, buff_size);
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
  char font_path[500];
  if (!LoadFont(&font_path, NULL, sizeof(font_path))) {
    SDL_Log("Failed to load font\n");
    return 1;
  }

  if (TTF_Init() < 0) {
    SDL_Log("Failed to initialize SDL_ttf: %s\n", TTF_GetError());
    return 1;
  }

  TTF_Font *font = TTF_OpenFont(font_path, 100);
  if (font == NULL) {
    SDL_Log("Failed to open font: %s\n", TTF_GetError());
    return 1;
  }
  FONT_DESCENT = TTF_FontDescent(font);
  FONT_HEIGHT = TTF_FontHeight(font);

  for (int i = 1; i <= 31; i++) {
    char content[10];
    sprintf(content, "%02d", i);

    SDL_Surface *text = TTF_RenderText_Blended(font, content, FG_COLOR);
    if (!text) {
      SDL_Log("Failed to texturize number: %s\n", TTF_GetError());
      return 1;
    }

    NUMBER_TEXTURES[i] = SDL_CreateTextureFromSurface(renderer, text);
    SDL_FreeSurface(text);
  }

  for (int i = 0; i < (int)(sizeof(WEEKDAYS) / sizeof(WEEKDAYS[0])); i++) {
    SDL_Surface *text = TTF_RenderText_Blended(font, WEEKDAYS[i], FG_COLOR);
    if (!text) {
      SDL_Log("Failed to texturize weekday: %s\n", TTF_GetError());
      return 1;
    }

    WEEKDAYS_TEXTURES[i] = SDL_CreateTextureFromSurface(renderer, text);
    SDL_FreeSurface(text);
  }

  return 0;
}

void RenderBoundingBox(SDL_Renderer *renderer, SDL_Rect *rect) {
  SDL_SetRenderDrawColor(renderer, FG_COLOR.r, FG_COLOR.g, FG_COLOR.b,
                         FG_COLOR.a);
  SDL_RenderDrawRect(renderer, rect);
}

void RenderBoxContent(SDL_Renderer *renderer, SDL_Rect *rect,
                      SDL_Texture *texture) {
  int padding_x = rect->w / 3;
  int padding_y = rect->h / 2;

  SDL_Rect destRect = {
      .x = rect->x + padding_x / 2,
      .y = rect->y + padding_y / 2,
      .w = rect->w - padding_x,
      .h = rect->h - padding_y,
  };

  SDL_Rect srcRect = {
      .x = 0,
      .y = 0,
      .w = 120,
      .h = FONT_HEIGHT + FONT_DESCENT + 3,
  };

  SDL_RenderCopy(renderer, texture, &srcRect, &destRect);
}

void RenderMonth(SDL_Renderer *renderer, SDL_Rect *root_rect) {
  int header_height = root_rect->h / 8;
  int column_width = root_rect->w / 7;
  int column_height = (root_rect->h - header_height) / 5;

  for (size_t i = 0; i < sizeof(WEEKDAYS) / sizeof(WEEKDAYS[0]); i++) {
    SDL_Rect r = {
        .x = root_rect->x + column_width * i,
        .y = root_rect->y,
        .w = column_width,
        .h = header_height,
    };
    RenderBoxContent(renderer, &r, WEEKDAYS_TEXTURES[i]);
    RenderBoundingBox(renderer, &r);
  }

  int day = 1;
  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 7; j++) {
      if (day <= 31) {
        SDL_Rect r = {
            .x = root_rect->x + column_width * j,
            .y = root_rect->y + header_height + column_height * i,
            .w = column_width,
            .h = column_height,
        };
        RenderBoxContent(renderer, &r, NUMBER_TEXTURES[day]);
        RenderBoundingBox(renderer, &r);
        day++;
      }
    }
  }
}

int main(void) {
  SDL_Log("Initializing calendar\n");

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
    SDL_SetRenderDrawColor(state.renderer, BG_COLOR.r, BG_COLOR.g, BG_COLOR.b,
                           BG_COLOR.a);
    SDL_RenderClear(state.renderer);
    SDL_GetWindowSize(state.window, &state.w, &state.h);

    SDL_Rect r = {.x = 50, .y = 50, .w = state.w - 100, .h = state.h - 100};
    RenderMonth(state.renderer, &r);

    SDL_RenderPresent(state.renderer);
    SDL_Delay(10);
  }

  SDL_Log("Exit calendar\n");
  SDL_DestroyRenderer(state.renderer);
  SDL_DestroyWindow(state.window);
  SDL_Quit();

  return 0;
}
