/*
 * MIT License
 *
 * Copyright (c) 2020 Sebastian Zander
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef HASH_TABLE_H_
#define HASH_TABLE_H_

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <limits.h>

/* This function signature (assign_table_elem_t) is the
 * heart of the hash table implementation.
 * addr is a pointer to the elements index into the table.
 * This is used to assign the new element. The old value
 * is passed in also for destruction. There are three cases:
 *
 * 1. Neither new_elem or old_elem is NULL.
 *   - This happens for three reasons:
 *     1. When there was an old value for the
 *       same key that was attempted to be inserted.
 *       new_elem is exactly the address passed into
 *       the add function.
 *     2. When get is called, new_elem points to
 *       an element in the table, while addr is
 *       the pointer passed in. While this
 *       reverts the meaning of the names,
 *       the implementation would be the same for
 *       both cases. old_elem will have the same
 *       address as new_elem in this case. Note
 *       that in the case that new_elem == old_elem,
 *       the function should NOT copy and allocate
 *       a new object.
 *     3. When rehashing the old keys and old values
 *       are moved to different indices. This can be
 *       detected by comparing the values of old and
 *       new. It is unessecary to reallocate the value
 *       in this case, but it IS necessary to assign
 *       the addr.
 *
 *   - The user may assign a newly
 *     allocated copy of the element, if so desired.
 *   - In this case, the user has the opportunity to
 *     free the address of the old element.
 * 2. new_elem is NULL, old_elem is not.
 *   - This happens when a value was removed or the
 *     entire table was freed.
 *   - In this case, the user has the opportunity to
 *     free the address of the old element.
 *   - Note that while the addr is still valid,
 *     new_elem obviously is not.
 * 3. old_elem is NULL, new_elem is not
 *   - This happens when a value was inserted for
 *     a new key.
 *   - The user may assign a newly
 *     allocated copy of the element, if so desired.
 *
 *  Note that both keys and values may be moved around
 *  in the table as the table resizes. This is then done
 *  with a call to memmove.
 *
 *  addr is the address to the table index where the
 *  new element should be located.
 *
 *  user_data is the pointer passed in on initiation.
 *  This may, for example, contain an arena-address.
 *
 *  Example:
 *
 *  Say the following table was created:
 *
 *  basic_hash_table_t stol;
 *  basic_hash_table_init(&stol, sizeof(short), sizeof(long),
 *      &hash_short, &compare_long,
 *      &assign_short, &assign_long, NULL);
 *
 *  The assign function might work like this:
 *
 *  void assign_long(void *addr, void *new_elem,
 *      void *old_elem, void *user_data)
 *  {
 *      if (new_elem)
 *          *((long *)addr) = *((long *)new_elem);
 *  }
 *
 *  Note that the old_elem is pointing to the value
 *  that was assigned. Simply freeing old_elem is an
 *  error since that is a pointer to an address in
 *  the table. To free, the pointer needs to be cast
 *  and dereferenced.
 */
typedef void (assign_table_elem_t) (
            void *addr,
            void *new_elem,
            void *old_elem,
            void *user_data);

// works like compar in qsort etc, just to make
// us be able to reuse the same function.
// This means we return 0 if the elements are equal.
typedef int (elem_compar_t) (const void *a, const void* b);

typedef size_t hash_t;
typedef hash_t (hash_function_t) (const void* key);

typedef enum elem_flags
{
    HASH_ELEM_EMPTY = 0,
    HASH_ELEM_USED = 1,
    HASH_ELEM_DELETED = 2
} elem_flags_t;

void hash_table_test();

typedef struct basic_hash_table basic_hash_table_t;

basic_hash_table_t *basic_hash_table_new(
        int key_size, int value_size,
        hash_function_t hash,
        elem_compar_t compar,
        assign_table_elem_t assign_key,
        assign_table_elem_t assign_value,
        void *user_data);

void basic_hash_table_delete(basic_hash_table_t *table);

// Return 1 if a new key was added, 0 if an old value existed for
// that key and was replaced.
int basic_hash_table_set(basic_hash_table_t *table, void *key, void *value);

// Returns 1 if there was a key inserted, otherwise 0. If 1 is returned
// out is populated with the value (assign_value is called), otherwise
// out is not changed in any way (useful if a default is wanted).
int basic_hash_table_get(basic_hash_table_t *table, void *key, void *out);

// Essentially the same as get, but the key/value pair is also removed from the
// table. out may be NULL, in which case assign_value is not called.
int basic_hash_table_remove(basic_hash_table_t *table, void *key, void *out);

typedef int basic_hash_table_iterator_t;

basic_hash_table_iterator_t basic_hash_table_begin(basic_hash_table_t *table);
basic_hash_table_iterator_t basic_hash_table_end(basic_hash_table_t *table);
basic_hash_table_iterator_t basic_hash_table_next(basic_hash_table_t *table,
        basic_hash_table_iterator_t iter,
        void *key, void *value);

// Robert Jenkins 32 bit hash function.
static inline hash_t jenkins_hash(hash_t a)
{
   a = (a+0x7ed55d16) + (a<<12);
   a = (a^0xc761c23c) ^ (a>>19);
   a = (a+0x165667b1) + (a<<5);
   a = (a+0xd3a2646c) ^ (a<<9);
   a = (a+0xfd7046c5) + (a<<3);
   a = (a^0xb55a4f09) ^ (a>>16);
   return a;
}

// Assume 32 bit or 64 bit hash_t.
#define FNV_PRIME sizeof(hash_t) == 8 ? 1099511628211UL : 16777619U
#define FNV_OFFSET sizeof(hash_t) == 8 ? 14695981039346656037UL : 2166136261U

static inline hash_t fnv_hash(const unsigned char* begin, const unsigned char *end)
{
    hash_t prime = FNV_PRIME;
    hash_t hash = FNV_OFFSET;
    while(begin != end)
    {
        hash = hash ^ *begin;
        hash = hash * prime;
        ++begin;
    }
    return hash;
}

static inline hash_t fnv_hash_bytes(const unsigned char *bytes, int byte_count)
{
    return fnv_hash(bytes, bytes + byte_count);
}

#define fnv_hash_value(val) fnv_hash((const unsigned char *)&val, ((const unsigned char *)&val + sizeof(val)))

static inline hash_t fnv_hash_u32(uint32_t val)
{
    return fnv_hash_value(val);
}

static inline hash_t fnv_hash_32(int32_t val) {
    return fnv_hash_value(val);
}

static inline hash_t fnv_hash_64(int64_t val)
{
    return fnv_hash_value(val);
}

static inline hash_t fnv_hash_u64(uint64_t val)
{
    return fnv_hash_value(val);
}

static inline hash_t fnv_hash_string(const char *str)
{
    hash_t prime = FNV_PRIME;
    hash_t hash = FNV_OFFSET;
    while(*str)
    {
        hash = hash ^ (unsigned char)(*str);
        hash = hash * prime;
        ++str;
    }
    return hash;
}

// Used to cascade hashes. This can be used to simply
// hash several values that are not easily combined into
// a byte array. For example:
//
//    hash_t hash = fnv_hash_combine(fnv_hash_string("foo"), fnv_hash_string("bar"));
static inline hash_t fnv_hash_combine(hash_t h0, hash_t h1)
{
    hash_t prime = FNV_PRIME;
    hash_t hash = h0;
    unsigned char *begin = (unsigned char *)(&h1);
    unsigned char *end = (unsigned char *)((&h1) + sizeof(hash_t));

    while(begin != end)
    {
        hash = hash ^ *begin;
        hash = hash * prime;
        ++begin;
    }
    return hash;
}

// Assigning strings is common and quite difficult to get right. It also serves
// as an example of how to assign elements that are owned by the hash table.
static void assign_string(void *addr,
            void *new_elem,
            void *old_elem,
            void *user_data)
{
    if (old_elem == new_elem)
    {
        *((char **)addr) = *((char **)new_elem);
        return;
    }

    if (old_elem)
        free(*(char **)old_elem);

    if (new_elem)
    {
        char *new_str = *(char **)(new_elem);
        *((char **)addr) = strdup(new_str);
    }
}

#endif
