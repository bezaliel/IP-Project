#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#include "allegro5/allegro.h"
#include "allegro5/allegro_image.h"
#include "allegro5/allegro_primitives.h"

#define TILEMAP_HEIGHT 1024 
#define TILEMAP_WIDTH 1024
#define TILE_SIZE 32
#define FPS 60
#define NTREES 4
#define NROCKS 2
#define NHOLES 4
#define NFLAGS 2
#define NBULLETS 10
#define BULLET_SPEED 3

#define WALKABLE 1
enum TileMapObjects {WALL = 2, TREE, ROCK, MANHOLE_IN, MANHOLE_OUT, FLAG_1, FLAG_2};
enum Teams{TEAM_1, TEAM_2};
enum Movements {UP, DOWN, LEFT, RIGHT};
enum Actions {FIRE, HOLE_IN, DROP_FLAG};
typedef struct{
	float x;
	float y;
} Point;
typedef struct{
	float module;
	float dx;
	float dy;
} Vector;
typedef struct{
	int id;
	int team;
	int live;
	int kills;
	int deaths;
	bool has_team_flag;
	bool has_enemy_flag;
	
	float speed_orto;
	float speed_diag;
	Point position;
} Player;

typedef struct{
	int l;
	int c;
	int type;
} TileObject;

typedef struct{
	int live;
	Vector speed;
	Point position;
} Bullet;

typedef struct{
	int src_l;
	int src_c;
	bool has_catched;
	TileObject tile;
} Flag;

//Objetos da tile simples
TileObject trees[NTREES];
TileObject rocks[NROCKS];
TileObject manholes_in[NHOLES/2];
TileObject manholes_out[NHOLES/2];

//Objetos da tile avançados
Flag flags[NFLAGS];

//Projéteis
Bullet bullets[NBULLETS];

ALLEGRO_BITMAP *bitmap_tree = NULL;
ALLEGRO_BITMAP *bitmap_rock = NULL;
ALLEGRO_BITMAP *bitmap_manhole_in = NULL;
ALLEGRO_BITMAP *bitmap_manhole_out = NULL;

//Funções do Tile map
int loadTileMapMatrix(char *map, int l, int c);
int showTileMapMatrix(char *map, int l, int c);
int getTileContent(char *map, int l, int c, int pos_x, int pos_y);
void setTileContent(char *map, int l, int c, int pos_x, int pos_y, int tile_id);
void showTileContent(int tile_object_id);

//Validar movimentos
int isWalkable(char *map, int l, int c, int pos_x, int pos_y);
int isPassable(char *map, int l, int c, int pos_x, int pos_y);

//Player
void spawnPlayer(Player *player, int id, int team, int x, int y);
void respawnPlayer(Player *player);

//Flag
int catchEnemyFlag(char *map, int l, int c, int pos_x, int pos_y, int player_team);
int catchTeamFlag(char *map, int l, int c, int pos_x, int pos_y, int player_team);
int dropEnemyFlag(char *map, int l, int c, int pos_x, int pos_y, int player_team);
int dropTeamFlag(char *map, int l, int c, int pos_x, int pos_y, int player_team);

int hasManHoleIn(char *map, int l, int c, int pos_x, int pos_y);

void processBullets(char *map, int l, int c);
void fireBullet(Vector *vector, float pos_x, float pos_y);

//Desenhar tiles individuais
void drawTrees();
void drawRocks();
void drawFlags();
void drawHolesIn();
void drawHolesOut();

void drawBullets();

int main(){
	register int i;
	int l = TILEMAP_HEIGHT/TILE_SIZE, c = TILEMAP_WIDTH/TILE_SIZE;
	char *map = (char *) malloc(sizeof(char)*l*c);
	loadTileMapMatrix(map, l, c);
	showTileMapMatrix(map, l, c);
	
	ALLEGRO_TIMER *timer = NULL;
    ALLEGRO_EVENT_QUEUE *queue = NULL;
    ALLEGRO_DISPLAY *display = NULL;
    bool redraw = true, run = true;
    
    //Iniciar componentes da Allegro
	al_init();
    al_init_image_addon();
    al_init_primitives_addon();
    al_install_mouse();
    al_install_keyboard();
    
    //Configurar propriedades do display
   	display = al_create_display(800, 640);
    al_set_window_title(display, "BUILD ZERO");
    al_clear_to_color(al_map_rgb(0, 0, 0));
    
    //Desenhar mapa estático
    ALLEGRO_BITMAP *background = al_load_bitmap("map.png");
    
    bitmap_tree = al_load_bitmap("tree.png");
	bitmap_rock = al_load_bitmap("rock.png");
    bitmap_manhole_out = al_load_bitmap("manhole_out.png");
	bitmap_manhole_in = al_load_bitmap("manhole_in.png");
    
    //Criar timer
    timer = al_create_timer(1.0 / FPS);
    
    //Criar fila de eventos e registrar fontes de eventos
    queue = al_create_event_queue();
    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_mouse_event_source());
    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_timer_event_source(timer));
    
    //Iniciar timer
    al_start_timer(timer);
    
    
    float mouse_x, mouse_y, module, w, h;
    bool dir[] = {0, 0, 0, 0};
    bool actions[] = {0, 0, 0};
    int flag_drop_timer = 0;
    
    Player *player = (Player *) malloc(sizeof(Player));
    spawnPlayer(player, 0, TEAM_1, 32*4+16, 48);
    Player *player_team = (Player *) malloc(sizeof(Player)*4);
    Player *enemy_team = (Player *) malloc(sizeof(Player)*5);
    
    ALLEGRO_TRANSFORM transform;
    ALLEGRO_EVENT event;
    Vector vector;
  	while(run){
  		w = al_get_display_width(display);
		h = al_get_display_height(display);
		
		//Capturar evento
		al_wait_for_event(queue, &event);
		
		if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) run = 0;
			
		//Capturar WASD
		if (event.type == ALLEGRO_EVENT_KEY_DOWN){
			if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) run = 0;
			if(event.keyboard.keycode == ALLEGRO_KEY_W) dir[UP] = 1;
			if(event.keyboard.keycode == ALLEGRO_KEY_S) dir[DOWN] = 1;
			if(event.keyboard.keycode == ALLEGRO_KEY_A) dir[LEFT] = 1;
			if(event.keyboard.keycode == ALLEGRO_KEY_D) dir[RIGHT] = 1;
			if(event.keyboard.keycode == ALLEGRO_KEY_E) actions[HOLE_IN] = 1;
			if(event.keyboard.keycode == ALLEGRO_KEY_Q) actions[DROP_FLAG] = 1;
		}
		if (event.type == ALLEGRO_EVENT_KEY_UP){
			if(event.keyboard.keycode == ALLEGRO_KEY_W) dir[UP] = 0;
			if(event.keyboard.keycode == ALLEGRO_KEY_S) dir[DOWN] = 0;
			if(event.keyboard.keycode == ALLEGRO_KEY_A) dir[LEFT] = 0;
			if(event.keyboard.keycode == ALLEGRO_KEY_D) dir[RIGHT] = 0;
			if(event.keyboard.keycode == ALLEGRO_KEY_E) actions[HOLE_IN] = 0;
			if(event.keyboard.keycode == ALLEGRO_KEY_Q) actions[DROP_FLAG] = 0;
		}
		
		//Capturar botão esquerdo do mouse
		if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
			if(event.mouse.button == 1){
				//Reverter transformação player_x + w/2
				mouse_x = event.mouse.x - w*0.5;
				mouse_y = event.mouse.y - h*0.5;
				module = sqrt(mouse_x*mouse_x+mouse_y*mouse_y);
				vector.dx = BULLET_SPEED / (module/mouse_x);
				vector.dy = BULLET_SPEED / (module/mouse_y);
				vector.module = BULLET_SPEED;
				fireBullet(&vector, player->position.x, player->position.y);
				//showTileContent(getTileContent(map, l, c, mouse_x, mouse_y));
			}
		}
		
		//Evento de timer: processar e redesenhar
		if (event.type == ALLEGRO_EVENT_TIMER){
			if(dir[UP]){
				vector.dx = 0, vector.dy = -player->speed_diag;
				if(dir[LEFT]){
					vector.dx -= player->speed_diag;
				}else if(dir[RIGHT]){
					vector.dx += player->speed_diag;
				}else{
					vector.dy = -player->speed_orto;
				}
				if(isWalkable(map, l, c, player->position.x + vector.dx, player->position.y + vector.dy)){
					player->position.x += vector.dx, player->position.y += vector.dy;
				}
			}else if(dir[DOWN]){
				vector.dx = 0, vector.dy = player->speed_diag;
				if(dir[LEFT]){
					vector.dx -= player->speed_diag;
				}else if(dir[RIGHT]){
					vector.dx += player->speed_diag;
				}else{
					vector.dy = +player->speed_orto;
				}
				if(isWalkable(map, l, c, player->position.x + vector.dx, player->position.y + vector.dy)){
					player->position.x += vector.dx, player->position.y += vector.dy;
				}
			}else if(dir[LEFT]){
				if(isWalkable(map, l, c, player->position.x - player->speed_orto, player->position.y))
					player->position.x -= player->speed_orto;
			}else if(dir[RIGHT]){
				if(isWalkable(map, l, c, player->position.x + player->speed_orto, player->position.y))
					player->position.x += player->speed_orto;
			}
			//collisionDetecion(player);
			if(player->live > 0){
				if(!flag_drop_timer){
					if(!player->has_team_flag){
						player->has_team_flag = catchTeamFlag(map, l, c, player->position.x, player->position.y, player->team);
						if(player->has_team_flag){
							printf("Catched Team flag!\n");
						}
					}
					if(!player->has_enemy_flag){
						player->has_enemy_flag = catchEnemyFlag(map, l, c, player->position.x, player->position.y, player->team);
						if(player->has_enemy_flag){
							printf("Catched Enemy flag!\n");
						}
					}
				}else{
					flag_drop_timer--;
				}
				if(actions[HOLE_IN] && hasManHoleIn(map, l, c, player->position.x, player->position.y)){
					if(player->position.x < TILEMAP_WIDTH/2){
						player->position.x = 11*32;
						player->position.y = 13*32;
					}else{
						player->position.x = TILEMAP_WIDTH-11*32;
						player->position.y = 13*32;
					}
				}
				if(actions[DROP_FLAG]){
					if(player->has_team_flag)
						player->has_team_flag = dropTeamFlag(map, l, c, player->position.x, player->position.y, player->team);
					if(player->has_enemy_flag)
						player->has_enemy_flag = dropEnemyFlag(map, l, c, player->position.x, player->position.y, player->team);
					flag_drop_timer = 120;
				}
			}else{
				if(player->has_team_flag)
					player->has_team_flag = dropTeamFlag(map, l, c, player->position.x, player->position.y, player->team);
				if(player->has_enemy_flag)
					player->has_enemy_flag = dropEnemyFlag(map, l, c, player->position.x, player->position.y, player->team);
			}
			processBullets(map, l, c);
			redraw = true;
		}
		//Redesenhar
		if (redraw && al_is_event_queue_empty(queue)) {
			redraw = false;
			al_identity_transform(&transform);
			al_translate_transform(&transform, -(player->position.x), -(player->position.y));
			al_translate_transform(&transform, w * 0.5, h * 0.5);
			al_use_transform(&transform);
			al_clear_to_color(al_map_rgb(0, 0, 0));
			al_draw_bitmap(background, 0, 0, 0);
			drawHolesIn();
			al_draw_filled_rectangle(player->position.x, player->position.y, player->position.x + 10, player->position.y + 10, al_map_rgb(255, 10, 26));
			drawBullets();
			drawTrees();
			drawRocks();
			drawHolesOut();
			drawFlags();
			al_flip_display();
		}
    }
    
    //Destruir objetos
    //al_destroy_bitmap(tiles);
    al_destroy_bitmap(background);
    al_destroy_event_queue(queue);
    al_destroy_timer(timer);
    al_destroy_display(display);
}

//Ler tilemap de arquivo de texto
int loadTileMapMatrix(char *map, int l, int c){
	register int i, t = 0, r = 0, hin = 0, hout = 0;
	char b;
	FILE *file = fopen("tilemap.txt", "r");
	for(i = 0; i < l*c; i++){
		b = fgetc(file);
		if(b == '\n') b = fgetc(file);
		if(isalpha(b)){
			b-='A'-10;
		}else if(isdigit(b)){
			b-='0';
		}
		if(b == FLAG_1){
			flags[0].tile.l = flags[0].src_l = i/c;
			flags[0].tile.c = flags[0].src_c = i%c;
			flags[0].tile.type = b;
			flags[0].has_catched = 0;
		}else if(b == FLAG_2){
			flags[1].tile.l = flags[1].src_l = i/c;
			flags[1].tile.c = flags[1].src_c = i%c;
			flags[1].tile.type = b;
			flags[1].has_catched = 0;
		}else if(b == TREE){
			trees[t].l = i/c;
			trees[t].c = i%c;
			trees[t].type = b;
			t++;
		}else if(b == ROCK){
			rocks[r].l = i/c;
			rocks[r].c = i%c;
			rocks[r].type = b;
			r++;
		}else if(b == MANHOLE_IN){
			manholes_in[hin].l = i/c;
			manholes_in[hin].c = i%c;
			manholes_in[hin].type = b;
			hin++;
		}else if(b == MANHOLE_OUT){
			manholes_out[hout].l = i/c;
			manholes_out[hout].c = i%c;
			manholes_out[hout].type = b;
			hout++;
		}
		*(map+i) = b;
	}
}

//Mostrar tilemap no terminal
int showTileMapMatrix(char *map, int l, int c){
	register int i, j;
	for(i = 0; i < l; i++){
		printf("%d", *(map+(i*c)));
		for(j = 1; j < c; j++){
			printf(" %d", *(map+(i*c+j)));
		}
		printf("\n");
	}
}
//Processar projéteis
void processBullets(char *map, int l, int c){
	register int i;
	for(i = 0; i < NBULLETS; i++){
		if(bullets[i].live){
			bullets[i].position.x += bullets[i].speed.dx;
			bullets[i].position.y += bullets[i].speed.dy;
			if(!isPassable(map, l, c, bullets[i].position.x, bullets[i].position.y)){
				bullets[i].live = 0;
			}else{
				bullets[i].live--;
			}
		}
	}
}
//Lançar projétil
void fireBullet(Vector *vector, float pos_x, float pos_y){
	register int i;
	for(i = 0; i < NBULLETS; i++){
		if(!bullets[i].live){
			bullets[i].position.x = pos_x;
			bullets[i].position.y = pos_y;
			bullets[i].speed.module = vector->module;
			bullets[i].speed.dx = vector->dx;
			bullets[i].speed.dy = vector->dy;
			bullets[i].live = 50;
			break;
		}
	}
}
//Desenhar bandeiras
void drawFlags(){
	register int i;
	int pos_x, pos_y;
	ALLEGRO_COLOR flag_colors[2] = {al_map_rgb(255, 10, 26), al_map_rgb(100, 10, 26)};
	for(i = 0; i < NFLAGS; i++){
		if(!flags[i].has_catched){
			pos_x = flags[i].tile.c*32+16;
			pos_y = flags[i].tile.l*32+16;
			al_draw_filled_circle(pos_x, pos_y, 10, flag_colors[i]);
		}
	}
}
//Desenhar projéteis
void drawBullets(){
	register int i;
	for(i = 0; i < NBULLETS; i++){
		if(bullets[i].live){
			al_draw_filled_circle(bullets[i].position.x, bullets[i].position.y, 2, al_map_rgb(255, 10, 26));
		}
	}
}
//Desenhar árvores
void drawTrees(){
	register int i;
	int pos_x, pos_y;
	for(i = 0; i < NTREES; i++){
		pos_x = trees[i].c*32-16;
		pos_y = trees[i].l*32-16;
		al_draw_bitmap(bitmap_tree, pos_x, pos_y, 0);
	}
}
void drawRocks(){
	register int i;
	int pos_x, pos_y;
	for(i = 0; i < NROCKS; i++){
		pos_x = rocks[i].c*32-16;
		pos_y = rocks[i].l*32-32;
		al_draw_bitmap(bitmap_rock, pos_x, pos_y, 0);
	}
}
//Desenhar entradas de bueiros
void drawHolesIn(){
	register int i;
	int pos_x, pos_y;
	for(i = 0; i < NHOLES/2; i++){
		pos_x = manholes_in[i].c*32;
		pos_y = manholes_in[i].l*32;
		al_draw_bitmap(bitmap_manhole_in, pos_x, pos_y, 0);
	}
}
//Desenhar saídas de bueiros
void drawHolesOut(){
	register int i;
	int pos_x, pos_y;
	for(i = 0; i < NHOLES/2; i++){
		pos_x = manholes_out[i].c*32-32;
		pos_y = manholes_out[i].l*32-64;
		al_draw_bitmap(bitmap_manhole_out, pos_x, pos_y, 0);
	}
}
//Verificar se é uma posição acessível ao player
int isWalkable(char *map, int l, int c, int pos_x, int pos_y){
	int row = pos_y/TILE_SIZE, col = pos_x/TILE_SIZE;
	if(!isPassable(map, l, c, pos_x, pos_y)){
		return 0;
	}
	if(*(map+(row*c + col)) == !WALKABLE){
		return 0;
	}
	return 1;
}

//Verificar se é uma posição transponível
int isPassable(char *map, int l, int c, int pos_x, int pos_y){
	int row = pos_y/TILE_SIZE, col = pos_x/TILE_SIZE;
	if(pos_x < 0 || pos_y < 0 || pos_x > TILEMAP_WIDTH || pos_y > TILEMAP_HEIGHT){
		return 0;
	}
	if(*(map+(row*c + col)) == WALL) return 0;
	if(*(map+(row*c + col)) == TREE) return 0;
	if(*(map+(row*c + col)) == ROCK) return 0;
	return 1;
}

// Verificar o que há na tile pela posição
int getTileContent(char *map, int l, int c, int pos_x, int pos_y){
	int pos_l = pos_y/TILE_SIZE, pos_c = pos_x/TILE_SIZE;
	return *(map+(pos_l*c+pos_c));
}

// Atribuir valor a tile pela posição
void setTileContent(char *map, int l, int c, int pos_x, int pos_y, int tile_id){
	int pos_l = pos_y/TILE_SIZE, pos_c = pos_x/TILE_SIZE;
	*(map+(pos_l*c+pos_c)) = tile_id;
}
void spawnPlayer(Player *player, int id, int team, int x, int y){
	//Informações serão atribuídas pelo servidor
	player->id = id;
	player->team = team;
	player->position.x = x;
	player->position.y = y;
	player->live = 100;
	player->has_team_flag = 0;
	player->has_enemy_flag = 0;
	player->speed_orto = 2;
	player->speed_diag = sqrt(player->speed_orto)/1.4142135;
	player->kills = player->deaths = 0;
}
void respawnPlayer(Player *player){
	if(player->team == TEAM_1){
		player->position.x = 48;
		player->position.y = 48;
	}else{
		player->position.x = TILEMAP_WIDTH-48;
		player->position.y = TILEMAP_HEIGHT-48;
	}
	player->live = 100;
}

//Capturar bandeira inimiga
int catchEnemyFlag(char *map, int l, int c, int pos_x, int pos_y, int player_team){
	int enemy_flag = (player_team == TEAM_1) ? FLAG_2 : FLAG_1;
	if(getTileContent(map, l, c, pos_x, pos_y) == enemy_flag){
		flags[enemy_flag-FLAG_1].has_catched = 1;
		setTileContent(map, l, c, pos_x, pos_y, WALKABLE);
		return 1;
	}
	return 0;
}

//Capturar bandeira do time
int catchTeamFlag(char *map, int l, int c, int pos_x, int pos_y, int player_team){
	int team_flag = (player_team == TEAM_1) ? FLAG_1 : FLAG_2;
	if(flags[team_flag-FLAG_1].tile.l != flags[team_flag-FLAG_1].src_l && flags[team_flag-FLAG_1].tile.c != flags[team_flag-FLAG_1].src_c){
		if(getTileContent(map, l, c, pos_x, pos_y) == team_flag){
			flags[team_flag-FLAG_1].has_catched = 1;
			setTileContent(map, l, c, pos_x, pos_y, WALKABLE);
			return 1;
		}
	}
	return 0;
}

// Soltar bandeira inimiga na tile
int dropEnemyFlag(char *map, int l, int c, int pos_x, int pos_y, int player_team){
	int pos_l = pos_y/TILE_SIZE, pos_c = pos_x/TILE_SIZE;
	if(getTileContent(map, l, c, pos_x, pos_y) == WALKABLE){
		int enemy_flag = (player_team == TEAM_1) ? FLAG_2 : FLAG_1;
		flags[enemy_flag-FLAG_1].tile.l = pos_l;
		flags[enemy_flag-FLAG_1].tile.c = pos_c;
		flags[enemy_flag-FLAG_1].has_catched = 0;
		setTileContent(map, l, c, pos_x, pos_y, enemy_flag);
		return 0;
	}
	return 1;
}

// Soltar bandeira do time na tile
int dropTeamFlag(char *map, int l, int c, int pos_x, int pos_y, int player_team){
	int pos_l = pos_y/TILE_SIZE, pos_c = pos_x/TILE_SIZE;
	if(getTileContent(map, l, c, pos_x, pos_y) == WALKABLE){
		int team_flag = (player_team == TEAM_1) ? FLAG_1 : FLAG_2;
		flags[team_flag-FLAG_1].tile.l = pos_l;
		flags[team_flag-FLAG_1].tile.c = pos_c;
		flags[team_flag-FLAG_1].has_catched = 0;
		setTileContent(map, l, c, pos_x, pos_y, team_flag);
		return 0;
	}
	return 1;
}
// Verificar se tem um bueiro
int hasManHoleIn(char *map, int l, int c, int pos_x, int pos_y){
	if(getTileContent(map, l, c, pos_x, pos_y) == MANHOLE_IN) return 1;
	return 0;
}
