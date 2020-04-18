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

#ifndef INTRUSIVE_LINKED_LIST_H
#define INTRUSIVE_LINKED_LIST_H

#include <stdio.h>
#include <assert.h>
/*
 * An instrusive doubly linked list.
 * The API is as bare metals as can be.
 * We do not need to store a single head or tail node.
 * We are free to specify our own node structures.
 *
 * This is intented to be used primarily as a base layer to
 * implement different types of lists for different structures.
 *
 * A node is defined by the data along with accessors for the
 * prev and next node refs. A function that operates on a
 * node expects the node along with the relevant accessors
 * (see signature for which ones, the name of the parameter will
 * be prev_acc for prev and next_acc for next). The accessor is
 * only called if the node is not null, so the accessor need not
 * do any null-checks itself. We expect all nodes of the same type
 * to have the same accessors.
 *
 * To demonstrate here is an are some examples:
 *
 * * Accessing the prev node of `node`:
 *
 * ```
 * void *prev = node_acc(prev_acc(node));
 * ```
 *
 * * a node struct implementation
 *
 * ```
 * typedef struct node node_t;
 *
 * struct node
 * {
 *   node_t *prev;
 *   node_t *next;
 *   int data;
 * };
 * ```
 *
 * Give then above example, here are the accessor functions:
 *
 * void **access_prev(void* node)
 * {
 *   return (void **)&((node_t *)node)->prev;
 * }
 *
 * void **access_next(void* node)
 * {
 *   return (void **)&((node_t *)node)->next;
 * }
 *
 */

#define ILL_NULL ((void *)0)
typedef void **(* link_accessor)(void *node);

/*
 * pass in the node's with its next accessor. returns next or ILL_NULL (0)
 */
static inline void *ill_next(void *node, link_accessor next_acc)
{
    if (node)
        return *next_acc(node);
    else
        return ILL_NULL;
}

/*
 * pass in the node's with its prev accessor. returns prev or ILL_NULL (0)
 */
static inline void *ill_prev(void *node, link_accessor prev_acc)
{
    if (node)
        return *prev_acc(node);
    else
        return ILL_NULL;
}

/*
 * links a node between two nodes.
 * If either (or both) of next and prev are NULL
 * this will be an append or prepend (or just void)
 * action on the list.
 * return node.
 */
static inline void *ill_link(
        void *node,
        void *prev_node,
        void *next_node,
        link_accessor prev_acc,
        link_accessor next_acc)
{
    assert(node);
    *prev_acc(node) = prev_node;
    *next_acc(node) = next_node;

    if (prev_node)
        *next_acc(prev_node) = node;
    if (next_node)
        *prev_acc(next_node) = node;
    return node;
}

static inline void *ill_link_after(void *node,
        void *prev_node,
        link_accessor prev_acc,
        link_accessor next_acc)
{
    return ill_link(node, prev_node, ill_next(prev_node, next_acc), prev_acc, next_acc);
}

static inline void *ill_link_before(void *node,
        void *next_node,
        link_accessor prev_acc,
        link_accessor next_acc)
{
    return ill_link(node, ill_prev(next_node, prev_acc), next_node, prev_acc, next_acc);
}

/*
 * unlinks a node while linking its next and prev together
 * as necessary. Returns node.
 */
static inline void *ill_unlink(
        void *node,
        link_accessor prev_acc,
        link_accessor next_acc)
{
    assert(node);
    void **prev = prev_acc(node);
    void **next = next_acc(node);
    if (*prev) *(next_acc(*prev)) = *next;
    if (*next) *(prev_acc(*next)) = *prev;

    *prev = ILL_NULL;
    *next = ILL_NULL;

    return node;
}

static inline void *ill_head(void *node, link_accessor prev_acc)
{
    void *head = node;
    while(node && (node = *prev_acc(head))) head = node;
    return head;
}

static inline void *ill_tail(void *node, link_accessor next_acc)
{
    void *tail = node;
    while(node && (node = *next_acc(tail))) tail = node;
    return tail;
}

/*
 * prepends node to the head of the list from `from`. Note that node doesn't have to be the
 * head, the head will be search from node. Returns node.
 */
static inline void *ill_link_head(void *from, void *node, link_accessor prev_acc, link_accessor next_acc)
{
    return ill_link(node, ILL_NULL, ill_head(from, prev_acc), prev_acc, next_acc);
}

/*
 * appends node to the tail of the list from `from`. Note that node doesn't have to be the
 * tail, the tail will be search from node. Returns node.
 */
static inline void *ill_link_tail(void *from, void *node, link_accessor prev_acc, link_accessor next_acc)
{
    return ill_link(node, ill_tail(from, next_acc), ILL_NULL, prev_acc, next_acc);
}

/*
 * unlinks head of the list from node. Note that node doesn't have to be the
 * head, the head will be search from node. Returns node.
 */
static inline void *ill_unlink_head(void *node, link_accessor prev_acc, link_accessor next_acc)
{
    return ill_unlink(ill_head(node, prev_acc), prev_acc, next_acc);
}

/*
 * unlinks tail of the list from node. Note that node doesn't have to be the
 * tail, the tail will be search from node. Returns node.
 */
static inline void *ill_unlink_tail(void *node, link_accessor prev_acc, link_accessor next_acc)
{
    return ill_unlink(ill_tail(node, next_acc), prev_acc, next_acc);
}

typedef struct ill_test_node ill_test_node_t;

struct ill_test_node
{
    ill_test_node_t *prev;
    ill_test_node_t *next;
    int data;
};

static inline ill_test_node_t ill_create_test_node(int data)
{
    ill_test_node_t ret = { };
    ret.data = data;
    return ret;
}

static inline void **ill_test_node_prev(void* node)
{
    return (void **)&(((ill_test_node_t *)node)->prev);
}

static inline void **ill_test_node_next(void* node)
{
    return (void **)&(((ill_test_node_t *)node)->next);
}

static void ill_test()
{
    ill_test_node_t an = ill_create_test_node(0);
    ill_test_node_t bn = ill_create_test_node(1);
    ill_test_node_t cn = ill_create_test_node(2);
    ill_test_node_t dn = ill_create_test_node(3);

    ill_link_tail(ILL_NULL, &an, ill_test_node_prev, ill_test_node_next);
    ill_link_tail(&an, &bn, ill_test_node_prev, ill_test_node_next);
    ill_link_tail(&an, &cn, ill_test_node_prev, ill_test_node_next);
    ill_link_tail(&an, &dn, ill_test_node_prev, ill_test_node_next);

    ill_test_node_t *iter = &an;
    int i = 0;
    while(iter)
    {
        assert(i == iter->data);
        i++;
        iter = ill_next(iter, ill_test_node_next);
    }
    assert(i == 4);

    iter = &dn;
    while(iter)
    {
        i--;
        assert(i == iter->data);
        iter = ill_prev(iter, ill_test_node_prev);
    }
    assert(i == 0);

    ill_unlink_head(&an, ill_test_node_prev, ill_test_node_next);
    iter = ill_head(&dn, ill_test_node_prev);
    i = 1;
    while(iter)
    {
        assert(i == iter->data);
        i++;
        iter = ill_next(iter, ill_test_node_next);
    }
    assert(i == 4);

    iter = ill_tail(&bn, ill_test_node_next);
    while(iter)
    {
        i--;
        assert(i == iter->data);
        iter = ill_prev(iter, ill_test_node_prev);
    }
    assert(i == 1);

    ill_unlink_tail(&bn, ill_test_node_prev, ill_test_node_next);

    iter = ill_head(&cn, ill_test_node_prev);
    i = 1;
    while(iter)
    {
        assert(i == iter->data);
        i++;
        iter = ill_next(iter, ill_test_node_next);
    }
    assert(i == 3);

    iter = ill_tail(&bn, ill_test_node_next);
    while(iter)
    {
        i--;
        assert(i == iter->data);
        iter = ill_prev(iter, ill_test_node_prev);
    }
    assert(i == 1);
}

#endif
