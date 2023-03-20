#include <process.h>
#include <stdio.h>
#include<windows.h>
#include<iostream>
#include<time.h>

using namespace std;

#define N 5             //Число философов
#define LEFT (i-1)%N    //Левый сосед философа с номером i
#define RIGHT (i+1)%N   //Правый сосед философа сномером i
#define THINKING 0      //Философ размышляет
#define HUNGRY 1        //Философ получается получить вилки
#define EATING 2        //Философ ест

int state[N];           //Состояния каждого философа


struct Philosopher      //Описание философа: номер, алгоритм
{
    int number;
    int algorithm;
};

struct Forks            //Описывает количество вилок у философа
{
    int left;           //0-нет вилки 
    int right;          //1-есть вилка
}forks[N];


CRITICAL_SECTION cs;        //Для критических секций: синхрон. процессов    
CRITICAL_SECTION cs_forks;  //и синхр. вилок

HANDLE philMutex[N];    //Каждому философу по мьютексу
HANDLE forkMutex[N];    //и каждой вилке

void think(int i)       //Моделирует размышления философа
{
    EnterCriticalSection(&cs);    //Вход в критическую секцию
    cout << "Philosopher number " << i << " thinking" << endl;
    LeaveCriticalSection(&cs);    //Выход из критической секции
}

void eat(int i)         //Моделирует обед философа
{
    EnterCriticalSection(&cs);    //Вход в критическую секцию
    cout << "Philosopher number " << i << " eating" << endl;
    LeaveCriticalSection(&cs);    //Выход из критической секции
}

void test(int i)    //Проверка возможности начать философом обед
{
    if (state[i] == HUNGRY && state[LEFT] != EATING && state[RIGHT] != EATING)
    {
        state[i] = EATING;
        ReleaseMutex(philMutex[i]);

    }
}

void take_forks(int i)  //Взятие вилок
{
    EnterCriticalSection(&cs_forks);              //Вход в критическую секцию
    state[i] = HUNGRY;                                //Фиксация наличия голодного философа
    test(i);                                        //Попытка получить две вилки
    LeaveCriticalSection(&cs_forks);              //Выход из критической секции
    WaitForSingleObject(philMutex[i], INFINITE);  //Блокировка если вилок не досталось


}

void put_forks(int i)   //Философ кладет вилки обратно
{
    EnterCriticalSection(&cs_forks);  //Вход в критическую секцию   
    state[i] = THINKING;                  //Философ перестал есть
    test(LEFT);                         //Проверить может ли есть сосед слева
    test(RIGHT);                        //Проверить может ли есть сосед справа
    LeaveCriticalSection(&cs_forks);  //Выход из критической секции
}

void take_left_fork(int i)  //Попытка взять левую вилку
{
    EnterCriticalSection(&cs);
    WaitForSingleObject(forkMutex[LEFT], INFINITE);   //Если вилка свободна  
    forks[i].left = 1;                                    //Берем ее
    LeaveCriticalSection(&cs);

}

void put_left_fork(int i)   //Кладем левую вилку
{
    WaitForSingleObject(forkMutex[LEFT], INFINITE);
    forks[i].left = 0;            //Кладем вилку
    ReleaseMutex(forkMutex[LEFT]);

}

void take_right_fork(int i) //Попытка взять правую вилку
{
    EnterCriticalSection(&cs);
    WaitForSingleObject(forkMutex[RIGHT], INFINITE);  //Если вилка свободна  
    forks[i].right = 1;                                   //Берем ее
    LeaveCriticalSection(&cs);

}

void put_right_fork(int i)  //Кладем правую вилку
{
    WaitForSingleObject(forkMutex[RIGHT], INFINITE);
    forks[i].right = 0;           //Кладем вилку
    ReleaseMutex(forkMutex[RIGHT]);

}
int test_fork(int i)    //Проверяем наличи у философа обеих вилок
{
    if (forks[i].left == 1 && forks[i].right == 1) //Если обе
    {
        ReleaseMutex(forkMutex[LEFT]);    //Разрешаем положить вилки
        ReleaseMutex(forkMutex[RIGHT]);   //
        return 1;                                   //Едим
    }
    else
        return 0;
}

DWORD WINAPI philosopher(void* lParam)  //Собственно модель философа
{
    Philosopher phil = *((Philosopher*)lParam);  //Получаем модель философа

    while (1)    //Моделируем обед
    {   //Берем вилки
        if (phil.algorithm == 0)   //Берем первой левую вилку
        {
            think(phil.number); //Думаем
            take_left_fork(phil.number);    //Берем левую
            take_right_fork(phil.number);   //и правую вилки
            if (test_fork(phil.number) == 1)           //Если обе 
                eat(phil.number);                   //то едим
        }
        else
            if (phil.algorithm == 1)   //Берем первую вилку случайно
            {
                int n = rand() % 2;
                think(phil.number); //Думаем
                if (n == 1)
                {

                    take_left_fork(phil.number);        //Берем вилки случайно
                    take_right_fork(phil.number);
                }
                else
                {
                    take_right_fork(phil.number);
                    take_left_fork(phil.number);
                }
                if (test_fork(phil.number) == 1)
                    eat(phil.number);
            }
            else
                if (phil.algorithm == 2)   //Берем обе вилки
                {
                    think(phil.number);     //Думаем
                    take_forks(phil.number);//Берем вилки 
                    eat(phil.number);       //Едим
                }
        //кладем вилки
        if (phil.algorithm == 0 && forks[phil.number].left == 1 && forks[phil.number].right == 1)
        {
            put_left_fork(phil.number);     //Кладем левую
            put_right_fork(phil.number);    //и правую по первому алгоритму
        }
        else    //Кладем вилки по второму алгоритму    
            if (phil.algorithm == 1 && forks[phil.number].left == 1 && forks[phil.number].right == 1)
            {
                int n = rand() % 2;
                if (n == 1)
                {
                    put_left_fork(phil.number);
                    put_right_fork(phil.number);
                }
                else    //Случайным образом
                {
                    put_right_fork(phil.number);
                    put_left_fork(phil.number);
                }
            }
            else
                if (phil.algorithm == 2)   //Кладем вилки по третьему алгоритму
                    put_forks(phil.number);

        Sleep(10);  //Даем время на переключение контекста
    }
}
int main()
{
    srand(time(NULL));  //Что бы числа были действительно случайными
    Philosopher phil[N];
    for (int i = 0; i < N; i++)  //Первоначально у философов нет вилок
    {
        forks[i].left = 0;
        forks[i].right = 0;
    }
    cout << "Please input number of algorithm(0-left, 1-random, 2-right): " << endl;
    for (int i = 0; i < N; i++)  //Задаем алгоритмы взятия вилок
    {
        phil[i].number = i;
        cout << "Philosopher number " << phil[i].number << ": ";
        cin >> phil[i].algorithm;
    }
    for (int i = 0; i < N; i++)  //Создаем мьютексы
    {
        philMutex[i] = CreateMutex(NULL, FALSE, NULL);
        forkMutex[i] = CreateMutex(NULL, FALSE, NULL);
    }
    InitializeCriticalSection(&cs);       //Инициализируем
    InitializeCriticalSection(&cs_forks); //критические секции

    DWORD id[N];        //Идентификаторы потоков
    HANDLE hThread[N];  //Потоки
    for (int i = 0; i < N; i++)//Создаем потоки
    {
        hThread[i] = CreateThread(NULL, NULL, &philosopher, &phil[i], NULL, &id[i]);
    }

    Sleep(INFINITE);    //Чтобы потоки успели выполнится с корректными значениями 
    while (1);
}
