#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>


typedef struct{
    int argc;
    char argv[10][50];
}command;

void handlePipe(char *,char *);
void executeCommand(char *,int,int,int,int);

command seperateOptions(char *com){
    int n=strlen(com),l=0;
    command c;
    c.argc=0;
    char temp[1000]="";
    for(int i=0;i<n;i++){
        if(com[i]==' '){
            temp[l++]='\0';
            strcpy(c.argv[c.argc++],temp);
            strcpy(temp,"");
            l=0;
        }
        else{
            temp[l++]=com[i];
        }
    }
    temp[l++]='\0';
    strcpy(c.argv[c.argc++],temp);
    return c;
}

void printPrompt(){
    printf("\033[0;34m");
    char path[1000];
    bzero(path,sizeof(path));
    getcwd(path,sizeof(path));
    printf("%s ",path);
    printf("\033[0m");
}

int isCommandAvailable(char *command){
    char t[1000]="/bin/";
    strcat(t,command);
    int fd=open(t,O_RDONLY);
    if(fd==-1)
    {
        char t1[1000]="/usr/bin/";
        strcat(t1,command);
        fd=open(t1,O_RDONLY);
        if(fd==-1)
            return 0;
        return 2;
    }
    else
        return 1;
}

void executeCommand(char *user_command,int isRedirect,int fdRed,int isPipe,int readEndPipe){
    command c=seperateOptions(user_command);
    int cmd=isCommandAvailable(c.argv[0]);    
    if(!cmd){
        printf("%s: Command not found!!!\n",c.argv[0]);
        return;
    }
    int time,pid=fork();
    if(pid==-1){
        printf("Error in child creation!!!\n");
        return;
    }
    if(pid==0){
        if(isPipe){
            dup2(readEndPipe,0);        
        }
        if(isRedirect){
            dup2(fdRed,1);  
        }
        char fullPath2[]="/usr/bin/",fullPath1[]="/bin/";
        int n=c.argc,j=0;
        char *args[n+1];
        for(int i=0;i<n;i++){
            if(!strcmp(c.argv[i]," ") || !strcmp(c.argv[i],""))
                continue;
            args[j++]=c.argv[i];
        }
        args[j]=NULL;
        if(cmd==2){
            execv(strcat(fullPath2,c.argv[0]),args);
        }
        else{
            execv(strcat(fullPath1,c.argv[0]),args);
        }
    }
    else{
        wait(&time);
    }
}

void handlePipe(char *srcCom,char *destCom){
    int pipefd[2],pid,time,p;
    p=pipe(pipefd);
    if(p==-1){
        printf("Error creating pipeline!!\n");
        return;
    }
    pid=fork();
    if(pid==-1){
        printf("Error in creating child!!\n");
        return;
    }
    if(pid==0){
        close(pipefd[0]);
        executeCommand(srcCom,1,pipefd[1],0,0);
    }
    else{
        wait(&time);
        close(pipefd[1]);
        executeCommand(destCom,0,0,1,pipefd[0]);
    }
}

void main(){
    int curPid,prevPid;
    char user_command[100];
    prevPid=getpid();
    while(1){
        curPid=getpid();
        if(curPid!=prevPid)
            return;
        printPrompt();
        printf("$ ");
        gets(user_command);
        if(strstr(user_command,">")!=NULL || strstr(user_command,"|")!=NULL){
            int i,n=strlen(user_command),error=0;
            char dest[50],arg[50];
            for(i=0;i<n;i++){
                if(user_command[i]=='>' || user_command[i]=='|'){
                    arg[i]='\0';
                    if(i>=(n-1))
                        error=1;
                    int j=0;
                    for(i=i+1;i<n;i++){
                        if(user_command[i]==' ')
                            continue;
                        dest[j++]=user_command[i];
                    }
                    dest[j]='\0';
                    break;
                }
                arg[i]=user_command[i];
            }
            if(error){
                printf("Invalid Command\n");
                continue; 
            }
            if(strstr(user_command,">")!=NULL){
                int fd=open(dest,O_WRONLY|O_CREAT|O_TRUNC,0777);
                if(fd==-1){
                    printf("Error in creation or opening of file : %s\n",dest);
                    continue;
                }
                executeCommand(arg,1,fd,0,0);
            }
            else{
                handlePipe(arg,dest);
            }
            continue;
        }
        executeCommand(user_command,0,0,0,0);
    }
}
