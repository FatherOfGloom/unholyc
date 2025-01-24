#include "unholy.h"

// append multiple items to a Vec
void vec_append(Vec* v, usize item_size, const void* new_items, usize count) {
    if (v->len + count > v->capacity) {
        if (v->capacity == 0) {
            v->capacity = VEC_INIT_CAPACITY;
        }

        while (v->len + count > v->capacity) {
            v->capacity *= VEC_GROWTH_MULTIPLIER;
        }

        v->items = realloc(v->items, v->capacity * item_size);
        assert(v->items != NULL && "unlucky peperony");
        discard memset(v->items + v->len, 0, (v->capacity - v->len) * item_size);
    }

    discard memcpy(v->items + v->len, new_items, count * item_size);
    v->len += count;
}

void vec_pop(Vec* v, usize item_size) {
    assert((v->len - item_size >= 0 || v->capacity == 0) && "cannot pop from empty vec");
    assert((!vec_is_null(v)) && "cannot pop from a null vec");
    discard memset(v->items + v->len - item_size, 0, item_size);   
    v->len = v->len - item_size;
}

// Free heap allocation
void vec_free(Vec* v) {
    if (v->items) {
        free(v->items);
    }

    v->items = NULL;
    v->len = 0;
    // we new to set capacity to 0 so that next append call will realloc the buffer;
    v->capacity = 0;
}

// Perform an operation with w vec via a function pointer
void vec_foreach(Vec* v, func_vec_t callback) {
    assert(callback != NULL && "null reference callback.");

    for (i32 i = 0; i < v->len; ++i) {
        callback(v, i);
    }
}

err_t vec_try_realloc(Vec* v, usize item_size, usize new_v_cap) {
    if (v->capacity >= new_v_cap) {
        return ERR;
    }

    v->capacity = new_v_cap;
    v->items = realloc(v->items, new_v_cap * item_size);
    assert(v->items && "unlucky peperony");
    discard memset(v->items + v->len, 0, (v->capacity - v->len) * item_size);

    return OK;
}

// Append a c string literal to a string 
// returns appended string's len in characters
usize str_append(Str* s, const char* cstr) {
    usize len = strlen(cstr);

    // if we need to explicitly allocate a null character via str_append("");
    if (len == 0) {
        vec_append(&s->v, sizeof(u8), "", 1);
        return len;
    }

    vec_append(&s->v, sizeof(u8), cstr, len); 
    return len;
}

// Append a single character to a string
void str_push(Str* s, char c) {
    vec_append(&s->v, sizeof(u8), &c, 1); 
}

// Allocate a dynamic string from a c string literal
Str str_from(const char* const cstr) {
    Str s = {0};

    if (str_append(&s, cstr) == 0) {
        // if we created a string from "" we want it to be considered empty but not null
        // so we set count to 0 because '\0' as a first byte is insignificant
        s.v.len = 0;
    }

    return s;
}

// Returns a null terminating c string
char* str_to_cstr(Str* s) {
    // The buf is memset to 0 after the allocation, so it automatically ends with '\0'
    // the only edge case is when the len is equal to capacity
    if (str_len(*s) == str_cap(*s)) {
        // we don't want to call str_push(s, '\0') because it will affect the len of the string
        // and if we try to reverse it with str_reverse(s) the null character will be moved to the first position in the given string
        discard str_try_realloc(s, str_cap(*s) * 2);
    }

    return (char*)str_raw(*s);
}

bool str_is_whitespace(Str* s) {
    if (str_is_empty(s)) {
        return false;
    }

    for (i32 i = 0; i < str_len(*s); ++i) {
        if (str_to_cstr(s)[i] != ' ') {
            return false;
        }
    }

    return true;
}

// Get a character from a string by index
u8 str_at(Str* s, i32 i) {
    assert(s->v.len != 0 && "cannot index an empty string");
    assert((i >= 0 && i < s->v.len) && "index out of string bounds"); 
    assert(s->v.items && "the target string is not initialized");

    return ((u8*)s->v.items)[i];
}

void str_set_at(Str* s, i32 i, u8 val) {
    assert(s->v.len != 0 && "cannot index an empty string");
    assert((i >= 0 && i < s->v.len) && "index out of string bounds");
    assert(s->v.items && "the target string is not initialized");

    *(((u8*)s->v.items) + i) = val;
}

// Reverses the given strings bytes
// note: if you append a null character to the end it will also be reversed so the basic
// strlen function will return zero if called on the raw cstring
char* str_reverse(Str* s) {
    i32 len = str_len(*s);

    char* cstr = str_to_cstr(s);

    for (i32 i = 0; i < len / 2; ++i) {
        u8 tmp = str_at(s, i);
        cstr[i] = str_at(s, len - 1 - i);
        cstr[len - 1 - i] = tmp;
    }

    return cstr;
}

// Returns a separately allocated copy (a deep copy) of a given dynamic string
Str str_clone(Str* s) {
    return str_from(str_to_cstr(s));
}

void str_append_move(Str* dst, Str* src) {
    vec_append(&dst->v, sizeof(u8), src->v.items, src->v.len);
    str_free(src);
}

void str_append_cpy(Str* dst, Str* src) {
    vec_append(&dst->v, sizeof(u8), src->v.items, src->v.len);
}

Slice str_to_slice(Str* s, i32 from, i32 to) {
    assert((0 <= from  && from < s->v.len) && "the 'from' index is not in range");
    assert((0 < to  && to <= s->v.len) && "the 'to' index is not in range");
    return (Slice){.data = s->v.items + from, .size = sizeof(u8), .len = to};
}

errno_t get_file_size(file_t* file, usize* size) {
    long saved = ftell(file);
    errno_t result = OK;

    if (saved < 0) {
        return_defer(errno); 
    }

    if (fseek(file, 0, SEEK_END) < 0) {
        return_defer(errno);
    }

    long ftell_result = ftell(file);

    if (ftell_result < 0) {
        return_defer(errno);
    }

    if (fseek(file, saved, SEEK_SET) < 0) {
        return_defer(errno);
    }

    *size = (usize)ftell_result;

defer:
    return result;
} 

errno_t file_read_to_end(const char* file_path, Str* s) {
    errno_t result = OK;
    file_t* f = NULL;

    f = fopen(file_path, "rb");
    
    if (f == NULL) {
        return_defer(errno);
    }

    usize file_size = 0; 
    errno_t err = get_file_size(f, &file_size); 

    if (err != OK) {
        return_defer(err);
    }

    discard str_try_realloc(s, file_size);

    fread(str_raw(*s), file_size, 1, f);

    if (ferror(f)) {
        return_defer(errno);
    }

    s->v.len = file_size;

defer:
    if (f != NULL) {
        fclose(f);
    }

    return result;
}

err_t file_write(const char* file_path, Slice s) {
    err_t result = OK;
    file_t* f = NULL;

    f = fopen(file_path, "wb");

    if (f == NULL) {
        return_defer(ERR);
    }

    fwrite(s.data, s.size, s.len, f);

    if (ferror(f)) {
        return_defer(ERR);
    }

defer:
    if (f) {
        fclose(f);
    }

    return result;
}

Slice new_slice(usize size, void* data, usize len) {
    return (Slice){.size = size, .data = data, .len = len};
}

LifetimeChunk* lifetime_alloc_chunk(Lifetime* l, usize cap_bytes) {
    usize alloc_size = cap_bytes * sizeof(u64) + sizeof(LifetimeChunk);

    LifetimeChunk* result = (LifetimeChunk*)malloc(alloc_size);
    assert(result && "Unlucky peperony");
    result->next = NULL;
    result->cap = cap_bytes;
    result->len_bytes = 0;

    return result;
}

void* lifetime_share(Lifetime* l, usize len_bytes) {
    void* result = NULL;
    usize size = (len_bytes + sizeof(u64) - 1) / sizeof(u64);

    if (l->end == NULL) {
        assert(l->begin == NULL);
        usize cap = max(LIFETIME_CAPACITY, len_bytes);
    }

    return result;
}

void lifetime_drop(Lifetime* l) {
    LifetimeChunk* c = l->begin;

    while (c) {
        LifetimeChunk* tmp = c;
        c = c->next;
        free(tmp);
    }

    l->begin = NULL;
    l->end = NULL;
}