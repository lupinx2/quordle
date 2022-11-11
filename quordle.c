/*
* A terminal based clone of the game Wordle by Josh Wardle.
* -lupinx2
*
* Missing features:
* Hard mode
* Statistics tracker [?]
* Share score with emojis
* linux support (file system)
* move the loading of the wordlist into a separate function
* move the input  into a global variable (carefully).
* qcheat.h - solves the current word.
*/

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

/* Console colors */
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

/* Constants */
#define WORDLIST_FILE "wordlist.txt"
#define STATS_FILE "stats.txt"
// length of words in the wordlist file
#define WORD_LENGTH 5
#define STR2(x) #x
#define STR(x) STR2(x)
// attempts remaining at the start of the game
#define ATTEMPT_MAX 6


/* gamestate variables */

// stores all the words in WORDLIST_FILE
// dynamically sized to [(length of WORDLIST_FILE) / (WORD_LENGTH+1)] [WORD_LENGTH]
char** wordList;

// pointer for the current word from wordList
int y_pointer;

// user's previous guesses and attempt status for each character in them.
// stores <ATTEMPT_MAX> guesses of <WORD_LENGTH> characters each.
// and attempt status for each character (see keysTracker).
char guesses[2][ATTEMPT_MAX][WORD_LENGTH];

// 2D array to keep track of letters attempt status.
// (0 = default / 1 = correct / 2 = wrong place / 3 = wrong letter)
char keysTracker[2][26];

// attempts remaining.
int attemptCount;

// random number set at StartGame()
int randInt;

// True when the game is in hard mode.
bool hardMode = false;

// used to convert char values to keysTracker index.
int asciiOffset = 97;

float statsTracker[10];



/* Function declarations */

// prints all 6 chars in the given row of wordList.
void PrintRow(int row);

// prints character in the given color.
// (0 = default, 1 = green, 2 = yellow, 3 = red, 4 = cyan, 5 = magenta)
void ColorPrint(char character, int color);

// clear the screen when called, then prints the alphabet as 3 rows.
// color of each char determined by attempt status (see keysTracker).
// also prints the number of attempts remaining, then calls PrintGuesses().
void PrintKeyboard();

// print the user's previous guesses with char color determined by attempt status (see keysTracker).
void PrintGuesses();

// gets the user's answer and copies it to the memory location *inputArray points to.
// calls ValidateAnswer first to check if the answer is valid.
// if the answer is 'dbug', 'hard', or 'skip', it interprets these as commands;
// to print debug info, toggle hard mode, or restart the game, respectively.
void GetUserAnswer(char *inputarray, long dictionarySize);

// returns 1 if answer matches a word in the wordList, 0 otherwise.
int ValidateAnswer(char answer[], long wordListLength);

// returns 1 if the answer is correct, 0 if not.
// also marks keys as used, records guesses, and substracts 1 from attemptCount.
int CheckAnswer(char answer[]);

// Loads stats.txt into the global variables, returns 0 if successful, 1 if not.
int LoadStats();

// Saves stats.txt from the global variables, adding the resuts of the current game.
// pass 1 as the argument when player wins a game, -1 when player loses.
// pass 0 to simply save the current stats without changing them.
void SaveStats(int gameResult);

// prints the User's stats.
void PrintStats();

// Looads stats.txt, picks a new word, resets attemptCount, and resets keysTracker.
void StartGame();

// prints KeyTracker, Guesses, attemptCount, y_pointer and the current word.
// also prints wordList up to the <wordListLength>th word.
void DebugPrint(long wordListLength);

/* ---main function--- */
int main(void)
{     
   char scanOut;
   FILE *dictionaryFile = fopen(WORDLIST_FILE, "r");
   if (dictionaryFile == NULL)
   {
      printf("Error opening file!\n");
      exit(1);
   }
   // find size of the wordlist .txt file, and allocate memory for wordList.
   fseek(dictionaryFile, 0L, SEEK_END);
   long dictionarySize = ftell(dictionaryFile)/6;
   fseek(dictionaryFile, 0L, SEEK_SET);
   wordList = malloc(dictionarySize * sizeof(char *));

   for(int i = 0; i < dictionarySize; i++)
   {
      wordList[i] = malloc(WORD_LENGTH * sizeof(char));
   }

   // read into wordlist from the .txt file
   for (int i_y = 0; i_y < dictionarySize; i_y++)
   {
      for (int i_x = 0; i_x < WORD_LENGTH; i_x++)
      {
         fscanf(dictionaryFile, " %c", &scanOut);
         wordList[i_y][i_x] = scanOut;
      }
   }
   fclose(dictionaryFile);

   // load stats from stats.txt
   LoadStats();

   //if (LoadStats() == 1);{
   //   printf("Error opening stats file!\n");
   //   exit(1);
   //}

   // start the game.
   bool keepPlayin = true;
   char inputYN[1];
   char input[WORD_LENGTH];
   input[WORD_LENGTH] = '\0'; // null terminate the string.
   while (keepPlayin)
   {
      StartGame();
      while (attemptCount > 0)
      {
         PrintKeyboard();
         GetUserAnswer(input, dictionarySize);
         // check the user's answer
         if (CheckAnswer(input) == 0)
         {
            // player won.            
            SaveStats(1);
            LoadStats();
            PrintKeyboard();
            printf("Correct!\n");
            printf("Clear screen and play again? (y/n)\n");
            scanf(" %1c", inputYN);
            if (inputYN[0] == 'n' || inputYN[0] == 'N')
            {
               keepPlayin = false;
               attemptCount = 0;
            }
            else
            {
               attemptCount = 0;
            }
         }
         else if (attemptCount == -1)
         {
            // player entered 'skip' command.
            break;
         }
         else if (attemptCount == 0)
         {
            // player lost.
            SaveStats(-1);
            LoadStats();
            PrintKeyboard();
            printf("The answer was: ");
            PrintRow(y_pointer);
            printf("\nOut of attempts, play again? (y/n)\n");
            scanf(" %1c", inputYN);
            if (inputYN[0] == 'n' || inputYN[0] == 'N')
            {
               keepPlayin = false;
            }
         }
      }
   }
   printf(ANSI_COLOR_RESET "Thank you for playing!\n");
   return 0;
}

/* Function definitions */

void PrintRow(int row)
{
   int i;
   for (i = 0; i < WORD_LENGTH; i++)
   {
      printf("%c", wordList[row][i]);
   }
}

void ColorPrint(char character, int color)
{
   switch (color)
   {
   case 0:
      printf("%c", character);
      break;
   case 1:
      printf(ANSI_COLOR_GREEN "%c" ANSI_COLOR_RESET, character);
      break;
   case 2:
      printf(ANSI_COLOR_YELLOW "%c" ANSI_COLOR_RESET, character);
      break;
   case 3:
      printf(ANSI_COLOR_RED "%c" ANSI_COLOR_RESET, character);
      break;
   case 4:
      printf(ANSI_COLOR_CYAN "%c" ANSI_COLOR_RESET, character);
      break;
   case 5:
      printf(ANSI_COLOR_MAGENTA "%c" ANSI_COLOR_RESET, character);
      break;
   }
}

void PrintKeyboard()
{
   system("clear");
   /*//CHEAT MODE FOR TESTING
   printf("Cheat: ");
   PrintRow(y_pointer);
   printf("\n");//*/
   char *line = "_________________\n";
   printf(ANSI_COLOR_CYAN "%s" ANSI_COLOR_RESET, line);
   // i keeps track of the next character to print, this only executes once.
   for (int i = 0; i < 26; i++)
   { // each for loop in here prints a row of the keyboard.
      for (int k = 0; k < 9; k++, i++)
      { 
         ColorPrint(keysTracker[0][i], keysTracker[1][i]);
         printf(" ");
      }
      printf("\n");
      for (int k = 0; k < 8; k++, i++)
      { 
         printf(" ");
         ColorPrint(keysTracker[0][i], keysTracker[1][i]);
      }
      printf("\n");
      for (int k = 0; k < 9; k++, i++)
      { 
         ColorPrint(keysTracker[0][i], keysTracker[1][i]);
         printf(" ");
      }
      printf("\n");
      // the following only runs once because by now i == 26
   }
   printf(ANSI_COLOR_CYAN "%s" ANSI_COLOR_RESET, line);
   printf(ANSI_COLOR_CYAN "Attempts: " ANSI_COLOR_RESET);
   printf(ANSI_COLOR_MAGENTA "%i\n" ANSI_COLOR_RESET, attemptCount);
   PrintGuesses();
   printf(ANSI_COLOR_CYAN "%s" ANSI_COLOR_RESET, line);
}

void PrintGuesses()
{
   int i;
   int j;
   // this prints the Guesses array in reverse order,
   // because it is written to using attemptCount as the index. (see CheckAnswer())
   for (int i = WORD_LENGTH; i >= attemptCount; i--)
   {
      for (j = 0; j < WORD_LENGTH; j++)
      {
         ColorPrint(guesses[0][i][j], guesses[1][i][j]);
      }
      printf("\n");
   }
}

void GetUserAnswer(char *inputArray, long dictionarySize){
   /*
   answer is the user's input while inside this function.    
   */
   char answer[WORD_LENGTH];
   answer[WORD_LENGTH] = '\0'; // null terminate the string.
   char *answerPointer = answer;// 
   // check if the input is a word in the wordlist.
   do
   {
      scanf(" %" STR(WORD_LENGTH) "s", answer);
      // print debug info when the user enters 'dbug'.
      if (strncmp(answer, "dbug", 4) == 0)
      {
         DebugPrint(dictionarySize);
      }
      // toggle hard mode when the user enters 'hard'.
      else if (strncmp(answer, "hard", 4) == 0)
      {
         if (attemptCount == ATTEMPT_MAX)
         {
            hardMode = !hardMode;
            printf(ANSI_COLOR_CYAN "Hard mode is now %s.\n", hardMode ? "on" : "off" ANSI_COLOR_RESET);
            printf(ANSI_COLOR_CYAN "_________________\n" ANSI_COLOR_RESET);      
         }
         else
         {
            printf(ANSI_COLOR_CYAN "Hard mode can only be toggled at the start of a game.\n" ANSI_COLOR_RESET);
            printf(ANSI_COLOR_CYAN "Enter 'skip' to start a new game.\n" ANSI_COLOR_RESET);
            printf(ANSI_COLOR_CYAN "_________________\n" ANSI_COLOR_RESET);
         }
      }
      // restart the game when the user enters 'skip'.
      else if (strncmp(answer, "skip", 4) == 0)
      {
         attemptCount = -1;
         break;
      }
      // print user's statses when the user enters 'scat'.
      else if (strncmp(answer, "stat", 4) == 0)
      {
         PrintStats();
      }
      // exit program when the user enters 'exit'.
      else if (strncmp(answer, "exit", 4) == 0)
      {
         exit(0);
      }
      // exit loop when the user enters a valid answer
      else if(ValidateAnswer(answer, dictionarySize) == 1)
      {
         break;         
      }
      else
      {
         printf(ANSI_COLOR_CYAN "Not in word list.\n" ANSI_COLOR_RESET);
         printf(ANSI_COLOR_CYAN "_________________\n" ANSI_COLOR_RESET);
      }
   }while(true);
   strncpy(inputArray, answer, WORD_LENGTH);
}

int ValidateAnswer(char answer[WORD_LENGTH], long wordListLength)
{
   for (int i = 0; i < wordListLength; i++)
   {
      if (strncmp(answer, wordList[i], WORD_LENGTH) == 0)
      {
         return 1;// word is in the wordlist.
      }
   }
   return 0;
}

int CheckAnswer(char answer[])
{  // Answer is the user's input;
   // Word is the word to be guessed.
   char word[2][5] = { "", "" };
   bool letterFound = false;
   int usedKey;
   // the word array's second row keeps track of which letters have already been found.
   // this is to ensure each letter marked green or yellow in the guesses array corresponds, one to one, to a letter in the Word.
   // see the 3Blue1Brown youtube video for full details. -> https://www.youtube.com/watch?v=fRed0Xmc2Wg
   for (int i = 0; i < WORD_LENGTH; i++)
   {
      word[0][i] = wordList[y_pointer][i];
   }

   for (int i = 0; i < WORD_LENGTH; i++)
   {  // note the letter we are checking into guesses array.
      guesses[0][attemptCount - 1][i] = answer[i];
      letterFound = false;      
      if (answer[i] == word[0][i])
      {// letter is correct at position <i>.
         letterFound = true;
         int usedKey = (int)answer[i] - asciiOffset;
         keysTracker[1][usedKey] = 1;
         guesses[1][attemptCount - 1][i] = 1;
         word[1][i] = 1;
      }
      else
      {// check if the letter is in the Word at any other position.
         for (int j = 0; j < WORD_LENGTH; j++)
         {
            if (answer[i] == word[0][j])
            {// letter is correct at some other position <j>.               
               if (word[1][j] == 0)
               {
                  letterFound = true;
                  int usedKey = (int)answer[i] - asciiOffset;
                  keysTracker[1][usedKey] = 2;
                  guesses[1][attemptCount - 1][i] = 2;
                  word[1][j] = 2;
                  break;
               }
            }
         }
         // break will continue here.
         if (!letterFound)
         {// letter is not correct at any position.            
            int usedKey = (int)answer[i] - asciiOffset;
            if (keysTracker[1][usedKey] == 0)
            {// Only mark the letter as red in the keysTracker
             // if it has not already been marked yellow or green.
               keysTracker[1][usedKey] = 3;
            }
            guesses[1][attemptCount - 1][i] = 3;
         }  
      }
   }

   attemptCount--;
   // check if the user has won.
   int correctLetters = 0;
   for (int i = 0; i < WORD_LENGTH; i++)
   {
      if ((word[1][i] == 1))
      {
         correctLetters++;
      }
   }
   if (correctLetters == WORD_LENGTH)
   {
      return 0;//win.
   }
   else
   {
      return 1;//keep playing.
   }
}

void DebugPrint(long wordListLength)
{
   printf("\n");
   printf("---DEBUG PRINT---\n");
   printf("attemptCount: %i\n", attemptCount);
   printf("y_pointer: %i\n", y_pointer);
   printf("hardMode: %i\n", hardMode);
   printf("answer:");
   PrintRow(y_pointer);
   printf("\n_____\n");
   printf("KeysTracker:\n");
   for (int i = 0; i < 26; i++)
   {
      printf("%c :", keysTracker[0][i]);
      printf("%i\n", keysTracker[1][i]);
   }
   printf("_____\n");
   printf("Guesses:\n");
   PrintGuesses();
   printf("_____\n");
   printf("StatsTracker:\n");
   for (int i = 0; i < 10; i++)
   {
      printf("[%i]: %f\n", i, statsTracker[i]);
   }
   printf("_____\n");
   printf("PrintStats:\n");
   PrintStats();
   printf("_____\n");
   printf("WordList (%i):\n", wordListLength);
/*    for (int i = 0; i < wordListLength; i++)
   {
      PrintRow(i);
      printf(", ");
   } */
   printf("\n---END DEBUG PRINT---\n");
   printf("\n");
}

int LoadStats()
{
      /*
      [0] P: 0
      [1] W: 0
      [2] S: 0
      [3] M: 0
      Guess Distribution
      [4] 1: 0
      [5] 2: 0
      [6] 3: 0
      [7] 4: 0
      [8] 5: 0
      [9] 6: 0
      */
      //P = Played, W = Wins, S = Streak, M = Max Streak.
   float scannerOut;
   FILE *statsFile = fopen(STATS_FILE, "r");
   if (statsFile == NULL)
   {
      printf("Error loading stats.txt file!\n");
      return (1);
   }
   // find size of the stats file and allocate memory for it.
   /*fseek(statsFile, 0L, SEEK_END);
   long statsSize = ftell(statsFile)/11;
   fseek(statsFile, 0L, SEEK_SET);
   wordList = malloc(statsSize * sizeof(char *));

   for(int i = 0; i < statsSize; i++)
   {
      statsTracker[i] = malloc(WORD_LENGTH * sizeof(float));
   }*/
   //write each value between the commas to afloat in the statsTracker array.
   for (int i = 0; i < (ATTEMPT_MAX+4); i++)
   {
      fscanf(statsFile, " %f", &scannerOut);
      statsTracker[i] = (float)scannerOut;
   }
   fclose(statsFile);
   return (0);   
}

void SaveStats(int gameResult)
{
 switch (gameResult){
   case 1:
      printf("DEBUG SaveStats(1) running\n");
      // Player wins.
      int attemptsItTook = ATTEMPT_MAX-attemptCount;
      statsTracker[0]++;//played
      statsTracker[1]++;//wins
      statsTracker[2]++;//streak
      if (statsTracker[2]>statsTracker[3])//longest streak
      {
         statsTracker[3] = statsTracker[2];
      }
      statsTracker[3+attemptsItTook]++;
      break;
   case -1:
      printf("DEBUG SaveStats(-1) running\n");
      // Player loses.
      statsTracker[0]++;//played
      statsTracker[2] = 0;//streak
      break;

   default:
      printf("DEBUG Savestats(%i) running\n", gameResult);
   // SaveStats called without a gameResult.
      printf("Debug: SaveStats called without a gameResult.\n");
      break;

   FILE *statsFile = fopen(STATS_FILE, "w");
   if (statsFile == NULL)
   {
      printf("Error saving stats.txt file!\n");
      return;         
   }
   for (int i = 0; i < (ATTEMPT_MAX+4); i++)
   {
      fprintf(statsFile, "%f ", statsTracker[i]);
   }
   fclose(statsFile);   
}
}

void PrintStats()
{
   printf("Played: %i\n", (int)statsTracker[0]);
   printf("Wins: %.*f%%\n", 2, (((float)statsTracker[1]/(float)statsTracker[0])*100));
   printf("Streak: %i\n", (int)statsTracker[2]);
   printf("Max streak: %i\n", (int)statsTracker[3]);
   printf("Guess Distribution:\n");
   for (int i = 4; i < (ATTEMPT_MAX+4); i++)
   {
      printf("%i{ %.*f\n", i-3, 2, (float)statsTracker[i]);
   }
}

void StartGame()
{
   // Load stats
   LoadStats();
   // reset word.
   srand(time(0));
   randInt = rand() % 2314;
   y_pointer = randInt;
   // reset keysTracker.
   int i;
   for (i = 0; i < 26; i++)
   {
      keysTracker[0][i] = (char)i + asciiOffset;
   }
   for (i = 0; i < 26; i++)
   {
      keysTracker[1][i] = 0;
   }
   // reset guesses.
   int jy, jx;
   for (jy = 0; jy < ATTEMPT_MAX; jy++)
   {
      for (jx = 0; jx < WORD_LENGTH; jx++)
      {
         guesses[0][jy][jx] = ' ';
         guesses[1][jy][jx] = 0;
      }
   }
   attemptCount = ATTEMPT_MAX;
}
