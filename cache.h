/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

typedef struct {
    int time;
    char url[MAXLINE];
    char payload[MAX_OBJECT_SIZE];
    int size; /*加这个是为了记录payload存了多少,方便给Rio_writen用*/
} line;

extern line* cache[10]; /*10因为cache总大小的限制*/

void init_cache();
int read_cache(char *url, int fd);
void write_cache(char *object, int size, char *url);
void free_cache();