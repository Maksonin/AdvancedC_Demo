#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <curses.h>
#include <inttypes.h>
#include <string.h>
#include <unistd.h>

#define MIN_Y    2
#define PLAYERS  5

double DELAY = 0.05;;
enum {LEFT=1, UP, RIGHT, DOWN, STOP_GAME=KEY_F(10),CONTROLS=5,PAUSE_GAME='p'};
enum {MAX_TAIL_SIZE=100, START_TAIL_SIZE=10, MAX_FOOD_SIZE=20, FOOD_EXPIRE_SECONDS=10,SEED_NUMBER=3};

// ����� ������ ���� ���������� �������
struct control_buttons
{
    int down;
    int up;
    int left;
    int right;
}control_buttons;

struct control_buttons default_controls[CONTROLS] = {{KEY_DOWN, KEY_UP, KEY_LEFT, KEY_RIGHT},
                                                    {'s', 'w', 'a', 'd'},
                                                    {'S', 'W', 'A', 'D'},
                                                    {'�', '�', '�', '�'},
                                                    {'�', '�', '�', '�'}};

/*
 ������ ������ �������� � ����
 x,y - ���������� ������� �������
 direction - ����������� ��������
 tsize - ������ ������
 *tail -  ������ �� �����
 */
typedef struct snake_t
{
    int x;
    int y;
    int direction;
    size_t tsize;
    struct tail_t *tail;
    struct control_buttons* controls;
} snake_t;

/*
 ����� ��� ������ ��������� �� ��������� x,y
 */
typedef struct tail_t
{
    int x;
    int y;
} tail_t;

struct food
{
    int x;
    int y;
    time_t put_time;
    char point;
    uint8_t enable;
} food[MAX_FOOD_SIZE];

void initTail(tail_t t[], size_t size)
{
    struct tail_t init_t={0,0};
    for(size_t i=0; i<size; i++)
    {
        t[i]=init_t;
    }
}
void initHead(snake_t *head, int x, int y)
{
    head->x = x;
    head->y = y;
    head->direction = RIGHT;
}

void initSnake(snake_t *head[], size_t size, int x, int y,int i)
{
    head[i]    = (snake_t*)malloc(sizeof(snake_t));
tail_t*  tail  = (tail_t*) malloc(MAX_TAIL_SIZE*sizeof(tail_t));
    initTail(tail, MAX_TAIL_SIZE);
    initHead(head[i], x, y);
    head[i]->tail     = tail; // ����������� � ������ �����
    head[i]->tsize    = size+1;
    head[i]->controls = default_controls;
    //~ head->controls = default_controls[1];
}

void initFood(struct food f[], size_t size)
{
    struct food init = {0,0,0,0,0};
    for(size_t i=0; i<size; i++)
    {
        f[i] = init;
    }
}

/*
 �������� ������ � ������ �������� ����������� ��������
 */
void go(struct snake_t *head)
{
    char ch = '@';
    int max_x=0, max_y=0;
    getmaxyx(stdscr, max_y, max_x); // macro - ������ ���������
    mvprintw(head->y, head->x, " "); // ������� ���� ������
    switch (head->direction)
    {
        case LEFT:
            if(head->x <= 0) // ����������� ��������, ����� ��
// ������� �� ������� ������
                head->x = max_x;
            mvprintw(head->y, --(head->x), "%c", ch);
        break;
        case RIGHT:
            if(head->x >= max_x)
                head->x = 0;
            mvprintw(head->y, ++(head->x), "%c", ch);
        break;
        case UP:
            if(head->y <= MIN_Y)
                head->y = max_y;
            mvprintw(--(head->y), head->x, "%c", ch);
        break;
        case DOWN:
            if(head->y >= max_y)
                head->y = MIN_Y;
            mvprintw(++(head->y), head->x, "%c", ch);
        break;
        default:
        break;
    }
    refresh();
}

void changeDirection(struct snake_t* snake, const int32_t key)
{
    for (int i = 0; i < CONTROLS; i++)
    {
        if (key == snake->controls[i].down)
            snake->direction = DOWN;
        else if (key == snake->controls[i].up)
            snake->direction = UP;
        else if (key == snake->controls[i].right)
            snake->direction = RIGHT;
        else if (key == snake->controls[i].left)
            snake->direction = LEFT;
    }
}

int checkDirection(snake_t* snake, int32_t key)
{
    for (int i = 0; i < CONTROLS; i++)
        if((snake->controls[i].down  == key && snake->direction==UP)    ||
           (snake->controls[i].up    == key && snake->direction==DOWN)  ||
           (snake->controls[i].left  == key && snake->direction==RIGHT) ||
           (snake->controls[i].right == key && snake->direction==LEFT))
        {
            return 0;
        }
    return 1;

}

/*
 �������� ������ � ������ �������� ������
 */
void goTail(struct snake_t *head)
{
    char ch = '*';
    mvprintw(head->tail[head->tsize-1].y, head->tail[head->tsize-1].x, " ");
    for(size_t i = head->tsize-1; i>0; i--)
    {
        head->tail[i] = head->tail[i-1];
        if( head->tail[i].y || head->tail[i].x)
            mvprintw(head->tail[i].y, head->tail[i].x, "%c", ch);
    }
    head->tail[0].x = head->x;
    head->tail[0].y = head->y;
}

/*
 ��������/���������� ������� ����� �� ����
 */
void putFoodSeed(struct food *fp)
{
    int max_x=0, max_y=0;
    char spoint[2] = {0};
    getmaxyx(stdscr, max_y, max_x);
    mvprintw(fp->y, fp->x, " ");
    fp->x = rand() % (max_x - 1);
    fp->y = rand() % (max_y - 2) + 1; //�� �������� ������� ������
    fp->put_time = time(NULL);
    fp->point = '$';
    fp->enable = 1;
    spoint[0] = fp->point;
    mvprintw(fp->y, fp->x, "%s", spoint);
}

/*
 ���������� ��� �� ����
 */
void putFood(struct food f[], size_t number_seeds)
{
    for(size_t i=0; i<number_seeds; i++)
    {
        putFoodSeed(&f[i]);
    }
}

void refreshFood(struct food f[], int nfood)
{
    for(size_t i=0; i<nfood; i++)
    {
        if( f[i].put_time )
        {
            if( !f[i].enable || (time(NULL) - f[i].put_time) > FOOD_EXPIRE_SECONDS )
            {
                putFoodSeed(&f[i]);
            }
        }
    }
}

void repairSeed(struct food f[], size_t nfood, struct snake_t *head)
{
    for( size_t i=0; i<head->tsize; i++ )
        for( size_t j=0; j<nfood; j++ )
        {
/* ���� ����� ��������� � ������ */
            if( f[j].x == head->tail[i].x && f[j].y == head->tail[i].y && f[i].enable )
            {
                mvprintw(1, 0, "Repair tail seed %u",j);
                putFoodSeed(&f[j]);
            }
        }
    for( size_t i=0; i<nfood; i++ )
        for( size_t j=0; j<nfood; j++ )
        {
/* ���� ��� ����� �� ����� ����� */
            if( i!=j && f[i].enable && f[j].enable && f[j].x == f[i].x && f[j].y == f[i].y && f[i].enable )
            {
                mvprintw(1, 0, "Repair same seed %u",j);
                putFoodSeed(&f[j]);
            }
        }
}


_Bool haveEat(struct snake_t *head, struct food f[])
{
    for(size_t i=0; i<MAX_FOOD_SIZE; i++)
        if( f[i].enable && head->x == f[i].x && head->y == f[i].y  )
        {
            f[i].enable = 0;
            return 1;
        }
    return 0;
}

/*
 ���������� ������ �� 1 �������
 */

void addTail(struct snake_t *head)
{
    if(head == NULL || head->tsize>MAX_TAIL_SIZE)
    {
        mvprintw(0, 0, "Can't add tail");
        return;
    }
    head->tsize++;
}

void printLevel(struct snake_t *head)
{
    int max_x = 0, max_y = 0;
    getmaxyx(stdscr, max_y, max_x);
    mvprintw(0, max_x - 10, "LEVEL: %d", head->tsize);
}

void printExit(struct snake_t *head)
{
    int max_x = 0, max_y = 0;
    getmaxyx(stdscr, max_y, max_x);
    mvprintw(max_y / 2, max_x / 2 - 5, "Your LEVEL is %d", head->tsize);
}

void update(struct snake_t *head, struct food f[], int key)
{
    clock_t begin = clock();
    go(head);
    goTail(head);
    if (checkDirection(head,key))
    {
        changeDirection(head, key);
    }
    refreshFood(food, SEED_NUMBER);// ��������� ���
    if (haveEat(head,food))
    {
        addTail(head);
        printLevel(head);
        DELAY -= 0.009;
    }
    refresh();//���������� ������, ������ ���� ��������
    while ((double)(clock() - begin)/CLOCKS_PER_SEC<DELAY)
    {}
}

void pause(void)
{
    int max_x = 0, max_y = 0;
    getmaxyx(stdscr, max_y, max_x);
    mvprintw(max_y / 2, max_x / 2 - 5, "Press P to continue game");
    while (getch() != PAUSE_GAME)
    {}
    mvprintw(max_y / 2, max_x / 2 - 5, "                        ");
}

_Bool isCrush(snake_t * snake)
{
        for(size_t i=1; i<snake->tsize; i++)
            if(snake->x == snake->tail[i].x && snake->y == snake->tail[i].y)
                return 1;
    return 0;
}



int main()
{
snake_t* snakes[PLAYERS];
    for (int i = 0; i < PLAYERS; i++)
        initSnake(snakes,START_TAIL_SIZE,10+i*10,10+i*10,i);
    initscr();
    keypad(stdscr, TRUE); // �������� F1, F2, ������� � �.�.
    raw();                // ��������� line buffering
    noecho();            // ��������� echo() ����� ��� ������ getch
    curs_set(FALSE);    //��������� ������
    mvprintw(0, 0,"Use arrows for control. Press 'F10' for EXIT");
    timeout(0);    //��������� ������� ����� ������� ������� � �����
    initFood(food, MAX_FOOD_SIZE);
    putFood(food, SEED_NUMBER);// ������ �����
    int key_pressed=0;
    while( key_pressed != STOP_GAME )
    {
        key_pressed = getch(); // ��������� �������
        for (int i = 0; i < PLAYERS; i++)
        {
            update(snakes[i], food, key_pressed);
            if(isCrush(snakes[i]))
                break;//!!!!!!
            repairSeed(food, SEED_NUMBER, snakes[i]);
        }
        if (key_pressed == PAUSE_GAME)
        {
            pause();
        }
    }
    for (int i = 0; i < PLAYERS; i++)
    {
        printExit(snakes[i]);
        free(snakes[i]->tail);
        free(snakes[i]);
    }
    endwin(); // ��������� ����� curses mod
    return 0;
}

