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

#include "basic_hash_table.h"

// comparers and assigners
struct basic_hash_table
{
    int key_size, value_size;
    int reserved, load;
    void *keys;
    void *values;
    unsigned char *flags;

    hash_function_t *hash;
    elem_compar_t *compar;
    assign_table_elem_t *assign_key;
    assign_table_elem_t *assign_value;
    void *user_data;
};

static void assign_short(void *addr,
            void *new_elem,
            void *old_elem,
            void *user_data)
{
    if (new_elem)
        *(short *)addr = *((short *)new_elem);
}

static void assign_long(void *addr,
            void *new_elem,
            void *old_elem,
            void *user_data)
{
    if (new_elem)
        *(long *)addr = *((long *)new_elem);
}

static hash_t hash_short(const void *key)
{
    return jenkins_hash(*((short *)key));
}

static int compare_short(const void *a, const void *b)
{
    return *((short *)a) - *((short *)b);
}

static hash_t hash_string(const void *key)
{
    char *str = *((char **)(key));
    unsigned long hash = 5381;
    int c;

    while ((c = *(str++)))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

static int compare_string(const void *a, const void *b)
{
    char *astr = *(char **)a;
    char *bstr = *(char **)b;
    int cmp = strcmp(astr, bstr);
    return cmp;
}

void test_stol()
{
    basic_hash_table_t *stol = basic_hash_table_new(sizeof(short), sizeof(long), 
        &hash_short, &compare_short,
        &assign_short, &assign_long, NULL);

    short keys[] = { 1, 6, 10, 2, 1000, 2342, 4, 5 };
    long values[] = { 10, 60, 100, 20, 10000, 23420, 40, 50 };
    int keys_count = sizeof(keys) / sizeof(keys[0]);
    int values_count = sizeof(values) / sizeof(values[0]);

    for(int i = 0; i < keys_count && i < values_count; ++i)
    {
        basic_hash_table_set(stol, keys + i, values + i);
    }

    for(int i = 0; i < keys_count && i < values_count; ++i)
    {
        long out;
        int has = basic_hash_table_get(stol, keys + i, &out);

        assert(has);
        assert(out == values[i]);
    }
    basic_hash_table_delete(stol);
}

void test_strtol()
{
    basic_hash_table_t *strtol;

    strtol = basic_hash_table_new(sizeof(char *), sizeof(long),
        &hash_string, &compare_string,
        &assign_string, &assign_long, NULL);

    char *keys[] = { "hej", "apa", "foo", "bar",
        "world", "hello", "sailor", "sebe" };
    long values[] = { 10, 60, 100, 20, 10000, 23420, 40, 50 };

    int keys_count = sizeof(keys) / sizeof(keys[0]);
    int values_count = sizeof(values) / sizeof(values[0]);

    for(int i = 0; i < keys_count && i < values_count; ++i)
    {
        basic_hash_table_set(strtol, keys + i, values + i);
    }

    for(int i = 0; i < keys_count && i < values_count; ++i)
    {
        long out;
        int has = basic_hash_table_get(strtol, keys + i, &out);

        assert(has);
        assert(out == values[i]);
        if (i % 2)
        {
            int rem = basic_hash_table_remove(strtol, keys + i, &out);
            assert(rem);
        }
    }

    for(int i = 0; i < keys_count && i < values_count; ++i)
    {
        long out;
        int has = basic_hash_table_get(strtol, keys + i, &out);

        assert((i % 2) || has);
        assert((i % 2) || (has && out == values[i]));
    }

    basic_hash_table_delete(strtol);
}

void hash_table_test()
{
    test_stol();
    test_strtol();
}

int basic_hash_table_set_inner(basic_hash_table_t *table,
        void *key, void *value, int reset);

static void* resize3(char **a, int elemsizea,
        char **b, int elemsizeb,
        char **c, int elemsizec,
        int oldsize, int newsize)
{
    char *np = malloc((elemsizea + elemsizeb + elemsizec) * newsize);
    void *ret = np;
    assert(np);
    *a = np;

    np += elemsizea * newsize;
    *b = np;

    np += elemsizeb * newsize;
    *c = np;

    return ret;
}

static void basic_hash_table_rehash(basic_hash_table_t *table, int oldsize, int newsize)
{
    void *base = table->keys;
    char *old_k = table->keys;
    char *old_v = table->values;
    unsigned char *old_f = table->flags;

    resize3(
        (char **)&table->keys, table->key_size,
        (char **)&table->values, table->value_size,
        (char **)&table->flags, sizeof(table->flags[0]),
        oldsize, newsize);

    memset(table->flags, HASH_ELEM_EMPTY, newsize);
    table->load = 0;
    table->reserved = newsize;

    for(int i = 0; i < oldsize; ++i) {
        if (*old_f & HASH_ELEM_USED)
            basic_hash_table_set_inner(table, old_k, old_v, 1);

        old_k += table->key_size;
        old_v += table->value_size;
        old_f += 1;
    }

    free(base);
}

static int primes[] = { 13, 17, 29, 47, 61, 97, 157, 251, 349 };

static inline int next_prime_size(int old)
{
    if (old < primes[0])
    {
        return primes[0];
    }

    int i = sizeof(primes) / sizeof(primes[0]) - 1;
    if (old >= primes[i])
        return (old * 2) - (old / 2);

    for(i = i - 1; i >= 0; --i)
        if (old >= primes[i])
            return primes[i + 1];

    return (old * 2) - (old / 2);
}

basic_hash_table_t *basic_hash_table_new(
        int key_size, int value_size,
        hash_function_t *hash,
        elem_compar_t *compar,
        assign_table_elem_t *assign_key,
        assign_table_elem_t *assign_value,
        void *user_data)
{
    basic_hash_table_t *table = malloc(sizeof(basic_hash_table_t));

    *table = (basic_hash_table_t){key_size, value_size, 0, 0,
        NULL, NULL, NULL,
        hash, compar, assign_key, assign_value, user_data};
    basic_hash_table_rehash(table, 0, primes[0]);
    return table;
}

void basic_hash_table_delete(basic_hash_table_t *table)
{
    void *base = table->keys;
    char *old_k = table->keys;
    char *old_v = table->values;
    unsigned char *old_f = table->flags;
    void *user_data = table->user_data;

    for(int i = 0; i < table->reserved; ++i)
    {
        if (*old_f == HASH_ELEM_USED)
        {
            table->assign_key(old_k, NULL, old_k, user_data);
            table->assign_value(old_v, NULL, old_v, user_data);
        }

        old_k += table->key_size;
        old_v += table->value_size;
        old_f += 1;
    }

    free(base);
    *table = (basic_hash_table_t){ };
    free(table);
}

int basic_hash_table_set_inner(basic_hash_table_t *table,
        void *key, void *value,
        int reset)
{
    hash_t hash = table->hash(key);
    int reserved = table->reserved;
    if (table->load > reserved / 2)
    {
        assert(!reset);
        basic_hash_table_rehash(table, reserved, next_prime_size(reserved));
    }
    while(1)
    {
        // because reserved might've changed we need to reread it.
        reserved = table->reserved;
        char *keys = table->keys;
        char *values = table->values;
        unsigned char *flags = table->flags;
        for (int i = 0; i < reserved; ++i) {
            hash_t index = (hash + i * i) % reserved;
            if (flags[index] == HASH_ELEM_EMPTY ||
                flags[index] == HASH_ELEM_DELETED) {
                // We never reassign keys, just pass NULL
                // ass old element
                table->assign_key(keys + (index * table->key_size),
                        key,
                        reset ? key : NULL, table->user_data);
                table->assign_value(values + (index * table->value_size),
                        value,
                        reset ? value : NULL, table->user_data);
                flags[index] = HASH_ELEM_USED;
                table->load++;
                return 1;
            }
            else if (table->compar(key, keys + (index * table->key_size)) == 0) {
                flags[index] = HASH_ELEM_USED;
                table->assign_value(values + (index * table->value_size),
                        value,
                        values + (index * table->value_size), table->user_data);
                return 0;
            }
        }

        // While we SHOULD be able to find a slot every time considering
        // we keep the load at < 1/2, I guess we just do this forever anyway
        basic_hash_table_rehash(table, reserved, next_prime_size(reserved));
    }
}

int basic_hash_table_set(basic_hash_table_t *table, void *key, void *value)
{
    return basic_hash_table_set_inner(table, key, value, 0);
}

int basic_hash_table_get_inner(basic_hash_table_t *table, void *key, void *value_out, int *index_out)
{
    int reserved = table->reserved;
    unsigned char *flags = table->flags;
    hash_t hash = table->hash(key);
    for (int i = 0; i < reserved; ++i) {
        hash_t index = (hash + i * i) % reserved;
        if ((flags[index] == HASH_ELEM_USED) &&
                table->compar(key, table->keys + (index * table->key_size)) == 0) {
            if (value_out)
            {
                // We never reassign keys, just pass NULL
                // as old element
                void *tv = table->values + (index * table->value_size);
                table->assign_value(value_out,
                        tv,
                        tv, table->user_data);
            }

            if (index_out)
                *index_out = index;

            return 1;
        }
        else if (flags[index] == HASH_ELEM_EMPTY)
        {
            return 0;
        }
    }
    return 0;
}

int basic_hash_table_get(basic_hash_table_t *table, void *key, void *value)
{
    return basic_hash_table_get_inner(table, key, value, 0);
}


int basic_hash_table_remove(basic_hash_table_t *table, void *key, void *out)
{
    int index;
    int removed = basic_hash_table_get_inner(table, key, out, &index);
    if (removed)
    {
        table->flags[index] = HASH_ELEM_DELETED;
        void* old_k = table->keys + index * table->key_size;
        void* old_v = table->values + index * table->value_size;

        table->assign_key(old_k, NULL, old_k, table->user_data);
        table->assign_value(old_v, NULL, old_v, table->user_data);
    }
    return removed;
}

basic_hash_table_iterator_t basic_hash_table_begin(basic_hash_table_t *table)
{
    return 0;
}

basic_hash_table_iterator_t basic_hash_table_end(basic_hash_table_t *table)
{
    return table->reserved;
}

basic_hash_table_iterator_t basic_hash_table_next(basic_hash_table_t *table,
        basic_hash_table_iterator_t iter,
        void *key, void *value)
{
    while(iter < table->reserved &&
        table->flags[iter] != HASH_ELEM_USED)
    {
        iter++;
    }

    if (iter >= table->reserved)
        return table->reserved;

    void *tk = table->keys + (iter * table->key_size);
    table->assign_key(key,
            tk,
            tk, table->user_data);
    void *tv = table->values + (iter * table->value_size);
    table->assign_value(value,
            tv,
            tv, table->user_data);

    iter++;
    return iter;
}
