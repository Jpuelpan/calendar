#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <fontconfig/fontconfig.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

SDL_Color BG_COLOR = {.r = 0, .g = 0, .b = 0, .a = SDL_ALPHA_OPAQUE};
SDL_Color FG_COLOR = {.r = 255, .g = 255, .b = 255, .a = SDL_ALPHA_OPAQUE};
const char *WEEKDAY_NAMES[] = {"Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"};
const char *MONTH_NAMES[] = {"January",   "February", "March",    "April",
                             "May",       "June",     "July",     "August",
                             "September", "October",  "November", "December"};

int FONT_HEIGHT = 0;
int FONT_DESCENT = 0;
SDL_Texture *SINGLE_NUMBER_TEXTURES[10];
SDL_Texture *NUMBER_TEXTURES[31];
SDL_Texture *WEEKDAY_TEXTURES[7];
SDL_Texture *MONTH_TEXTURES[12];

typedef struct {
  struct tm *today;
  struct tm *current_month;
  int total_days;
  bool is_fullscreen;

  SDL_Renderer *renderer;
} AppState;

float min(float a, float b) { return a < b ? a : b; }

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

  int year = state->current_month->tm_year;
  int month = state->current_month->tm_mon + 1;

  if (month == 2) {
    state->total_days = 28;
    if (year % 4 == 0) {
      if (year % 100 != 0 || (year % 400) == 0) {
        state->total_days++;
      }
    }
  } else {
    state->total_days = 30 + ((month + (month / 8)) % 2);
  }

  LogDate("TODAY", state->today);
  LogDate("MONTH", state->current_month);
}

void SetMonthToday(AppState *state) {
  state->current_month->tm_mon = state->today->tm_mon;
  state->current_month->tm_year = state->today->tm_year;
  mktime(state->current_month);

  LogDate("TODAY", state->today);
  LogDate("MONTH", state->current_month);
}

bool LoadFont(void *buff, char *font_name, size_t buff_size) {
  FcConfig *config = NULL;

  if (!FcInit()) {
    SDL_Log("Failed to initialize FontConfig\n");
    return false;
  }
  SDL_Log("Initialized FontConfig\n");

  config = FcInitLoadConfigAndFonts();
  if (config == NULL) {
    SDL_Log("Failed to load FontConfig configuration\n");
    return false;
  }
  SDL_Log("Initialized FontConfig configuration and fonts\n");

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

int InitTextures(SDL_Renderer *renderer, char *font_name) {
  char font_path[500];
  if (!LoadFont(&font_path, font_name, sizeof(font_path))) {
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

  SDL_Log("Font Height: %d\n", FONT_HEIGHT);
  SDL_Log("Font Descent: %d\n", FONT_DESCENT);

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

  for (int i = 0; i < 10; i++) {
    char digit[2];
    digit[0] = 48 + i;
    digit[1] = '\0';

    SDL_Surface *text = TTF_RenderText_Blended(font, digit, FG_COLOR);
    if (!text) {
      SDL_Log("Failed to texturize number: %s\n", TTF_GetError());
      return 1;
    }

    SINGLE_NUMBER_TEXTURES[i] = SDL_CreateTextureFromSurface(renderer, text);
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

void RenderBoundingBox(SDL_Renderer *renderer, SDL_Rect *rect,
                       SDL_Color *color) {
  if (color == NULL) {
    SDL_SetRenderDrawColor(renderer, FG_COLOR.r, FG_COLOR.g, FG_COLOR.b,
                           FG_COLOR.a);
  } else {
    SDL_SetRenderDrawColor(renderer, color->r, color->g, color->b, color->a);
  }

  SDL_RenderDrawRect(renderer, rect);
}

void RenderWeekName(SDL_Renderer *renderer, SDL_Rect *rect, int week_day) {
  SDL_Texture *texture = WEEKDAY_TEXTURES[week_day];
  if (texture == NULL) {
    return;
  }

  int padding_x = rect->w / 3;
  int padding_y = rect->h / 3;
  SDL_Rect inner_rect = {
      .x = rect->x + padding_x / 2,
      .y = rect->y + padding_y / 2,
      .w = rect->w - padding_x,
      .h = rect->h - padding_y,
  };

  int texture_width = 0;
  int texture_height = 0;
  SDL_QueryTexture(texture, NULL, NULL, &texture_width, &texture_height);

  float k = min((float)inner_rect.w / (float)texture_width,
                (float)inner_rect.h / (float)texture_height);

  float dest_width = texture_width * k;
  float dest_height = texture_height * k;

  SDL_Rect dest = {
      .x = inner_rect.x + ((float)inner_rect.w / 2) - (dest_width / 2),
      .y = inner_rect.y + ((float)inner_rect.h / 2) - (dest_height / 2),
      .w = dest_width,
      .h = dest_height,
  };

  if (SDL_RenderCopy(renderer, texture, NULL, &dest) < 0) {
    SDL_Log("Failed to render texture: %s\n", SDL_GetError());
  }
}

void RenderDayNumber(SDL_Renderer *renderer, SDL_Rect *rect, int day_number) {
  SDL_Texture *texture = NUMBER_TEXTURES[day_number];
  if (texture == NULL) {
    return;
  }

  int padding_x = rect->w / 3;
  int padding_y = rect->h / 3;
  SDL_Rect inner_rect = {
      .x = rect->x + padding_x / 2,
      .y = rect->y + padding_y / 2,
      .w = rect->w - padding_x,
      .h = rect->h - padding_y,
  };

  int texture_width = 0;
  int texture_height = 0;
  SDL_QueryTexture(texture, NULL, NULL, &texture_width, &texture_height);

  float k = min((float)inner_rect.w / (float)texture_width,
                (float)inner_rect.h / (float)texture_height);

  float dest_width = texture_width * k;
  float dest_height = texture_height * k;

  SDL_Rect dest = {
      .x = inner_rect.x + ((float)inner_rect.w / 2) - (dest_width / 2),
      .y = inner_rect.y + ((float)inner_rect.h / 2) - (dest_height / 2),
      .w = dest_width,
      .h = dest_height,
  };

  if (SDL_RenderCopy(renderer, texture, NULL, &dest) < 0) {
    SDL_Log("Failed to render texture: %s\n", SDL_GetError());
  }
}

SDL_Texture *GetYearTexture(SDL_Renderer *renderer, int year) {
  char str_year[12];
  sprintf(str_year, "%d", year);
  int year_len = (int)strlen(str_year);

  SDL_Texture *digit_textures[year_len];
  int digit_height = 0;
  int digit_width = 0;

  for (int i = 0; i < year_len; i++) {
    int idx = str_year[i] - 48;
    digit_textures[i] = SINGLE_NUMBER_TEXTURES[idx];

    int w = 0, h = 0;
    SDL_QueryTexture(digit_textures[i], NULL, NULL, &w, &h);
    digit_height = h;
    digit_width = w;
  }
  int full_width = digit_width * year_len;

  SDL_Texture *target_texture =
      SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB32,
                        SDL_TEXTUREACCESS_TARGET, full_width, digit_height);

  if (!target_texture) {
    SDL_Log("Failed to creat texture: %s\n", SDL_GetError());
    return NULL;
  }

  SDL_SetRenderTarget(renderer, target_texture);
  SDL_SetRenderDrawColor(renderer, BG_COLOR.r, BG_COLOR.g, BG_COLOR.b,
                         BG_COLOR.a);
  SDL_RenderClear(renderer);
  for (int i = 0; i < year_len; i++) {
    SDL_Rect dest = {
        .x = digit_width * i,
        .y = 0,
        .w = digit_width,
        .h = digit_height,
    };

    if (SDL_RenderCopy(renderer, digit_textures[i], NULL, &dest) < 0) {
      SDL_Log("Failed to render texture: %s\n", SDL_GetError());
    }
  }

  SDL_SetRenderTarget(renderer, NULL);
  return target_texture;
}

void RenderMonthTitle(AppState *state, SDL_Rect *rect) {
  SDL_Texture *month_texture = MONTH_TEXTURES[state->current_month->tm_mon];
  int month_width = 0;
  int month_height = 0;
  SDL_QueryTexture(month_texture, NULL, NULL, &month_width, &month_height);

  SDL_Texture *year_texture =
      GetYearTexture(state->renderer, state->current_month->tm_year + 1900);
  int year_width = 0;
  int year_height = 0;
  SDL_QueryTexture(year_texture, NULL, NULL, &year_width, &year_height);

  int spacing = year_width / 4;
  int target_width = month_width + year_width + spacing;
  int target_height = month_height;
  SDL_Texture *target_texture =
      SDL_CreateTexture(state->renderer, SDL_PIXELFORMAT_ARGB32,
                        SDL_TEXTUREACCESS_TARGET, target_width, target_height);

  SDL_SetRenderTarget(state->renderer, target_texture);
  SDL_SetRenderDrawColor(state->renderer, BG_COLOR.r, BG_COLOR.g, BG_COLOR.b,
                         BG_COLOR.a);
  SDL_RenderClear(state->renderer);

  SDL_Rect dest = {
      .x = 0,
      .y = 0,
      .w = month_width,
      .h = target_height,
  };
  if (SDL_RenderCopy(state->renderer, month_texture, NULL, &dest) < 0) {
    SDL_Log("Failed to render month texture: %s\n", SDL_GetError());
  }

  dest.x = month_width + spacing;
  dest.w = year_width;
  if (SDL_RenderCopy(state->renderer, year_texture, NULL, &dest) < 0) {
    SDL_Log("Failed to render month texture: %s\n", SDL_GetError());
  }

  float k = min((float)rect->w / (float)target_width,
                (float)rect->h / (float)target_height);

  float new_width = target_width * k;
  dest.x = rect->x + ((float)rect->w / 2) - (new_width / 2);
  dest.y = rect->y;
  dest.w = new_width;
  dest.h = target_height * k;

  SDL_SetRenderTarget(state->renderer, NULL);
  if (SDL_RenderCopy(state->renderer, target_texture, NULL, &dest) < 0) {
    SDL_Log("Failed to render texture: %s\n", SDL_GetError());
  }
  // RenderBoundingBox(state->renderer, rect, NULL);
}

void RenderMonth(AppState *state, SDL_Rect *root_rect) {
  int title_height = root_rect->h / 8;
  int header_height = root_rect->h / 9;
  int column_width = root_rect->w / 7;
  int column_height = (root_rect->h - title_height - header_height) / 6;

  // Render title with month name
  SDL_Rect title_rect = {
      .x = root_rect->x,
      .y = root_rect->y,
      .w = root_rect->w,
      .h = title_height,
  };

  RenderMonthTitle(state, &title_rect);

  // Render header with week day names
  for (size_t i = 0; i < sizeof(WEEKDAY_NAMES) / sizeof(WEEKDAY_NAMES[0]);
       i++) {
    SDL_Rect r = {
        .x = root_rect->x + column_width * i,
        .y = root_rect->y + title_height,
        .w = column_width,
        .h = header_height,
    };
    RenderWeekName(state->renderer, &r, i);
  }

  // Render actual calendar
  int offset = state->current_month->tm_wday == 0
                   ? 6
                   : state->current_month->tm_wday - 1;
  int day = 1;
  for (int i = 0; i < 6; i++) {
    for (int j = 0; j < 7; j++) {
      if ((i == 0 && j < offset) || day > state->total_days) {
        continue;
      }

      SDL_Rect r = {
          .x = root_rect->x + column_width * j,
          .y = root_rect->y + title_height + header_height + column_height * i,
          .w = column_width,
          .h = column_height,
      };

      SDL_Texture *texture = NUMBER_TEXTURES[day];
      SDL_SetTextureColorMod(texture, FG_COLOR.r, FG_COLOR.g, FG_COLOR.b);

      if (day == state->today->tm_mday &&
          state->current_month->tm_mon == state->today->tm_mon &&
          state->current_month->tm_year == state->today->tm_year) {
        SDL_SetRenderDrawColor(state->renderer, FG_COLOR.r, FG_COLOR.g,
                               FG_COLOR.b, FG_COLOR.a);
        SDL_RenderFillRect(state->renderer, &r);
        SDL_SetTextureColorMod(texture, BG_COLOR.r, BG_COLOR.g, BG_COLOR.b);
      }

      // RenderBoundingBox(state->renderer, &r, NULL);
      RenderDayNumber(state->renderer, &r, day);
      day++;
    }
  }
}

void RenderApp(AppState *state, SDL_Window *window) {
  SDL_RenderSetScale(state->renderer, 1.0f, 1.0f);
  SDL_SetRenderDrawColor(state->renderer, BG_COLOR.r, BG_COLOR.g, BG_COLOR.b,
                         BG_COLOR.a);
  SDL_RenderClear(state->renderer);

  int w, h = 0;
  SDL_GetWindowSize(window, &w, &h);

  int pad_x = w / 15;
  int pad_y = h / 15;

  SDL_Rect r = {
      .x = pad_x,
      .y = pad_x,
      .w = w - (pad_x * 2),
      .h = h - (pad_y * 2),
  };
  RenderMonth(state, &r);
  SDL_RenderPresent(state->renderer);
}

int main(int argc, char *argv[]) {
  SDL_Log("Initializing calendar\n");

  char font_name[512] = {'\0'};
  for (int i = 0; i < argc; i++) {
    if (strcmp(argv[i], "-font") == 0) {
      memcpy(font_name, argv[i + 1], sizeof(font_name));
    }
  }

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
      .is_fullscreen = false,
      .today = &today_tm,
      .current_month = &month_tm,
  };

  UpdateMonth(&state, 0);

  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    SDL_Log("Failed to initialize SDL: %s\n", SDL_GetError());
    return 1;
  }
  SDL_Log("Initialized SDL\n");

  SDL_Window *window =
      SDL_CreateWindow("Calendar", SDL_WINDOWPOS_CENTERED,
                       SDL_WINDOWPOS_CENTERED, 600, 600, SDL_WINDOW_RESIZABLE);
  if (window == NULL) {
    SDL_Log("Failed to create window: %s\n", SDL_GetError());
    return 1;
  }
  SDL_Log("Window initialized\n");

  state.renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (state.renderer == NULL) {
    SDL_Log("Failed to create renderer: %s\n", SDL_GetError());
    return 1;
  }
  SDL_Log("Renderer initialized\n");

  if (InitTextures(state.renderer, font_name) != 0) {
    SDL_Log("Failed to load textures\n");
    return 1;
  }

  bool is_running = true;
  RenderApp(&state, window);

  SDL_Event e;
  while (SDL_WaitEvent(&e) && is_running) {
    // SDL_Log("event: %d\n", e.type);

    switch (e.type) {
    case SDL_QUIT:
      is_running = false;
      break;

    case SDL_KEYDOWN:
      switch (e.key.keysym.sym) {
      case SDLK_ESCAPE:
      case SDLK_q:
        is_running = false;
        break;

      case SDLK_f:
        state.is_fullscreen = !state.is_fullscreen;
        SDL_SetWindowFullscreen(
            window, state.is_fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
        break;

      case SDLK_h:
      case SDLK_LEFT:
        UpdateMonth(&state, -1);
        break;

      case SDLK_l:
      case SDLK_RIGHT:
        UpdateMonth(&state, 1);
        break;

      case SDLK_SPACE:
        SetMonthToday(&state);
        break;
      }

      break;
    }

    RenderApp(&state, window);
  }

  SDL_Log("Exit calendar\n");
  SDL_DestroyRenderer(state.renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
