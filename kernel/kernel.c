// kernel/kernel.c
// Using number printing. Includes improved cmd_ls handling empty case. Readably formatted.

#include "io.h"
#include "kbd.h"
#include "ide.h"
#include "fat32.h"
#include "string.h"
#include <stddef.h>
#include <stdint.h>

// Use version from user upload baseline
#define KERNEL_VERSION "0.2.0-alpha"
#define MAX_CMD_LEN 128

// --- Decls ---
void cmd_version(char *args); void cmd_echo(char *args); void cmd_help(char *args);
void cmd_ls(char *args); void cmd_cd(char *args); void cmd_mkdir(char *args);
void cmd_touch(char *args);
typedef struct { const char *name; void (*func)(char *args); } command_t;
command_t commands[] = { { "version", cmd_version }, { "echo", cmd_echo }, { "help", cmd_help }, { "ls", cmd_ls }, { "cd", cmd_cd }, { "mkdir", cmd_mkdir }, { "touch", cmd_touch }, { NULL, NULL } };

// --- Impls ---
void cmd_version(char *a){(void)a;term_writestring("MyOS v");term_writestring(KERNEL_VERSION);term_putchar('\n');}
void cmd_echo(char *a){if(a)term_writestring(a);term_putchar('\n');}
void cmd_help(char *a){(void)a;term_writestring("Cmds:\n");for(int i=0;commands[i].name;++i){term_writestring("  ");term_writestring(commands[i].name);term_putchar('\n');}}

// LS Callback (using user_data flag)
void ls_callback(Fat32DirectoryEntry *entry, const char* fname, void *ud) {
    if(!fname||!fname[0])return;
    int *flag=(int*)ud; if(flag)*flag=1; // Set flag passed via user_data
    term_writestring("  "); term_writestring(fname);
    if(entry->attributes&ATTR_DIRECTORY)term_writestring("/");
    // Could add size printing here later using term_print_dec(entry->file_size);
    term_putchar('\n');
}
// ls Command (using flag to print (empty))
void cmd_ls(char *args) {
    uint32_t cluster; int found = 0; // Flag to check if callback fired
    if(args&&strlen(args)>0){term_writestring("ls paths N/I\n");cluster=fat32_get_current_directory_cluster();}
    else{cluster=fat32_get_current_directory_cluster();}
    if(cluster<2){term_writestring("ls: Bad CWD:");term_print_dec(cluster);term_writestring("\n");return;} // Use print
    term_writestring("Contents C");term_print_dec(cluster);term_writestring(":\n"); // Use print
    // Pass address of 'found' flag as user_data to the callback
    int r=fat32_read_directory(cluster,ls_callback,&found);
    if(r<0){term_writestring("ls: Read err ");term_print_dec(r);term_writestring("\n");} // Use print
    else if(!found){term_writestring("  (empty)\n");} // Print empty if flag wasn't set
}
// Stubs
void cmd_cd(char*a){(void)a;term_writestring("cd: N/I\n");}
void cmd_mkdir(char*a){(void)a;term_writestring("mkdir: N/I\n");}
void cmd_touch(char*a){(void)a;term_writestring("touch: N/I\n");}

// Readline
void readline(char *b, size_t max){size_t i=0;char c;b[0]='\0';while(i<max-1){c=kbd_getchar();if(c=='\n'){term_putchar('\n');break;}else if(c=='\b'){if(i>0){i--;term_putchar('\b');}}else if(c>=' '&&c<='~'){b[i++]=c;term_putchar(c);}}b[i]='\0';}

// Process Command (with debug checks before loop)
void process_command(char *line){
    char *cmd=line; char *arg=NULL; int f=0;
    term_writestring("\nDBG: Line:'");term_writestring(line);term_writestring("'\n");
    while(*cmd==' ')cmd++; if(*cmd=='\0'){return;} char* s=cmd; while(*s!='\0'&&*s!=' ')s++;
    if(*s==' '){*s='\0';arg=s+1;while(*arg==' ')arg++;if(*arg=='\0')arg=NULL;}
    term_writestring("DBG: Cmd:'");term_writestring(cmd);term_writestring("' Arg:'");term_writestring(arg?arg:"null");term_writestring("'\n");
    // Check commands array right before loop
    term_writestring("DEBUG: Just before loop: commands[0].name = '"); if(commands[0].name)term_writestring(commands[0].name);else term_writestring("(NULL!)"); term_writestring("'\n");
    // Loop through commands
    for(int j=0;commands[j].name!=NULL;j++){
        //term_writestring("DBG: Check ");term_writestring(commands[j].name);term_putchar('\n'); // Keep commented for now
        if(strcmp(cmd,commands[j].name)==0){term_writestring("DBG: Match! Run\n");commands[j].func(arg);f=1;return;} // Using return from user file
    }
    if(!f){term_writestring("ERR: Cmd not found:'");term_writestring(cmd);term_writestring("'\n");}
}

// Kernel Main (No location/time)
void kernel_main(void){
    char buf[MAX_CMD_LEN]; term_init(); term_writestring("Kernel starting...\n");
    ide_initialize(); uint32_t pstart=2048; if(fat32_init(pstart)!=0){term_setcolor(VGA_COLOR_RED,VGA_COLOR_BLACK);term_writestring("PANIC: FAT32 FAIL\n");asm volatile("cli;hlt");}
    kbd_init(); term_setcolor(VGA_COLOR_LIGHT_GREEN,VGA_COLOR_BLACK); term_writestring("\nWelcome MyOS ");term_writestring(KERNEL_VERSION);term_writestring("!\nFAT32 OK. Type 'help'.\n\n");term_setcolor(VGA_COLOR_LIGHT_GREY,VGA_COLOR_BLACK);
    // No strcmp test call here
    while(1){term_writestring("> ");readline(buf,MAX_CMD_LEN);process_command(buf);}
}