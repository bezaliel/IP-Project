#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>

#include "allegro5/allegro.h"
#include "allegro5/allegro_image.h"
#include "allegro5/allegro_primitives.h"

#define HEIGHT 1024 
#define WIDTH 1024
#define TILE_SIZE 32
#define FPS 60

#define WALKABLE 1
enum TileMapObjects{WALL = 2, TREE, ROCK, FLAG, MANHOLE, POINT1, POINT2};
enum Movements{UP, DOWN, LEFT, RIGHT};
enum Actions{FIRE};

// Funções do Tile map
int loadTileMapMatrix(char *map, int l, int c);
int showTileMapMatrix(char *map, int l, int c);
int getTileContent(char *map, int l, int c, int pos_x, int pos_y);
void showTileContent(int tile_object_id);
void tile_map_draw(int scroll_x){
	
}

// Validar movimentos
int isWalkable(char *map, int l, int c, int pos_x, int pos_y);
int isImpassable(char *map, int l, int c, int pos_x, int pos_y);

// Flag
int hasFlag(char *map, int l, int c, int pos_x, int pos_y);
int catchFlag(char *map, int l, int c, int pos_x, int pos_y);
int dropFlag(char *map, int l, int c, int pos_x, int pos_y);

int main(){
	register int i;
	int l = HEIGHT/TILE_SIZE, c = WIDTH/TILE_SIZE;
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
    
    //Draw Map
    ALLEGRO_BITMAP *background = al_load_bitmap("tilemap.png");
	ALLEGRO_BITMAP *tiles = al_create_bitmap(1024, 1024);
	al_set_target_bitmap(tiles);
	al_clear_to_color(al_map_rgba(0, 0, 0, 0));
	al_draw_bitmap(background, 0, 0, 0);
	al_set_target_backbuffer(display);
   	al_draw_bitmap(tiles, 0, 0, 0);
    
    //Criar timer
    timer = al_create_timer(1.0 / FPS);
    
    //Criar fila de eventos e registrar eventos
    queue = al_create_event_queue();
    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_mouse_event_source());
    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_timer_event_source(timer));
    
    //Iniciar timer
    al_start_timer(timer);
    float x, y;
    int transform = 0;
    int scroll_x = 0, scroll_y = 0;
    int player_x = 48, player_y = 48, player_speed = 3;
    char dir[] = {0, 0, 0, 0};
  	while(run){
		ALLEGRO_EVENT event;

		//Capturar evento
		al_wait_for_event(queue, &event);

		if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
			break;
		if (event.type == ALLEGRO_EVENT_KEY_DOWN){
			if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
				break;
			if(event.keyboard.keycode == ALLEGRO_KEY_W) dir[UP] = 1;
			if(event.keyboard.keycode == ALLEGRO_KEY_S) dir[DOWN] = 1;
			if(event.keyboard.keycode == ALLEGRO_KEY_A) dir[LEFT] = 1;
			if(event.keyboard.keycode == ALLEGRO_KEY_D) dir[RIGHT] = 1;
		}
		if (event.type == ALLEGRO_EVENT_KEY_UP){
			if(event.keyboard.keycode == ALLEGRO_KEY_W) dir[UP] = 0;
			if(event.keyboard.keycode == ALLEGRO_KEY_S) dir[DOWN] = 0;
			if(event.keyboard.keycode == ALLEGRO_KEY_A) dir[LEFT] = 0;
			if(event.keyboard.keycode == ALLEGRO_KEY_D) dir[RIGHT] = 0;
		}
		if (event.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
			if(event.mouse.button == 1){
				x = event.mouse.x+player_x-400;
				y = event.mouse.y+player_y-320;
				printf("Mouse click: %.2f %.2f\n", x, y);
				showTileContent(getTileContent(map, l, c, x, y));
			}
		}
		//Evento de timer, redesenhar
		if (event.type == ALLEGRO_EVENT_TIMER){
			if(dir[UP]){
				if(isWalkable(map, l, c, player_x, player_y-player_speed)){
					player_y -= player_speed;
				}
			}
			if(dir[DOWN]){
				if(isWalkable(map, l, c, player_x, player_y+player_speed)){
					player_y += player_speed;
				}
			}
			if(dir[LEFT]){
				if(isWalkable(map, l, c, player_x-player_speed, player_y)){
					player_x -= player_speed;
				}
			}
			if(dir[RIGHT]){
				if(isWalkable(map, l, c, player_x+player_speed, player_y)){
					player_x += player_speed;
				}
			}
			transform = 1;
			redraw = true;
		}
		//Redesenhar
		if (redraw && al_is_event_queue_empty(queue)) {
			redraw = false;
			transform = 0;
			ALLEGRO_TRANSFORM transform;
			int w = al_get_display_width(display);
			int h = al_get_display_height(display);
			al_identity_transform(&transform);
			al_translate_transform(&transform, -player_x, -player_y);
			al_translate_transform(&transform, w * 0.5, h * 0.5);
			al_use_transform(&transform);
			al_clear_to_color(al_map_rgb(0, 0, 0));
			al_draw_bitmap(background, 0, 0, 0);
			al_draw_filled_rectangle(player_x, player_y, player_x + 10, player_y + 10, al_map_rgb(255, 10, 26));
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
int loadTileMapMatrix(char *map, int l, int c){
	register int i, j;
	char b;
	FILE *file = fopen("tilemap.txt", "r");
	for(i = 0; i < l*c; i++){
		b = fgetc(file);
		if(b == '\n') b = fgetc(file);
		*(map+i) = b-'0';
	}
}
int showTileMapMatrix(char *map, int l, int c){
	register int i, j;
	for(i = 0; i < l; i++){
		printf("%d", *(map));
		for(j = 1; j < c; j++){
			printf(" %d", *(map+(i*c+j)));
		}
		printf("\n");
	}
}
int isWalkable(char *map, int l, int c, int pos_x, int pos_y){
	int row = pos_y/TILE_SIZE, col = pos_x/TILE_SIZE;
	if(pos_x < 0 || pos_y < 0 || pos_x > WIDTH || pos_y > HEIGHT || isImpassable(map, l, c, pos_x, pos_y) || *(map+(row*c + col)) == !WALKABLE) return 0;
	return 1;
}
int isImpassable(char *map, int l, int c, int pos_x, int pos_y){
	int row = pos_y/TILE_SIZE, col = pos_x/TILE_SIZE;
	if(*(map+(row*c + col)) == WALL) return 1;
	if(*(map+(row*c + col)) == TREE) return 1;
	if(*(map+(row*c + col)) == ROCK) return 1;
	return 0;
}
int catchFlag(char *map, int l, int c, int pos_x, int pos_y){
	if(hasFlag(map, l, c, pos_x, pos_y)){
		return 1;
	}
	return 0;
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
		case FLAG:{
			printf("FLAG.\n");
			break;
		}
		case MANHOLE:{
			printf("MANHOLE.\n");
			break;
		}
	}
}
int getTileContent(char *map, int l, int c, int pos_x, int pos_y){
	int pos_l = pos_y/TILE_SIZE, pos_c = pos_x/TILE_SIZE;
	return *(map+(pos_l*c+pos_c));
}
int hasFlag(char *map, int l, int c, int pos_x, int pos_y){
	int pos_l = pos_y/TILE_SIZE, pos_c = pos_x/TILE_SIZE;
	if(*(map+(pos_l*c+pos_c)) == FLAG) return 1;
	return 0;
}
int dropFlag(char *map, int l, int c, int pos_x, int pos_y){
	int pos_l = pos_y/TILE_SIZE, pos_c = pos_x/TILE_SIZE;
	*(map+(pos_l*c+pos_c)) = FLAG;
}
