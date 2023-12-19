#include "csapp.h"
#include "cache.h"

int readcnt;
int cache_time = 0;
sem_t mutex,w;
line* cache[10];

//从cache的一个set中找空行
static int get_empty_line() {
    for (int i=0; i<10; i++) {
        if (cache[i]->url == 0) {
            return i;
        }
    }
    return -1;
}

//用LRU策略从cache的一个set中找替换的行
static int get_evict_line() {
    int min_time_line = 0;
    int min_time = cache[min_time_line] -> time;
    for (int i=0; i<10; i++) {
        if (cache[i] -> time < min_time) {
            min_time_line = i;
            min_time = cache[i] -> time;
        }
    }
    return min_time_line;    
}

static void check_cache(int lineno) {
    printf("line: %d, checking..\n",lineno);
    for (int i=0; i<10; i++) {
        printf("%s\n",cache[i]->url);
        printf("%d\n",cache[i]->size);
        printf("%s\n",cache[i]->payload);
    }    
}

void init_cache() {
    for (int i=0; i<10; i++) {
        cache[i] = calloc(1,sizeof(line));
    }
    Sem_init(&mutex, 0, 1);
    Sem_init(&w, 0, 1);
}

/*成功则返回第几个line，失败返回-1*/
int read_cache(char *url, int fd) {
    check_cache(__LINE__);
    
    P(&mutex);
    printf("here\n");
    readcnt++;
    if (readcnt == 1) {
        P(&w);
    }
    V(&mutex);

    int target_line = -1;
    for (int i=0; i<10; i++) {
        if (!strcmp((cache[i]->url),url)) {
            target_line = i;
            Rio_writen(fd, cache[i]->payload, cache[i]->size);
            break;
        }
    }

    P(&mutex);
    readcnt--;
    if (readcnt == 0) {
        V(&w);
    }
    V(&mutex);

    return target_line;   

}

void write_cache(char *object, int size, char *url) {
    P(&w);
    cache_time ++;

    int target_line;
    target_line = get_empty_line();
    if (target_line != -1) {
        memcpy(cache[target_line]->payload, object, size);
        strcpy(cache[target_line]->url, url);
        cache[target_line]->time = cache_time;
        cache[target_line]->size = size;
    }
    else {
        target_line = get_evict_line();
        printf("target_line:%d\n",target_line);
        memcpy(cache[target_line]->payload, object, size);
        strcpy(cache[target_line]->url, url);
        cache[target_line]->time = cache_time;
        cache[target_line]->size = size;
    }
    check_cache(__LINE__);
    V(&w);
}

//释放malloc为cache分配的空间
void free_cache() {
    for (int i = 0; i<10; i++) {
        free(cache[i]);
    }
}
