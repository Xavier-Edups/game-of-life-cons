//Eduardo Xavier Dantas - 190086530

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#define MAXSIZE 100

//Estrutura de coordenadas de uma célula
struct Coord
{
  int x;
  int y;
};

void *Cell (void *arg);  //Threads com as células
void *Game();            //Thread que controla o fluxo do jogo
void showGrid();         //Mostra estado atual da matriz do jogo
void changeState(struct Coord c);   //Troca estado entre vivo e morto
int checkVizinhos(struct Coord c);  //Retorn quantas células vizinhas estão vivas

int **grid; //Matriz compartilhada
int size;   //Tamanho da matriz size*size
sem_t s;
pthread_mutex_t game_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t cell_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cell_cond1 = PTHREAD_COND_INITIALIZER;
pthread_cond_t cell_cond2 = PTHREAD_COND_INITIALIZER;

int main(){
  //Mensagem inicial e configuração de tamanho
  printf("Jogo da vida (por John Conway)\n");
  printf("\nTamanho da matriz: ");
  if (!scanf(" %d", &size) || size <= 0){
    printf("Valor de tamanho inválido. O valor deve ser numérico e maior do que 0.\n");
    return 1;
  }
  else if (size > MAXSIZE){
    printf("Tamanho muito grande. (Max %d)\n", MAXSIZE);
    return 1;
  }
	
  //Criação de threads e matriz
  pthread_t tid[size * size];
  int *id;
	
  //Matriz compartilhada entre os processos
  grid = malloc(size * sizeof *grid);
  for (int i = 0; i < size; ++i){
    grid[i] = malloc(size * sizeof *grid[i]);
  }

  //Criação das threads cell
  for (int i = 0; i < size*size; ++i){
    id = (int *) malloc(sizeof(int));
    *id = i;
    if (pthread_create(&tid[i], NULL, Cell, (void *) (id))){
      printf("Erro na criacao da thread %d\n", i);
      exit(1);
    }
  }

  //Inicializa celulas como mortas
  for (int i = 0; i < size; ++i){
    for (int j = 0; j < size; ++j){
      grid[i][j] = 0;
    }
  }

  //Mostra a matriz
  showGrid();

  //Configura celulas vivas
  printf("\nNúmero de células inicias vivas: ");
  int n;
  if(!scanf(" %d", &n)){
    printf("O valor deve ser um número.\n");
    exit(1);
  }
  struct Coord *live;
  live = malloc(n * sizeof(struct Coord));
  if (n){
    printf("Insira as coordenadas separadas por um espaço (Ex: 2 0)\n");
    for (int i = 1; i <= n; ++i){
      printf("Célula #%d: ", i);
      if (!scanf(" %d %d", &live[i-1].y, &live[i-1].x)){
        printf("Valor inserido inválido.\n");
        exit(1);
      }
      if (live[i-1].x >= size || live[i-1].y >= size){
        printf("Valor inserido fora da matriz.\n");
        exit(1);
      }
    }
    for (int i = 0; i < n; ++i){
      grid[live[i].y][live[i].x] = 1;
    }
  }
  free(live);

  showGrid();
  printf("\nSeu jogo começará em breve.\n");
  sleep(4);

  //Inicializãção do semáforo
  sem_init(&s, 0, size*size);

  //Inicia jogo
  Game();

  //Libera espaço da memória compartilhada
  for (int i = 0; i < size; ++i)
    free(grid[i]);
  free(grid);
	
  return 0;
}

void *Cell (void *arg){
  int id = *(int *)arg;
  struct Coord this;
  this.x = id % size;
  this.y = id / size;
  while(1)
  {
    pthread_mutex_lock(&cell_lock);
    pthread_cond_wait(&cell_cond1, &cell_lock);
    int toggle_state = 0;                     //Determina estado futuro da célula
    int live_vizinhos = checkVizinhos(this);  //Número de vizinhos vivos
    if (grid[this.y][this.x]){	              //Regras para células vivas
      if (live_vizinhos < 2 || live_vizinhos > 3){
        toggle_state = 1;
      }
    }
    else{  //Regras para celulas mortas
      if (live_vizinhos == 3){
        toggle_state = 1;
      }
    }
    sem_wait(&s);	
    pthread_cond_wait(&cell_cond2, &cell_lock); //Espera sinal da thread Game
    if (toggle_state)	//Troca de estado se necessário
      changeState(this);
    pthread_mutex_unlock(&cell_lock);
  }
}

void *Game(){
  int count = 1;
  while(1) {
    printf("\e[1;1H\e[2J");
    printf("Jogo da vida (por John Conway)\n");
    printf("\n%dª geração:\n", count);
    pthread_mutex_lock(&game_lock);
    showGrid();
    ++count;
    sleep(1);
    sem_init(&s, 0, size*size);
    pthread_cond_broadcast(&cell_cond1);  //Libera as células para a sua determinação de próximo estado
    int semval;
    do{
      sem_getvalue(&s, &semval);
    }
    while (semval != 0);  //Espera que todas as células terminem de determinar seu próximo estado
    sem_destroy(&s);
    pthread_cond_broadcast(&cell_cond2);  //Libera célula para trocar de estado se necessário
    pthread_mutex_unlock(&game_lock);
  }
}

void showGrid() {
  printf("\n");
  for (int i = 0; i < size; ++i){
    for (int j = 0; j < size; ++j){
      printf("%d ", grid[i][j]);
    }
    printf("\n");
  }
}

void changeState(struct Coord c) {
  grid[c.y][c.x] = !(grid[c.y][c.x]);
}

int checkVizinhos(struct Coord c) {
  int live_vizinhos = 0;
  if (c.x - 1 >= 0 && c.y - 1 >= 0){      //x-1,y-1
    if (grid[c.y-1][c.x-1] == 1)
      ++live_vizinhos;
  }
  if (c.y - 1 >= 0){                      //x,y-1
    if (grid[c.y-1][c.x] == 1)
      ++live_vizinhos;
  }
  if (c.x + 1 < size && c.y - 1 >= 0){    //x+1,y-1
    if (grid[c.y-1][c.x+1] == 1)
      ++live_vizinhos;
  }
  if (c.x - 1 >= 0){                      //x-1,y
    if (grid[c.y][c.x-1] == 1)
      ++live_vizinhos;
  }
  if (c.x + 1 < size){                    //x+1,y
    if (grid[c.y][c.x+1] == 1)
      ++live_vizinhos;
  }
  if (c.x - 1 >= 0 && c.y + 1 < size){    //x-1,y+1
    if (grid[c.y+1][c.x-1] == 1)
      ++live_vizinhos;
  }
  if (c.y + 1 < size){                    //x,y+1
    if (grid[c.y+1][c.x] == 1)
      ++live_vizinhos;
  }
  if (c.x + 1 < size && c.y + 1 < size){  //x+1,y+1
    if (grid[c.y+1][c.x+1] == 1)
      ++live_vizinhos;
  }

  return live_vizinhos;
}
