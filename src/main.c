#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <time.h>

#define KEYSPACE 32
#define KEYDEL   80
#define KEYESC   27

#define MIN_NUM_ROOM 4
#define MAX_NUM_ROOM 8

#define MIN_ROOM_HEIGHT 4
#define MAX_ROOM_HEIGHT 8

#define MIN_ROOM_WIDTH 6
#define MAX_ROOM_WIDTH 12


typedef struct {
	int x; // Top left x-cordinate (x+width < COLS)
	int y; // Top left y-cordinate (y+width < LINES)
	int width;
	int height; // if either height or width equal to 0, room is invalid.
} Room;

typedef struct {
	int x;
	int y;
	unsigned int hp;
	unsigned int mana;
	unsigned int str;
	unsigned int wit;
	unsigned int dex;
	unsigned int gold;
} Player;

int nrooms = 0;

Player*
setplayer(unsigned int str,
	  unsigned int wit,
	  unsigned int dex)
{
	Player* player = malloc(sizeof(Player));
	//
	/* Player starts at the middle of the screen */
	player->x = COLS/2;
	player->y = LINES/2;
	player->hp = str * 10;
	player->mana = wit * 10;
	player->wit = wit;
	player->str = str;
	player->dex = dex;
	player->gold = 0;
	//
	return player;
}

void
delplayer(Player *player)
{
	free(player);
}

void
delrooms(Room **rooms)
{
	for (int i = 0; i < nrooms; i++) {
		free(rooms[i]);
	}
	free(rooms);
}



void setscr(void)
{
	initscr(); /* initialize ncurses: memory & clear screen */
	noecho(); /* do not write keypresses to screen */
	keypad(stdscr, TRUE); /* enable keypad to avoid escape sequences */
	curs_set(0); /* hide cursor */
}

/* Movements happen step by step. This function returns true or false when
 * depending if there will be a collision ahead.
 */
int
colliding(int y_amount, int x_amount, Player* player)
{
	int new_x_pos = player->x + x_amount;
	int new_y_pos = player->y + y_amount;

	int at_edge = (new_x_pos < 0) || (new_x_pos >= COLS) || (new_y_pos < 0) || (new_y_pos >= LINES);

	if (at_edge) {
		return 1;
	}

	switch (mvinch(player->y + y_amount, player->x + x_amount)) {
	case '-':
		return 1;
	case '|':
		return 1;
	}

	return 0;
}

int
isvalidroom(int ncreatedrooms, Room *room, Room **rooms)
{
	int x_rb = room->x + room->width; /* x coordinate or right bottom */
	int y_rb = room->y + room->height; /* y coordinate or right bottom */

	/* Check if room is visible. This check should not be needed when
	 * initilized correctly.
	 */
	int inmap = (room->x >= 0) || (x_rb >= COLS) || (room->y >= 0) || (y_rb >= LINES);

	if (!inmap) {
		free(room);
		return 0;
	}

	/* Check if wanted room collides with any other*/
	int x_collision, y_collision;
	for (int i = 0; i < ncreatedrooms; i++) {
		x_collision = room->x < rooms[i]->x+rooms[i]->width &&
			      x_rb > rooms[i]->x;
		y_collision = room->y < rooms[i]->y+rooms[i]->height &&
			      y_rb > rooms[i]->y;
		if (x_collision && y_collision) {
			free(room);
			return 0;
		}
	}

	return 1;
}

Room *
createroom(int x, int y, int width, int height)
{
	Room* room = malloc(sizeof(Room));
	/* Check if created room possible fits to screen */
	room->x = x;
	room->y = y;
	room->width = width;
	room->height = height;


	return room;
}

Room **
genrooms(void)
{
	nrooms = MIN_NUM_ROOM + rand()%(MAX_NUM_ROOM-MIN_NUM_ROOM);
	Room** rooms = malloc(nrooms * sizeof(Room));
	int ncreatedrooms = 0, width, height, y, x;

	srand(time(NULL));
	for (int i = 0; i < nrooms; i++) {
		width = MIN_ROOM_WIDTH + rand()%(MAX_ROOM_WIDTH-MIN_ROOM_WIDTH);
		height = MIN_ROOM_HEIGHT + rand()%(MAX_ROOM_HEIGHT-MIN_ROOM_HEIGHT);
		y = rand()%(LINES-height); // Generate rand 0...LINES
		x = rand()%(COLS-width); // Generate rand 0 ... COLS
		rooms[i] = createroom(x, y, width, height);
		if (isvalidroom(ncreatedrooms, rooms[i], rooms))
			ncreatedrooms++;
		else
			i--;
	}
	return rooms;
}

void
drawmap(Room **rooms)
{
	/* We need exactly room's width+1 space: |    |\0. */
	char room[MAX_ROOM_WIDTH+1] = "|";
	char edge[MAX_ROOM_WIDTH+1];
	int y; /* y coordinate to draw */

	for (int i = 0; i < nrooms; i++) {
		y = rooms[i]->y;
		/* Print top*/
		memset(&edge, '-', rooms[i]->width);
		edge[rooms[i]->width] = '\0';
		mvprintw(y++, rooms[i]->x, "%s", edge);

		/* Print empty space of room */
		memset(&room[1], ' ', rooms[i]->width-1); // Fill with dashes	
		room[rooms[i]->width-1] = '|';
		room[rooms[i]->width] = '\0';
		for (int j = 0; j < rooms[i]->height-1; j++)
			mvprintw(y++, rooms[i]->x, "%s", room);

		/* Print bottom*/
		mvprintw(y, rooms[i]->x, "%s", edge);
	}
	
}



void
player_move(int y_amount, int x_amount, Player *player)
{
	if (!colliding(y_amount, x_amount, player)) {
		player->y += y_amount;
		player->x += x_amount;
	}
	mvprintw(player->y, player->x, "@");
}

void
spawnplayer(Player *player, Room **rooms)
{
	/* Pick a room for the user to start.
	 * For now, first room is the selected room.
	 */
	player->x = rooms[0]->x + rooms[0]->width/2;
	player->y = rooms[0]->y + rooms[0]->height/2;
	player_move(0, 0, player);
}

int
handle_input(int key, Player *player, Room **rooms)
{
	switch (key) {
	case 'q': /* FALLTHROUGH */
	case 'Q': /* FALLTHROUGH */
	case KEYESC:
		return 0;
	case 'h': /* FALLTHROUGH */
	case KEY_LEFT:
		drawmap(rooms);
		player_move(0, -1, player);
		break;
	case 'j': /* FALLTHROUGH */
	case KEY_DOWN:
		drawmap(rooms);
		player_move(1, 0, player);
		break;
	case 'k': /* FALLTHROUGH */
	case KEY_UP:
		drawmap(rooms);
		player_move(-1, 0, player);
		break;
	case 'l': /* FALLTHROUGH */
	case KEY_RIGHT:
		drawmap(rooms);
		player_move(0, 1, player);
		break;

	}
	return 1;
}

int
main(void)
{
	int key = 1;
	setscr();

	Player *player = setplayer(10, 10, 10);
	Room **rooms = genrooms();

	/* Initial drawing */
	drawmap(rooms);
	spawnplayer(player, rooms);

	while (handle_input(key, player, rooms)) {
		key = getch();
	}

	delplayer(player);
	delrooms(rooms);

	endwin();
	return 0;
}
