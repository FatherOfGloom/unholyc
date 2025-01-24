#ifndef UNHOLY_C_H_
#define UNHOLY_C_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <errno.h>

// #NAMING CONVENTIONS
// functions		        ->	function_name(x)
// private functions        -> _function_name(x)
// variables			    ->	variable_name
// macros (constants) 	    ->	CONSTANT_NAME
// macros (functions)       -> 	function_name(x)
// private macros (func)    ->  __macro_name(x)
// structs				    -> 	TypeName
// typedef				    -> 	type_name_t
// enums				    -> 	Enum_Name

#define __i64 long long

typedef signed char     i8;
typedef unsigned char   u8;
typedef short           i16;
typedef unsigned short  u16;
typedef int             i32;
typedef unsigned        u32;
typedef __i64           i64;
typedef unsigned __i64  u64;
typedef float           f32;
typedef size_t          usize;

#define __cat(a, b) a ## b
#define __xcat(a, b) __cat(a, b)

#define __cat3(a, b, c) a ## b ## c
#define __xcat3(a, b, c) __cat3(a, b, c)
// gets a unique private name for macro generated variables: __0_p_name
#define __p(x) __xcat3(__, __LINE__, _p_ ## x)

#if defined(__GNUC__) || defined(__GNUG__)
  #define __max_ex(a, b) ({ __typeof__ (a) __p(arg1) = (a); __typeof__ (b) __p(arg2) = (b); __p(arg1) > __p(arg2) ? (a) : (b); })
  #define __min_ex(a, b) ({ __typeof__ (a) __p(arg1) = (a); __typeof__ (b) __p(arg2) = (b); __p(arg1) < __p(arg2) ? (a) : (b); })
// in case a garbonzo compiler is used
#else
  #define __max_ex(a, b) (((a) > (b)) ? (a) : (b))
  #define __min_ex(a, b) (((a) < (b)) ? (a) : (b))
#endif  // defined(__GNUC__) || defined(__GNUG__)

#define max __max_ex
#define min __min_ex

typedef void (*act_t)(void);

#define invoke(callback) do { if (callback != NULL) { callback(); } } while (0)

typedef int err_t;
typedef int errno_t;
typedef FILE file_t;

#define ERR -1
#define OK 0

#define return_defer(val) do { result = (val); goto defer; } while(0)

#define loop for(;;)

#define foreach(T, item, vec) for (struct { usize __get_iter_fe(vec); T item; } __get_struct_fe(vec) = { .__get_iter_fe(vec) = 0, .item = (vec)->len > 0 ? (vec)->items[0] : 0 }; \
    (__get_struct_fe(vec)).__get_iter_fe(vec) < (vec)->len; \
    ++((__get_struct_fe(vec)).__get_iter_fe(vec)), (__get_struct_fe(vec)).item = __get_val_fe(vec))

#define __get_struct_fe(vec) __s_fe_##vec
#define __get_iter_fe(vec) __iter_fe_##vec
#define __get_val_fe(vec) ((vec)->items[__get_struct_fe(vec).__get_iter_fe(vec)])
#define f_val(vec) __get_val_fe(vec)
#define f_idx(vec) __get_iter_fe(vec)

#define enumerate(vec) for (usize __get_iter(vec) = 0; __get_iter(vec) < (vec)->count; ++(__get_iter(vec)))
#define e_val(vec) __get_val(vec)
#define e_idx(vec) __get_iter(vec)
#define __get_iter(vec) __iter_##vec
#define __get_val(vec) ((vec)->items[__get_iter(vec)])

#define free_checked(ptr) do { if ((ptr)) { free((ptr)); } } while(0)
#define nameof(x) #x
#define discard (void)
#define btostr(x) ((x) ? "true" : "false")

// A generic growable array
typedef struct Vec {
    void* items;
    usize len;
    usize capacity;
} Vec;

#define VEC_INIT_CAPACITY 256
#define VEC_GROWTH_MULTIPLIER 2

// Callback with Vec pointer parameter
typedef void (*func_vec_t)(Vec*, ...);

// returns whether a given vec has zero items
#define vec_is_empty(v) ((v)->len == 0)

#define vec_push(v, item_size, val) vec_append(v, item_size, val, 1);

// returns whether a given vec has heap allocations
#define vec_is_null(v) ((v)->items == NULL)


void vec_append(Vec* v, usize item_size, const void* new_items, usize count);
void vec_pop(Vec* v, usize item_size);
void vec_free(Vec* v);
void vec_foreach(Vec* v, func_vec_t callback);
err_t vec_try_realloc(Vec* v, usize item_size, usize new_v_cap);

typedef struct Slice {
    void* data;
    usize len;
    usize size;
} Slice;

// A dynamic string
typedef struct Str {
    // u8 vector
    Vec v;
} Str;

#define STR_FMT "%.*s"

#define str_len(s) ((s).v.len)
#define str_cap(s) ((s).v.capacity)
#define str_raw(s) ((s).v.items)
#define str_null_or_empty(s) ((s).v.len == 0 || (s).v.items == NULL)

// Enlarges the buffer in the inner Vec without affecting the string's len
// only does the reallocation if the new size is bigger than the given string's capacity
#define str_try_realloc(s, new_cap) vec_try_realloc(&((s)->v), sizeof(u8), (new_cap)) 

// Free heap allocations
#define str_free(s) vec_free(&((s)->v))

// Print a string to stdout
#define str_print(s) printf(STR_FMT, str_len(*s), str_to_cstr(s)) 

// returns whether a given string has zero characters
#define str_is_empty(s) vec_is_empty(&((s)->v))

// returns whether a string has no heap allocations
#define str_is_null(s) vec_is_null(&s->v)

usize str_append(Str* s, const char* cstr);
void str_push(Str* s, char c);
Str str_from(const char* const cstr);
char* str_to_cstr(Str* s);
bool str_is_whitespace(Str* s);
u8 str_at(Str* s, i32 i);
void str_set_at(Str* s, i32 i, u8 val);
char* str_reverse(Str* s);
Str str_clone(Str* s);
void str_append_move(Str* dst, Str* src);
void str_append_cpy(Str* dst, Str* src);
Slice str_to_slice(Str* s, i32 from, i32 to);
errno_t get_file_size(file_t* file, usize* size);

// Tagged union for generic results
typedef struct Res {
    err_t kind;
    union {
        void* data;
        Str error;
    };
} Res;

#define unwrap(T, res) ({ assert((res.kind != ERR && str_to_cstr(res.error))); (T)(*((res).data)); })
#define Res(T) Res

typedef struct File {
    file_t* f;
    const char* file_path;
} File;

#define unwrap_file(res) unwrap(File, res)

Res(File) file_open_or_create(const char* file_path);
errno_t file_read(File* f, Str* s);
void file_read_exact(File* f, Slice* s);
usize file_write(File* f, Slice s);
void file_close(File* f);


typedef struct LifetimeChunk {
    struct LifetimeChunk* next;
    usize len_bytes;
    usize cap;
    u64 data[];
} LifetimeChunk;

typedef struct Lifetime {
    LifetimeChunk* begin;
    LifetimeChunk* end;
} Lifetime;

#define LIFETIME_CAPACITY 1024

void* lifetime_share(Lifetime* l, usize size);
void lifetime_drop(Lifetime* l);

#endif // UNHOLY_C_H_