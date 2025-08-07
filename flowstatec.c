#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>
#ifdef _WIN32
    #include <windows.h>
    #define CLEAR_CMD "cls"
    #define sleep(x) Sleep((x) * 1000)
#else
    #include <unistd.h>
    #define CLEAR_CMD "clear"
#endif
#ifdef _WIN32
    #include <conio.h> // for _kbhit(), _getch()
#else
    #include <termios.h>
    #include <fcntl.h>
#endif

#ifndef _WIN32
int kbhit(void) {
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if (ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}
#endif


#define MAX_SESSIONS 100
#define INPUT_SIZE 256
#define LINE "────────────────────────────"

#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_CYAN    "\033[96m"
#define COLOR_BOLD    "\033[1m"

typedef enum { STATE_FOCUS, STATE_BREAK, STATE_PAUSED } State;

int paused = 0; 

void clear_screen() {
    system(CLEAR_CMD);
}

void wait_for_enter() {
    printf("Press [ENTER] to continue...");
    while(getchar() != '\n');
}

void print_time(int seconds, char *buf, size_t len) {
    int h = seconds / 3600;
    int m = (seconds % 3600) / 60;
    int s = seconds % 60;
    snprintf(buf, len, "%dh %dm %ds", h, m, s);
}

void display_stats(int total_focus, int total_break, int focus_count, int break_count) {
    char f_str[64], b_str[64];
    print_time(total_focus, f_str, sizeof(f_str));
    print_time(total_break, b_str, sizeof(b_str));
    printf(COLOR_CYAN LINE "\n");
    printf("Focus: %s | Breaks: %s\n", f_str, b_str);
    printf("Sessions: %d | Breaks taken: %d\n", focus_count, break_count);
    printf(LINE COLOR_RESET "\n");
}

int loading_bar(int total_sec, State state, const char *task, int session_num,
                int total_focus, int total_break, int focus_count, int break_count) {
    const int width = 30;
    int sec = 0;

    while (sec <= total_sec) {
#ifdef _WIN32
        if (_kbhit()) {
            int ch = _getch();
            if (ch == 'p' || ch == 'P') paused = !paused;
        }
#else
        if (kbhit()) {
            int ch = getchar();
            if (ch == 'p' || ch == 'P') paused = !paused;
        }
#endif

        printf("\033[H\033[J");
        display_stats(total_focus, total_break, focus_count, break_count);

        printf("%s%s Session #%d\n", COLOR_BOLD, state == STATE_FOCUS ? "Focus" : "Break", session_num);
        printf("Task: %s%s\n", COLOR_RESET, task);

        char timer_str[64];
        print_time(sec, timer_str, sizeof(timer_str));
        printf("Elapsed Time: %s\n", timer_str);

        if (paused) {
            printf(COLOR_YELLOW "Paused. Press 'p' to resume.\n" COLOR_RESET);
        } else {
            printf("\n%s[", state == STATE_FOCUS ? COLOR_GREEN : COLOR_YELLOW);
            int pos = (int)((float)sec / total_sec * width);
            for (int i = 0; i < width; i++) {
                printf(i < pos ? "█" : " ");
            }
            printf("] %3d%%\n" COLOR_RESET, (int)((float)sec / total_sec * 100));
        }

        if (!paused) {
            sec++;
        }

        sleep(1);
    }

    return 0;
}

int calculate_break_time(int focus_sec, int session_num) {
    struct { int max_sec; int break_min; } table[] = {
        {600, 1}, 
        {1500, 5}, 
        {3000, 10}, 
        {5400, 15}, 
        {INT_MAX, 20}
    };

    int min = 20;
    for (int i = 0; i < sizeof(table)/sizeof(table[0]); i++) {
        if (focus_sec < table[i].max_sec) {
            min = table[i].break_min;
            break;
        }
    }
    return (session_num % 4 == 0) ? min * 3 * 60 : min * 60;
}

int main() {
    char input[INPUT_SIZE], topic[INPUT_SIZE];
    int focus_times[MAX_SESSIONS] = {0};
    int break_times[MAX_SESSIONS] = {0};
    char notes[MAX_SESSIONS][INPUT_SIZE] = {{0}};
    int focus_total = 0, break_total = 0;
    int focus_count = 0, break_count = 0;
    int use_timer = 0, custom_focus = 1500, custom_break = 300;

    clear_screen();
    printf(COLOR_CYAN COLOR_BOLD "┌──────────────────────────┐\n");
    printf("│   Welcome to FlowTimer   │\n");
    printf("└──────────────────────────┘" COLOR_RESET "\n\n");

    printf("What is your main focus today?\n> ");
    fgets(topic, sizeof(topic), stdin);
    topic[strcspn(topic, "\n")] = '\0';

    int valid_mode = 0;
    while (!valid_mode) {
        printf("\nChoose mode:\n1. Open-ended (ENTER to stop)\n2. Fixed timer\n> ");
        fgets(input, sizeof(input), stdin);

        if (input[0] == '1') {
            use_timer = 0;
            valid_mode = 1;
        } else if (input[0] == '2') {
            use_timer = 1;
            valid_mode = 1;
        } else {
            printf(COLOR_RED "Invalid input. Please enter '1' or '2'.\n" COLOR_RESET);
        }
    }

    if (use_timer) {
        int min = 0;

        while (1) {
            printf("Enter focus duration in minutes (default 25): ");
            fgets(input, sizeof(input), stdin);
            if (input[0] == '\n') {
                break;
            }
            min = atoi(input);
            if (min > 0) {
                custom_focus = min * 60;
                break;
            } else {
                printf(COLOR_RED "Please enter a positive number.\n" COLOR_RESET);
            }
        }

        while (1) {
            printf("Enter break duration in minutes (default 5): ");
            fgets(input, sizeof(input), stdin);
            if (input[0] == '\n') {
                break;
            }
            min = atoi(input);
            if (min > 0) {
                custom_break = min * 60;
                break;
            } else {
                printf(COLOR_RED "Please enter a positive number.\n" COLOR_RESET);
            }
        }
    }

    while (focus_count < MAX_SESSIONS) {
        clear_screen();
        focus_count++;
        printf(COLOR_GREEN "Focus Session #%d\nTask: %s\n" COLOR_RESET, focus_count, topic);

        int focus_time = 0;
        if (use_timer) {
            paused = 0;  
            loading_bar(custom_focus, STATE_FOCUS, topic, focus_count, focus_total, break_total, focus_count - 1, break_count);
            focus_time = custom_focus;
        } else {
            time_t start = time(NULL);
            time_t paused_start = 0;
            int paused_duration = 0;
            paused = 0;  

            printf("Press [p] to pause at anytime\n");
            printf("Open-ended mode: Press ENTER to end the session. Press 'p' to pause/resume.\n");

            while (1) {
#ifdef _WIN32
                if (_kbhit()) {
                    int ch = _getch();
                    if (ch == '\r') break; // ENTER
                    else if (ch == 'p' || ch == 'P') {
                        if (!paused) {
                            paused = 1;
                            paused_start = time(NULL);
                        } else {
                            paused = 0;
                            paused_duration += time(NULL) - paused_start;
                        }
                    }
                }
#else
                if (kbhit()) {
                    int ch = getchar();
                    if (ch == '\n') break; // ENTER
                    else if (ch == 'p' || ch == 'P') {
                        if (!paused) {
                            paused = 1;
                            paused_start = time(NULL);
                        } else {
                            paused = 0;
                            paused_duration += time(NULL) - paused_start;
                        }
                    }
                }
#endif

                printf("\033[H\033[J");
                display_stats(focus_total, break_total, focus_count - 1, break_count);
                printf(COLOR_BOLD COLOR_GREEN "Focus Session #%d\n" COLOR_RESET, focus_count);
                printf(COLOR_BOLD "Task: " COLOR_RESET "%s\n", topic);


                int elapsed = (int)(time(NULL) - start - paused_duration);

                char timer_str[64];
                print_time(elapsed, timer_str, sizeof(timer_str));
                printf("Elapsed Time: %s\n", timer_str);

                if (paused) {
                    printf(COLOR_YELLOW "Paused. Press 'p' to resume.\n" COLOR_RESET);
                } else {
                    printf("\nPress ENTER to end this session.\n");
                }

                sleep(1);
            }
            focus_time = (int)(time(NULL) - start - paused_duration);
        }
        focus_times[focus_count] = focus_time;
        focus_total += focus_time;

        clear_screen();
        char buf[64];
        print_time(focus_time, buf, sizeof(buf));
        printf("Session complete. Duration: %s\nWhat did you do?\n> ", buf);
        fgets(notes[focus_count], sizeof(notes[focus_count]), stdin);
        notes[focus_count][strcspn(notes[focus_count], "\n")] = '\0';

        int break_time = use_timer ? custom_break : calculate_break_time(focus_time, focus_count);
        break_times[++break_count] = break_time;
        break_total += break_time;

        printf("\nTake a break (%d minutes).\nPress [ENTER] to begin or 'q' to quit: ", break_time / 60);
        fgets(input, sizeof(input), stdin);

        if (tolower(input[0]) == 'q') break;

        if (input[0] != '\n') {
            printf(COLOR_RED "Invalid input. Starting break anyway.\n" COLOR_RESET);
            sleep(2);
        }
        paused = 0;


        loading_bar(break_time, STATE_BREAK, "Break", focus_count, focus_total, break_total, focus_count, break_count);
    }

    printf("\nWould you like to save your stats? (Y/n): ");
    fgets(input, sizeof(input), stdin);

    if (tolower(input[0]) != 'n') {
        char filename[64];
        time_t now = time(NULL);
        strftime(filename, sizeof(filename), "FlowStats_%Y-%m-%d_%H.%M.%S.txt", localtime(&now));
        FILE *file = fopen(filename, "w");
        if (file) {
            fprintf(file, "Focus topic: %s\n", topic);
            char fstr[64], bstr[64];
            print_time(focus_total, fstr, sizeof(fstr));
            print_time(break_total, bstr, sizeof(bstr));
            fprintf(file, "Total Focus Time: %s\nTotal Break Time: %s\n\n", fstr, bstr);

            for (int i = 1; i <= focus_count; i++) {
                char session_str[64];
                print_time(focus_times[i], session_str, sizeof(session_str));
                fprintf(file, "Session %d: %s\n  Notes:  %s\n\n", i, session_str, notes[i]);
            }

            fclose(file);
            printf("Saved to %s\n", filename);
        } else {
            perror("Error writing file");
        }
    }

    printf("All done! Press [ENTER] to exit.\n");
    wait_for_enter();
    return 0;
}
