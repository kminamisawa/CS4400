#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>

void select_mode(char mode_flag, char **pString, bool *result, int i);
bool mode_a(char *string);
bool mode_b(char *string);
bool mode_c(char *string);

bool isUpper(char string);
bool isEven(int number);

void swap(char *string, int i, int i1);
void insert(char *string, int i);
void remove_chars(char* str, char c); //remove all char 'c' from str.


int main(int argc, char *argv[]) {
    char mode_flag = 'a';
    bool t_flag = false;
    char **args;
    
    int input_count = 0;
    
    int i;
    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-a") == 0) {
            mode_flag = 'a';
        } else if (strcmp(argv[i], "-b") == 0) {
            mode_flag = 'b';
        } else if (strcmp(argv[i], "-c") == 0) {
            mode_flag = 'c';
        } else if (strcmp(argv[i], "-t") == 0) {
            t_flag = true;
        }
        else {    
            if (args == NULL) {
                args = malloc((argc - i) * sizeof(char *));
            }
            args[input_count] = argv[i];
            input_count++;
        }
    }
   
    bool *result = malloc((argc - i) * sizeof(bool *));

    select_mode(mode_flag, args, result, input_count);
    
    int index;
    for (index = 0; index < input_count; index++) {
        if (t_flag) {
            if (result[index]){
                printf("%s\n", args[index]);
            }
        } else {
            if(result[index]){
                printf("%s\n", "yes" );
            }else{
                printf("%s\n", "no");
            }
        }
    }
    return 0;
}

void select_mode(char mode_flag, char **args, bool *result, int arg_count) {
    int index;
    switch(mode_flag){
        case 'a':
        {
            for (index = 0; index < arg_count; index++) {
                if (mode_a(args[index])) {
                    result[index] = true;
                    swap(args[index], 'G', 'D');   
                }else{
                    result[index] = false;
                }
            }
            break;
        }
        case 'b':
        {
            for (index = 0; index < arg_count; index++) {
                if (mode_b(args[index])) {
                    result[index] = true;
                    insert(args[index], 'H');
                } else {
                    result[index] = false;

                }
            }
            break;
        }
        case 'c':
        {
          for (index = 0; index < arg_count; index++) {
            if (mode_c(args[index])) {
                result[index] = true;
                remove_chars(args[index], 'A');
            } else {
                result[index] = false;
            }
            break;
        }
        default:
        {
            break;
        }
    }
  }
}

bool mode_a(char *word) {
    int b_count = 0;
    int underScore_1_count = 0;
    int y_count = 0;
    int underScore_2_count = 0;
    int upper_count = 0;

    bool b_complete = false;
    bool underScore_1_complete = false;
    bool y_complete = false;
    bool underScore_2_complete = false;
    bool upper_complete = false;

    int index;
    for (index = 0; index < strlen(word); index++){
        char charAtIndex = word[index];
        
        if(charAtIndex == 'b' && !b_complete && !underScore_1_complete){
            b_count++;
            if(5 <= b_count){
                b_complete = true;
            }
        }else if(charAtIndex == '_' && !underScore_1_complete && 2 <= b_count){        
            if(!b_complete){
                b_complete = true; //"b" should not appear anymore
            }
            underScore_1_count++; //MAYBE NOT NEEDED
            underScore_1_complete = true; //sicne there is only 1 appearance.
        }else if(charAtIndex == 'y' && underScore_1_complete && !y_complete && !underScore_2_complete){
            y_count++;
        }else if(charAtIndex == '_' && 0 < y_count && !underScore_2_complete && !upper_complete){
            y_complete = true;
            // if((y_count % 2) == 0){
            if(isEven(y_count)){
                //printf("%s\n", "odd number");
                return false; // Repetitions of 'y' has to be odd number.
            }
            underScore_2_count++;
            underScore_2_complete = true;
        }else if(isUpper(charAtIndex) && underScore_2_complete){
            upper_count++;
        }else{
            //printf("%s \n", "Char does not match with condition");
            return false;
        }
    }

    // if((upper_count%2) == 0){
    if(isEven(upper_count)){
        //printf("%s \n", "Even number of uppercase characters or no uppercase characters");
        return false;
    }else{
        upper_complete = true;
    }

    if(!b_complete || !underScore_1_complete || !y_complete
        || !underScore_2_complete || !upper_complete){
        //printf("%s \n", "One or more condition does not match");
        return false;
    }

    return true;
}

bool mode_b(char *word) {
    int g_count = 0;
    int equal_count = 0;
    int decimal_count = 0;
    int w_count = 0;
    int comma_count = 0;
    int upper_count = 0;
    char odd_position_character = ' ';

    bool g_complete = false;
    bool equal_complete = false;
    bool decimal_complete = false;
    bool w_complete = false;
    bool comma_complete = false;
    bool odd_pos_complete = false;
    bool upper_complete = false;

    int index;
    for (index = 0; index < strlen(word); index++){
        char charAtIndex = word[index];
        
        if(charAtIndex == 'g' && !g_complete && !equal_complete){
            g_count++;
        }else if(charAtIndex == '=' && !equal_complete && decimal_count <= 0){
            if(!g_complete){
                g_complete = true; //"g" should not appear anymore
            }

            equal_count++; //MAYBE NOT NEEDED
            equal_complete = true; //sicne there is only 1 appearance.
        }else if(isdigit(charAtIndex) && equal_complete && !decimal_complete && !w_complete){
            decimal_count++;
            if(3 <= decimal_count){
                decimal_complete = true;
            }

            // if((decimal_count-1) % 2 != 0){  //position
            if(!isEven(decimal_count-1)){ //position
                odd_position_character = charAtIndex; //save the odd positioned character in X
            }
        }else if(charAtIndex == 'w' && 0 < decimal_count && !w_complete && !comma_complete){
            w_count++;
        }else if(charAtIndex == ',' && 0 < w_count && !comma_complete && !odd_pos_complete){
            if(!w_complete){
                w_complete = true;
            }

            comma_count++;
            comma_complete = true; //since there is only one
        }else if(comma_complete && odd_position_character == charAtIndex &&!odd_pos_complete && !upper_complete){
            odd_pos_complete = true;
        }else if(odd_pos_complete && isUpper(charAtIndex)){
            upper_count++;
        }else{
            //printf("%s \n", "Char does not match with condition");
            return false;
        }
    }

    // if((upper_count%2) == 0){
     if(isEven(upper_count)){
        //printf("%s \n", "Even number of uppercase characters or no uppercase characters");
        return false;
    }else{
        upper_complete = true;
    }

    if(!g_complete || !equal_complete || !decimal_complete || !w_complete 
        || !comma_complete || !odd_pos_complete || !upper_complete){
        //printf("%s \n", "One or more condition does not match");
        return false;
    }

    return true;
}

bool mode_c(char *word){
    int g_count = 0;
    int comma_count = 0;
    int upper_count = 0; //seqience X
    int u_count = 0;
    int equal_count = 0;
    int decimal_count = 0;
    int even_pos_char_count = 0;

    bool g_complete = false;
    bool comma_complete = false;
    bool upper_complete = false;
    bool u_complete = false;
    bool equal_complete = false;
    bool decimal_complete = false;
    bool even_pos_char_complete = false;

    int *sequenceX;
    sequenceX = malloc((strlen(word)) * sizeof(char *));
    int sequenceX_index = 0;

    int index;
    for (index = 0; index < strlen(word); index++){
        char charAtIndex = word[index];
        
        if(charAtIndex == 'g' && !g_complete && !comma_complete){
            g_count++;
        }else if(charAtIndex == ',' && !comma_complete && 0 < g_count){
            if(!g_complete){
                g_complete = true; //"g" should not appear anymore
            }
            // if((g_count%2) == 0){
            if(isEven(g_count)){
                return false; // Repitetion of g has to be odd
            }
            comma_count++; //MAYBE NOT NEEDED
            comma_complete = true; //sicne there is only 1 appearance.
        }else if(isUpper(charAtIndex) && comma_complete && !upper_complete && !u_complete){
            sequenceX[upper_count] = word[index];  //for later condition
            upper_count++;
        }else if(charAtIndex == 'u' && 0 < upper_count && !u_complete && !equal_complete){
            // if((upper_count%2)==0){
            if(isEven(upper_count)){
                return false; //even number of upper case letters
            }
            upper_complete = true;
            u_count++;
        }else if(charAtIndex == '=' && 0 < u_count && !equal_complete && !decimal_complete){
            // if((u_count%2)==0){
             if(isEven(u_count)){
                return false; //even number of 'u'
            }
            u_complete = true;

            equal_count++; //maybe not needed
            equal_complete = true; //since there is only one
        }else if(isdigit(charAtIndex) && equal_complete && !decimal_complete && !even_pos_char_complete){
            decimal_count++;
        }else if(charAtIndex == sequenceX[sequenceX_index] && 1 <= decimal_count && decimal_count <= 3 && !even_pos_char_complete){
            decimal_complete = true;
            sequenceX_index += 2; //2 for even position
        }else{
            //printf("%s \n", "Char does not match with condition");
            return false;
        }
    }
    even_pos_char_complete = true;


    if(!g_complete || !comma_complete || !upper_complete || !u_complete 
        || !equal_complete || !decimal_complete || !even_pos_char_complete){
        //printf("%s \n", "One or more condition does not match");
        return false;
    }

    return true;
}

bool isEven(int number){
    int remainder = number & 1;
    if(remainder == 0){
        return true;
    }else{
        return false;
    }
}

void swap(char *word, int c1, int c2) {
    int index;
    for (index = 0; index < strlen(word); index++) {
        char tem = word[index];
        if (word[index] == c1) {
            word[index] = c2;
        } else if (word[index] == c2) {
            word[index] = c1;
        }
    }
    
}

bool isUpper(char letter) {
    if (letter >= 65 && letter <= 90) {
        return true;
    } else {
        return false;
    }
}

void insert(char *word, int i) {
    char *result;
    result = malloc(((strlen(word) << 1) + 1));
    int resultCount = 0;
    int b_count = 0;
    int index;
    for (index = 0; index < strlen(word); index++) {
        char letter = word[index];
        if(letter == 'B'){
            result[resultCount] = 'H';
            resultCount++;
            result[resultCount] = letter;
            b_count++;
        } else{
            result[resultCount] = letter;
        }
        resultCount++;
    }

    result[((strlen(word) + b_count) )] = '\0';
    stpcpy(word, result);
}

void remove_chars(char* str, char c) {
    char *toRead = str, *toWrite = str;
    while (*toRead) {
        *toWrite = *toRead++;
        toWrite += (*toWrite != c);
    }
    *toWrite = '\0';
}
