#include <SDL2/SDL.h>
#include <stdio.h>
#include <iostream>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define R 150
SDL_Window *window = NULL;
SDL_Renderer *render = NULL;
bool gameIsRun=false;



int initializing(){
if(SDL_Init(SDL_INIT_VIDEO)!=0){
std::cout<<"Error: SDL failed to initialize\n"<<"SDL Error:"<<" "<<SDL_GetError()<<'\n';
return 0;
}


 window = SDL_CreateWindow("Drawing Circle",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,SCREEN_WIDTH,SCREEN_HEIGHT, 0);
if (!window)
    {
       std::cout<<"Error: failed to open window\n"<<"SDL Error:"<<" "<<SDL_GetError()<<'\n';
       return 0;
    }



    render = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!render)
    {
        std::cout<<"Error:failed to create renderer\n"<<"SDL Error:"<<" "<<SDL_GetError()<<'\n';
        return 0;
    }

	return 1;
}

void  event_loop()
{
  SDL_Event e;

  while(SDL_PollEvent(&e))
     {
       if(e.type==SDL_QUIT) {gameIsRun=false; break;}
     }

}


void draw_Circle( int cX, int cY, int r) {
    for (int x = -r; x <= r; x++) {
        for (int y = -r; y <= r; y++) {
            if (x*x + y*y <= r*r) {
                SDL_RenderDrawPoint(render, cX + x, cY + y);
            }
        }
    }
}

int main(int argc,char *argv[])
{
gameIsRun=initializing();

while(gameIsRun)
{
	event_loop();
    SDL_SetRenderDrawColor(render, 0, 0, 250, 0);
	SDL_RenderClear(render);
	SDL_SetRenderDrawColor(render,0, 0, 0, 250);
    draw_Circle(SCREEN_WIDTH/2,SCREEN_HEIGHT/2,R);
	SDL_RenderPresent(render);
}
SDL_DestroyWindow(window);
SDL_Quit();
return 0;
}