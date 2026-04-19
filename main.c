#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdbool.h>

typedef struct {
  bool running;
} AppState;

int main(){
  printf("Initializing calendar\n");

  AppState state = {
    .running = true
  };

  if(SDL_Init(SDL_INIT_VIDEO) != 0){
    SDL_Log("Failed to initialize SDL: %s\n", SDL_GetError());
    return 1;
  }

  SDL_Window *win = SDL_CreateWindow("Calendar", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 480, 0);
  if(win == NULL){
    SDL_Log("Failed to create window: %s\n", SDL_GetError());
    return 1;
  }

  SDL_Renderer *renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
  if(renderer == NULL){
    SDL_Log("Failed to create renderer: %s\n", SDL_GetError());
    return 1;
  }

  while(state.running){
    SDL_Event e;
    while(SDL_PollEvent(&e)){
      SDL_Log("event: %d\n", e.type);

      if(e.type == SDL_QUIT){
        state.running = false;
        break;
      }else if(e.type == SDL_KEYDOWN){
        if(e.key.keysym.sym == SDLK_ESCAPE || e.key.keysym.sym == SDLK_q){
          state.running = false;
          break;
        }
      }
    }

    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    SDL_Delay(100);
  }

  SDL_Log("Exit calendar\n");
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(win);
  SDL_Quit();

  return 0;
}
