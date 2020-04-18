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

#ifndef DEQUE_H_
#define DEQUE_H_

#include "intrusive_linked_list.h"
#include <stdlib.h>

// This is a double ended queue with a simpler interface than
// intrusive_linked_list

typedef struct deque_node deque_node_t;

struct deque_node
{
    deque_node_t *prev;
    deque_node_t *next;
    void *data;
};

typedef struct deque
{
    deque_node_t *tail;
    deque_node_t *head;
    int length;
} deque_t;

static inline deque_node_t *deque_create_node(void *data)
{
    deque_node_t *ret = malloc(sizeof(deque_node_t));
    assert(ret);
    ret->data = data;
    ret->next = NULL;
    ret->prev = NULL;
    return ret;
}

static inline void deque_free_node(deque_node_t  *node)
{
    *node = (deque_node_t){ };
    free(node);
}

static inline void deque_init(deque_t *deque)
{
    assert(deque && "Cannot init null deque");
    deque->tail = NULL;
    deque->head = NULL;
    deque->length = 0;
}

static inline void deque_clear(deque_t *deque)
{
    assert(deque && "Cannot clear null deque");
    while (deque->tail)
    {
        deque_node_t *tail = deque->tail;
        deque->tail = deque->tail->prev;
        free(tail);
    }
    deque->length = 0;
}

static inline void **deque_prev_(void *node)
{
    return (void **)(&((deque_node_t *)node)->prev);
}

static inline void **deque_next_(void *node)
{
    return (void **)(&((deque_node_t *)node)->next);
}

static inline void deque_push_back(deque_t *deque, void *data)
{
    assert(deque && "Cannot push null deque");
    deque_node_t *node = deque_create_node(data);
    deque->tail = ill_link(node, deque->tail, NULL, deque_prev_, deque_next_);
    deque->length++;
    if (!deque->head) deque->head = deque->tail;
}

static inline void deque_push_front(deque_t *deque, void *data)
{
    assert(deque && "Cannot push null deque");
    deque_node_t *node = deque_create_node(data);
    deque->head = ill_link(node, NULL, deque->head, deque_prev_, deque_next_);
    deque->length++;
    if (!deque->tail) deque->tail = deque->head;
}

static inline void *deque_pop_back(deque_t *deque)
{
    assert(deque && "Cannot pop null deque");
    assert(deque->tail && "Cannot pop empty deque");
    deque_node_t *prev = deque->tail->prev;

    ill_unlink(deque->tail, deque_prev_, deque_next_);
    void *data = deque->tail->data;
    deque_free_node(deque->tail);
    deque->tail = prev;
    deque->length--;
    if (!prev)
        deque->head = NULL;
    return data;
}

static inline void *deque_pop_front(deque_t *deque)
{
    assert(deque && "Cannot pop null deque");
    assert(deque->head && "Cannot pop empty deque");
    deque_node_t *next = deque->head->next;

    ill_unlink(deque->head, deque_prev_, deque_next_);
    void *data = deque->head->data;
    deque_free_node(deque->head);
    deque->head = next;
    deque->length--;
    if (!next)
        deque->tail = NULL;
    return data;
}

static inline void *deque_peek_back(deque_t *deque)
{
    assert(deque && "Cannot peek null deque");
    assert(deque->tail && "Cannot peek empty deque");
    return deque->tail->data;
}

static inline void *deque_peek_front(deque_t *deque)
{
    assert(deque && "Cannot peek null deque");
    assert(deque->head && "Cannot peek empty deque");
    return deque->head->data;
}

static inline int deque_empty(deque_t *deque)
{
    assert(deque && "Cannot read null deque");
    // if tail is null, then head is also null
    return deque->tail == NULL;
}

static inline int deque_length(deque_t *deque)
{
    return deque->length;
}

typedef struct deque_test_data
{
    int value;
} deque_test_data_t;

static inline void deque_test()
{
    deque_test_data_t one = {1};
    deque_test_data_t two = {2};
    deque_test_data_t three = {3};

    deque_t stack;
    deque_init(&stack);
    deque_push_back(&stack, &one);
    deque_push_back(&stack, &two);
    deque_push_back(&stack, &three);

    assert(!deque_empty(&stack));
    assert(((deque_test_data_t *)deque_pop_back(&stack))->value == 3);
    assert(!deque_empty(&stack));
    assert(((deque_test_data_t *)deque_pop_back(&stack))->value == 2);
    assert(!deque_empty(&stack));
    assert(((deque_test_data_t *)deque_pop_back(&stack))->value == 1);
    assert(deque_empty(&stack));

    deque_clear(&stack);

    deque_t queue;
    deque_init(&queue);
    deque_push_back(&queue, &one);
    deque_push_back(&queue, &two);
    deque_push_back(&queue, &three);

    assert(!deque_empty(&queue));
    assert(((deque_test_data_t *)deque_pop_front(&queue))->value == 1);
    assert(!deque_empty(&queue));
    assert(((deque_test_data_t *)deque_pop_front(&queue))->value == 2);
    assert(!deque_empty(&queue));
    assert(((deque_test_data_t *)deque_pop_front(&queue))->value == 3);
    assert(deque_empty(&queue));

    deque_clear(&queue);
}

#endif
