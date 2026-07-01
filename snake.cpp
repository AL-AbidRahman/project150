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

// ─── Constants ────────────────────────────────────────────────────────────────
const int CELL   = 20;
const int COLS   = 30;
const int ROWS   = 30;
const int WIN_W  = COLS * CELL;
const int WIN_H  = ROWS * CELL + 60;
const int HUD_H  = 60;
const std::string SAVE_FILE = "highscores.dat";

// Difficulty settings: {baseSpeed, speedInc, label}
struct DiffConfig { int baseSpeed; int speedInc; const char* label; const char* desc; };
const DiffConfig DIFF[3] = {
    {200, 2,  "EASY",   "Slow & relaxed"},
    {140, 3,  "MEDIUM", "Classic speed" },
    {80,  5,  "HARD",   "Blink and die" }
};

// ─── Colours ──────────────────────────────────────────────────────────────────
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
const Color C_DIM       = {0,   0,   0,   160};
const Color C_EASY      = {80,  200, 100, 255};
const Color C_MED       = {240, 180, 50,  255};
const Color C_HARD      = {220, 60,  60,  255};
const Color C_ACCENT    = {100, 160, 255, 255};

// ─── Screen state ─────────────────────────────────────────────────────────────
enum Screen { SCREEN_MENU, SCREEN_GAME, SCREEN_SCORES };

// ─── Direction ────────────────────────────────────────────────────────────────
enum Dir { UP, DOWN, LEFT, RIGHT };
struct Point { int x, y; };
bool operator==(Point a, Point b){ return a.x==b.x && a.y==b.y; }

// ─── High score storage ───────────────────────────────────────────────────────
struct ScoreEntry { int score; int diff; };  // diff: 0=easy,1=med,2=hard

std::vector<ScoreEntry> loadScores(){
    std::vector<ScoreEntry> v;
    std::ifstream f(SAVE_FILE, std::ios::binary);
    if(!f) return v;
    int n=0; f.read((char*)&n,4);
    for(int i=0;i<n&&i<20;i++){
        ScoreEntry e; f.read((char*)&e,sizeof(e)); v.push_back(e);
    }
    return v;
}

void saveScore(int score, int diff){
    auto v = loadScores();
    v.push_back({score, diff});
    // sort descending
    std::sort(v.begin(),v.end(),[](auto&a,auto&b){return a.score>b.score;});
    if(v.size()>10) v.resize(10);
    std::ofstream f(SAVE_FILE, std::ios::binary);
    int n=(int)v.size(); f.write((char*)&n,4);
    for(auto& e:v) f.write((char*)&e,sizeof(e));
}

int getBestScore(int diff){
    auto v=loadScores();
    int best=0;
    for(auto& e:v) if(e.diff==diff && e.score>best) best=e.score;
    return best;
}

// ─── Helpers ──────────────────────────────────────────────────────────────────
void setColor(SDL_Renderer* r, Color c){
    SDL_SetRenderDrawColor(r,c.r,c.g,c.b,c.a);
}
void fillRect(SDL_Renderer* r,int x,int y,int w,int h){
    SDL_Rect rc{x,y,w,h}; SDL_RenderFillRect(r,&rc);
}
void drawBorder(SDL_Renderer* r,int x,int y,int w,int h,int t,Color c){
    setColor(r,c);
    fillRect(r,x,y,w,t);
    fillRect(r,x,y+h-t,w,t);
    fillRect(r,x,y,t,h);
    fillRect(r,x+w-t,y,t,h);
}

void drawText(SDL_Renderer* rnd, TTF_Font* font, const std::string& s,
              int cx, int cy, Color c, bool center=true){
    if(!font||s.empty()) return;
    SDL_Color sc{c.r,c.g,c.b,c.a};
    SDL_Surface* surf=TTF_RenderText_Blended(font,s.c_str(),sc);
    if(!surf) return;
    SDL_Texture* tex=SDL_CreateTextureFromSurface(rnd,surf);
    int tw=surf->w,th=surf->h; SDL_FreeSurface(surf);
    SDL_Rect dst{center?cx-tw/2:cx, center?cy-th/2:cy, tw, th};
    SDL_RenderCopy(rnd,tex,nullptr,&dst);
    SDL_DestroyTexture(tex);
}

// Draw snake-pattern background (animated)
void drawMenuBG(SDL_Renderer* r, Uint32 ticks){
    setColor(r,C_BG); fillRect(r,0,0,WIN_W,WIN_H);
    // animated grid dots
    for(int x=0;x<COLS;x++) for(int y=0;y<ROWS+3;y++){
        float phase=(float)(x+y)*0.4f - ticks*0.001f;
        Uint8 alpha=(Uint8)(18+12*sinf(phase));
        SDL_SetRenderDrawColor(r,80,180,100,alpha);
        SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
        fillRect(r,x*CELL+9,y*CELL+9,3,3);
        SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_NONE);
    }
}

void drawCell(SDL_Renderer* r,int gx,int gy,Color fill,Color border){
    int px=gx*CELL,py=gy*CELL+HUD_H;
    setColor(r,border); fillRect(r,px+1,py+1,CELL-2,CELL-2);
    setColor(r,fill);   fillRect(r,px+3,py+3,CELL-6,CELL-6);
}

// ─── Game struct ──────────────────────────────────────────────────────────────
struct Game {
    std::deque<Point> snake;
    Dir   dir, nextDir;
    Point food;
    bool  running, dead, paused;
    int   score, highScore, speed;
    int   diff;   // 0/1/2
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
        diff=d;
        snake.clear();
        int mx=COLS/2,my=ROWS/2;
        snake.push_back({mx,my});
        snake.push_back({mx-1,my});
        snake.push_back({mx-2,my});
        dir=nextDir=RIGHT;
        spawnFood();
        running=true; dead=false; paused=false;
        score=0;
        highScore=getBestScore(diff);
        speed=DIFF[diff].baseSpeed;
        tickTimer=SDL_GetTicks();
    }

    void handleKey(SDL_Keycode k){
        if(dead){ return; }
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
        tickTimer=now;
        dir=nextDir;
        Point head=snake.front();
        switch(dir){
            case UP:    head.y--; break;
            case DOWN:  head.y++; break;
            case LEFT:  head.x--; break;
            case RIGHT: head.x++; break;
        }
        if(head.x<0||head.x>=COLS||head.y<0||head.y>=ROWS){die();return;}
        for(auto& s:snake) if(s==head){die();return;}
        snake.push_front(head);
        if(head==food){
            score++;
            if(score>highScore) highScore=score;
            speed=std::max(40,speed-DIFF[diff].speedInc);
            spawnFood();
        } else {
            snake.pop_back();
        }
    }
    void die(){
        dead=true; running=false;
        saveScore(score,diff);
    }
};

// ─── Menu state ───────────────────────────────────────────────────────────────
struct Menu {
    int  selectedDiff = 1;
    int  mouseX=0, mouseY=0;
    bool hoverPlay=false, hoverScores=false, hoverQuit=false;
    // stored button rects for click/hover detection
    int playX=0,playY=0,playW=0,playH=0;
    int scoresX=0,scoresY=0,scoresW=0,scoresH=0;
    int quitX=0,quitY=0,quitW=0,quitH=0;
};

// ─── Draw: background image (solid colored panels as "background art") ────────
void drawMenuBackground(SDL_Renderer* r, TTF_Font* fBig, TTF_Font* fSmall,
                        Uint32 ticks, SDL_Texture* bgTex){
    drawMenuBG(r,ticks);

    // If background image loaded, draw it semi-transparently
    if(bgTex){
        SDL_SetTextureAlphaMod(bgTex,60);
        SDL_Rect dst{0,0,WIN_W,WIN_H};
        SDL_RenderCopy(r,bgTex,nullptr,&dst);
        SDL_SetTextureAlphaMod(bgTex,255);
    }
}

void drawMenuScreen(SDL_Renderer* r, TTF_Font* fBig, TTF_Font* fMed,
                    TTF_Font* fSmall, Menu& menu, Uint32 ticks,
                    SDL_Texture* bgTex){

    drawMenuBackground(r,fBig,fSmall,ticks,bgTex);

    // Update hover states from mouse position
    auto inRect=[&](int rx,int ry,int rw,int rh){
        return menu.mouseX>=rx&&menu.mouseX<=rx+rw&&menu.mouseY>=ry&&menu.mouseY<=ry+rh;
    };

    // Title with pulse — no dark panel, just vivid colors
    float pulse=0.85f+0.15f*sinf(ticks*0.003f);
    Color titleCol={(Uint8)(60*pulse),(Uint8)(255*pulse),(Uint8)(100*pulse),255};
    drawText(r,fBig,"SNAKE",WIN_W/2,88,titleCol);
    drawText(r,fSmall,"CLASSIC EDITION",WIN_W/2,126,{80,255,130,255});

    // ── Difficulty panel ──────────────────────────────────────────
    int py=168;
    drawText(r,fSmall,"SELECT DIFFICULTY",WIN_W/2,py,{255,230,0,255});
    py+=30;

    Color diffCols[3]={C_EASY,C_MED,C_HARD};
    // Vivid unselected colors — bright enough to read on any bg without dark panel
    Color diffUnsel[3]={
        { 50,255, 120,255},  // easy   — vivid green
        { 30,180,255,255},   // medium — ocean blue
        {255,  60, 60,255},  // hard   — vivid red
    };
    for(int i=0;i<3;i++){
        bool sel=(menu.selectedDiff==i);
        int bx=WIN_W/2-120, bw=240, bh=52, by=py+i*62;
        bool hov=inRect(bx,by,bw,bh);

        // Box fill — semi-transparent colored bg so it stands out on photo
        SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
        if(sel){
            Color dc=diffCols[i];
            SDL_SetRenderDrawColor(r,dc.r,dc.g,dc.b,100);
        } else if(hov){
            SDL_SetRenderDrawColor(r,255,255,255,40);
        } else {
            SDL_SetRenderDrawColor(r,0,0,0,80);
        }
        fillRect(r,bx,by,bw,bh);
        SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_NONE);

        // Border
        Color bc= sel ? diffCols[i] : (hov ? diffUnsel[i] : Color{180,180,200,255});
        drawBorder(r,bx,by,bw,bh,sel?2:1,bc);

        // Label — always vivid, no dark panel needed
        Color lc = sel ? diffCols[i] : diffUnsel[i];
        drawText(r,fMed,DIFF[i].label,WIN_W/2,by+16,lc);
        // Desc — vivid cyan so it pops on any background
        Color dc2 = sel ? Color{100,255,230,255} : Color{0,230,255,255};
        drawText(r,fSmall,DIFF[i].desc,WIN_W/2,by+36,dc2);
    }
    py += 3*62 + 10;

    // ── PLAY button ──────────────────────────────────────────────
    {
        int bx=WIN_W/2-90,bw=180,bh=48,by=py+10;
        menu.playX=bx; menu.playY=by; menu.playW=bw; menu.playH=bh;
        menu.hoverPlay=inRect(bx,by,bw,bh);
        float pp=menu.hoverPlay ? 1.0f : 0.7f+0.3f*sinf(ticks*0.005f);

        SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
        // Filled bg — solid when hovered
        if(menu.hoverPlay){
            SDL_SetRenderDrawColor(r,60,200,90,220);
        } else {
            SDL_SetRenderDrawColor(r,(Uint8)(C_HEAD.r*pp),(Uint8)(C_HEAD.g*pp),(Uint8)(C_HEAD.b*pp),80);
        }
        fillRect(r,bx,by,bw,bh);
        SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_NONE);
        drawBorder(r,bx,by,bw,bh,2,menu.hoverPlay?Color{150,255,170,255}:C_HEAD);
        // Always white text on play button
        drawText(r,fMed,"PLAY",WIN_W/2,by+24,{255,255,255,255});    }
    py+=66;

    // ── HIGH SCORES button ───────────────────────────────────────
    {
        int bx=WIN_W/2-90,bw=180,bh=40,by=py+6;
        menu.scoresX=bx; menu.scoresY=by; menu.scoresW=bw; menu.scoresH=bh;
        menu.hoverScores=inRect(bx,by,bw,bh);

        SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
        if(menu.hoverScores){
            SDL_SetRenderDrawColor(r,200,100,0,180);
        } else {
            SDL_SetRenderDrawColor(r,180,90,0,30);
        }
        fillRect(r,bx,by,bw,bh);
        SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_NONE);
        drawBorder(r,bx,by,bw,bh,menu.hoverScores?2:1,
                   menu.hoverScores?Color{255,200,80,255}:Color{255,140,0,255});
        // Bright text always
        drawText(r,fSmall,"HIGH SCORES",WIN_W/2,by+20,
                 menu.hoverScores?Color{255,255,255,255}:Color{255,140,0,255});
    }
    py+=56;

    // ── QUIT button ──────────────────────────────────────────────
    {
        int bx=WIN_W/2-70,bw=140,bh=34,by=py+4;
        menu.quitX=bx; menu.quitY=by; menu.quitW=bw; menu.quitH=bh;
        menu.hoverQuit=inRect(bx,by,bw,bh);

        SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
        if(menu.hoverQuit) { SDL_SetRenderDrawColor(r,180,50,50,160); fillRect(r,bx,by,bw,bh); }
        SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_NONE);
        drawBorder(r,bx,by,bw,bh,menu.hoverQuit?2:1,
                   menu.hoverQuit?Color{255,100,100,255}:Color{100,100,115,255});
        drawText(r,fSmall,"QUIT  [Q]",WIN_W/2,by+17,
                 menu.hoverQuit?Color{255,100,100,255}:Color{255,80,200,255});
    }

    // Controls hint — bright yellow
    drawText(r,fSmall,"UP/DOWN to select difficulty   ENTER to play",
             WIN_W/2,WIN_H-18,{255,230,0,255});
}

// ─── Draw: scores screen ──────────────────────────────────────────────────────
void drawScoresScreen(SDL_Renderer* r, TTF_Font* fBig, TTF_Font* fMed,
                      TTF_Font* fSmall, Uint32 ticks){
    drawMenuBG(r,ticks);

    drawText(r,fBig,"HIGH SCORES",WIN_W/2,50,C_GOLD);
    drawText(r,fSmall,"Press ESC or BACKSPACE to go back",WIN_W/2,WIN_H-20,{80,80,100,255});

    auto scores=loadScores();
    Color diffCols[3]={C_EASY,C_MED,C_HARD};

    if(scores.empty()){
        drawText(r,fMed,"No scores yet — go play!",WIN_W/2,WIN_H/2,{100,100,120,255});
        return;
    }

    // Header row
    int ty=100;
    drawText(r,fSmall,"RANK",  80,  ty,{100,100,120,255});
    drawText(r,fSmall,"SCORE", WIN_W/2,ty,{100,100,120,255});
    drawText(r,fSmall,"DIFF",  WIN_W-80,ty,{100,100,120,255});
    ty+=8;
    setColor(r,{40,40,55,255});
    fillRect(r,30,ty,WIN_W-60,1);
    ty+=18;

    for(int i=0;i<(int)scores.size();i++){
        auto& e=scores[i];
        Color rc=(i==0)?C_GOLD:(i==1)?Color{180,180,190,255}:(i==2)?Color{180,120,60,255}:C_TEXT;
        std::string rank= (i==0)?"#1 GOLD":(i==1)?"#2 SILVER":(i==2)?"#3 BRONZE":"#"+std::to_string(i+1);
        drawText(r,fSmall,rank,80,ty,rc);
        drawText(r,fMed,std::to_string(e.score),WIN_W/2,ty,C_TEXT);
        drawText(r,fSmall,DIFF[e.diff].label,WIN_W-80,ty,diffCols[e.diff]);
        ty+=36;
    }
}

// ─── Draw: HUD ────────────────────────────────────────────────────────────────
void drawHUD(SDL_Renderer* r, TTF_Font* fBig, TTF_Font* fSmall, const Game& g){
    setColor(r,C_HUD_BG); fillRect(r,0,0,WIN_W,HUD_H);
    setColor(r,{40,40,50,255}); fillRect(r,0,HUD_H-1,WIN_W,1);

    Color dc[]={C_EASY,C_MED,C_HARD};
    drawText(r,fBig,"SNAKE",WIN_W/2,18,C_HEAD);
    drawText(r,fSmall,DIFF[g.diff].label,WIN_W/2,42,dc[g.diff]);
    drawText(r,fSmall,"SCORE: "+std::to_string(g.score),80,42,C_TEXT);
    drawText(r,fSmall,"BEST:  "+std::to_string(g.highScore),WIN_W-80,42,C_GOLD);
}

// ─── Draw: food ───────────────────────────────────────────────────────────────
void drawFood(SDL_Renderer* r, Point f, Uint32 ticks){
    float pulse=0.5f+0.5f*SDL_sinf(ticks*0.005f);
    int glow=(int)(3+3*pulse);
    int px=f.x*CELL,py=f.y*CELL+HUD_H;
    SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
    setColor(r,{C_FOOD_GLOW.r,C_FOOD_GLOW.g,C_FOOD_GLOW.b,(Uint8)(80*pulse)});
    fillRect(r,px-glow,py-glow,CELL+2*glow,CELL+2*glow);
    SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_NONE);
    setColor(r,{(Uint8)(C_FOOD.r-20),(Uint8)(C_FOOD.g-20),(Uint8)(C_FOOD.b-20),255});
    fillRect(r,px+2,py+2,CELL-4,CELL-4);
    setColor(r,C_FOOD); fillRect(r,px+4,py+4,CELL-8,CELL-8);
    setColor(r,{255,200,200,255}); fillRect(r,px+5,py+5,3,3);
}

// ─── Draw: snake ─────────────────────────────────────────────────────────────
void drawSnake(SDL_Renderer* r, const std::deque<Point>& snake){
    for(int i=(int)snake.size()-1;i>=0;i--){
        auto& p=snake[i];
        if(i==0){
            drawCell(r,p.x,p.y,C_HEAD,{30,80,40,255});
            int px=p.x*CELL,py=p.y*CELL+HUD_H;
            setColor(r,{10,10,10,255});
            fillRect(r,px+5,py+5,3,3);
            fillRect(r,px+12,py+5,3,3);
        } else {
            Color fill=(i%2==0)?C_BODY:C_BODY2;
            drawCell(r,p.x,p.y,fill,{20,60,30,255});
        }
    }
}

// ─── Draw: game grid ─────────────────────────────────────────────────────────
void drawGrid(SDL_Renderer* r){
    setColor(r,C_GRID);
    for(int x=0;x<=COLS;x++) SDL_RenderDrawLine(r,x*CELL,HUD_H,x*CELL,WIN_H);
    for(int y=0;y<=ROWS;y++) SDL_RenderDrawLine(r,0,y*CELL+HUD_H,WIN_W,y*CELL+HUD_H);
}

// ─── Draw: game overlay (death / pause) ───────────────────────────────────────
void drawGameOverlay(SDL_Renderer* r, TTF_Font* fBig, TTF_Font* fMed,
                     TTF_Font* fSmall, const Game& g, Uint32 ticks){
    SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
    setColor(r,C_OVERLAY); fillRect(r,0,HUD_H,WIN_W,WIN_H-HUD_H);
    SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_NONE);
    int cy=HUD_H+(WIN_H-HUD_H)/2;
    if(g.dead){
        float pulse=0.7f+0.3f*SDL_sinf(ticks*0.004f);
        Color rc={255,(Uint8)(80*pulse),(Uint8)(80*pulse),255};
        drawText(r,fBig,"GAME OVER",WIN_W/2,cy-70,rc);
        drawText(r,fMed,"Score: "+std::to_string(g.score),WIN_W/2,cy-20,C_TEXT);
        if(g.score>0&&g.score==g.highScore)
            drawText(r,fMed,"NEW HIGH SCORE!",WIN_W/2,cy+20,C_GOLD);
        drawText(r,fSmall,"R / ENTER  --->  Restart",WIN_W/2,cy+60,{180,180,200,255});
        drawText(r,fSmall,"ESC        --->  Main Menu",WIN_W/2,cy+84,{180,180,200,255});
    } else if(g.paused){
        drawText(r,fBig,"PAUSED",WIN_W/2,cy-30,C_TEXT);
        drawText(r,fSmall,"P / ESC  --->  Resume",WIN_W/2,cy+20,{180,180,200,255});
        drawText(r,fSmall,"M        --->  Main Menu",WIN_W/2,cy+44,{180,180,200,255});
    }
}

// ─── Font loader ─────────────────────────────────────────────────────────────
TTF_Font* loadFont(int size){
    const char* paths[]={
        "C:\\Windows\\Fonts\\arialbd.ttf",
        "C:\\Windows\\Fonts\\arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSansBold.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        nullptr
    };
    for(int i=0;paths[i];i++){
        TTF_Font* f=TTF_OpenFont(paths[i],size);
        if(f) return f;
    }
    return nullptr;
}

// ─── Main ────────────────────────────────────────────────────────────────────
int main(int /*argc*/, char** /*argv*/){
    srand((unsigned)time(nullptr));
    if(SDL_Init(SDL_INIT_VIDEO)<0){ std::cerr<<"SDL: "<<SDL_GetError()<<"\n"; return 1; }
    if(TTF_Init()<0){ std::cerr<<"TTF: "<<TTF_GetError()<<"\n"; return 1; }

    SDL_Window* win=SDL_CreateWindow("Snake — Classic",
        SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,WIN_W,WIN_H,SDL_WINDOW_SHOWN);
    if(!win){ std::cerr<<"Window: "<<SDL_GetError()<<"\n"; return 1; }

    SDL_Renderer* rnd=SDL_CreateRenderer(win,-1,
        SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    if(!rnd){ std::cerr<<"Renderer: "<<SDL_GetError()<<"\n"; return 1; }
    SDL_SetRenderDrawBlendMode(rnd,SDL_BLENDMODE_BLEND);

    TTF_Font* fBig  =loadFont(36);
    TTF_Font* fMed  =loadFont(22);
    TTF_Font* fSmall=loadFont(14);

    // Try to load background image (put bg.bmp in project_150 folder)
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

                // ── MENU input ──
                if(screen==SCREEN_MENU){
                    if(k==SDLK_q||k==SDLK_ESCAPE) quit=true;
                    if(k==SDLK_UP||k==SDLK_w)
                        menu.selectedDiff=(menu.selectedDiff+2)%3;
                    if(k==SDLK_DOWN||k==SDLK_s)
                        menu.selectedDiff=(menu.selectedDiff+1)%3;
                    if(k==SDLK_RETURN||k==SDLK_SPACE){
                        game.reset(menu.selectedDiff);
                        screen=SCREEN_GAME;
                    }
                    if(k==SDLK_h||k==SDLK_F1) screen=SCREEN_SCORES;
                }
                // ── SCORES input ──
                else if(screen==SCREEN_SCORES){
                    if(k==SDLK_ESCAPE||k==SDLK_BACKSPACE||k==SDLK_q)
                        screen=SCREEN_MENU;
                }
                // ── GAME input ──
                else if(screen==SCREEN_GAME){
                    if(k==SDLK_q) quit=true;
                    if(game.dead){
                        if(k==SDLK_r||k==SDLK_RETURN){
                            game.reset(menu.selectedDiff);
                        }
                        if(k==SDLK_ESCAPE||k==SDLK_m){
                            screen=SCREEN_MENU;
                        }
                    } else {
                        if(k==SDLK_m&&game.paused) screen=SCREEN_MENU;
                        game.handleKey(k);
                    }
                }
            }
            // Mouse clicks on menu buttons
            if(ev.type==SDL_MOUSEBUTTONDOWN&&screen==SCREEN_MENU){
                int mx=ev.button.x,my=ev.button.y;
                // Diff boxes
                int py0=198;
                for(int i=0;i<3;i++){
                    int by=py0+i*62,bx=WIN_W/2-120;
                    if(mx>=bx&&mx<=bx+240&&my>=by&&my<=by+52)
                        menu.selectedDiff=i;
                }
                if(menu.hoverPlay)   { game.reset(menu.selectedDiff); screen=SCREEN_GAME; }
                if(menu.hoverScores) screen=SCREEN_SCORES;
                if(menu.hoverQuit)   quit=true;
            }
            if(ev.type==SDL_MOUSEMOTION&&screen==SCREEN_MENU){
                menu.mouseX=ev.motion.x;
                menu.mouseY=ev.motion.y;
            }
        }

        if(screen==SCREEN_GAME) game.tick();

        // ── Render ──
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
            if(game.dead||game.paused)
                drawGameOverlay(rnd,fBig,fMed,fSmall,game,ticks);
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