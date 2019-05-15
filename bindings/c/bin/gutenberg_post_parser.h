/*

Gutengerg Post Parser, the C bindings.

Warning, this file is autogenerated by `cbindgen`.
Do not modify this manually.
Run `just build-c` to rebuild it.

*/

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
    const char *pointer;
    uintptr_t length;
} Slice_c_char;

typedef enum {
    Some,
    None,
} Option_c_char_Tag;

typedef struct {
    Slice_c_char _0;
} Some_Body;

typedef struct {
    Option_c_char_Tag tag;
    union {
        Some_Body some;
    };
} Option_c_char;

typedef enum {
    Block,
    Phrase,
} Node_Tag;

typedef struct {
    Slice_c_char namespace_;
    Slice_c_char name;
    Option_c_char attributes;
    const void *children;
} Block_Body;

typedef struct {
    Slice_c_char _0;
} Phrase_Body;

typedef struct {
    Node_Tag tag;
    union {
        Block_Body block;
        Phrase_Body phrase;
    };
} Node;

typedef struct {
    const Node *buffer;
    uintptr_t length;
} Vector_Node;

typedef enum {
    Ok,
    Err,
} Result_Tag;

typedef struct {
    Vector_Node _0;
} Ok_Body;

typedef struct {
    Result_Tag tag;
    union {
        Ok_Body ok;
    };
} Result;

Result parse(const char *pointer);
