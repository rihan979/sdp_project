#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// -------------------- STRUCTURES --------------------
struct Stage {
    char name[30];
    int prize;
    int time_limit;
    int questions_count;
    int has_helpline;
};

// -------------------- GLOBAL VARIABLES --------------------
struct Stage stages[4] = {
    {"Explorer Stage", 10000, 15, 5, 0},
    {"Challenger Stage", 50000, 20, 5, 0},
    {"Warrior Stage", 100000, 25, 5, 1},
    {"Legend Stage", 500000, 30, 5, 1}
};

// -------------------- FUNCTION DECLARATIONS --------------------
void initialize_game();
void display_welcome();
void display_rules();
void display_leaderboard();
int start_game();
void clear_screen();

// -------------------- MAIN FUNCTION --------------------
int main() {
    initialize_game();
    display_welcome();

    int choice;
    do {
        printf("\n*** MAIN MENU ***\n");
        printf("==================\n");
        printf("1. [>] Start Game\n");
        printf("2. [?] View Rules\n");
        printf("3. [*] Leaderboard\n");
        printf("4. [X] Exit\n");
        printf("Enter your choice (1-4): ");

        scanf("%d", &choice);

        switch(choice) {
            case 1:
                if (start_game()) {
                    printf("\n*** CONGRATULATIONS! YOU'RE THE QUIZILLIONAIRE! ***\n");
                } else {
                    printf("\n*** Game Over! Better luck next time! ***\n");
                }
                break;
            case 2:
                display_rules();
                break;
            case 3:
                display_leaderboard();
                break;
            case 4:
                printf("\nThanks for playing Quizillionaire! Goodbye!\n");
                break;
            default:
                printf("\n[!] Invalid choice! Please try again.\n");
        }
    } while (choice != 4);

    return 0;
}

// -------------------- FUNCTION DEFINITIONS --------------------

// Initialize game data
void initialize_game() {
    srand(time(NULL));
    clear_screen();
    printf("[+] Game initialized successfully!\n");
    printf("[*] Total stages: %d\n\n", (int)(sizeof(stages)/sizeof(stages[0])));

    for (int i = 0; i < 4; i++) {
        printf("Stage %d: %s\n", i + 1, stages[i].name);
        printf("   Prize: $%d\n", stages[i].prize);
        printf("   Time/question: %d sec\n", stages[i].time_limit);
        printf("   Questions: %d\n", stages[i].questions_count);
        if (stages[i].has_helpline)
            printf("   Helpline: Available\n");
        else
            printf("   Helpline: Not available\n");
        printf("-----------------------------------\n");
    }
}

// Display welcome screen
void display_welcome() {
    printf("===============================================\n");
    printf("   WELCOME TO QUIZILLIONAIRE: THE BRAIN BATTLE!\n");
    printf("===============================================\n");
    printf("\n[$] Test your knowledge across multiple categories!\n");
    printf("[>] Answer all questions to become the Quizillionaire!\n");
    printf("[!] Beat the timer and win amazing prizes!\n\n");
}

// Display rules (simple demo)
void display_rules() {
    clear_screen();
    printf("*** GAME RULES ***\n");
    printf("==================\n");
    printf("1. Answer questions correctly to progress through stages.\n");
    printf("2. One wrong answer ends the game.\n");
    printf("3. Some stages include a helpline option.\n");
    printf("4. Each stage has a specific time limit per question.\n");
    printf("\nPress Enter to return to menu...");
    getchar();
    getchar();
}

// Dummy game start (for menu testing)
int start_game() {
    clear_screen();
    printf("[Game Simulation Started]\n");
    printf("Imagine questions appearing here...\n");
    printf("Simulating a win/lose outcome randomly...\n");

    int result = rand() % 2; // Random win/lose
    return result;
}

// Dummy leaderboard
void display_leaderboard() {
    clear_screen();
    printf("*** HALL OF FAME ***\n");
    printf("====================\n\n");
    printf("1. Alex       | $100000\n");
    printf("2. Maya       | $50000\n");
    printf("3. Rihan      | $10000\n");
    printf("\nPress Enter to return to menu...");
    getchar();
    getchar();
}

// Clear screen (cross-platform)
void clear_screen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}
