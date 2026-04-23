#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <fontconfig/fontconfig.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

SDL_Color BG_COLOR = {.r = 0, .g = 0, .b = 0, .a = SDL_ALPHA_OPAQUE};
SDL_Color FG_COLOR = {.r = 255, .g = 255, .b = 255, .a = SDL_ALPHA_OPAQUE};
const char *WEEKDAY_NAMES[] = {"Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"};
const char *MONTH_NAMES[] = {"January",   "February", "March",    "April",
                             "May",       "June",     "July",     "August",
                             "September", "October",  "November", "December"};

int FONT_HEIGHT = 0;
int FONT_DESCENT = 0;
SDL_Texture *NUMBER_TEXTURES[31];
SDL_Texture *WEEKDAY_TEXTURES[7];
SDL_Texture *MONTH_TEXTURES[12];

typedef struct {
  struct tm *today;
  struct tm *current_month;
  SDL_Renderer *renderer;
  SDL_Window *window;
  int w;
  int h;
  bool running;
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

  for (int i = 0; i < (int)(sizeof(WEEKDAY_NAMES) / sizeof(WEEKDAY_NAMES[0]));
       i++) {
    SDL_Surface *text =
        TTF_RenderText_Blended(font, WEEKDAY_NAMES[i], FG_COLOR);
    if (!text) {
      SDL_Log("Failed to texturize weekday: %s\n", TTF_GetError());
      return 1;
    }

    WEEKDAY_TEXTURES[i] = SDL_CreateTextureFromSurface(renderer, text);
    SDL_FreeSurface(text);
  }

  for (int i = 0; i < (int)(sizeof(MONTH_TEXTURES) / sizeof(MONTH_TEXTURES[0]));
       i++) {
    SDL_Surface *text = TTF_RenderText_Blended(font, MONTH_NAMES[i], FG_COLOR);
    if (!text) {
      SDL_Log("Failed to texturize month name: %s\n", TTF_GetError());
      return 1;
    }

    MONTH_TEXTURES[i] = SDL_CreateTextureFromSurface(renderer, text);
    SDL_FreeSurface(text);
  }

  return 0;
}

void RenderBoundingBox(SDL_Renderer *renderer, SDL_Rect *rect) {
  SDL_SetRenderDrawColor(renderer, FG_COLOR.r, FG_COLOR.g, FG_COLOR.b,
                         FG_COLOR.a);
  SDL_RenderDrawRect(renderer, rect);
}

void RenderBoxTexture(SDL_Renderer *renderer, SDL_Rect *rect,
                      SDL_Texture *texture) {
  int padding_x = rect->w / 3;
  int padding_y = rect->h / 2;

  SDL_Rect destRect = {
      .x = rect->x + padding_x / 2,
      .y = rect->y + padding_y / 2,
      .w = rect->w - padding_x,
      .h = rect->h - padding_y,
  };

  int texture_width = 0;
  SDL_QueryTexture(texture, NULL, NULL, &texture_width, NULL);

  SDL_Rect srcRect = {
      .x = 0,
      .y = 0,
      .w = texture_width,
      .h = FONT_HEIGHT + FONT_DESCENT + 3,
  };

  SDL_RenderCopy(renderer, texture, &srcRect, &destRect);
}

void RenderMonth(AppState *state, SDL_Rect *root_rect) {
  int title_height = root_rect->h / 8;
  int header_height = root_rect->h / 9;
  int column_width = root_rect->w / 7;
  int column_height = (root_rect->h - title_height - header_height) / 5;

  // Render title with month name
  SDL_Rect title_rect = {
      .x = root_rect->x,
      .y = root_rect->y,
      .w = root_rect->w,
      .h = title_height,
  };

  RenderBoxTexture(state->renderer, &title_rect,
                   MONTH_TEXTURES[state->current_month->tm_mon]);
  /* RenderBoundingBox(state->renderer, &title_rect); */

  // Render header with week day names
  for (size_t i = 0; i < sizeof(WEEKDAY_NAMES) / sizeof(WEEKDAY_NAMES[0]);
       i++) {
    SDL_Rect r = {
        .x = root_rect->x + column_width * i,
        .y = root_rect->y + title_height,
        .w = column_width,
        .h = header_height,
    };
    RenderBoxTexture(state->renderer, &r, WEEKDAY_TEXTURES[i]);
    /* RenderBoundingBox(state->renderer, &r); */
  }

  // Render actual calendar
  int offset = state->current_month->tm_wday == 0
                   ? 6
                   : state->current_month->tm_wday - 1;
  int day = 1;
  for (int i = 0; i < 5; i++) {
    for (int j = 0; j < 7; j++) {
      SDL_Rect r = {
          .x = root_rect->x + column_width * j,
          .y = root_rect->y + title_height + header_height + column_height * i,
          .w = column_width,
          .h = column_height,
      };
      /* RenderBoundingBox(state->renderer, &r); */

      if (i == 0 && j < offset) {
        RenderBoxTexture(state->renderer, &r, NUMBER_TEXTURES[0]);
      } else if (day <= 31) {
        RenderBoxTexture(state->renderer, &r, NUMBER_TEXTURES[day]);
        day++;
      }
    }
  }
}

void LogDate(char *msg, struct tm *tm) {
  SDL_Log("%s: %d - %d - %d [%d]\n", msg, tm->tm_year + 1900, tm->tm_mon,
          tm->tm_mday, tm->tm_wday);
}

void UpdateMonth(AppState *state, int count) {
  int next_month = state->current_month->tm_mon + count;
  int next_year = state->current_month->tm_year;

  if (next_month < 0) {
    next_month = 11;
    next_year = state->current_month->tm_year - 1;
  } else if (next_month > 11) {
    next_month = 0;
    next_year = state->current_month->tm_year + 1;
  }

  state->current_month->tm_mon = next_month;
  state->current_month->tm_year = next_year;
  mktime(state->current_month);
  LogDate("CURRENT_MONTH", state->current_month);
}

void SetMonthToday(AppState *state) {
  state->current_month->tm_mon = state->today->tm_mon;
  state->current_month->tm_mday = state->today->tm_mday;
  state->current_month->tm_year = state->today->tm_year;

  mktime(state->current_month);
  mktime(state->today);
}

int main(void) {
  SDL_Log("Initializing calendar\n");
  time_t today_timer = time(NULL);
  time_t month_timer = time(NULL);
  struct tm today_tm;
  struct tm month_tm;

  localtime_r(&today_timer, &today_tm);
  localtime_r(&month_timer, &month_tm);

  month_tm.tm_mday = 1;
  month_tm.tm_sec = 0;
  month_tm.tm_min = 0;
  month_tm.tm_hour = 0;

  AppState state = {
      .running = true,
      .today = &today_tm,
      .current_month = &month_tm,
      .w = 600,
      .h = 600,
  };

  UpdateMonth(&state, 0);

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

        switch (e.key.keysym.sym) {
        case SDLK_ESCAPE:
        case SDLK_q:
          CloseApp(&state);
          break;

        case SDLK_h:
        case SDLK_k:
        case SDLK_LEFT:
          UpdateMonth(&state, -1);
          break;

        case SDLK_l:
        case SDLK_j:
        case SDLK_RIGHT:
          UpdateMonth(&state, 1);
          break;

        case SDLK_t:
          SetMonthToday(&state);
          break;
        }

        break;
      }
    }

    SDL_RenderSetScale(state.renderer, 1.0f, 1.0f);
    SDL_SetRenderDrawColor(state.renderer, BG_COLOR.r, BG_COLOR.g, BG_COLOR.b,
                           BG_COLOR.a);
    SDL_RenderClear(state.renderer);
    SDL_GetWindowSize(state.window, &state.w, &state.h);

    SDL_Rect r = {
        .x = 50,
        .y = 50,
        .w = state.w - 100,
        .h = state.h - 100,
    };
    RenderMonth(&state, &r);

    SDL_RenderPresent(state.renderer);
    SDL_Delay(10);
  }

  SDL_Log("Exit calendar\n");
  SDL_DestroyRenderer(state.renderer);
  SDL_DestroyWindow(state.window);
  SDL_Quit();

  return 0;
}
