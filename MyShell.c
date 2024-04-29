#pragma warning(disable:4996)
/*
内部命令：cat,touch,rm,whoami,ln -s,cp
*/
#define CP 10
#define CAT 11
#define TOUCH 12
#define RM 13
#define WHOAMI 14
#define LN_S 15
#define EXTERNAL 16
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/stat.h>
#include<sys/syscall.h>
#include<sys/types.h>
#include<dirent.h>
#include<fcntl.h>
#include<sys/wait.h>
#define MAX_SIZE 1024
char user_name[100];
char command[MAX_SIZE];
char* main_dir=NULL;
char* path_value = NULL;
char work_dir[MAX_SIZE];
int AnalysisCmd(char input[MAX_SIZE]);
int My_cp(char cmd[MAX_SIZE]);
int My_CreateSoftLink(char cmd[MAX_SIZE]);
int My_cat(char cmd[MAX_SIZE]);
int My_rm(char cmd[MAX_SIZE]);
int My_touch(char cmd[MAX_SIZE]);
int HandleExterCommand(char cmd[MAX_SIZE]);
int My_whoami(char cmd[MAX_SIZE]);
int main() {
    getlogin_r(user_name, sizeof(user_name));
    //初始化环境变量
    main_dir = getenv("HOME");
    path_value = getenv("PATH");
    if (path_value && main_dir) {
        strcpy(work_dir, main_dir);
        int choice;
        while (1) {
            printf("[%s@localhost %s]# ", user_name, work_dir);
            fgets(command, MAX_SIZE, stdin);
            if (command[strlen(command) - 1] == '\n') command[strlen(command) - 1] = '\0';
            if (!strcmp(command, "exit")) break;
            else if (strstr(command, "PATH")) system(command);   //测试时方便显示$PATH的值，并非实现的内部命令
            else {
                choice = AnalysisCmd(command);
                switch (choice) {
                case CAT:My_cat(command); break;
                case TOUCH:My_touch(command); break;
                case RM:My_rm(command); break;
                case WHOAMI:My_whoami(command); break;
                case LN_S:My_CreateSoftLink(command); break;
                case CP:My_cp(command); break;
                case EXTERNAL: {
                    int process_sta=HandleExterCommand(command);
                    if (!process_sta) printf("bash:%s:command not found\n", command);
                    break;
                }
                default:printf("程序出错\n"); break;
                }
            }
        }
    }
    else  printf("环境变量初始化失败\n");
    return 0;
}

int AnalysisCmd(char input[MAX_SIZE]) {  
    if (input[0] == 'c' && input[1] == 'p' && input[2] == ' ') return CP;
    else if (input[0]=='w'&&input[1]=='h'&&input[2]=='o'&&input[3]=='a'&&input[4]=='m'&&input[5]=='i') return WHOAMI;
    else if (input[0] == 'c' && input[1] == 'a' && input[2] == 't' && input[3] == ' ') {
        if (input[4] == '-') {
            if (input[6] == ' ') return CAT;
            else return -1;
        }
        else return CAT;
    }
    else if (input[0] == 't' && input[1] == 'o' && input[2] == 'u' && input[3] == 'c' && input[4] == 'h' && input[5] == ' ') return TOUCH;
    else if (input[0] == 'r' && input[1] == 'm' && input[2] == ' ') return RM;
    else if (input[0] == 'l' && input[1] == 'n' && input[2] == ' ' && input[3] == '-' && input[4] == 's') return LN_S;
    else return EXTERNAL;
}

int My_whoami(char cmd[MAX_SIZE]) {  //whoami或whoami >>other.txt或whoami >other.txt
    if (!strcmp(cmd, "whoami")) printf("%s\n", user_name);
    else {
        char* x1 = strstr(cmd, ">>");
        char* x2 = strstr(cmd, ">");
        if (x1) {
            char dest_name[256];
            strncpy(dest_name, cmd + 9, strlen(cmd) - 9);
            FILE* dest_fp = fopen(dest_name, "a");
            if (dest_fp) {
                fputs(user_name, dest_fp);
                fclose(dest_fp);
            }
            else return 0;
        }
        else {
            char dest_name[256];
            strncpy(dest_name, cmd + 8, strlen(cmd) - 8);
            FILE* dest_fp = fopen(dest_name, "w");
            if (dest_fp) {
                fputs(user_name, dest_fp);
                fclose(dest_fp);
            }
            else return 0;
        }
    }
}

int My_cat(char cmd[MAX_SIZE]) {  //cat file.txt或cat file.txt >>other.txt或cat file.txt >other.txt
    char* x1 = strstr(cmd, ">>");
    char* x2 = strstr(cmd, ">");
    if (x1) {  //追加
        char source_name[256];
        char dest_name[256];
        strncpy(source_name, cmd + 4, x1 - 1- (cmd + 4));
        strncpy(dest_name, x1 + 2, strlen(cmd) - (x1 + 2 - cmd));
        FILE* source_fp = fopen(source_name, "r");
        if (source_fp) {
            FILE* dest_fp = fopen(dest_name, "a");
            if (dest_fp) {
                char content[256];
                while (!feof(source_fp) && (fgets(content, sizeof(content), source_fp) != NULL)) {
                    fputs(content, dest_fp);
                }
                fclose(dest_fp);
                fclose(source_fp);
            }
            else {
                fclose(source_fp);
                return 0;
            }
        }
        else {
            printf("bash:%s:no such file or directory\n", source_name);
            return 0;
        }
    }
    else if (x2) {  //覆盖
        char source_name[256];
        char dest_name[256];
        strncpy(source_name, cmd + 4, x2 - 1 - (cmd + 4));
        strncpy(dest_name, x2 + 1, strlen(cmd) - (x2+1 - cmd));
        FILE* source_fp = fopen(source_name, "r");
        if (source_fp) {
            FILE* dest_fp = fopen(dest_name, "w");
            if (dest_fp) {
                char content[256];
                while (!feof(source_fp) && (fgets(content, sizeof(content), source_fp) != NULL)) {
                    fputs(content, dest_fp);
                }
                fclose(dest_fp);
                fclose(source_fp);
            }
            else {
                fclose(source_fp);
                return 0;
            }
        }
        else {
            printf("bash:%s:no such file or directory\n", source_name);
            return 0;
        }
    }
    else {
        char file_name[512] = "\0";
        int start;
        char flag = '\0';
        if (cmd[4] == '-') {
            start = 7;
            flag = cmd[5];
        }
        else start = 4;
        strncpy(file_name, cmd + start, strlen(cmd) - start);
        FILE* fp = fopen(file_name, "r");
        char content[512] = "\0";
        if (fp) {
            int line_num = 1;
            while (!feof(fp) && (fgets(content, sizeof(content), fp)) != NULL) {
                if (flag == 'n') printf("    %d  ", line_num);
                printf("%s", content);
                line_num += 1;
                strcpy(content, "\0");
            }
            printf("\n");
            fclose(fp);
            return 1;
        }
        else {
            printf("bash:%s:no such file or directory\n", file_name);
            return 0;
        }
    }
}

int My_rm(char cmd[MAX_SIZE]) { //rm file.txt或者rm direct
    char rubbish[256] = "\0";
    strncpy(rubbish, cmd + 3, strlen(cmd) - 3);
    struct stat status;
    stat(rubbish, &status);
    if (S_ISDIR(status.st_mode)) {
        rmdir(rubbish);
        return 1;
    }
    else if (S_ISREG(status.st_mode)) {
        unlink(rubbish);
        return 1;
    }
    else {
        printf("bash:%s:no such file or directory\n", rubbish);
        return 0;
    }
}

int My_cp(char cmd[MAX_SIZE]) {  //cp source.txt dest.txt
    char source_file[256] = "\0";
    char dest_file[256] = "\0";
    int i = 3;
    while (cmd[i] != ' ') { i++; }
    i += 1;
    strncpy(source_file, cmd + 3, i - 4);
    strncpy(dest_file, cmd+ i, strlen(cmd) - i);
    int source_res = syscall(SYS_open, source_file, O_RDONLY);
    if (source_res == -1) {
        printf("bash:%s:no such file or directory\n", source_file);
        return 0;
    }
    int dest_res = syscall(SYS_open, dest_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if ( dest_res== -1) {
        printf("bash:%s:no such file or directory\n", dest_file);
        return 0;
    }
    char buffer[128] = "\0";
    int read_bytes;
    while ((read_bytes = syscall(SYS_read, source_res, buffer, sizeof(buffer))) > 0) {
        syscall(SYS_write, dest_res, buffer, read_bytes);
    }
    syscall(SYS_close, source_res);
    syscall(SYS_close, dest_res);
    return 1;
}

int My_touch(char cmd[MAX_SIZE]) { //touch file.txt
    char file_name[256] = "\0";
    strncpy(file_name, cmd + 6, strlen(cmd) - 6);
    FILE* fp = fopen(file_name, "w");
    if (fp) {
        fclose(fp);
        return 1;
    }
    else return 0;
}

int My_CreateSoftLink(char cmd[MAX_SIZE]) { //ln -s /var/www/html test
    char source_file[512] = "\0";
    char target_file[512] = "\0";
    int source_start = 6, source_end, target_start, target_end;
    for (int i = 6; i < strlen(cmd); i++) {
        if (cmd[i] == ' ') {
            source_end = i;
            break;
        }
    }
    strncpy(source_file, cmd + source_start, source_end - source_start);
    target_start = source_end + 1;
    target_end = strlen(cmd);
    strncpy(target_file, cmd + target_start, target_end - target_start);
    int result = symlink(source_file,target_file);
    return result;
}

int HandleExterCommand(char cmd[MAX_SIZE]) {  //处理外部命令（包括一些千奇百怪的用户输入qaq^_^）   abc:def:gek:lmn
    int pid = fork();
    if (pid == 0) {
        execvp(cmd, NULL);
        return 0;
    }
    else {
        int child_sta;
        waitpid(pid,&child_sta,0);
        if (WIFEXITED(child_sta)) return 1;
        else {
            return 0;
        }
    }
}