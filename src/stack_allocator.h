#pragma once

struct stack_allocator {
    int segment_size_bytes;
    int alignment;
    char *segment;
    char *current;

    bool allocate(int size, int alignment_arg, int external_alignment_arg);

    void *push(int size_bytes);

    int get_total_allocated_bytes();

    void reset();


    void free();

    void print() const;
};
