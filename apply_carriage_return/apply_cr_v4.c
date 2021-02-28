#include <stdio.h>

int main(void) {
    char *line = NULL;
    size_t line_cap = 0;
    ssize_t line_len = 0;
    
    while ( 0<=(line_len = getline(&line, &line_cap, stdin)) ) {
        line[line_len] = '\n';
        char *ini = line, *end = ini;
        for (char *now = ini; *now!='\n'; now++)
            if ('\r'!=*now) 
                *ini++ = *now;
            else {
                if (ini>end) end=ini;
                ini = line;
            }
        
        if (ini>end) end=ini;
        if (line[line_len-1]=='\n') *end++ = '\n';
        fwrite(line, 1, end-line, stdout);
    }
    
    return line_len!=-1?0:ferror(stdin);
}
