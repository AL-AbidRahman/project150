#include <SDL2/SDL.h>
#include <stdio.h>
#include <iostream>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
int R=50;
SDL_Window *window = NULL;
SDL_Renderer *render = NULL;
bool gameIsRun=false;
//for first circle
int X=0;
int Y=SCREEN_HEIGHT/2;
//for second circle
int x=SCREEN_WIDTH/2;
int y=SCREEN_HEIGHT-R;
bool clse=false;
int r_color=255;
int  b_color=0;
int  f=0;

int initializing(){
if(SDL_Init(SDL_INIT_VIDEO)!=0){
std::cout<<"Error: SDL failed to initialize\n"<<"SDL Error:"<<" "<<SDL_GetError()<<'\n';
return 0;
}


 window = SDL_CreateWindow("Drawing Circle",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,SCREEN_WIDTH,SCREEN_HEIGHT, 0);
if (!window)
    {
       std::cout<<"Error: SDL failed to open window\n"<<"SDL Error:"<<" "<<SDL_GetError()<<'\n';
       return 0;;
    }



    render = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!render)
    {
        std::cout<<"Error: SDL failed to create renderer\n"<<"SDL Error:"<<" "<<SDL_GetError()<<'\n';
        return 0;;
    }

	return 1;
}


bool collision(int x,int y,int X,int Y)
{
int d;
d=(x-X)*(x-X)+(y-Y)*(y-Y);
if(d<=4*R*R) return true;
return false;
}

void update()
{
 clse=collision(x,y,X,Y);
 if(clse) {
    if(f==0){r_color=0;b_color=255;f=1;}
    else {r_color=255;b_color=0;f=0;}
    
    }
 X+=2;
if(X>=SCREEN_WIDTH+R) X=0;
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



void Draw()
{

    SDL_SetRenderDrawColor(render, 0, 255, 0, 0);
	SDL_RenderClear(render);

    update();

	SDL_SetRenderDrawColor(render, r_color, 0,b_color, 0);
    draw_Circle(X,Y,R);

    SDL_SetRenderDrawColor(render, b_color, 0,r_color, 0);
    draw_Circle(x,y,R);
	SDL_RenderPresent(render);

}

void  event_loop()
{
  SDL_Event e;

  while(SDL_PollEvent(&e))
     {
       if(e.type==SDL_QUIT) {gameIsRun=false; break;}
       else if (e.type == SDL_KEYDOWN) {
                        if(e.key.keysym.sym==SDLK_UP) 
                        {
                            y-=10;
                            if(y==R) {x=SCREEN_WIDTH/2;y=SCREEN_HEIGHT-R;}
                        }
                        if(e.key.keysym.sym== SDLK_DOWN)
                        {
                             y+=10;
                            if(y+R>SCREEN_HEIGHT) {x=SCREEN_WIDTH/2;y=SCREEN_HEIGHT-R;}

                        }
                        if(e.key.keysym.sym== SDLK_LEFT)
                        {
                            x-=10;
                            if(x==R) {x=SCREEN_WIDTH/2;y=SCREEN_HEIGHT-R;}
                             
                        }
                        if(e.key.keysym.sym==SDLK_RIGHT)
                        {
                          
                            x+=10;
                            if(x+R==SCREEN_WIDTH) {x=SCREEN_WIDTH/2;y=SCREEN_HEIGHT-R;}
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
    Draw();
}

SDL_DestroyWindow(window);
SDL_Quit();

return 0;
}