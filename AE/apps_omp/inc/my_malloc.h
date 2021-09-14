namespace hclib {

template<typename T>
T* _my_malloc(size_t count) {
    return (T*) malloc(sizeof(T) * count);
}
template<typename T>
T** _my_malloc(size_t row, size_t col) {
    T** var = (T**) malloc(sizeof(T*) * row);
    for(int j=0; j<row; j++) {
        var[j] = (T*) malloc(sizeof(T) * col);
    }
    return var;
}
template<typename T>
void _my_free(T* mem, size_t row) {
    for(int j=0; j<row; j++) {
        free(mem[j]);
    }
    free(mem);
}
template<typename T>
void _my_free(T* mem) {
        free(mem);
}
#define my_malloc _my_malloc
#define my_free _my_free
}
