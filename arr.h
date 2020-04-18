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

#ifndef UTIL_H_
#define UTIL_H_

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

// arr_free
// arr_size
// arr_push
// arr_pop
// arr_clear
// arr_resize
// arr_reserve
// usage:
//   int *test = NULL;
//   arr_push(test, 0);
//   arr_push(test, 1);
//   for(int i = 0; i < arr_size(test); ++i)
//   {
//     printf("%d ", test[i];
//   }
//   arr_free(test); // test now set to NULL.

#define ARR_MAGIC_ 0xFEFE0403
typedef struct
{
    uint64_t magic; // magic is used to make sure we are not making a mistake
    int e, r, s; // element_size, reserve, size
} arr_struct_;
#define default_arr_size 10

#define arr_meta_(a) (((arr_struct_ *)(a)) - 1)

#define assert_magic(a) (assert(a == NULL || arr_meta_(a)->magic == ARR_MAGIC_))

void *arr_init_(void **a, int elem_size);

void *arr_realloc_(void **a, int ns);

#define arr_init(a) ((!(a)) ? (arr_init_((void **)(&(a)), sizeof((a)[0]))) : (a))

// This is quite dumb. We malloc and then realloc
// if we reserve a NULL array. Oh welp. We can
// fix that later.
#define arr_reserve(a, ns) do {\
    assert_magic(a);\
    if (!(a))\
        arr_init_((void **)(&(a)), sizeof((a)[0]));\
    if ((arr_meta_(a))->r < (ns))\
        arr_realloc_((void **)(&(a)), ((ns) * 2 + default_arr_size));\
    assert((arr_meta_(a))->r >= (ns));\
    assert_magic(a);\
} while(0)

#define arr_resize(a, ns) do {\
    assert_magic(a);\
    arr_reserve((a), ns);\
    arr_meta_(a)->s = ns;\
    assert_magic(a);\
} while(0)

#define arr_size(a) ((a) ? arr_meta_(a)->s : 0)

#define arr_push(a, v) do { \
    assert_magic((a));\
    arr_resize((a), (arr_size(a) + 1));\
    (a)[((arr_size(a)) - 1)] = (v);\
    assert_magic(a);\
} while(0)

#define arr_insert(a, v, i) do { \
    assert((i) >= 0);\
    assert_magic(a);\
    if (!a)\
    {\
        arr_push((a), v);\
    }\
    else \
    {\
        int size = arr_size(a);\
        assert(i >= 0 && i < size + 1);\
        arr_resize((a), size + 1);\
        int elem_size = arr_meta_(a)->e;\
        memmove(((char *)(a)) + ((i) + 1) * elem_size, \
                ((char *)(a)) + (i) * elem_size, \
                (size - (i)) * elem_size); \
        (a)[i] = (v);\
    }\
    assert_magic(a);\
} while(0)

#define arr_remove(a, i) do { \
        assert(i >= 0);\
        assert_magic(a);\
        void *p = a;\
        if (p)\
        {\
            int size = arr_size(a) - 1;\
            assert((i) >= 0 && (i) < size + 1);\
            int elem_size = arr_meta_(a)->e;\
            memmove((char *)p + (i) * elem_size, \
                    (char *)p + ((i) + 1) * elem_size, \
                    (size - (i)) * elem_size); \
            arr_resize((a), size);\
        }\
        assert_magic(a);\
    } while(0)


#define arr_pop(a) ((arr_meta_(a)->s = arr_size(a) - 1))

#define arr_first(a) ((a)[0])
#define arr_last(a) ((a)[arr_size(a) - 1])

#define arr_free(a) ((a) ? ((free(arr_meta_(a))), ((a) = NULL)) : NULL)
#define arr_clear(a) arr_resize((a), 0)

#define arr_begin(a) (a)
#define arr_end(a) ((a) + arr_size(a))

// This works as .NETs Array.BinarySearch
// Let i be the return value.
// if i < 0
//   the the value was not found.
//   Inserting the value into ~i of src
//   will result in src being sorted provided
//   it was sorted before. Note that it may be true
//   that (i == length of src).
// otherwise i is the index in src of the value.
//
// The parameters are identical to those to bsearch
static inline int arr_binarysearch(void *key, void *base,
        size_t num, size_t size,
        int (*compar) (const void* a, const void *b))
{
    int l = 0, r = num - 1;
    while (l <= r)
    {
        int m = (l + r) / 2;
        int cmp = compar((char *)base + (m * size), key);
        if (cmp < 0)
        {
            l = m + 1;
        }
        else if (cmp > 0)
        {
            r = m - 1;
        }
        else
        {
            return m;
        }
    }
    return ~l;
}

void arr_test();

#endif
