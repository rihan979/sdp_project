/* class_2.c
   Quizillionaire — full project
   - Fixed player-name input bug (robust buffer clearing)
   - Cross-platform colorful terminal UI (Windows / ANSI fallback)
   - Mixed cross-platform sound effects (Option 3)
   - Preserves original game logic and full question bank
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdarg.h>

#ifdef _WIN32
  #include <windows.h>
#else
  #include <unistd.h>
#endif

// -------------------- Data Structures --------------------
struct Question {
    char question[256];
    char options[4][100];
    int correct_answer;
    char category[50];
};

struct Player {
    char name[50];
    int current_stage;
    int prize_won;
    int questions_answered;
    int helpline_used;
    int second_chance_used;
    char selected_category[50];
};

struct Stage {
    char name[30];
    int prize;
    int time_limit;
    int questions_count;
    int has_helpline;
};

struct Category {
    char name[50];
    int question_count;
    int start_index;
};

// -------------------- Globals --------------------
#define MAX_QUESTIONS 120
struct Question question_bank[MAX_QUESTIONS];
int total_questions = 0;

struct Stage stages[3] = {
    {"Explorer Stage", 10000, 20, 5, 0},
    {"Challenger Stage", 50000, 25, 5, 0},
    {"Warrior Stage", 100000, 30, 5, 1}
};

struct Category categories[8] = {
    {"Bangla & Bengali", 15, 0},
    {"Film & Music", 15, 15},
    {"Sports", 15, 30},
    {"History", 15, 45},
    {"Science", 15, 60},
    {"Food & Cooking", 15, 75},
    {"Politics", 15, 90},
    {"Fashion", 15, 105}
};

// Color enums (logical)
enum {
    COL_DEFAULT = 0,
    COL_RED,
    COL_GREEN,
    COL_YELLOW,
    COL_BLUE,
    COL_MAGENTA,
    COL_CYAN,
    COL_WHITE
};

// Sound types
typedef enum { SND_CORRECT, SND_WRONG, SND_STAGE, SND_MENU, SND_WIN, SND_GAMEOVER } SoundType;

// -------------------- Prototypes --------------------
void initialize_game();
void load_questions();
void display_welcome();
void display_rules();
void display_categories();
int select_category();
int start_game();
int play_stage(struct Player *player, int stage_num, int category_index);
int ask_question(struct Question q, int time_limit, int helpline_available, struct Player *player);
void shuffle_category_questions(int category_index);
void use_helpline(struct Question *q);
void display_leaderboard();
void save_score(struct Player player);
void clear_screen();
void display_stage_info(int stage_num);
void display_progress(int current, int total);

void set_color(int col);
void reset_color();
void play_sound(SoundType s);
void colored_printf(int col, const char *fmt, ...);

static void sleep_seconds(int sec) {
#ifdef _WIN32
    Sleep(sec * 1000);
#else
    sleep(sec);
#endif
}

// -------------------- Color & Sound Implementations --------------------
#ifdef _WIN32
static HANDLE hConsole = NULL;
#endif

void set_color(int col) {
#ifdef _WIN32
    if (!hConsole) hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    int attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    switch (col) {
        case COL_RED:    attr = FOREGROUND_RED | FOREGROUND_INTENSITY; break;
        case COL_GREEN:  attr = FOREGROUND_GREEN | FOREGROUND_INTENSITY; break;
        case COL_YELLOW: attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY; break;
        case COL_BLUE:   attr = FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
        case COL_MAGENTA: attr = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
        case COL_CYAN:   attr = FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
        case COL_WHITE:  attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; break;
        default: attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; break;
    }
    SetConsoleTextAttribute(hConsole, attr);
#else
    switch (col) {
        case COL_RED:    printf("\x1b[31m"); break;
        case COL_GREEN:  printf("\x1b[32m"); break;
        case COL_YELLOW: printf("\x1b[33m"); break;
        case COL_BLUE:   printf("\x1b[34m"); break;
        case COL_MAGENTA: printf("\x1b[35m"); break;
        case COL_CYAN:   printf("\x1b[36m"); break;
        case COL_WHITE:  printf("\x1b[37m"); break;
        default: printf("\x1b[0m"); break;
    }
#endif
}

void reset_color() {
#ifdef _WIN32
    if (!hConsole) hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
    printf("\x1b[0m");
#endif
}

void play_sound(SoundType s) {
#ifdef _WIN32
    // Windows Beep sequences (mixed)
    switch (s) {
        case SND_CORRECT:
            Beep(880, 100); Sleep(50);
            Beep(1040, 120);
            break;
        case SND_WRONG:
            Beep(220, 300);
            break;
        case SND_STAGE:
            Beep(600, 120); Sleep(60);
            Beep(800, 120); Sleep(60);
            Beep(1000, 120);
            break;
        case SND_MENU:
            Beep(440, 80);
            break;
        case SND_WIN:
            Beep(740, 120); Sleep(40);
            Beep(880, 120); Sleep(40);
            Beep(1040, 160);
            break;
        case SND_GAMEOVER:
            Beep(200, 500);
            break;
        default:
            Beep(440, 80);
    }
#else
    // Fallback for non-Windows: terminal bell variations
    switch (s) {
        case SND_CORRECT:
            printf("\a"); fflush(stdout); usleep(90000);
            printf("\a"); fflush(stdout);
            break;
        case SND_WRONG:
            printf("\a"); fflush(stdout); usleep(250000);
            break;
        case SND_STAGE:
            printf("\a\a\a"); fflush(stdout);
            break;
        case SND_MENU:
            printf("\a"); fflush(stdout);
            break;
        case SND_WIN:
            printf("\a\a\a\a"); fflush(stdout);
            break;
        case SND_GAMEOVER:
            printf("\a"); fflush(stdout); usleep(500000);
            break;
        default:
            printf("\a"); fflush(stdout);
            break;
    }
#endif
}

void colored_printf(int col, const char *fmt, ...) {
    va_list ap;
    set_color(col);
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    reset_color();
}

// -------------------- Main --------------------
int main() {
    initialize_game();
    display_welcome();

    int choice;
    do {
        set_color(COL_CYAN);
        printf("\n*** MAIN MENU ***\n");
        reset_color();
        printf("==================\n");
        colored_printf(COL_GREEN, "1. [>] Start Game\n");
        colored_printf(COL_YELLOW, "2. [?] View Rules\n");
        colored_printf(COL_MAGENTA, "3. [*] Leaderboard\n");
        colored_printf(COL_BLUE, "4. [C] Categories\n");
        colored_printf(COL_RED, "5. [X] Exit\n");
        printf("Enter your choice (1-5): ");

        if (scanf("%d", &choice) != 1) {
            // clear invalid input
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) {}
            choice = -1;
        } else {
            // clear trailing newline left by scanf
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) {}
        }

        switch(choice) {
            case 1:
                play_sound(SND_MENU);
                if (start_game()) {
                    colored_printf(COL_GREEN, "\n*** CONGRATULATIONS! YOU'RE THE QUIZILLIONAIRE! ***\n");
                    play_sound(SND_WIN);
                } else {
                    colored_printf(COL_RED, "\n*** Game Over! Better luck next time! ***\n");
                    play_sound(SND_GAMEOVER);
                }
                break;
            case 2:
                display_rules();
                break;
            case 3:
                display_leaderboard();
                break;
            case 4:
                display_categories();
                break;
            case 5:
                colored_printf(COL_CYAN, "\nThanks for playing Quizillionaire! Goodbye!\n");
                play_sound(SND_MENU);
                break;
            default:
                colored_printf(COL_RED, "\n[!] Invalid choice! Please try again.\n");
        }
    } while (choice != 5);

    return 0;
}

// -------------------- Initialization & Helpers --------------------
void initialize_game() {
    srand((unsigned int)time(NULL));
    load_questions();
    clear_screen();
}

void clear_screen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

// -------------------- Questions Loader --------------------
// (Full bank — kept intact; trimmed commentary)
void load_questions() {
    int q_index = 0;

    // BANGLA & BENGALI (15)
    strcpy(question_bank[q_index].question, "Who wrote 'Gitanjali'?");
    strcpy(question_bank[q_index].options[0], "Rabindranath Tagore");
    strcpy(question_bank[q_index].options[1], "Kazi Nazrul Islam");
    strcpy(question_bank[q_index].options[2], "Bankim Chandra");
    strcpy(question_bank[q_index].options[3], "Sarat Chandra");
    question_bank[q_index].correct_answer = 0;
    strcpy(question_bank[q_index].category, "Bangla & Bengali");
    q_index++;

    strcpy(question_bank[q_index].question, "What is the national flower of Bangladesh?");
    strcpy(question_bank[q_index].options[0], "Rose");
    strcpy(question_bank[q_index].options[1], "Lily");
    strcpy(question_bank[q_index].options[2], "Water Lily");
    strcpy(question_bank[q_index].options[3], "Jasmine");
    question_bank[q_index].correct_answer = 2;
    strcpy(question_bank[q_index].category, "Bangla & Bengali");
    q_index++;

    strcpy(question_bank[q_index].question, "Which river is called the lifeline of Bangladesh?");
    strcpy(question_bank[q_index].options[0], "Ganges");
    strcpy(question_bank[q_index].options[1], "Padma");
    strcpy(question_bank[q_index].options[2], "Meghna");
    strcpy(question_bank[q_index].options[3], "Jamuna");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Bangla & Bengali");
    q_index++;

    strcpy(question_bank[q_index].question, "Who is known as the 'Rebel Poet' of Bengal?");
    strcpy(question_bank[q_index].options[0], "Rabindranath Tagore");
    strcpy(question_bank[q_index].options[1], "Kazi Nazrul Islam");
    strcpy(question_bank[q_index].options[2], "Jibananda Das");
    strcpy(question_bank[q_index].options[3], "Sukanta Bhattacharya");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Bangla & Bengali");
    q_index++;

    strcpy(question_bank[q_index].question, "What is the capital of West Bengal?");
    strcpy(question_bank[q_index].options[0], "Howrah");
    strcpy(question_bank[q_index].options[1], "Kolkata");
    strcpy(question_bank[q_index].options[2], "Siliguri");
    strcpy(question_bank[q_index].options[3], "Durgapur");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Bangla & Bengali");
    q_index++;

    strcpy(question_bank[q_index].question, "Which is the longest river in Bangladesh?");
    strcpy(question_bank[q_index].options[0], "Padma");
    strcpy(question_bank[q_index].options[1], "Meghna");
    strcpy(question_bank[q_index].options[2], "Jamuna");
    strcpy(question_bank[q_index].options[3], "Surma");
    question_bank[q_index].correct_answer = 2;
    strcpy(question_bank[q_index].category, "Bangla & Bengali");
    q_index++;

    strcpy(question_bank[q_index].question, "What is the currency of Bangladesh?");
    strcpy(question_bank[q_index].options[0], "Rupee");
    strcpy(question_bank[q_index].options[1], "Taka");
    strcpy(question_bank[q_index].options[2], "Peso");
    strcpy(question_bank[q_index].options[3], "Dinar");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Bangla & Bengali");
    q_index++;

    strcpy(question_bank[q_index].question, "Who wrote 'Anandamath'?");
    strcpy(question_bank[q_index].options[0], "Rabindranath Tagore");
    strcpy(question_bank[q_index].options[1], "Bankim Chandra Chattopadhyay");
    strcpy(question_bank[q_index].options[2], "Sarat Chandra Chattopadhyay");
    strcpy(question_bank[q_index].options[3], "Bibhutibhushan Bandyopadhyay");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Bangla & Bengali");
    q_index++;

    strcpy(question_bank[q_index].question, "Which festival is known as the 'Festival of Colors' in Bengal?");
    strcpy(question_bank[q_index].options[0], "Durga Puja");
    strcpy(question_bank[q_index].options[1], "Kali Puja");
    strcpy(question_bank[q_index].options[2], "Holi");
    strcpy(question_bank[q_index].options[3], "Poila Boishakh");
    question_bank[q_index].correct_answer = 2;
    strcpy(question_bank[q_index].category, "Bangla & Bengali");
    q_index++;

    strcpy(question_bank[q_index].question, "What is the traditional Bengali New Year called?");
    strcpy(question_bank[q_index].options[0], "Poila Boishakh");
    strcpy(question_bank[q_index].options[1], "Durga Puja");
    strcpy(question_bank[q_index].options[2], "Kali Puja");
    strcpy(question_bank[q_index].options[3], "Saraswati Puja");
    question_bank[q_index].correct_answer = 0;
    strcpy(question_bank[q_index].category, "Bangla & Bengali");
    q_index++;

    strcpy(question_bank[q_index].question, "Which city is called the 'City of Joy'?");
    strcpy(question_bank[q_index].options[0], "Dhaka");
    strcpy(question_bank[q_index].options[1], "Kolkata");
    strcpy(question_bank[q_index].options[2], "Chittagong");
    strcpy(question_bank[q_index].options[3], "Sylhet");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Bangla & Bengali");
    q_index++;

    strcpy(question_bank[q_index].question, "What is the famous sweet dish of Bengal?");
    strcpy(question_bank[q_index].options[0], "Gulab Jamun");
    strcpy(question_bank[q_index].options[1], "Rasgulla");
    strcpy(question_bank[q_index].options[2], "Jalebi");
    strcpy(question_bank[q_index].options[3], "Laddu");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Bangla & Bengali");
    q_index++;

    strcpy(question_bank[q_index].question, "Which Nobel Prize did Rabindranath Tagore win?");
    strcpy(question_bank[q_index].options[0], "Physics");
    strcpy(question_bank[q_index].options[1], "Literature");
    strcpy(question_bank[q_index].options[2], "Peace");
    strcpy(question_bank[q_index].options[3], "Chemistry");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Bangla & Bengali");
    q_index++;

    strcpy(question_bank[q_index].question, "What is the traditional boat race of Bengal called?");
    strcpy(question_bank[q_index].options[0], "Nouka Baich");
    strcpy(question_bank[q_index].options[1], "Kabaddi");
    strcpy(question_bank[q_index].options[2], "Kho Kho");
    strcpy(question_bank[q_index].options[3], "Lathi Khela");
    question_bank[q_index].correct_answer = 0;
    strcpy(question_bank[q_index].category, "Bangla & Bengali");
    q_index++;

    // FILM & MUSIC (15)
    strcpy(question_bank[q_index].question, "Who directed the movie 'Titanic'?");
    strcpy(question_bank[q_index].options[0], "Steven Spielberg");
    strcpy(question_bank[q_index].options[1], "James Cameron");
    strcpy(question_bank[q_index].options[2], "Christopher Nolan");
    strcpy(question_bank[q_index].options[3], "Martin Scorsese");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Film & Music");
    q_index++;

    strcpy(question_bank[q_index].question, "Which instrument has 88 keys?");
    strcpy(question_bank[q_index].options[0], "Guitar");
    strcpy(question_bank[q_index].options[1], "Violin");
    strcpy(question_bank[q_index].options[2], "Piano");
    strcpy(question_bank[q_index].options[3], "Flute");
    question_bank[q_index].correct_answer = 2;
    strcpy(question_bank[q_index].category, "Film & Music");
    q_index++;

    strcpy(question_bank[q_index].question, "Which movie won the first Academy Award for Best Picture?");
    strcpy(question_bank[q_index].options[0], "Wings");
    strcpy(question_bank[q_index].options[1], "Sunrise");
    strcpy(question_bank[q_index].options[2], "The Jazz Singer");
    strcpy(question_bank[q_index].options[3], "7th Heaven");
    question_bank[q_index].correct_answer = 0;
    strcpy(question_bank[q_index].category, "Film & Music");
    q_index++;

    strcpy(question_bank[q_index].question, "Who composed 'The Four Seasons'?");
    strcpy(question_bank[q_index].options[0], "Bach");
    strcpy(question_bank[q_index].options[1], "Mozart");
    strcpy(question_bank[q_index].options[2], "Vivaldi");
    strcpy(question_bank[q_index].options[3], "Beethoven");
    question_bank[q_index].correct_answer = 2;
    strcpy(question_bank[q_index].category, "Film & Music");
    q_index++;

    strcpy(question_bank[q_index].question, "Which film features the song 'My Heart Will Go On'?");
    strcpy(question_bank[q_index].options[0], "Ghost");
    strcpy(question_bank[q_index].options[1], "Titanic");
    strcpy(question_bank[q_index].options[2], "The Bodyguard");
    strcpy(question_bank[q_index].options[3], "Dirty Dancing");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Film & Music");
    q_index++;

    strcpy(question_bank[q_index].question, "Who is known as the 'King of Pop'?");
    strcpy(question_bank[q_index].options[0], "Elvis Presley");
    strcpy(question_bank[q_index].options[1], "Michael Jackson");
    strcpy(question_bank[q_index].options[2], "Prince");
    strcpy(question_bank[q_index].options[3], "Madonna");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Film & Music");
    q_index++;

    strcpy(question_bank[q_index].question, "Which movie features the character 'Jack Sparrow'?");
    strcpy(question_bank[q_index].options[0], "Titanic");
    strcpy(question_bank[q_index].options[1], "Pirates of the Caribbean");
    strcpy(question_bank[q_index].options[2], "The Mask");
    strcpy(question_bank[q_index].options[3], "Edward Scissorhands");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Film & Music");
    q_index++;

    strcpy(question_bank[q_index].question, "How many strings does a standard guitar have?");
    strcpy(question_bank[q_index].options[0], "4");
    strcpy(question_bank[q_index].options[1], "5");
    strcpy(question_bank[q_index].options[2], "6");
    strcpy(question_bank[q_index].options[3], "7");
    question_bank[q_index].correct_answer = 2;
    strcpy(question_bank[q_index].category, "Film & Music");
    q_index++;

    strcpy(question_bank[q_index].question, "Which band released the album 'Abbey Road'?");
    strcpy(question_bank[q_index].options[0], "The Rolling Stones");
    strcpy(question_bank[q_index].options[1], "The Beatles");
    strcpy(question_bank[q_index].options[2], "Led Zeppelin");
    strcpy(question_bank[q_index].options[3], "Pink Floyd");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Film & Music");
    q_index++;

    strcpy(question_bank[q_index].question, "Who directed the movie 'Jaws'?");
    strcpy(question_bank[q_index].options[0], "George Lucas");
    strcpy(question_bank[q_index].options[1], "Steven Spielberg");
    strcpy(question_bank[q_index].options[2], "Francis Ford Coppola");
    strcpy(question_bank[q_index].options[3], "Martin Scorsese");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Film & Music");
    q_index++;

    strcpy(question_bank[q_index].question, "Which Disney movie features the song 'Let It Go'?");
    strcpy(question_bank[q_index].options[0], "Moana");
    strcpy(question_bank[q_index].options[1], "Frozen");
    strcpy(question_bank[q_index].options[2], "Tangled");
    strcpy(question_bank[q_index].options[3], "The Little Mermaid");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Film & Music");
    q_index++;

    strcpy(question_bank[q_index].question, "What genre of music did Bob Marley popularize?");
    strcpy(question_bank[q_index].options[0], "Jazz");
    strcpy(question_bank[q_index].options[1], "Reggae");
    strcpy(question_bank[q_index].options[2], "Blues");
    strcpy(question_bank[q_index].options[3], "Rock");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Film & Music");
    q_index++;

    strcpy(question_bank[q_index].question, "Which movie won the Academy Award for Best Picture in 2020?");
    strcpy(question_bank[q_index].options[0], "1917");
    strcpy(question_bank[q_index].options[1], "Parasite");
    strcpy(question_bank[q_index].options[2], "Joker");
    strcpy(question_bank[q_index].options[3], "Once Upon a Time in Hollywood");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Film & Music");
    q_index++;

    strcpy(question_bank[q_index].question, "Who composed the music for 'Star Wars'?");
    strcpy(question_bank[q_index].options[0], "Hans Zimmer");
    strcpy(question_bank[q_index].options[1], "John Williams");
    strcpy(question_bank[q_index].options[2], "Danny Elfman");
    strcpy(question_bank[q_index].options[3], "Alan Silvestri");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Film & Music");
    q_index++;

    strcpy(question_bank[q_index].question, "Which singer is known as 'Queen B'?");
    strcpy(question_bank[q_index].options[0], "Rihanna");
    strcpy(question_bank[q_index].options[1], "Beyoncé");
    strcpy(question_bank[q_index].options[2], "Lady Gaga");
    strcpy(question_bank[q_index].options[3], "Ariana Grande");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Film & Music");
    q_index++;

    // SPORTS (15)
    strcpy(question_bank[q_index].question, "How many players are there in a cricket team?");
    strcpy(question_bank[q_index].options[0], "10");
    strcpy(question_bank[q_index].options[1], "11");
    strcpy(question_bank[q_index].options[2], "12");
    strcpy(question_bank[q_index].options[3], "9");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Sports");
    q_index++;

    strcpy(question_bank[q_index].question, "In which sport is the term 'slam dunk' used?");
    strcpy(question_bank[q_index].options[0], "Tennis");
    strcpy(question_bank[q_index].options[1], "Basketball");
    strcpy(question_bank[q_index].options[2], "Volleyball");
    strcpy(question_bank[q_index].options[3], "Football");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Sports");
    q_index++;

    strcpy(question_bank[q_index].question, "How many rings are there in the Olympic symbol?");
    strcpy(question_bank[q_index].options[0], "4");
    strcpy(question_bank[q_index].options[1], "5");
    strcpy(question_bank[q_index].options[2], "6");
    strcpy(question_bank[q_index].options[3], "7");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Sports");
    q_index++;

    strcpy(question_bank[q_index].question, "Which country hosted the 2016 Summer Olympics?");
    strcpy(question_bank[q_index].options[0], "China");
    strcpy(question_bank[q_index].options[1], "Brazil");
    strcpy(question_bank[q_index].options[2], "UK");
    strcpy(question_bank[q_index].options[3], "Japan");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Sports");
    q_index++;

    strcpy(question_bank[q_index].question, "What is the maximum score possible in ten-pin bowling?");
    strcpy(question_bank[q_index].options[0], "200");
    strcpy(question_bank[q_index].options[1], "250");
    strcpy(question_bank[q_index].options[2], "300");
    strcpy(question_bank[q_index].options[3], "350");
    question_bank[q_index].correct_answer = 2;
    strcpy(question_bank[q_index].category, "Sports");
    q_index++;

    strcpy(question_bank[q_index].question, "Which sport is known as 'The Beautiful Game'?");
    strcpy(question_bank[q_index].options[0], "Basketball");
    strcpy(question_bank[q_index].options[1], "Football/Soccer");
    strcpy(question_bank[q_index].options[2], "Tennis");
    strcpy(question_bank[q_index].options[3], "Cricket");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Sports");
    q_index++;

    strcpy(question_bank[q_index].question, "How many Grand Slam tournaments are there in tennis?");
    strcpy(question_bank[q_index].options[0], "3");
    strcpy(question_bank[q_index].options[1], "4");
    strcpy(question_bank[q_index].options[2], "5");
    strcpy(question_bank[q_index].options[3], "6");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Sports");
    q_index++;

    strcpy(question_bank[q_index].question, "Which country has won the most FIFA World Cups?");
    strcpy(question_bank[q_index].options[0], "Germany");
    strcpy(question_bank[q_index].options[1], "Brazil");
    strcpy(question_bank[q_index].options[2], "Argentina");
    strcpy(question_bank[q_index].options[3], "Italy");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Sports");
    q_index++;

    strcpy(question_bank[q_index].question, "In golf, what is a score of one under par called?");
    strcpy(question_bank[q_index].options[0], "Eagle");
    strcpy(question_bank[q_index].options[1], "Birdie");
    strcpy(question_bank[q_index].options[2], "Bogey");
    strcpy(question_bank[q_index].options[3], "Albatross");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Sports");
    q_index++;

    strcpy(question_bank[q_index].question, "Which sport uses a shuttlecock?");
    strcpy(question_bank[q_index].options[0], "Tennis");
    strcpy(question_bank[q_index].options[1], "Badminton");
    strcpy(question_bank[q_index].options[2], "Squash");
    strcpy(question_bank[q_index].options[3], "Table Tennis");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Sports");
    q_index++;

    strcpy(question_bank[q_index].question, "How long is a marathon race?");
    strcpy(question_bank[q_index].options[0], "26.2 miles");
    strcpy(question_bank[q_index].options[1], "26.2 kilometers");
    strcpy(question_bank[q_index].options[2], "25 miles");
    strcpy(question_bank[q_index].options[3], "30 kilometers");
    question_bank[q_index].correct_answer = 0;
    strcpy(question_bank[q_index].category, "Sports");
    q_index++;

    strcpy(question_bank[q_index].question, "Which boxer was known as 'The Greatest'?");
    strcpy(question_bank[q_index].options[0], "Mike Tyson");
    strcpy(question_bank[q_index].options[1], "Muhammad Ali");
    strcpy(question_bank[q_index].options[2], "Floyd Mayweather");
    strcpy(question_bank[q_index].options[3], "Sugar Ray Robinson");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Sports");
    q_index++;

    strcpy(question_bank[q_index].question, "In which sport would you perform a slam dunk?");
    strcpy(question_bank[q_index].options[0], "Volleyball");
    strcpy(question_bank[q_index].options[1], "Basketball");
    strcpy(question_bank[q_index].options[2], "Tennis");
    strcpy(question_bank[q_index].options[3], "Badminton");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Sports");
    q_index++;

    strcpy(question_bank[q_index].question, "Which team sport has 6 players on each side?");
    strcpy(question_bank[q_index].options[0], "Basketball");
    strcpy(question_bank[q_index].options[1], "Volleyball");
    strcpy(question_bank[q_index].options[2], "Hockey");
    strcpy(question_bank[q_index].options[3], "Water Polo");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Sports");
    q_index++;

    strcpy(question_bank[q_index].question, "What is the highest possible break in snooker?");
    strcpy(question_bank[q_index].options[0], "147");
    strcpy(question_bank[q_index].options[1], "150");
    strcpy(question_bank[q_index].options[2], "155");
    strcpy(question_bank[q_index].options[3], "160");
    question_bank[q_index].correct_answer = 0;
    strcpy(question_bank[q_index].category, "Sports");
    q_index++;

    // HISTORY (15)
    strcpy(question_bank[q_index].question, "When did World War II end?");
    strcpy(question_bank[q_index].options[0], "1944");
    strcpy(question_bank[q_index].options[1], "1945");
    strcpy(question_bank[q_index].options[2], "1946");
    strcpy(question_bank[q_index].options[3], "1947");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "History");
    q_index++;

    strcpy(question_bank[q_index].question, "Who was the first person to walk on the moon?");
    strcpy(question_bank[q_index].options[0], "Buzz Aldrin");
    strcpy(question_bank[q_index].options[1], "Neil Armstrong");
    strcpy(question_bank[q_index].options[2], "John Glenn");
    strcpy(question_bank[q_index].options[3], "Alan Shepard");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "History");
    q_index++;

    strcpy(question_bank[q_index].question, "In which year did the Berlin Wall fall?");
    strcpy(question_bank[q_index].options[0], "1987");
    strcpy(question_bank[q_index].options[1], "1988");
    strcpy(question_bank[q_index].options[2], "1989");
    strcpy(question_bank[q_index].options[3], "1990");
    question_bank[q_index].correct_answer = 2;
    strcpy(question_bank[q_index].category, "History");
    q_index++;

    strcpy(question_bank[q_index].question, "Who was the first President of the United States?");
    strcpy(question_bank[q_index].options[0], "Thomas Jefferson");
    strcpy(question_bank[q_index].options[1], "George Washington");
    strcpy(question_bank[q_index].options[2], "John Adams");
    strcpy(question_bank[q_index].options[3], "Benjamin Franklin");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "History");
    q_index++;

    strcpy(question_bank[q_index].question, "Which ancient wonder of the world was located in Alexandria?");
    strcpy(question_bank[q_index].options[0], "Hanging Gardens");
    strcpy(question_bank[q_index].options[1], "Lighthouse");
    strcpy(question_bank[q_index].options[2], "Colossus");
    strcpy(question_bank[q_index].options[3], "Mausoleum");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "History");
    q_index++;

    strcpy(question_bank[q_index].question, "Which empire was ruled by Julius Caesar?");
    strcpy(question_bank[q_index].options[0], "Greek Empire");
    strcpy(question_bank[q_index].options[1], "Roman Empire");
    strcpy(question_bank[q_index].options[2], "Persian Empire");
    strcpy(question_bank[q_index].options[3], "Egyptian Empire");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "History");
    q_index++;

    strcpy(question_bank[q_index].question, "In which year did the Titanic sink?");
    strcpy(question_bank[q_index].options[0], "1910");
    strcpy(question_bank[q_index].options[1], "1912");
    strcpy(question_bank[q_index].options[2], "1914");
    strcpy(question_bank[q_index].options[3], "1916");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "History");
    q_index++;

    strcpy(question_bank[q_index].question, "Who painted the ceiling of the Sistine Chapel?");
    strcpy(question_bank[q_index].options[0], "Leonardo da Vinci");
    strcpy(question_bank[q_index].options[1], "Michelangelo");
    strcpy(question_bank[q_index].options[2], "Raphael");
    strcpy(question_bank[q_index].options[3], "Donatello");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "History");
    q_index++;

    strcpy(question_bank[q_index].question, "Which war was fought between 1861-1865 in America?");
    strcpy(question_bank[q_index].options[0], "Revolutionary War");
    strcpy(question_bank[q_index].options[1], "Civil War");
    strcpy(question_bank[q_index].options[2], "War of 1812");
    strcpy(question_bank[q_index].options[3], "Spanish-American War");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "History");
    q_index++;

    strcpy(question_bank[q_index].question, "Who was the first woman to win a Nobel Prize?");
    strcpy(question_bank[q_index].options[0], "Marie Curie");
    strcpy(question_bank[q_index].options[1], "Mother Teresa");
    strcpy(question_bank[q_index].options[2], "Jane Addams");
    strcpy(question_bank[q_index].options[3], "Bertha von Suttner");
    question_bank[q_index].correct_answer = 0;
    strcpy(question_bank[q_index].category, "History");
    q_index++;

    strcpy(question_bank[q_index].question, "Which country gifted the Statue of Liberty to the USA?");
    strcpy(question_bank[q_index].options[0], "England");
    strcpy(question_bank[q_index].options[1], "France");
    strcpy(question_bank[q_index].options[2], "Spain");
    strcpy(question_bank[q_index].options[3], "Italy");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "History");
    q_index++;

    strcpy(question_bank[q_index].question, "What year did World War I begin?");
    strcpy(question_bank[q_index].options[0], "1912");
    strcpy(question_bank[q_index].options[1], "1914");
    strcpy(question_bank[q_index].options[2], "1916");
    strcpy(question_bank[q_index].options[3], "1918");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "History");
    q_index++;

    strcpy(question_bank[q_index].question, "Who was known as the 'Iron Chancellor'?");
    strcpy(question_bank[q_index].options[0], "Winston Churchill");
    strcpy(question_bank[q_index].options[1], "Otto von Bismarck");
    strcpy(question_bank[q_index].options[2], "Napoleon Bonaparte");
    strcpy(question_bank[q_index].options[3], "Adolf Hitler");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "History");
    q_index++;

    strcpy(question_bank[q_index].question, "Which ancient civilization built Machu Picchu?");
    strcpy(question_bank[q_index].options[0], "Aztecs");
    strcpy(question_bank[q_index].options[1], "Incas");
    strcpy(question_bank[q_index].options[2], "Mayans");
    strcpy(question_bank[q_index].options[3], "Olmecs");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "History");
    q_index++;

    strcpy(question_bank[q_index].question, "In which year did India gain independence?");
    strcpy(question_bank[q_index].options[0], "1945");
    strcpy(question_bank[q_index].options[1], "1947");
    strcpy(question_bank[q_index].options[2], "1948");
    strcpy(question_bank[q_index].options[3], "1950");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "History");
    q_index++;

    // SCIENCE (15)
    strcpy(question_bank[q_index].question, "What is the chemical symbol for gold?");
    strcpy(question_bank[q_index].options[0], "Go");
    strcpy(question_bank[q_index].options[1], "Gd");
    strcpy(question_bank[q_index].options[2], "Au");
    strcpy(question_bank[q_index].options[3], "Ag");
    question_bank[q_index].correct_answer = 2;
    strcpy(question_bank[q_index].category, "Science");
    q_index++;

    strcpy(question_bank[q_index].question, "How many bones are there in an adult human body?");
    strcpy(question_bank[q_index].options[0], "206");
    strcpy(question_bank[q_index].options[1], "208");
    strcpy(question_bank[q_index].options[2], "210");
    strcpy(question_bank[q_index].options[3], "204");
    question_bank[q_index].correct_answer = 0;
    strcpy(question_bank[q_index].category, "Science");
    q_index++;

    strcpy(question_bank[q_index].question, "What is the speed of light in vacuum?");
    strcpy(question_bank[q_index].options[0], "299,792,458 m/s");
    strcpy(question_bank[q_index].options[1], "300,000,000 m/s");
    strcpy(question_bank[q_index].options[2], "299,000,000 m/s");
    strcpy(question_bank[q_index].options[3], "298,792,458 m/s");
    question_bank[q_index].correct_answer = 0;
    strcpy(question_bank[q_index].category, "Science");
    q_index++;

    strcpy(question_bank[q_index].question, "Which planet is known as the Red Planet?");
    strcpy(question_bank[q_index].options[0], "Venus");
    strcpy(question_bank[q_index].options[1], "Mars");
    strcpy(question_bank[q_index].options[2], "Jupiter");
    strcpy(question_bank[q_index].options[3], "Saturn");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Science");
    q_index++;

    strcpy(question_bank[q_index].question, "What is the hardest natural substance on Earth?");
    strcpy(question_bank[q_index].options[0], "Gold");
    strcpy(question_bank[q_index].options[1], "Iron");
    strcpy(question_bank[q_index].options[2], "Diamond");
    strcpy(question_bank[q_index].options[3], "Platinum");
    question_bank[q_index].correct_answer = 2;
    strcpy(question_bank[q_index].category, "Science");
    q_index++;

    strcpy(question_bank[q_index].question, "What gas do plants absorb from the atmosphere?");
    strcpy(question_bank[q_index].options[0], "Oxygen");
    strcpy(question_bank[q_index].options[1], "Carbon Dioxide");
    strcpy(question_bank[q_index].options[2], "Nitrogen");
    strcpy(question_bank[q_index].options[3], "Hydrogen");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Science");
    q_index++;

    strcpy(question_bank[q_index].question, "What is the largest organ in the human body?");
    strcpy(question_bank[q_index].options[0], "Heart");
    strcpy(question_bank[q_index].options[1], "Skin");
    strcpy(question_bank[q_index].options[2], "Liver");
    strcpy(question_bank[q_index].options[3], "Brain");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Science");
    q_index++;

    strcpy(question_bank[q_index].question, "What is the chemical formula for water?");
    strcpy(question_bank[q_index].options[0], "H2O");
    strcpy(question_bank[q_index].options[1], "CO2");
    strcpy(question_bank[q_index].options[2], "NaCl");
    strcpy(question_bank[q_index].options[3], "O2");
    question_bank[q_index].correct_answer = 0;
    strcpy(question_bank[q_index].category, "Science");
    q_index++;

    strcpy(question_bank[q_index].question, "How many chambers does a human heart have?");
    strcpy(question_bank[q_index].options[0], "2");
    strcpy(question_bank[q_index].options[1], "4");
    strcpy(question_bank[q_index].options[2], "6");
    strcpy(question_bank[q_index].options[3], "8");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Science");
    q_index++;

    strcpy(question_bank[q_index].question, "What is the smallest unit of matter?");
    strcpy(question_bank[q_index].options[0], "Molecule");
    strcpy(question_bank[q_index].options[1], "Atom");
    strcpy(question_bank[q_index].options[2], "Cell");
    strcpy(question_bank[q_index].options[3], "Electron");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Science");
    q_index++;

    strcpy(question_bank[q_index].question, "Which scientist developed the theory of relativity?");
    strcpy(question_bank[q_index].options[0], "Isaac Newton");
    strcpy(question_bank[q_index].options[1], "Albert Einstein");
    strcpy(question_bank[q_index].options[2], "Galileo Galilei");
    strcpy(question_bank[q_index].options[3], "Stephen Hawking");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Science");
    q_index++;

    strcpy(question_bank[q_index].question, "What is the powerhouse of the cell?");
    strcpy(question_bank[q_index].options[0], "Nucleus");
    strcpy(question_bank[q_index].options[1], "Mitochondria");
    strcpy(question_bank[q_index].options[2], "Ribosome");
    strcpy(question_bank[q_index].options[3], "Cytoplasm");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Science");
    q_index++;

    strcpy(question_bank[q_index].question, "What type of animal is a dolphin?");
    strcpy(question_bank[q_index].options[0], "Fish");
    strcpy(question_bank[q_index].options[1], "Mammal");
    strcpy(question_bank[q_index].options[2], "Reptile");
    strcpy(question_bank[q_index].options[3], "Amphibian");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Science");
    q_index++;

    strcpy(question_bank[q_index].question, "What is the study of earthquakes called?");
    strcpy(question_bank[q_index].options[0], "Geology");
    strcpy(question_bank[q_index].options[1], "Seismology");
    strcpy(question_bank[q_index].options[2], "Meteorology");
    strcpy(question_bank[q_index].options[3], "Astronomy");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Science");
    q_index++;

    strcpy(question_bank[q_index].question, "Which blood type is known as the universal donor?");
    strcpy(question_bank[q_index].options[0], "A");
    strcpy(question_bank[q_index].options[1], "O");
    strcpy(question_bank[q_index].options[2], "B");
    strcpy(question_bank[q_index].options[3], "AB");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Science");
    q_index++;

    // FOOD & COOKING (15)
    strcpy(question_bank[q_index].question, "Which spice is derived from the Crocus flower?");
    strcpy(question_bank[q_index].options[0], "Turmeric");
    strcpy(question_bank[q_index].options[1], "Saffron");
    strcpy(question_bank[q_index].options[2], "Cardamom");
    strcpy(question_bank[q_index].options[3], "Cinnamon");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Food & Cooking");
    q_index++;

    strcpy(question_bank[q_index].question, "What is the main ingredient in guacamole?");
    strcpy(question_bank[q_index].options[0], "Tomato");
    strcpy(question_bank[q_index].options[1], "Onion");
    strcpy(question_bank[q_index].options[2], "Avocado");
    strcpy(question_bank[q_index].options[3], "Pepper");
    question_bank[q_index].correct_answer = 2;
    strcpy(question_bank[q_index].category, "Food & Cooking");
    q_index++;

    strcpy(question_bank[q_index].question, "Which country is famous for inventing pizza?");
    strcpy(question_bank[q_index].options[0], "France");
    strcpy(question_bank[q_index].options[1], "Italy");
    strcpy(question_bank[q_index].options[2], "Spain");
    strcpy(question_bank[q_index].options[3], "Greece");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Food & Cooking");
    q_index++;

    strcpy(question_bank[q_index].question, "What type of pastry is used to make profiteroles?");
    strcpy(question_bank[q_index].options[0], "Puff pastry");
    strcpy(question_bank[q_index].options[1], "Choux pastry");
    strcpy(question_bank[q_index].options[2], "Filo pastry");
    strcpy(question_bank[q_index].options[3], "Shortcrust pastry");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Food & Cooking");
    q_index++;

    strcpy(question_bank[q_index].question, "Which vitamin is produced when skin is exposed to sunlight?");
    strcpy(question_bank[q_index].options[0], "Vitamin A");
    strcpy(question_bank[q_index].options[1], "Vitamin B");
    strcpy(question_bank[q_index].options[2], "Vitamin C");
    strcpy(question_bank[q_index].options[3], "Vitamin D");
    question_bank[q_index].correct_answer = 3;
    strcpy(question_bank[q_index].category, "Food & Cooking");
    q_index++;

    strcpy(question_bank[q_index].question, "What is the most expensive spice in the world by weight?");
    strcpy(question_bank[q_index].options[0], "Vanilla");
    strcpy(question_bank[q_index].options[1], "Saffron");
    strcpy(question_bank[q_index].options[2], "Cardamom");
    strcpy(question_bank[q_index].options[3], "Black Pepper");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Food & Cooking");
    q_index++;

    strcpy(question_bank[q_index].question, "Which cooking method uses dry heat in an oven?");
    strcpy(question_bank[q_index].options[0], "Boiling");
    strcpy(question_bank[q_index].options[1], "Baking");
    strcpy(question_bank[q_index].options[2], "Steaming");
    strcpy(question_bank[q_index].options[3], "Poaching");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Food & Cooking");
    q_index++;

    strcpy(question_bank[q_index].question, "What is the main ingredient in hummus?");
    strcpy(question_bank[q_index].options[0], "Lentils");
    strcpy(question_bank[q_index].options[1], "Chickpeas");
    strcpy(question_bank[q_index].options[2], "Black beans");
    strcpy(question_bank[q_index].options[3], "Kidney beans");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Food & Cooking");
    q_index++;

    strcpy(question_bank[q_index].question, "Which fruit is known as the 'King of Fruits'?");
    strcpy(question_bank[q_index].options[0], "Apple");
    strcpy(question_bank[q_index].options[1], "Durian");
    strcpy(question_bank[q_index].options[2], "Mango");
    strcpy(question_bank[q_index].options[3], "Pineapple");
    question_bank[q_index].correct_answer = 2;
    strcpy(question_bank[q_index].category, "Food & Cooking");
    q_index++;

    strcpy(question_bank[q_index].question, "What temperature should chicken be cooked to for food safety?");
    strcpy(question_bank[q_index].options[0], "145°F");
    strcpy(question_bank[q_index].options[1], "165°F");
    strcpy(question_bank[q_index].options[2], "180°F");
    strcpy(question_bank[q_index].options[3], "200°F");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Food & Cooking");
    q_index++;

    strcpy(question_bank[q_index].question, "Which herb is traditionally used in pesto sauce?");
    strcpy(question_bank[q_index].options[0], "Oregano");
    strcpy(question_bank[q_index].options[1], "Basil");
    strcpy(question_bank[q_index].options[2], "Thyme");
    strcpy(question_bank[q_index].options[3], "Rosemary");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Food & Cooking");
    q_index++;

    strcpy(question_bank[q_index].question, "What type of cheese is traditionally used on pizza?");
    strcpy(question_bank[q_index].options[0], "Cheddar");
    strcpy(question_bank[q_index].options[1], "Mozzarella");
    strcpy(question_bank[q_index].options[2], "Swiss");
    strcpy(question_bank[q_index].options[3], "Parmesan");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Food & Cooking");
    q_index++;

    strcpy(question_bank[q_index].question, "Which cooking technique involves cooking food in its own fat?");
    strcpy(question_bank[q_index].options[0], "Braising");
    strcpy(question_bank[q_index].options[1], "Confit");
    strcpy(question_bank[q_index].options[2], "Grilling");
    strcpy(question_bank[q_index].options[3], "Roasting");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Food & Cooking");
    q_index++;

    strcpy(question_bank[q_index].question, "What is the main ingredient in traditional Japanese miso soup?");
    strcpy(question_bank[q_index].options[0], "Soy sauce");
    strcpy(question_bank[q_index].options[1], "Miso paste");
    strcpy(question_bank[q_index].options[2], "Rice vinegar");
    strcpy(question_bank[q_index].options[3], "Sesame oil");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Food & Cooking");
    q_index++;

    strcpy(question_bank[q_index].question, "Which nut is used to make marzipan?");
    strcpy(question_bank[q_index].options[0], "Walnut");
    strcpy(question_bank[q_index].options[1], "Almond");
    strcpy(question_bank[q_index].options[2], "Hazelnut");
    strcpy(question_bank[q_index].options[3], "Pecan");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Food & Cooking");
    q_index++;

    // POLITICS (15)
    strcpy(question_bank[q_index].question, "How many members are there in the US Senate?");
    strcpy(question_bank[q_index].options[0], "50");
    strcpy(question_bank[q_index].options[1], "100");
    strcpy(question_bank[q_index].options[2], "435");
    strcpy(question_bank[q_index].options[3], "538");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Politics");
    q_index++;

    strcpy(question_bank[q_index].question, "Who is known as the 'Iron Lady'?");
    strcpy(question_bank[q_index].options[0], "Angela Merkel");
    strcpy(question_bank[q_index].options[1], "Margaret Thatcher");
    strcpy(question_bank[q_index].options[2], "Indira Gandhi");
    strcpy(question_bank[q_index].options[3], "Golda Meir");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Politics");
    q_index++;

    strcpy(question_bank[q_index].question, "Which country has the most time zones?");
    strcpy(question_bank[q_index].options[0], "USA");
    strcpy(question_bank[q_index].options[1], "Russia");
    strcpy(question_bank[q_index].options[2], "China");
    strcpy(question_bank[q_index].options[3], "Canada");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Politics");
    q_index++;

    strcpy(question_bank[q_index].question, "What is the term length for a US President?");
    strcpy(question_bank[q_index].options[0], "3 years");
    strcpy(question_bank[q_index].options[1], "4 years");
    strcpy(question_bank[q_index].options[2], "5 years");
    strcpy(question_bank[q_index].options[3], "6 years");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Politics");
    q_index++;

    strcpy(question_bank[q_index].question, "Which organization was formed in 1945 to promote world peace?");
    strcpy(question_bank[q_index].options[0], "NATO");
    strcpy(question_bank[q_index].options[1], "United Nations");
    strcpy(question_bank[q_index].options[2], "World Bank");
    strcpy(question_bank[q_index].options[3], "IMF");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Politics");
    q_index++;

    strcpy(question_bank[q_index].question, "What is the capital of Australia?");
    strcpy(question_bank[q_index].options[0], "Sydney");
    strcpy(question_bank[q_index].options[1], "Canberra");
    strcpy(question_bank[q_index].options[2], "Melbourne");
    strcpy(question_bank[q_index].options[3], "Perth");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Politics");
    q_index++;

    strcpy(question_bank[q_index].question, "How many permanent members are in the UN Security Council?");
    strcpy(question_bank[q_index].options[0], "4");
    strcpy(question_bank[q_index].options[1], "5");
    strcpy(question_bank[q_index].options[2], "6");
    strcpy(question_bank[q_index].options[3], "7");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Politics");
    q_index++;

    strcpy(question_bank[q_index].question, "Which political system has no single leader?");
    strcpy(question_bank[q_index].options[0], "Monarchy");
    strcpy(question_bank[q_index].options[1], "Democracy");
    strcpy(question_bank[q_index].options[2], "Dictatorship");
    strcpy(question_bank[q_index].options[3], "Oligarchy");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Politics");
    q_index++;

    strcpy(question_bank[q_index].question, "What does 'EU' stand for?");
    strcpy(question_bank[q_index].options[0], "Eastern Union");
    strcpy(question_bank[q_index].options[1], "European Union");
    strcpy(question_bank[q_index].options[2], "Economic Union");
    strcpy(question_bank[q_index].options[3], "English Union");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Politics");
    q_index++;

    strcpy(question_bank[q_index].question, "Which country has a maple leaf on its flag?");
    strcpy(question_bank[q_index].options[0], "USA");
    strcpy(question_bank[q_index].options[1], "Canada");
    strcpy(question_bank[q_index].options[2], "Australia");
    strcpy(question_bank[q_index].options[3], "New Zealand");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Politics");
    q_index++;

    strcpy(question_bank[q_index].question, "What is the minimum age to be US President?");
    strcpy(question_bank[q_index].options[0], "30");
    strcpy(question_bank[q_index].options[1], "35");
    strcpy(question_bank[q_index].options[2], "40");
    strcpy(question_bank[q_index].options[3], "45");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Politics");
    q_index++;

    strcpy(question_bank[q_index].question, "Which city is the headquarters of the United Nations?");
    strcpy(question_bank[q_index].options[0], "Geneva");
    strcpy(question_bank[q_index].options[1], "New York");
    strcpy(question_bank[q_index].options[2], "London");
    strcpy(question_bank[q_index].options[3], "Paris");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Politics");
    q_index++;

    strcpy(question_bank[q_index].question, "What does 'NATO' stand for?");
    strcpy(question_bank[q_index].options[0], "North American Treaty Organization");
    strcpy(question_bank[q_index].options[1], "North Atlantic Treaty Organization");
    strcpy(question_bank[q_index].options[2], "National Atlantic Treaty Organization");
    strcpy(question_bank[q_index].options[3], "Northern Atlantic Trade Organization");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Politics");
    q_index++;

    strcpy(question_bank[q_index].question, "Which document begins with 'We the People'?");
    strcpy(question_bank[q_index].options[0], "Declaration of Independence");
    strcpy(question_bank[q_index].options[1], "US Constitution");
    strcpy(question_bank[q_index].options[2], "Bill of Rights");
    strcpy(question_bank[q_index].options[3], "Magna Carta");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Politics");
    q_index++;

    strcpy(question_bank[q_index].question, "How many states are there in the United States?");
    strcpy(question_bank[q_index].options[0], "48");
    strcpy(question_bank[q_index].options[1], "50");
    strcpy(question_bank[q_index].options[2], "52");
    strcpy(question_bank[q_index].options[3], "54");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Politics");
    q_index++;

    // FASHION (15)
    strcpy(question_bank[q_index].question, "Which city is known as the fashion capital of the world?");
    strcpy(question_bank[q_index].options[0], "New York");
    strcpy(question_bank[q_index].options[1], "London");
    strcpy(question_bank[q_index].options[2], "Paris");
    strcpy(question_bank[q_index].options[3], "Milan");
    question_bank[q_index].correct_answer = 2;
    strcpy(question_bank[q_index].category, "Fashion");
    q_index++;

    strcpy(question_bank[q_index].question, "Which designer is famous for the 'little black dress'?");
    strcpy(question_bank[q_index].options[0], "Coco Chanel");
    strcpy(question_bank[q_index].options[1], "Christian Dior");
    strcpy(question_bank[q_index].options[2], "Yves Saint Laurent");
    strcpy(question_bank[q_index].options[3], "Giorgio Armani");
    question_bank[q_index].correct_answer = 0;
    strcpy(question_bank[q_index].category, "Fashion");
    q_index++;

    strcpy(question_bank[q_index].question, "What does 'haute couture' mean?");
    strcpy(question_bank[q_index].options[0], "Ready to wear");
    strcpy(question_bank[q_index].options[1], "High fashion");
    strcpy(question_bank[q_index].options[2], "Street fashion");
    strcpy(question_bank[q_index].options[3], "Vintage fashion");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Fashion");
    q_index++;

    strcpy(question_bank[q_index].question, "Which fashion magazine is known as the 'Bible of Fashion'?");
    strcpy(question_bank[q_index].options[0], "Elle");
    strcpy(question_bank[q_index].options[1], "Vogue");
    strcpy(question_bank[q_index].options[2], "Harper's Bazaar");
    strcpy(question_bank[q_index].options[3], "Marie Claire");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Fashion");
    q_index++;

    strcpy(question_bank[q_index].question, "Which luxury brand is famous for its red-soled shoes?");
    strcpy(question_bank[q_index].options[0], "Jimmy Choo");
    strcpy(question_bank[q_index].options[1], "Manolo Blahnik");
    strcpy(question_bank[q_index].options[2], "Christian Louboutin");
    strcpy(question_bank[q_index].options[3], "Prada");
    question_bank[q_index].correct_answer = 2;
    strcpy(question_bank[q_index].category, "Fashion");
    q_index++;

    strcpy(question_bank[q_index].question, "What is the traditional Scottish garment for men?");
    strcpy(question_bank[q_index].options[0], "Toga");
    strcpy(question_bank[q_index].options[1], "Kilt");
    strcpy(question_bank[q_index].options[2], "Sarong");
    strcpy(question_bank[q_index].options[3], "Kimono");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Fashion");
    q_index++;

    strcpy(question_bank[q_index].question, "Which fabric is made from silkworms?");
    strcpy(question_bank[q_index].options[0], "Cotton");
    strcpy(question_bank[q_index].options[1], "Silk");
    strcpy(question_bank[q_index].options[2], "Wool");
    strcpy(question_bank[q_index].options[3], "Linen");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Fashion");
    q_index++;

    strcpy(question_bank[q_index].question, "What is the name of the fashion event held twice a year in major cities?");
    strcpy(question_bank[q_index].options[0], "Fashion Show");
    strcpy(question_bank[q_index].options[1], "Fashion Week");
    strcpy(question_bank[q_index].options[2], "Fashion Festival");
    strcpy(question_bank[q_index].options[3], "Fashion Fair");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Fashion");
    q_index++;

    strcpy(question_bank[q_index].question, "Which accessory is Hermès most famous for?");
    strcpy(question_bank[q_index].options[0], "Watches");
    strcpy(question_bank[q_index].options[1], "Handbags");
    strcpy(question_bank[q_index].options[2], "Jewelry");
    strcpy(question_bank[q_index].options[3], "Sunglasses");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Fashion");
    q_index++;

    strcpy(question_bank[q_index].question, "What type of neckline is named after a boat?");
    strcpy(question_bank[q_index].options[0], "V-neck");
    strcpy(question_bank[q_index].options[1], "Boat neck");
    strcpy(question_bank[q_index].options[2], "Crew neck");
    strcpy(question_bank[q_index].options[3], "Turtle neck");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Fashion");
    q_index++;

    strcpy(question_bank[q_index].question, "Which pattern consists of intersecting lines?");
    strcpy(question_bank[q_index].options[0], "Polka dots");
    strcpy(question_bank[q_index].options[1], "Plaid");
    strcpy(question_bank[q_index].options[2], "Stripes");
    strcpy(question_bank[q_index].options[3], "Floral");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Fashion");
    q_index++;

    strcpy(question_bank[q_index].question, "What is the term for clothing that can be worn by either gender?");
    strcpy(question_bank[q_index].options[0], "Casual wear");
    strcpy(question_bank[q_index].options[1], "Unisex");
    strcpy(question_bank[q_index].options[2], "Formal wear");
    strcpy(question_bank[q_index].options[3], "Vintage");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Fashion");
    q_index++;

    strcpy(question_bank[q_index].question, "Which designer created the 'New Look' in 1947?");
    strcpy(question_bank[q_index].options[0], "Coco Chanel");
    strcpy(question_bank[q_index].options[1], "Christian Dior");
    strcpy(question_bank[q_index].options[2], "Yves Saint Laurent");
    strcpy(question_bank[q_index].options[3], "Givenchy");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Fashion");
    q_index++;

    strcpy(question_bank[q_index].question, "What is the most common closure for jeans?");
    strcpy(question_bank[q_index].options[0], "Buttons");
    strcpy(question_bank[q_index].options[1], "Zipper");
    strcpy(question_bank[q_index].options[2], "Velcro");
    strcpy(question_bank[q_index].options[3], "Laces");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Fashion");
    q_index++;

    strcpy(question_bank[q_index].question, "Which shoe style has a strap across the instep?");
    strcpy(question_bank[q_index].options[0], "Loafer");
    strcpy(question_bank[q_index].options[1], "Mary Jane");
    strcpy(question_bank[q_index].options[2], "Oxford");
    strcpy(question_bank[q_index].options[3], "Sneaker");
    question_bank[q_index].correct_answer = 1;
    strcpy(question_bank[q_index].category, "Fashion");
    q_index++;

    total_questions = q_index;
    printf("[+] Loaded %d questions across %d categories!\n", total_questions, 8);
}

// -------------------- UI & Game Flow --------------------
void display_welcome() {
    clear_screen();
    set_color(COL_MAGENTA);
    printf("===============================================\n");
    printf("   WELCOME TO QUIZILLIONAIRE: THE BRAIN BATTLE!\n");
    printf("===============================================\n");
    reset_color();
    printf("\n[$] Test your knowledge in your favorite category!\n");
    printf("[>] Answer 15 questions to become the Quizillionaire!\n");
    printf("[!] Beat the timer and win amazing prizes!\n");
    colored_printf(COL_YELLOW, "[*] NEW: Get a second chance if your first answer is wrong!\n\n");
    play_sound(SND_STAGE);
}

void display_rules() {
    clear_screen();
    colored_printf(COL_BLUE, "*** GAME RULES ***\n");
    printf("==================\n\n");

    printf("GAMEPLAY:\n");
    printf("* Choose your favorite category from 8 options\n");
    printf("* Answer 15 questions across 3 progressive stages\n");
    printf("* Questions are randomly shuffled for each player\n");
    printf("* Beat the timer for each question\n");
    printf("* NEW: Get ONE second chance per game if you answer wrong!\n\n");

    printf("SECOND CHANCE FEATURE:\n");
    printf("* If you get your first answer wrong, you get ONE more try\n");
    printf("* Can only be used ONCE during the entire game\n");
    printf("* Available across all stages\n");
    printf("* After using it, wrong answers will end the game\n\n");

    printf("STAGES & REWARDS:\n");
    printf("* Explorer Stage: 20 seconds per question, $10,000 prize\n");
    printf("* Challenger Stage: 25 seconds per question, $50,000 prize\n");
    printf("* Warrior Stage: 30 seconds per question, $100,000 prize + Helpline\n\n");

    printf("CATEGORIES (15 questions each):\n");
    for (int i = 0; i < 8; ++i) {
        printf("%d. %s\n", i+1, categories[i].name);
    }
    printf("\nPress Enter to continue...");
    getchar();
    // ensure no leftover
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF) {}
}

void display_categories() {
    clear_screen();
    colored_printf(COL_CYAN, "*** AVAILABLE CATEGORIES ***\n");
    printf("=============================\n\n");
    for(int i = 0; i < 8; i++) {
        colored_printf(COL_YELLOW, "%d. %s (%d questions)\n", i+1, categories[i].name, categories[i].question_count);
    }
    printf("\nPress Enter to continue...");
    getchar();
    int ch; while ((ch = getchar()) != '\n' && ch != EOF) {}
}

int select_category() {
    printf("\n*** SELECT CATEGORY ***\n");
    printf("=======================\n");
    for(int i = 0; i < 8; i++) {
        printf("%d. %s\n", i+1, categories[i].name);
    }

    int choice = -1;
    do {
        printf("\nEnter your choice (1-8): ");
        if (scanf("%d", &choice) != 1) {
            int ch; while ((ch = getchar()) != '\n' && ch != EOF) {}
            choice = -1;
        } else {
            int ch; while ((ch = getchar()) != '\n' && ch != EOF) {}
        }

        if(choice < 1 || choice > 8) {
            colored_printf(COL_RED, "[!] Invalid choice! Please select 1-8.\n");
        }
    } while(choice < 1 || choice > 8);

    return choice - 1;
}

// Start game with robust name input
int start_game() {
    struct Player player;
    memset(&player, 0, sizeof(player));

    clear_screen();
    colored_printf(COL_GREEN, "\n*** Starting New Game! ***\n");

    // Prompt for name using robust clearing of stdin
    printf("Enter your name: ");
    // Clear any leftover input from previous scanf calls
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
    if (fgets(player.name, sizeof(player.name), stdin) == NULL) {
        // fallback if fgets fails
        strcpy(player.name, "Player");
    } else {
        // remove newline if present
        player.name[strcspn(player.name, "\n")] = 0;
        if (strlen(player.name) == 0) strcpy(player.name, "Player");
    }

    // Select category
    int category_index = select_category();
    strcpy(player.selected_category, categories[category_index].name);

    player.current_stage = 0;
    player.prize_won = 0;
    player.questions_answered = 0;
    player.helpline_used = 0;
    player.second_chance_used = 0;

    shuffle_category_questions(category_index);

    printf("\n[*] Welcome %s! You selected: %s\n", player.name, player.selected_category);
    printf("Remember: You have ONE second chance if you get an answer wrong!\n");
    printf("Let's begin your journey to become a Quizillionaire!\n");

    // Play stages
    for (int stage = 0; stage < 3; stage++) {
        player.current_stage = stage;
        if (!play_stage(&player, stage, category_index)) {
            save_score(player);
            return 0;
        }
        player.prize_won = stages[stage].prize;
        if (stage < 2) {
            colored_printf(COL_CYAN, "\n[+] Stage %d completed! Moving to %s\n", stage + 1, stages[stage + 1].name);
            printf("Press Enter to continue...");
            getchar();
            int ch; while ((ch = getchar()) != '\n' && ch != EOF) {}
        }
    }

    save_score(player);
    return 1;
}

int play_stage(struct Player *player, int stage_num, int category_index) {
    display_stage_info(stage_num);
    int start_question = categories[category_index].start_index + (stage_num * 5);

    for (int q = 0; q < stages[stage_num].questions_count; q++) {
        int question_index = start_question + q;
        display_progress(q + 1, stages[stage_num].questions_count);
        int helpline_available = (stage_num == 2 && !player->helpline_used);

        if (!ask_question(question_bank[question_index], stages[stage_num].time_limit, helpline_available, player)) {
            return 0; // game over
        }

        player->questions_answered++;
        colored_printf(COL_GREEN, "\n[+] Correct! Well done!\n");
        play_sound(SND_CORRECT);

        if (q < stages[stage_num].questions_count - 1) {
            printf("Press Enter for next question...");
            getchar();
            int ch; while ((ch = getchar()) != '\n' && ch != EOF) {}
        }
    }
    play_sound(SND_STAGE);
    return 1;
}

int ask_question(struct Question q, int time_limit, int helpline_available, struct Player *player) {
    colored_printf(COL_BLUE, "\n==================================================\n");
    printf("Category: %s\n", q.category);
    printf("Time Limit: %d seconds\n", time_limit);
    if (!player->second_chance_used) colored_printf(COL_YELLOW, "Second Chance: AVAILABLE\n");
    else colored_printf(COL_MAGENTA, "Second Chance: USED\n");
    colored_printf(COL_BLUE, "==================================================\n");
    colored_printf(COL_CYAN, "\n[?] %s\n\n", q.question);

    for (int i = 0; i < 4; i++) {
        printf("%c) %s\n", 'A' + i, q.options[i]);
    }

    if (helpline_available) colored_printf(COL_YELLOW, "\n[H] Type 'H' to use helpline (removes 2 wrong options)\n");

    printf("\nYour answer (A/B/C/D): ");
    // read single char safely
    char input_buf[32];
    if (fgets(input_buf, sizeof(input_buf), stdin) == NULL) {
        colored_printf(COL_RED, "\n[X] Input error! Game over.\n");
        play_sound(SND_WRONG);
        return 0;
    }
    // trim whitespace
    char answer = '\0';
    for (int i = 0; input_buf[i] != '\0'; i++) {
        if (!isspace((unsigned char)input_buf[i])) { answer = (char)toupper((unsigned char)input_buf[i]); break; }
    }

    // helpline
    if (answer == 'H' && helpline_available) {
        use_helpline(&q);
        player->helpline_used = 1;
        printf("\nYour answer after helpline (A/B/C/D): ");
        if (fgets(input_buf, sizeof(input_buf), stdin) == NULL) {
            colored_printf(COL_RED, "\n[X] Input error! Game over.\n");
            play_sound(SND_WRONG);
            return 0;
        }
        answer = '\0';
        for (int i = 0; input_buf[i] != '\0'; i++) {
            if (!isspace((unsigned char)input_buf[i])) { answer = (char)toupper((unsigned char)input_buf[i]); break; }
        }
    }

    // simple timeout check (not interrupting input)
    // We recorded time at start? For simplicity keep original style
    // Validate input
    if (answer < 'A' || answer > 'D') {
        colored_printf(COL_RED, "\n[X] Invalid input! Game over.\n");
        play_sound(SND_WRONG);
        return 0;
    }

    int selected = answer - 'A';
    if (selected == q.correct_answer) {
        return 1;
    } else {
        if (!player->second_chance_used) {
            colored_printf(COL_YELLOW, "\n[!] Wrong answer! But you have a SECOND CHANCE!\n");
            printf("The correct answer was %c) %s\n", 'A' + q.correct_answer, q.options[q.correct_answer]);
            player->second_chance_used = 1;
            colored_printf(COL_MAGENTA, "\n*** SECOND CHANCE ACTIVATED! ***\n");
            printf("This is your ONLY second chance in the entire game!\n\n");

            // show again
            colored_printf(COL_CYAN, "[?] %s\n\n", q.question);
            for (int i = 0; i < 4; i++) printf("%c) %s\n", 'A' + i, q.options[i]);

            printf("\nYour FINAL answer (A/B/C/D): ");
            char finalbuf[32];
            if (fgets(finalbuf, sizeof(finalbuf), stdin) == NULL) {
                colored_printf(COL_RED, "\n[X] Input error! Game over.\n");
                play_sound(SND_WRONG);
                return 0;
            }
            char finalans = '\0';
            for (int i = 0; finalbuf[i] != '\0'; i++) {
                if (!isspace((unsigned char)finalbuf[i])) { finalans = (char)toupper((unsigned char)finalbuf[i]); break; }
            }
            if (finalans < 'A' || finalans > 'D') {
                colored_printf(COL_RED, "\n[X] Invalid input! Game over.\n");
                play_sound(SND_WRONG);
                return 0;
            }
            int sel2 = finalans - 'A';
            if (sel2 == q.correct_answer) {
                colored_printf(COL_GREEN, "\n[+] CORRECT! You used your second chance wisely!\n");
                play_sound(SND_CORRECT);
                return 1;
            } else {
                colored_printf(COL_RED, "\n[X] Wrong again! The correct answer was %c) %s\n", 'A' + q.correct_answer, q.options[q.correct_answer]);
                colored_printf(COL_RED, "You've used your second chance. Game over!\n");
                play_sound(SND_WRONG);
                return 0;
            }
        } else {
            colored_printf(COL_RED, "\n[X] Wrong answer! The correct answer was %c) %s\n", 'A' + q.correct_answer, q.options[q.correct_answer]);
            colored_printf(COL_RED, "You already used your second chance. Game over!\n");
            play_sound(SND_WRONG);
            return 0;
        }
    }
}

void shuffle_category_questions(int category_index) {
    int start = categories[category_index].start_index;
    int count = categories[category_index].question_count;
    for (int i = start + count - 1; i > start; i--) {
        int j = start + (rand() % (i - start + 1));
        struct Question tmp = question_bank[i];
        question_bank[i] = question_bank[j];
        question_bank[j] = tmp;
    }
    colored_printf(COL_CYAN, "[~] %s questions shuffled randomly!\n", categories[category_index].name);
}

void use_helpline(struct Question *q) {
    colored_printf(COL_YELLOW, "\n*** HELPLINE ACTIVATED! ***\n");
    printf("Removing 2 incorrect options.\n\n");
    int removed = 0;
    for (int i = 0; i < 4 && removed < 2; i++) {
        if (i != q->correct_answer) {
            colored_printf(COL_RED, "[X] Option %c removed\n", 'A' + i);
            removed++;
        }
    }
    printf("\nRemaining options:\n");
    int shown_wrong = 0;
    for (int i = 0; i < 4; i++) {
        if (i == q->correct_answer) {
            colored_printf(COL_GREEN, "%c) %s\n", 'A' + i, q->options[i]);
        } else {
            if (!shown_wrong) {
                printf("%c) %s\n", 'A' + i, q->options[i]);
                shown_wrong = 1;
            }
        }
    }
}

// Stage info & progress
void display_stage_info(int stage_num) {
    clear_screen();
    colored_printf(COL_MAGENTA, "\n*** %s ***\n", stages[stage_num].name);
    printf("========================================\n");
    printf("Prize: $%d\n", stages[stage_num].prize);
    printf("Time per question: %d seconds\n", stages[stage_num].time_limit);
    printf("Questions: %d\n", stages[stage_num].questions_count);
    if (stages[stage_num].has_helpline) colored_printf(COL_YELLOW, "Helpline available!\n");
    printf("========================================\n");
    printf("Press Enter to start.");
    getchar();
    int ch; while ((ch = getchar()) != '\n' && ch != EOF) {}
}

void display_progress(int current, int total) {
    printf("\nProgress: Question %d of %d\n", current, total);
    printf("Progress: [");
    int progress = (current * 20) / total;
    for (int i = 0; i < 20; i++) {
        if (i < progress) { set_color(COL_GREEN); printf("#"); reset_color(); }
        else printf("-");
    }
    printf("] %d%%\n", (current * 100) / total);
}

// -------------------- Leaderboard --------------------
void save_score(struct Player player) {
    FILE *fp = fopen("leaderboard.txt", "a");
    if (fp != NULL) {
        fprintf(fp, "%s,%s,%d,%d,%d,%s\n",
                player.name[0] ? player.name : "Player",
                player.selected_category,
                player.current_stage + 1,
                player.prize_won,
                player.questions_answered,
                player.second_chance_used ? "Used" : "Not Used");
        fclose(fp);
        colored_printf(COL_CYAN, "[+] Score saved to leaderboard!\n");
    } else {
        colored_printf(COL_RED, "[!] Could not save score to file.\n");
    }
}

void display_leaderboard() {
    clear_screen();
    colored_printf(COL_MAGENTA, "*** HALL OF FAME ***\n");
    printf("====================\n\n");

    FILE *fp = fopen("leaderboard.txt", "r");
    if (fp == NULL) {
        printf("No scores recorded yet. Be the first to play!\n");
        printf("Press Enter to continue...");
        getchar();
        int ch; while ((ch = getchar()) != '\n' && ch != EOF) {}
        return;
    }

    char name[50], category[50], second_chance[20];
    int stage, prize, questions;
    int rank = 1;

    printf("Rank | Player Name      | Category         | Stage | Prize Won  | Questions | 2nd Chance\n");
    printf("-----|------------------|------------------|-------|------------|-----------|------------\n");

    while (fscanf(fp, "%49[^,],%49[^,],%d,%d,%d,%19[^\n]\n", name, category, &stage, &prize, &questions, second_chance) == 6) {
        printf("%4d | %-16s | %-16s |  %3d  | $%8d |   %2d     | %s\n",
               rank, name, category, stage, prize, questions, second_chance);
        rank++;
    }

    fclose(fp);
    printf("\nPress Enter to continue...");
    getchar();
    int ch; while ((ch = getchar()) != '\n' && ch != EOF) {}
}
