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

#include <stdio.h>
#include "arr.h"

void *arr_init_(void **a, int elem_size)
{
    assert(!*a);
    arr_struct_ *s = malloc(default_arr_size * elem_size + sizeof(arr_struct_));
    if (!s) return NULL;

    s->s = 0;
    s->r = default_arr_size;
    s->e = elem_size;
    s->magic = ARR_MAGIC_;
    assert_magic(*a);
    *a = s + 1;
    return a;
}

void *arr_realloc_(void **a, int ns)
{
    assert(*a);
    assert_magic(*a);
    arr_struct_ *s = arr_meta_(*a);

    s = realloc(s, ns * s->e + sizeof(arr_struct_));
    assert(s);
    assert(s->magic == ARR_MAGIC_);

    s->r = ns;
    *a = s + 1;
    return a;
}

int compare_int(const void *a, const void *b) {
    return (*((int *)a) > *((int *)b)) - (*((int *)a) < *((int *)b));
}

void arr_test()
{
    int *test = NULL;

    arr_reserve(test, 1);
    printf("test: %p ->", test);
    arr_reserve(test, 30);
    printf("%p\n", test);
    arr_push(test, 10);
    arr_push(test, 20);
    for(int i = 0; i < arr_size(test); ++i)
    {
        printf("%d\n", test[i]);
    }
    while(arr_size(test) != 0)
    {
        arr_pop(test);
    }
    arr_free(test);
    assert(!test);

    char *str_arr = NULL;
    char *str = "Hello world from test_arr";
    char *begin = str;
    while (*begin != '\0')
    {
        arr_push(str_arr, *begin++);
    }
    for(int i = 0; i < arr_size(str_arr); ++i)
    {
        printf("%c", str_arr[i]);
    }
    printf("\n");
    arr_free(str_arr);

    assert(!str_arr);
    char *str2 = calloc(strlen(str) * 4 + 1, sizeof(char));
    strcat(str2, str);
    strcat(str2, str);
    strcat(str2, str);
    strcat(str2, str);
    begin = str2;
    while (*begin != '\0')
    {
        arr_push(str_arr, *begin++);
    }
    begin = arr_begin(str_arr);
    for(; begin != arr_end(str_arr); ++begin)
    {
        printf("%c", *begin);
    }
    printf("\n");
    free(str2);
    printf("\n");

    int unordered[] = { 22, 43, 5, 2, 8, 3, 2,
        246, 235, 3, 4, 2, 6, 5, 15, 266,
        23, 235, 2, 160, 3, 26, 124, 156, 16, 426, 26,
        26, 261, 8, 890, 789, 33, 26, 26, 798, 15, 89,
        27, 262, 9, 891, 790, 34, 27, 27, 799, 16, 90
    };
    int *ordered = NULL;
    printf("unordered:  ");
    for(int i = 0; i < sizeof(unordered) / sizeof(unordered[0]); ++i)
    {
        printf("%d", unordered[i]);
        if (i + 1 != sizeof(unordered) / sizeof(unordered[0]))
            printf(", ");

        int index = arr_binarysearch(&unordered[i],
                ordered, arr_size(ordered),
                sizeof(int), compare_int);
        if (index < 0) index = ~index;
        arr_insert(ordered, unordered[i], index);
    }
    printf("\n");
    printf("ordered:    ");
    int *ib = arr_begin(ordered);
    for(; ib != arr_end(ordered); ++ib)
    {
        printf("%d", *ib);
        if (ib + 1 != arr_end(ordered))
            printf(", ");
    }
    printf("\n");
    int *distinct = NULL;
    for(int i = 0; i < sizeof(unordered) / sizeof(unordered[0]); ++i)
    {
        int index = arr_binarysearch(&unordered[i],
                ordered, arr_size(ordered),
                sizeof(int), compare_int);

        arr_remove(ordered, index);

        index = arr_binarysearch(&unordered[i],
                ordered, arr_size(ordered),
                sizeof(int), compare_int);

        if (index < 0 || index == arr_size(ordered))
        {
            index = arr_binarysearch(&unordered[i],
                    distinct, arr_size(distinct),
                    sizeof(int), compare_int);
            assert(index < 0);
            arr_insert(distinct, unordered[i], ~index);
        }
    }
    ib = arr_begin(distinct);
    printf("distinct:   ");
    for(; ib != arr_end(distinct); ++ib)
    {
        printf("%d", *ib);
        if (ib + 1 != arr_end(distinct))
            printf(", ");
    }
    printf("\n");
    arr_free(ordered);
    arr_free(str_arr);
}
