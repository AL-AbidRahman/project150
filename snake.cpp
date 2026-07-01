#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <deque>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <string>
#include <algorithm>
#include <fstream>

const int CELL   = 20;
const int COLS   = 30;
const int ROWS   = 30;
const int WIN_W  = COLS * CELL;
const int WIN_H  = ROWS * CELL + 60;
const int HUD_H  = 60;
const std::string SAVE_FILE = "highscores.dat";

struct DiffConfig { int baseSpeed; int speedInc; const char* label; const char* desc; };
const DiffConfig DIFF[3] = {
    {200, 2, "EASY",   "Slow & relaxed"},
    {140, 3, "MEDIUM", "Classic speed" },
    {80,  5, "HARD",   "Blink and die" }
};

struct Color { Uint8 r, g, b, a; };
const Color C_BG        = {15,  15,  20,  255};
const Color C_GRID      = {25,  25,  35,  255};
const Color C_HEAD      = {80,  220, 120, 255};
const Color C_BODY      = {50,  170, 80,  255};
const Color C_BODY2     = {40,  140, 65,  255};
const Color C_FOOD      = {230, 70,  80,  255};
const Color C_FOOD_GLOW = {255, 120, 130, 200};
const Color C_HUD_BG    = {10,  10,  15,  255};
const Color C_TEXT      = {200, 200, 210, 255};
const Color C_GOLD      = {255, 200, 50,  255};
const Color C_OVERLAY   = {0,   0,   0,   200};
const Color C_EASY      = {80,  220, 100, 255};
const Color C_MED       = {80,  220, 100, 255};
const Color C_HARD      = {220, 60,  60,  255};

enum Screen { SCREEN_MENU, SCREEN_GAME, SCREEN_SCORES };
enum Dir    { UP, DOWN, LEFT, RIGHT };
struct Point { int x, y; };
bool operator==(Point a, Point b){ return a.x==b.x && a.y==b.y; }

struct ScoreEntry { int score; int diff; };

std::vector<ScoreEntry> loadScores(){
    std::vector<ScoreEntry> v;
    std::ifstream f(SAVE_FILE, std::ios::binary);
    if(!f) return v;
    int n=0; f.read((char*)&n,4);
    for(int i=0;i<n&&i<20;i++){ ScoreEntry e; f.read((char*)&e,sizeof(e)); v.push_back(e); }
    return v;
}
void saveScore(int score, int diff){
    auto v=loadScores();
    v.push_back({score,diff});
    std::sort(v.begin(),v.end(),[](auto&a,auto&b){return a.score>b.score;});
    if(v.size()>10) v.resize(10);
    std::ofstream f(SAVE_FILE,std::ios::binary);
    int n=(int)v.size(); f.write((char*)&n,4);
    for(auto& e:v) f.write((char*)&e,sizeof(e));
}
int getBestScore(int diff){
    auto v=loadScores(); int best=0;
    for(auto& e:v) if(e.diff==diff&&e.score>best) best=e.score;
    return best;
}

void setColor(SDL_Renderer* r, Color c){ SDL_SetRenderDrawColor(r,c.r,c.g,c.b,c.a); }
void fillRect(SDL_Renderer* r,int x,int y,int w,int h){ SDL_Rect rc{x,y,w,h}; SDL_RenderFillRect(r,&rc); }
void drawBorder(SDL_Renderer* r,int x,int y,int w,int h,int t,Color c){
    setColor(r,c);
    fillRect(r,x,y,w,t); fillRect(r,x,y+h-t,w,t);
    fillRect(r,x,y,t,h); fillRect(r,x+w-t,y,t,h);
}
void drawText(SDL_Renderer* rnd,TTF_Font* font,const std::string& s,int cx,int cy,Color c,bool center=true){
    if(!font||s.empty()) return;
    SDL_Color sc{c.r,c.g,c.b,c.a};
    SDL_Surface* surf=TTF_RenderText_Blended(font,s.c_str(),sc);
    if(!surf) return;
    SDL_Texture* tex=SDL_CreateTextureFromSurface(rnd,surf);
    int tw=surf->w,th=surf->h; SDL_FreeSurface(surf);
    SDL_Rect dst{center?cx-tw/2:cx,center?cy-th/2:cy,tw,th};
    SDL_RenderCopy(rnd,tex,nullptr,&dst);
    SDL_DestroyTexture(tex);
}

void drawCell(SDL_Renderer* r,int gx,int gy,Color fill,Color border){
    int px=gx*CELL,py=gy*CELL+HUD_H;
    setColor(r,border); fillRect(r,px+1,py+1,CELL-2,CELL-2);
    setColor(r,fill);   fillRect(r,px+3,py+3,CELL-6,CELL-6);
}

struct Game {
    std::deque<Point> snake;
    Dir dir,nextDir; Point food;
    bool running,dead,paused;
    int score,highScore,speed,diff;
    Uint32 tickTimer;

    void spawnFood(){
        std::vector<Point> free;
        for(int x=0;x<COLS;x++) for(int y=0;y<ROWS;y++){
            Point p{x,y}; bool ok=true;
            for(auto& s:snake) if(s==p){ok=false;break;}
            if(ok) free.push_back(p);
        }
        if(!free.empty()) food=free[rand()%free.size()];
    }
    void reset(int d){
        diff=d; snake.clear();
        int mx=COLS/2,my=ROWS/2;
        snake.push_back({mx,my}); snake.push_back({mx-1,my}); snake.push_back({mx-2,my});
        dir=nextDir=RIGHT; spawnFood();
        running=true; dead=false; paused=false;
        score=0; highScore=getBestScore(diff);
        speed=DIFF[diff].baseSpeed; tickTimer=SDL_GetTicks();
    }
    void handleKey(SDL_Keycode k){
        if(dead) return;
        switch(k){
            case SDLK_UP:    case SDLK_w: if(dir!=DOWN)  nextDir=UP;    break;
            case SDLK_DOWN:  case SDLK_s: if(dir!=UP)    nextDir=DOWN;  break;
            case SDLK_LEFT:  case SDLK_a: if(dir!=RIGHT) nextDir=LEFT;  break;
            case SDLK_RIGHT: case SDLK_d: if(dir!=LEFT)  nextDir=RIGHT; break;
            case SDLK_p: case SDLK_ESCAPE: paused=!paused; break;
            default: break;
        }
    }
    void tick(){
        if(!running||dead||paused) return;
        Uint32 now=SDL_GetTicks();
        if(now-tickTimer<(Uint32)speed) return;
        tickTimer=now; dir=nextDir;
        Point head=snake.front();
        switch(dir){ case UP:head.y--;break; case DOWN:head.y++;break; case LEFT:head.x--;break; case RIGHT:head.x++;break; }
        if(head.x<0||head.x>=COLS||head.y<0||head.y>=ROWS){die();return;}
        for(auto& s:snake) if(s==head){die();return;}
        snake.push_front(head);
        if(head==food){ score++; if(score>highScore)highScore=score; speed=std::max(40,speed-DIFF[diff].speedInc); spawnFood(); }
        else snake.pop_back();
    }
    void die(){ dead=true; running=false; saveScore(score,diff); }
};

struct Menu {
    int selectedDiff=1, mouseX=0, mouseY=0;
    bool hoverPlay=false, hoverScores=false, hoverQuit=false;
};

void drawMenuScreen(SDL_Renderer* r, TTF_Font* fBig, TTF_Font* fMed,
                    TTF_Font* fSmall, Menu& menu, Uint32 ticks, SDL_Texture* bgTex){

    setColor(r,{10,10,10,255}); fillRect(r,0,0,WIN_W,WIN_H);

    if(bgTex){
        SDL_SetTextureAlphaMod(bgTex,255);
        SDL_Rect dst{0,0,WIN_W,WIN_H};
        SDL_RenderCopy(r,bgTex,nullptr,&dst);
    }

    auto inRect=[&](int rx,int ry,int rw,int rh){
        return menu.mouseX>=rx&&menu.mouseX<=rx+rw&&menu.mouseY>=ry&&menu.mouseY<=ry+rh;
    };

    float pulse=0.88f+0.12f*sinf(ticks*0.003f);
    Color titleCol={(Uint8)(50*pulse),(Uint8)(230*pulse),(Uint8)(80*pulse),255};
    drawText(r,fBig,"SNAKE",WIN_W/2,70,titleCol);
    drawText(r,fSmall,"CLASSIC EDITION",WIN_W/2,112,{60,230,100,255});

    int py=150;
    drawText(r,fSmall,"SELECT DIFFICULTY",WIN_W/2,py,{255,255,255,255});
    py+=32;


    Color diffLabel[3]={
        {80, 230, 100, 255},
        {80, 230, 100, 255},
        {220, 55,  55, 255},
    };
    Color descCol={30, 180, 255, 255};

    int bw=WIN_W-290, bx=(WIN_W-bw)/2;

    for(int i=0;i<3;i++){
        bool sel=(menu.selectedDiff==i);
        int bh=62, by=py+i*70;
        bool hov=inRect(bx,by,bw,bh);

        SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
        if(sel){
            SDL_SetRenderDrawColor(r,0,0,0,120);
        } else if(hov){
            SDL_SetRenderDrawColor(r,255,255,255,30);
        } else {
            SDL_SetRenderDrawColor(r,0,0,0,80);
        }
        fillRect(r,bx,by,bw,bh);
        SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_NONE);

        Color bc = sel ? Color{255,255,255,255} : (hov ? Color{220,220,220,255} : Color{200,200,200,255});
        drawBorder(r,bx,by,bw,bh,sel?2:1,bc);

        drawText(r,fMed, DIFF[i].label, WIN_W/2, by+20, diffLabel[i]);
        drawText(r,fSmall,DIFF[i].desc, WIN_W/2, by+44, descCol);
    }
    py += 3*70 + 8;

    {
        int pbw=WIN_W-290, pbx=(WIN_W-pbw)/2, pbh=56, pby=py+8;
        menu.hoverPlay=inRect(pbx,pby,pbw,pbh);
        SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
        if(menu.hoverPlay) SDL_SetRenderDrawColor(r,50,180,70,220);
        else               SDL_SetRenderDrawColor(r,30,120,50,160);
        fillRect(r,pbx,pby,pbw,pbh);
        SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_NONE);
        drawBorder(r,pbx,pby,pbw,pbh,2,
                   menu.hoverPlay?Color{100,255,120,255}:Color{80,200,100,255});
        drawText(r,fMed,"PLAY",WIN_W/2,pby+28,{255,255,255,255});
    }
    py+=72;

    {
        int sbw=WIN_W-290, sbx=(WIN_W-sbw)/2, sbh=48, sby=py+4;
        menu.hoverScores=inRect(sbx,sby,sbw,sbh);
        SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
        if(menu.hoverScores) SDL_SetRenderDrawColor(r,180,80,0,200);
        else                 SDL_SetRenderDrawColor(r,0,0,0,80);
        fillRect(r,sbx,sby,sbw,sbh);
        SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_NONE);
        drawBorder(r,sbx,sby,sbw,sbh,menu.hoverScores?2:1,
                   menu.hoverScores?Color{255,180,60,255}:Color{255,140,0,255});
        drawText(r,fSmall,"HIGH SCORES",WIN_W/2,sby+24,
                 menu.hoverScores?Color{255,255,255,255}:Color{255,140,0,255});
    }
    py+=60;

    {
        int qbw=WIN_W-290, qbx=(WIN_W-qbw)/2, qbh=40, qby=py+2;
        menu.hoverQuit=inRect(qbx,qby,qbw,qbh);
        SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
        if(menu.hoverQuit){ SDL_SetRenderDrawColor(r,160,30,30,180); fillRect(r,qbx,qby,qbw,qbh); }
        SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_NONE);
        drawBorder(r,qbx,qby,qbw,qbh,menu.hoverQuit?2:1,
                   menu.hoverQuit?Color{255,80,80,255}:Color{180,180,200,255});
        drawText(r,fSmall,"QUIT  [Q]",WIN_W/2,qby+20,
                 menu.hoverQuit?Color{255,100,100,255}:Color{255,80,200,255});
    }

    drawText(r,fSmall,"UP/DOWN to select difficulty   ENTER to play",
             WIN_W/2,WIN_H-18,{255,255,255,255});
}

void drawScoresScreen(SDL_Renderer* r,TTF_Font* fBig,TTF_Font* fMed,TTF_Font* fSmall,Uint32 ticks){
    setColor(r,C_BG); fillRect(r,0,0,WIN_W,WIN_H);
    drawText(r,fBig,"HIGH SCORES",WIN_W/2,50,C_GOLD);
    drawText(r,fSmall,"Press ESC or BACKSPACE to go back",WIN_W/2,WIN_H-20,{150,150,170,255});
    auto scores=loadScores();
    Color diffCols[3]={C_EASY,C_MED,C_HARD};
    if(scores.empty()){ drawText(r,fMed,"No scores yet - go play!",WIN_W/2,WIN_H/2,{100,100,120,255}); return; }
    int ty=100;
    drawText(r,fSmall,"RANK",80,ty,{120,120,140,255});
    drawText(r,fSmall,"SCORE",WIN_W/2,ty,{120,120,140,255});
    drawText(r,fSmall,"DIFF",WIN_W-80,ty,{120,120,140,255});
    ty+=8; setColor(r,{40,40,55,255}); fillRect(r,30,ty,WIN_W-60,1); ty+=18;
    for(int i=0;i<(int)scores.size();i++){
        auto& e=scores[i];
        Color rc=(i==0)?C_GOLD:(i==1)?Color{180,180,190,255}:(i==2)?Color{180,120,60,255}:C_TEXT;
        std::string rank=(i==0)?"#1 GOLD":(i==1)?"#2 SILVER":(i==2)?"#3 BRONZE":"#"+std::to_string(i+1);
        drawText(r,fSmall,rank,80,ty,rc);
        drawText(r,fMed,std::to_string(e.score),WIN_W/2,ty,C_TEXT);
        drawText(r,fSmall,DIFF[e.diff].label,WIN_W-80,ty,diffCols[e.diff]);
        ty+=36;
    }
}

void drawHUD(SDL_Renderer* r,TTF_Font* fBig,TTF_Font* fSmall,const Game& g){
    setColor(r,C_HUD_BG); fillRect(r,0,0,WIN_W,HUD_H);
    setColor(r,{40,40,50,255}); fillRect(r,0,HUD_H-1,WIN_W,1);
    Color dc[]={C_EASY,C_MED,C_HARD};
    drawText(r,fBig,"SNAKE",WIN_W/2,18,C_HEAD);
    drawText(r,fSmall,DIFF[g.diff].label,WIN_W/2,42,dc[g.diff]);
    drawText(r,fSmall,"SCORE: "+std::to_string(g.score),80,42,C_TEXT);
    drawText(r,fSmall,"BEST:  "+std::to_string(g.highScore),WIN_W-80,42,C_GOLD);
}

void drawFood(SDL_Renderer* r,Point f,Uint32 ticks){
    float pulse=0.5f+0.5f*SDL_sinf(ticks*0.005f);
    int glow=(int)(3+3*pulse), px=f.x*CELL, py=f.y*CELL+HUD_H;
    SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
    setColor(r,{C_FOOD_GLOW.r,C_FOOD_GLOW.g,C_FOOD_GLOW.b,(Uint8)(80*pulse)});
    fillRect(r,px-glow,py-glow,CELL+2*glow,CELL+2*glow);
    SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_NONE);
    setColor(r,{(Uint8)(C_FOOD.r-20),(Uint8)(C_FOOD.g-20),(Uint8)(C_FOOD.b-20),255});
    fillRect(r,px+2,py+2,CELL-4,CELL-4);
    setColor(r,C_FOOD); fillRect(r,px+4,py+4,CELL-8,CELL-8);
    setColor(r,{255,200,200,255}); fillRect(r,px+5,py+5,3,3);
}

void drawSnake(SDL_Renderer* r,const std::deque<Point>& snake){
    for(int i=(int)snake.size()-1;i>=0;i--){
        auto& p=snake[i];
        if(i==0){
            drawCell(r,p.x,p.y,C_HEAD,{30,80,40,255});
            int px=p.x*CELL,py=p.y*CELL+HUD_H;
            setColor(r,{10,10,10,255});
            fillRect(r,px+5,py+5,3,3); fillRect(r,px+12,py+5,3,3);
        } else {
            drawCell(r,p.x,p.y,(i%2==0)?C_BODY:C_BODY2,{20,60,30,255});
        }
    }
}

void drawGrid(SDL_Renderer* r){
    setColor(r,C_GRID);
    for(int x=0;x<=COLS;x++) SDL_RenderDrawLine(r,x*CELL,HUD_H,x*CELL,WIN_H);
    for(int y=0;y<=ROWS;y++) SDL_RenderDrawLine(r,0,y*CELL+HUD_H,WIN_W,y*CELL+HUD_H);
}

void drawGameOverlay(SDL_Renderer* r,TTF_Font* fBig,TTF_Font* fMed,TTF_Font* fSmall,const Game& g,Uint32 ticks){
    SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
    setColor(r,C_OVERLAY); fillRect(r,0,HUD_H,WIN_W,WIN_H-HUD_H);
    SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_NONE);
    int cy=HUD_H+(WIN_H-HUD_H)/2;
    if(g.dead){
        float pulse=0.7f+0.3f*SDL_sinf(ticks*0.004f);
        Color rc={255,(Uint8)(80*pulse),(Uint8)(80*pulse),255};
        drawText(r,fBig,"GAME OVER",WIN_W/2,cy-70,rc);
        drawText(r,fMed,"Score: "+std::to_string(g.score),WIN_W/2,cy-20,C_TEXT);
        if(g.score>0&&g.score==g.highScore) drawText(r,fMed,"NEW HIGH SCORE!",WIN_W/2,cy+20,C_GOLD);
        drawText(r,fSmall,"R / ENTER  --->  Restart",   WIN_W/2,cy+60,{200,200,220,255});
        drawText(r,fSmall,"ESC        --->  Main Menu",  WIN_W/2,cy+84,{200,200,220,255});
    } else if(g.paused){
        drawText(r,fBig,"PAUSED",WIN_W/2,cy-30,C_TEXT);
        drawText(r,fSmall,"P / ESC  --->  Resume",    WIN_W/2,cy+20,{200,200,220,255});
        drawText(r,fSmall,"M        --->  Main Menu", WIN_W/2,cy+44,{200,200,220,255});
    }
}

TTF_Font* loadFont(int size){
    const char* paths[]={
        "C:\\Windows\\Fonts\\arialbd.ttf",
        "C:\\Windows\\Fonts\\arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSansBold.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        nullptr
    };
    for(int i=0;paths[i];i++){ TTF_Font* f=TTF_OpenFont(paths[i],size); if(f) return f; }
    return nullptr;
}

int main(int,char**){
    srand((unsigned)time(nullptr));
    if(SDL_Init(SDL_INIT_VIDEO)<0){ std::cerr<<"SDL: "<<SDL_GetError()<<"\n"; return 1; }
    if(TTF_Init()<0){ std::cerr<<"TTF: "<<TTF_GetError()<<"\n"; return 1; }

    SDL_Window* win=SDL_CreateWindow("Snake - Classic",
        SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,WIN_W,WIN_H,SDL_WINDOW_SHOWN);
    SDL_Renderer* rnd=SDL_CreateRenderer(win,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    SDL_SetRenderDrawBlendMode(rnd,SDL_BLENDMODE_BLEND);

    TTF_Font* fBig  =loadFont(42);
    TTF_Font* fMed  =loadFont(26);
    TTF_Font* fSmall=loadFont(16);

    SDL_Texture* bgTex=nullptr;
    SDL_Surface* bgSurf=SDL_LoadBMP("bg.bmp");
    if(bgSurf){ bgTex=SDL_CreateTextureFromSurface(rnd,bgSurf); SDL_FreeSurface(bgSurf); }

    Screen screen=SCREEN_MENU;
    Menu   menu;
    Game   game;
    bool   quit=false;
    SDL_Event ev;

    while(!quit){
        Uint32 ticks=SDL_GetTicks();
        while(SDL_PollEvent(&ev)){
            if(ev.type==SDL_QUIT) quit=true;
            if(ev.type==SDL_KEYDOWN){
                SDL_Keycode k=ev.key.keysym.sym;
                if(screen==SCREEN_MENU){
                    if(k==SDLK_q||k==SDLK_ESCAPE) quit=true;
                    if(k==SDLK_UP  ||k==SDLK_w) menu.selectedDiff=(menu.selectedDiff+2)%3;
                    if(k==SDLK_DOWN||k==SDLK_s) menu.selectedDiff=(menu.selectedDiff+1)%3;
                    if(k==SDLK_RETURN||k==SDLK_SPACE){ game.reset(menu.selectedDiff); screen=SCREEN_GAME; }
                    if(k==SDLK_h||k==SDLK_F1) screen=SCREEN_SCORES;
                } else if(screen==SCREEN_SCORES){
                    if(k==SDLK_ESCAPE||k==SDLK_BACKSPACE||k==SDLK_q) screen=SCREEN_MENU;
                } else if(screen==SCREEN_GAME){
                    if(k==SDLK_q) quit=true;
                    if(game.dead){
                        if(k==SDLK_r||k==SDLK_RETURN) game.reset(menu.selectedDiff);
                        if(k==SDLK_ESCAPE||k==SDLK_m) screen=SCREEN_MENU;
                    } else {
                        if(k==SDLK_m&&game.paused) screen=SCREEN_MENU;
                        game.handleKey(k);
                    }
                }
            }
            if(ev.type==SDL_MOUSEBUTTONDOWN&&screen==SCREEN_MENU){
                int mx=ev.button.x, my=ev.button.y;
                int bw=WIN_W-160, bx=(WIN_W-bw)/2;
                for(int i=0;i<3;i++){
                    int by=182+i*70;
                    if(mx>=bx&&mx<=bx+bw&&my>=by&&my<=by+62) menu.selectedDiff=i;
                }
                if(menu.hoverPlay)  { game.reset(menu.selectedDiff); screen=SCREEN_GAME; }
                if(menu.hoverScores) screen=SCREEN_SCORES;
                if(menu.hoverQuit)   quit=true;
            }
            if(ev.type==SDL_MOUSEMOTION&&screen==SCREEN_MENU){
                menu.mouseX=ev.motion.x; menu.mouseY=ev.motion.y;
            }
        }

        if(screen==SCREEN_GAME) game.tick();

        setColor(rnd,C_BG); SDL_RenderClear(rnd);
        if(screen==SCREEN_MENU){
            drawMenuScreen(rnd,fBig,fMed,fSmall,menu,ticks,bgTex);
        } else if(screen==SCREEN_SCORES){
            drawScoresScreen(rnd,fBig,fMed,fSmall,ticks);
        } else {
            drawGrid(rnd);
            drawFood(rnd,game.food,ticks);
            drawSnake(rnd,game.snake);
            drawHUD(rnd,fBig,fSmall,game);
            if(game.dead||game.paused) drawGameOverlay(rnd,fBig,fMed,fSmall,game,ticks);
        }
        SDL_RenderPresent(rnd);
        SDL_Delay(8);
    }

    if(bgTex) SDL_DestroyTexture(bgTex);
    if(fBig)   TTF_CloseFont(fBig);
    if(fMed)   TTF_CloseFont(fMed);
    if(fSmall) TTF_CloseFont(fSmall);
    TTF_Quit();
    SDL_DestroyRenderer(rnd);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}