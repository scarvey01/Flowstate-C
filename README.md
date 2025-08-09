# Flowstate-C
**!THIS IS NOT MY ORIGINAL IDEA!** 

The original was made by [ThatCosmicStorm](https://github.com/ThatCosmicStorm/FlowState/tree/fa4b036821880f1daeedf73a45c979af6ff8cf7f) in Python.

I just made this because I wanted a project little learning project in C after watching a few great videos by [jdh](https://www.youtube.com/@jdh)

## The Original
The original FlowState is a command-line application written in Python. It tracks how long you have been focused and then gives you a break time based on your focus duration. It's currently being expanded to include a GUI using the tkinter library. 

Since the most recent commit is in a non-functional state, Im referencing the last working [commit](https://github.com/ThatCosmicStorm/FlowState/tree/f917d9460e51a000fa7c64f80dc7437feef4d343), which still uses a command-line interface. That version includes the following features:

- Takes note of focus time
- Allots appropriate break time
- Working timer
- Excludes idle time from total focus time
- Shows accurate statistics at program end
- Displays all focus and break times accurately
- Able to exit program after a focus session or break
- Waits for input (ENTER key) to start any focus session or break
- Sets a "main focus" and gives reminders every focus session
- Allows manual logging of what exactly was done during a focus session
- Better printing system that avoids clogging the terminal
- Automatically logs all stats in a new text file
- Play a sound to denote that a break has ended

Focus session:

![python focus image](https://github.com/scarvey01/Flowstate-C/blob/main/images/Screenshot_2025-08-05_19-39-59.png?raw=true)

Break Timer:

![python break timer](https://github.com/scarvey01/Flowstate-C/blob/main/images/Screenshot_2025-08-05_19-40-29.png?raw=true)



## This Version
This C version is also a command-line application. It includes all of the features from the original (except for sound notifications), along with some additional features I thought would be useful. I've also changed the implementation of some parts of the original program. more details below.

- ANSI styling for colors and formatting
- [Unicode box characters](https://en.wikipedia.org/wiki/Box-drawing_characters) for cleaner UI
- Cross-platform support (Windows and Unix-like systems)
- Logs total elapsed time (focus + break) and date/time of each session in the output text file
- Live stats dashboard during sessions
- User-customizable session lengths
- Option to choose between open-ended focus sessions or timer-based sessions
- Loading bars for fixed-length focus and break sessions
- Pause/resume functionality for focus sessions (fixed by chatgpt)
- Text color changes depending on current state (paused, focusing, break, etc.)
  
left: open-ended focus session | Right: Fixed Timer session
![c focus sessions](https://github.com/scarvey01/Flowstate-C/blob/main/images/Screenshot_2025-08-05_19-31-01.png?raw=true)

Break timer:

![c break session](https://github.com/scarvey01/Flowstate-C/blob/main/images/Screenshot_2025-08-05_19-32-25.png?raw=true)


## Coding
### Break time calculator logic
Python (original)
```
def calculate_break_time(focus_duration, session_number):
    if focus_duration < 600:      # < 10 min
        return 60                 # 1 min
    elif focus_duration < 1500:   # < 25 min
        return 300                # 5 min
    elif focus_duration < 3000:   # < 50 min
        return 600                # 10 min
    elif focus_duration < 5400:   # < 90 min
        return 900                # 15 min
    else:
        return 1200               # 20 min

    if session_number % 4 == 0:
        return break_time * 3
```

C
```
int calculate_break_time(int focus_sec, int session_num) {
    struct { int max_sec; int break_min; } table[] = {
        {600, 1},       // < 10 min =1  min break
        {1500, 5},      // < 25 min = 5 min break
        {3000, 10},     // < 50 min = 10 min break
        {5400, 15},     // < 90 min = 15 min break
        {INT_MAX, 20}   // > 90 min = 20 min break
    };

    int min = 20; // default if no match
    for (int i = 0; i < sizeof(table)/sizeof(table[0]); i++) {
        if (focus_sec < table[i].max_sec) {
            min = table[i].break_min;
            break;
        }
    }

    return (session_num % 4 == 0) ? min * 3 * 60 : min * 60;
}

```

**Explanation:**
The way I did it is more data driven using a struct array and a loop. The original uses hardcoded logic. My method allows for easier future expansion and is cleaner/more readable overall because you don’t need to write a bunch of elif blocks.


**Loading Bar**
```
int loading_bar(int total_sec, State state, const char *task, int session_num,
                int total_focus, int total_break, int focus_count, int break_count) {
    const int width = 30; //defines length of loading bar
    int sec = 0; //secs elapsed

    while (sec <= total_sec) {
#ifdef _WIN32 //cross platform support for pausing
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

        printf("\033[H\033[J"); //this is the ansi escape code that makes it so the screen doesnt flash anytime the time and loading bar is updated
        display_stats(total_focus, total_break, focus_count, break_count);

        printf("%s%s Session #%d\n", COLOR_BOLD, state == STATE_FOCUS ? "Focus" : "Break", session_num);
        printf("Task: %s%s\n", COLOR_RESET, task);

        char timer_str[64];  //shows time
        print_time(sec, timer_str, sizeof(timer_str));
        printf("Elapsed Time: %s\n", timer_str);

        if (paused) {
            printf(COLOR_YELLOW "Paused. Press 'p' to resume.\n" COLOR_RESET);
        } else {
            printf("\n%s[", state == STATE_FOCUS ? COLOR_GREEN : COLOR_YELLOW); //changes bar color based on current state
            int pos = (int)((float)sec / total_sec * width); //calculates how much of the bar should be filled
            for (int i = 0; i < width; i++) { //Prints the filled part and the remaining part
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
```
[ansi code resource](https://gist.github.com/ConnerWill/d4b6c776b509add763e17f9f113fd25b)

```\033[H\033[J```

- ```\033``` marks the beggining of ansi escape sequence
- ```[H``` moves cursor to the top left
- ```[J``` clears screen from the cursor’s current position (top-left) to the end of the screen

I couldnt use my clear_screen function here because clearing the screen every time the funcion looped it caused the screen to flicker every second.

[good source for loading bar](https://gist.github.com/amullins83/24b5ef48657c08c4005a8fab837b7499)

### Time stamped file save
```
char filename[64];
time_t now = time(NULL); // Get the current time
// Format the time into a filename like "FlowStats_2025-08-05_14.39.10.txt"
strftime(filename, sizeof(filename), "FlowStats_%Y-%m-%d_%H.%M.%S.txt", localtime(&now));
```

### Pausing Explanation
Just because ChatGPT help make this for doesnt mean its not important to understand. I would also like to say I had it working but in a much less efficient way. 

```
int kbhit(void) {
    struct termios oldt, newt;
    int ch;
    int oldf;

    // 1. Save current terminal settings
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;

    // 2. Disable canonical mode and echoing
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    // 3. Set stdin to non-blocking mode
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    // 4. Try to read a character without blocking
    ch = getchar();

    // 5. Restore terminal settings and blocking mode
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    // 6. If a key was pressed, put it back in input buffer and return 1
    if (ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }

    // No key pressed, return 0
    return 0;
}
```
- Canonical mode: Terminal buffers input until you press [ENTER]. Disabling it allows reading input character by character immediately.

- Echoing: When its enabled characters you type are shown on the screen. Disabling echoing hides typed characters.

- Non-blocking mode: Makes input functions like ```getchar()``` return immediately even if no input is available (instead of waiting).

- [kbhit source](https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/kbhit?view=msvc-170)

here is what I originally had if your wondering:
```
while (sec <= total_sec) {            
    if (paused) {                    
        printf("Paused. Press 'p' to resume.\n"); 
        int ch = getchar();           // Wait here for the user to press a key (blocking)
        
        if (ch == 'p' || ch == 'P')
            paused = 0;               
    } else {                         
        printf("Running... Press 'p' to pause\n"); 
        
        if (kbhit()) {               
            int ch = getchar();      
            
            if (ch == 'p' || ch == 'P')
                paused = 1;          
        }
        
        sleep(1);                   
        
        sec++;                      
    }
}
```
**disadvantages of this method**
1. Blocking input during pause
- When paused, getchar() waits blocking for user input, freezing the entire program.

2. Repeated terminal mode changes inside kbhit()
- The kbhit() function modifies terminal modes every time it’s called (disabling canonical mode, echo, and non-blocking mode, then restoring them).
- This constant toggling adds unnecessary overhead and **caused terminal flickering**.

There might be easier ways to do this than what ChatGPT suggested, but this function is very important because it controls input timing and user experience.



## Future Improvements 
I dont actually plan on working on this anymore I have already learned a lot and I am kind of sick of this particular project because im just using it as a stepping stone to one day make a game in C and I also dont really plan on using this program. I just wanted to include this section to show improvements others could could make if they also wanted to make this project.

### 1. Text file output

Currently its pretty ugly


``` Example
Focus topic: balls
Total Focus Time: 0h 2m 0s
Total Break Time: 0h 2m 0s

Session 1: 0h 1m 0s
  Notes:  bals

Session 2: 0h 1m 0s
  Notes:  balls


```

The original python version was cleaner displayed more data

```Example
********************

TOTAL Stats:

You focused for 0h, 0m, 35s over 1 focus session.
And you spent 0h, 0m, 0s over 0 breaks.

********************

Session-by-session Stats:

Your main focus was on BALLS.

Focus Session #1:
You focused for 0h, 0m, 35s during this session.
You wrote down "BALLS".
```

This could be improved using unicode characters. Maybe you could show the stats in a chart? 

This is a really easy fix it would just take a little bit of time.

### 2. Quit at anytime

 Quitting at anytime would be very convenient. I was thinking you could save your information and you could come back to it using a json file.

### 3. ncurses Library
[wiki](https://en.wikipedia.org/wiki/Ncurses)

the ncurses library is a great library I came acress. It can make UI improvements like drawing boxes but also would have made some of my problems go away regarding the progress bar (which took a while to make) and the screen flickering.

[Loading bar example](https://community.unix.com/t/ncurses-status-bar/376028/2) I havent tested any of this 

### conclusions

this program is far from perfect but I still think its a pretty good starting point. One day in the future I would like to work on a game in C with no game engine (not anything as good as jdh lol). This was definently a good simple project to start with so thanks @ThatCosmicStorm for the idea.   
