#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "allegro5/allegro.h"
#include "allegro5/allegro_image.h"
#include "allegro5/allegro_primitives.h"

#define TILEMAP_HEIGHT 1024 
#define TILEMAP_WIDTH 1024
#define TILE_SIZE 32
#define FPS 60
#define NTREES 4
#define NFLAGS 2

#define WALKABLE 1
enum TileMapObjects {WALL = 2, TREE, ROCK, MANHOLE_IN, MANHOLE_OUT, FLAG_1, FLAG_2};
enum Teams{TEAM_1, TEAM_2};
enum Movements {UP, DOWN, LEFT, RIGHT};
enum Actions {FIRE, HOLE, SUICIDE};
typedef struct {
	int id;
	int team;
	int live;
	int kills;
	int deaths;
	bool has_team_flag;
	bool has_enemy_flag;
	int x;
	int y;
	int speed;
} Player;

typedef struct{
	int src_l;
	int src_c;
} Tree;

typedef struct{
	int src_l;
	int src_c;
	int current_l;
	int current_c;
	bool has_catched;
} Flag;
Flag flags[NFLAGS];
Tree trees[NTREES];

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
int hasManHole(char *map, int l, int c, int pos_x, int pos_y);

//Desenhar tiles individuais
void drawFlags();
void drawTrees();

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
    ALLEGRO_BITMAP *background = al_load_bitmap("tilemap.png");
	ALLEGRO_BITMAP *tiles = al_create_bitmap(TILEMAP_WIDTH, TILEMAP_HEIGHT);
	al_set_target_bitmap(tiles);
	al_clear_to_color(al_map_rgba(0, 0, 0, 0));
	al_draw_bitmap(background, 0, 0, 0);
	al_set_target_backbuffer(display);
   	al_draw_bitmap(tiles, 0, 0, 0);
    
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
    
    
    float mouse_x, mouse_y, w, h;
    bool dir[] = {0, 0, 0, 0};
    bool actions[] = {0, 0, 0};
    Player *player = (Player *) malloc(sizeof(Player));
    spawnPlayer(player, 0, TEAM_1, 32*4+16, 48);
    Player *player_team = (Player *) malloc(sizeof(Player)*4);
    Player *enemy_team = (Player *) malloc(sizeof(Player)*5);
    ALLEGRO_TRANSFORM transform;
    ALLEGRO_EVENT event;
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
			if(event.keyboard.keycode == ALLEGRO_KEY_Q) actions[HOLE] = 1;
			if(event.keyboard.keycode == ALLEGRO_KEY_E) actions[SUICIDE] = 1;
		}
		if (event.type == ALLEGRO_EVENT_KEY_UP){
			if(event.keyboard.keycode == ALLEGRO_KEY_W) dir[UP] = 0;
			if(event.keyboard.keycode == ALLEGRO_KEY_S) dir[DOWN] = 0;
			if(event.keyboard.keycode == ALLEGRO_KEY_A) dir[LEFT] = 0;
			if(event.keyboard.keycode == ALLEGRO_KEY_D) dir[RIGHT] = 0;
			if(event.keyboard.keycode == ALLEGRO_KEY_Q) actions[HOLE] = 0;
			if(event.keyboard.keycode == ALLEGRO_KEY_E) actions[SUICIDE] = 0;
		}
		
		//Capturar botão esquerdo do mouse
		if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
			if(event.mouse.button == 1){
				mouse_x = event.mouse.x + player->x - w*0.5;
				mouse_y = event.mouse.y + player->y - h*0.5;
				showTileContent(getTileContent(map, l, c, mouse_x, mouse_y));
			}
		}
		
		//Evento de timer: processar e redesenhar
		
		if (event.type == ALLEGRO_EVENT_TIMER){
			if(dir[UP])
				if(isWalkable(map, l, c, player->x, player->y - player->speed))
					player->y -= player->speed;
			if(dir[DOWN])
				if(isWalkable(map, l, c, player->x, player->y + player->speed))
					player->y += player->speed;
			if(dir[LEFT])
				if(isWalkable(map, l, c, player->x - player->speed, player->y))
					player->x -= player->speed;
			if(dir[RIGHT])
				if(isWalkable(map, l, c, player->x + player->speed, player->y))
					player->x += player->speed;
			//collisionDetecion(player);
			if(player->live > 0){
				if(!player->has_team_flag){
					player->has_team_flag = catchTeamFlag(map, l, c, player->x, player->y, player->team);
					if(player->has_team_flag){
						printf("Catched Team flag!\n");
					}
				}
				if(!player->has_enemy_flag){
					player->has_enemy_flag = catchEnemyFlag(map, l, c, player->x, player->y, player->team);
					if(player->has_enemy_flag){
						printf("Catched Enemy flag!\n");
					}
				}
				if(actions[HOLE] && hasManHole(map, l, c, player->x, player->y)){
					if(player->x < TILEMAP_WIDTH/2){
						player->x = 11*32;
						player->y = 13*32;
					}else{
						player->x = TILEMAP_WIDTH-11*32;
						player->y = 13*32;
					}
				}
				if(actions[SUICIDE]) player->live = 0;
			}else{
				if(player->has_team_flag)
					player->has_team_flag = dropTeamFlag(map, l, c, player->x, player->y, player->team);
				if(player->has_enemy_flag)
					player->has_enemy_flag = dropEnemyFlag(map, l, c, player->x, player->y, player->team);
				respawnPlayer(player);
			}
			redraw = true;
		}
		//Redesenhar
		if (redraw && al_is_event_queue_empty(queue)) {
			redraw = false;
			al_identity_transform(&transform);
			al_translate_transform(&transform, -(player->x), -(player->y));
			al_translate_transform(&transform, w * 0.5, h * 0.5);
			al_use_transform(&transform);
			al_clear_to_color(al_map_rgb(0, 0, 0));
			al_draw_bitmap(background, 0, 0, 0);
			al_draw_filled_rectangle(player->x, player->y, player->x + 10, player->y + 10, al_map_rgb(255, 10, 26));
			drawFlags();
			drawTrees();
			al_flip_display();
		}
    }
    
    //Destruir objetos
    al_destroy_bitmap(tiles);
    al_destroy_bitmap(background);
    al_destroy_event_queue(queue);
    al_destroy_timer(timer);
    al_destroy_display(display);
}

//Ler tilemap de arquivo de texto
int loadTileMapMatrix(char *map, int l, int c){
	register int i, t = 0;
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
			flags[0].current_l = flags[0].src_l = i/c;
			flags[0].current_c = flags[0].src_c = i%c;
			flags[0].has_catched = 0;
		}
		if(b == FLAG_2){
			flags[1].current_l = flags[1].src_l = i/c;
			flags[1].current_c = flags[1].src_c = i%c;
			flags[1].has_catched = 0;
		}
		if(b == TREE){
			trees[t].src_l = i/c;
			trees[t].src_c = i%c;
			t++;
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
//Desenhar bandeiras
void drawFlags(){
	register int i;
	int pos_x, pos_y;
	ALLEGRO_COLOR flag_colors[2] = {al_map_rgb(255, 10, 26), al_map_rgb(100, 10, 26)};
	for(i = 0; i < 2; i++){
		if(!flags[i].has_catched){
			pos_x = flags[i].current_c*32+16;
			pos_y = flags[i].current_l*32+16;
			al_draw_filled_circle(pos_x, pos_y, 10, flag_colors[i]);
		}
	}
}
void drawTrees(){
	register int i;
	int pos_x, pos_y;
	for(i = 0; i < NTREES; i++){
		pos_x = trees[i].src_c*32+16;
		pos_y = trees[i].src_l*32+16;
		al_draw_filled_circle(pos_x, pos_y, 16, al_map_rgb(200, 125, 25));
	}
}
//Verificar se é uma posição acessível ao player
int isWalkable(char *map, int l, int c, int pos_x, int pos_y){
	int row = pos_y/TILE_SIZE, col = pos_x/TILE_SIZE;
	if(pos_x < 0 || pos_y < 0 || pos_x > TILEMAP_WIDTH || pos_y > TILEMAP_HEIGHT){
		return 0;
	}
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
	if(*(map+(row*c + col)) == WALL) return 0;
	if(*(map+(row*c + col)) == TREE) return 0;
	if(*(map+(row*c + col)) == ROCK) return 0;
	return 1;
}


void showTileContent(int tile_object_id){
	switch(tile_object_id){
		case !WALKABLE:{
			printf("NO WALLKABLE PLACE.\n");
			break;
		}
		case WALKABLE:{
			printf("WALLKABLE PLACE.\n");
			break;
		}
		case WALL:{
			printf("WALL.\n");
			break;
		}
		case TREE:{
			printf("TREE.\n");
			break;
		}
		case ROCK:{
			printf("ROCK.\n");
			break;
		}
		case MANHOLE_IN:{
			printf("MANHOLE IN.\n");
			break;
		}
		case MANHOLE_OUT:{
			printf("MANHOLE OUT.\n");
			break;
		}
		case FLAG_1:{
			printf("FLAG_1.\n");
			break;
		}
		case FLAG_2:{
			printf("FLAG 2.\n");
			break;
		}
	}
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
	player->x = x;
	player->y = y;
	player->live = 100;
	player->has_team_flag = 0;
	player->has_enemy_flag = 0;
	player->speed = 2;
	player->kills = player->deaths = 0;
}
void respawnPlayer(Player *player){
	if(player->team == TEAM_1){
		player->x = 48;
		player->y = 48;
	}else{
		player->x = TILEMAP_WIDTH-48;
		player->y = TILEMAP_HEIGHT-48;
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
	if(flags[team_flag-FLAG_1].current_l != flags[team_flag-FLAG_1].src_l && flags[team_flag-FLAG_1].current_c != flags[team_flag-FLAG_1].src_c){
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
	int enemy_flag = (player_team == TEAM_1) ? FLAG_2 : FLAG_1;
	flags[enemy_flag-FLAG_1].current_l = pos_l;
	flags[enemy_flag-FLAG_1].current_c = pos_c;
	flags[enemy_flag-FLAG_1].has_catched = 0;
	setTileContent(map, l, c, pos_x, pos_y, enemy_flag);
	return 0;
}

// Soltar bandeira do time na tile
int dropTeamFlag(char *map, int l, int c, int pos_x, int pos_y, int player_team){
	int pos_l = pos_y/TILE_SIZE, pos_c = pos_x/TILE_SIZE;
	int team_flag = (player_team == TEAM_1) ? FLAG_1 : FLAG_2;
	flags[team_flag-FLAG_1].current_l = pos_l;
	flags[team_flag-FLAG_1].current_c = pos_c;
	flags[team_flag-FLAG_1].has_catched = 0;
	setTileContent(map, l, c, pos_x, pos_y, team_flag);
	return 0;
}
// Verificar se tem um bueiro
int hasManHole(char *map, int l, int c, int pos_x, int pos_y){
	if(getTileContent(map, l, c, pos_x, pos_y) == MANHOLE_IN) return 1;
	return 0;
}
