#include "huff.h"

#include <stdlib.h>
#include <string.h>

/* Internal generic data structures */

/* Linked list */
struct node
{
  void *data;
  struct node *next;
  struct node *prev;
};
struct list_
{
  /* Points to a dummy starting node */
  struct node *first;
  /* Points to a dummy ending node */
  struct node *last;
  int size;
};
typedef struct list_ *list;

static int list_init(list *l);
static int list_destroy(list *l);
static int list_size(list *l);
static void *list_data(list *l, int index);
static int list_ins(list *l, void *data, int index);
static void *list_remove(list *l, int index);

static struct node *list_node_(list *l, int index);

static int list_init(list *l)
{
  list realL;
  if (l == NULL)
    goto out;

  realL = calloc(1, sizeof(*realL));
  if (realL == NULL)
    goto out;

  realL->first = calloc(1, sizeof(*(realL->first)));
  if (realL->first == NULL)
    goto out1;

  realL->last = calloc(1, sizeof(*(realL->last)));
  if (realL->last == NULL)
    goto out2;
  
  realL->first->next = realL->last;
  realL->last->prev = realL->first;
  *l = realL;
  return 0;

out2:
  free(realL->first);
  realL->first = NULL;
out1:
  free(realL);
  realL = NULL;
out:
  return -1;
}
static int list_destroy(list *l)
{
  struct node *last;
  struct node *cur;
  if (l == NULL || *l == NULL)
    return -1;
  /* Walk the list, freeing the nodes */
  cur = (*l)->first;
  
  while (cur != NULL) {
    last = cur;
    cur = cur->next;
    free(last);
    last = NULL;
  }

  free(*l);
  *l = NULL;

  return 0;
}
static int list_size(list *l)
{
  if (l == NULL || *l == NULL)
    return -1;

  return (*l)->size;
}
static void *list_data(list *l, int index)
{
  struct node *n;
  if (l == NULL || *l == NULL || index < 0 || index >= (*l)->size)
    return NULL;

  n = list_node_(l, index);
  if (n == NULL)
    return NULL;

  return n->data;
}
static int list_ins(list *l, void *data, int index)
{
  struct node *before;
  struct node *at;
  struct node *after;
  if (l == NULL || *l == NULL || index < 0 || index > (*l)->size)
    return -1;
  
  before = list_node_(l, index-1);
  if (before == NULL)
    return -1;

  after = before->next;
  if (after == NULL)
    return -1;

  at = calloc(1, sizeof(*at));
  if (at == NULL)
    return -1;
  
  at->data = data;
  at->prev = before;
  at->next = after;

  before ->next = at;
  after->prev = at;

  (*l)->size++;
  return 0;
}
static void *list_remove(list *l, int index)
{
  struct node *before;
  struct node *at;
  struct node *after;
  void *ret;
  if (l == NULL || *l == NULL || index < 0 || index >= (*l)->size)
    return NULL;

  at = list_node_(l, index);
  if (at == NULL)
    return NULL;

  before = at->prev;
  after = at->next;
  if (before == NULL || after == NULL)
    return NULL;

  ret = at->data;
  free(at);
  at = NULL;

  before->next = after;
  after->prev = before;

  (*l)->size--;
  return ret;
}
/* Pass -1 to get the dummy first node
   Pass size to get the dummy last node */
static struct node *list_node_(list *l, int index)
{
  int useFirst;
  list realL;
  if (l == NULL || *l == NULL || index < -1 || index > (*l)->size)
    return NULL;

  realL = *l;
  useFirst = (index < realL->size/2);
  if (useFirst) {
    int thisIdx = -1;
    struct node *cur = realL->first;
    while (thisIdx < index && cur != NULL) {
      cur = cur->next;
      thisIdx++;
    }

    return cur;
  } else {
    int thisIdx = realL->size;
    struct node *cur = realL->last;
    while (thisIdx > index && cur != NULL) {
      cur = cur->prev;
      thisIdx--;
    }

    return cur;
  }
}

/* Priority queue wrapper interface for above list */

struct priorityitem
{
  void *data;
  int priority;
};
struct priority_
{
  list l;
};
typedef struct priority_ *priority;

static int priority_init(priority *pq);
static int priority_destroy(priority *pq);
static int priority_add(priority *pq, void *data, int priority);
static void *priority_peek(priority *pq);
static void *priority_dequeue(priority *pq);
static int priority_size(priority *pq);

static int priority_init(priority *pq)
{
  priority realPq;
  int res;
  if (pq == NULL)
    return -1;

  realPq = calloc(1, sizeof(*realPq));
  if (realPq == NULL)
    return -1;

  res = list_init(&realPq->l);
  if (res) {
    free(realPq);
    realPq = NULL;
    return -1;
  }

  *pq = realPq;
  return 0;
}
static int priority_destroy(priority *pq)
{
  priority realPq;
  int res;
  if (pq == NULL || *pq == NULL)
    return -1;

  realPq = *pq;

  /* This frees all the priorityitem structs we calloc'd */
  while (priority_size(pq) > 0)
    priority_dequeue(pq);

  res = list_destroy(&realPq->l);

  free(*pq);
  *pq = NULL;

  return res;
}
static int priority_add(priority *pq, void *data, int priority)
{
  struct priorityitem *thisItem;
  int size;
  int insertIdx;
  list l;
  int res;
  if (pq == NULL || *pq == NULL)
    goto out;

  thisItem = calloc(1, sizeof(*thisItem));
  if (thisItem == NULL)
    goto out;
  
  thisItem->data = data;
  thisItem->priority = priority;

  l = (*pq)->l;

  size = list_size(&l);

  if (size == -1) {
    goto out1;
  }
  else if (size == 0) {
    insertIdx = 0;
  } else {
    int lastPri;
    int i = 0;

    /* This may be unreasonably slow because of the repeated linked list random access
       Who knows though, haven't profiled */
    do {
      struct priorityitem *item = list_data(&l, i);
      if (item == NULL)
        goto out1;

      lastPri = item->priority;
      i++;
    } while (lastPri < priority && i < size);

    insertIdx = i-1;
  }

  res = list_ins(&l, thisItem, insertIdx);
  if (res == -1)
    goto out1;

  return 0;
out1:
  free(thisItem);
  thisItem = NULL;
out:
  return -1;
}
static void *priority_peek(priority *pq)
{
  struct priorityitem *item;
  if (pq == NULL || *pq == NULL)
    return NULL;

  item = list_data(&(*pq)->l, 0);
  if (item == NULL)
    return NULL;

  return item->data;
}
static void *priority_dequeue(priority *pq)
{
  struct priorityitem *item;
  if (pq == NULL || *pq == NULL)
    return NULL;

  item = list_remove(&(*pq)->l, 0);
  if (item == NULL)
    return NULL;

  return item->data;
}
static int priority_size(priority *pq)
{
  if (pq == NULL || *pq == NULL)
    return -1;

  return list_size(&(*pq)->l);
}

/* Main huff code */

/* freqdic */
typedef int ctr;

struct huff_freqdic_
{
  ctr counts[256];
  ctr totalCount;
};

int huff_freqdic_init(huff_freqdic *dic)
{
  huff_freqdic realDic;
  if (dic == NULL)
    return HUFF_BADOBJ;

  realDic = calloc(1, sizeof(*realDic));
  if (realDic == NULL)
    return HUFF_NOMEM;

  /* One EOF will never be accounted for in the other counts */
  realDic->totalCount = 1;

  *dic = realDic;
  return HUFF_SUCCESS;
}

int huff_freqdic_destroy(huff_freqdic *dic)
{
  if (dic == NULL || *dic == NULL)
    return HUFF_BADOBJ;

  free(*dic);
  *dic = NULL;

  return HUFF_SUCCESS;
}

int huff_freqdic_feeddata(huff_freqdic *dic, const uint8_t* data, int length)
{
  huff_freqdic realDic;
  struct huff_freqdic_ workingDic;
  int i;

  if (dic == NULL || *dic == NULL)
    return HUFF_BADOBJ;

  if (length < 0)
    return HUFF_BADLENGTH;

  realDic = *dic;

  /* Copy the freq counts so we can return the originals if they overflow */
  memcpy(&workingDic, realDic, sizeof(workingDic));

  for (i = 0; i < length; i++) {
    ctr before = workingDic.totalCount;
    if (before+1 < before) {
      /* We overflowed */
      return HUFF_TOOMUCHDATA;
    } else {
      workingDic.counts[data[i]]++;
      workingDic.totalCount++;
    }
  }

  memcpy(realDic, &workingDic, sizeof(workingDic));
  return HUFF_SUCCESS;
}

/* Huffman tree */
#define HUFF_EOF 256

struct huff_node
{
  ctr weight;
  int isLeaf;
  union {
    struct {
      struct huff_node *left;
      struct huff_node *right;
    };
    int c;
  };
};
struct huff_tree_
{
  struct huff_node *root;
};
typedef struct huff_tree_ *huff_tree;

static int huff_tree_init(huff_tree *tree, huff_freqdic *dic);
static int huff_tree_destroy(huff_tree* tree);
static int huff_tree_encode(huff_tree *tree, uint8_t in, const uint8_t **outBits, int nBits);
/* Returns -1 if an error occurs,
   0 if no error occurred but no output chars are finished yet,
   1 if no error occurred and an output char was finished - the output char is written to the |out| argument */
static int huff_tree_decode(huff_tree *tree, int bit, uint8_t *out);

static void huff_node_free_(struct huff_node *node);

static int huff_tree_init(huff_tree *tree, huff_freqdic *dic)
{
  huff_freqdic realDic;
  huff_tree realTree;
  priority pq;
  int res;
  int i;

  if (tree == NULL || dic == NULL || *dic == NULL)
    goto out;

  realDic = *dic;
  realTree = calloc(1, sizeof(*realTree));
  if (realTree == NULL)
    goto out;

  res = priority_init(&pq);
  if (res)
    goto out1;

  /* Add all the characters (plus EOF) to the priority queue with their counts as priorities */
  for (i = 0; i < 257; i++) {
    struct huff_node *node = malloc(sizeof(*node));
    if (node == NULL)
      goto out2;

    node->isLeaf = 1;
    node->c = i;
    node->weight = realDic->counts[i];

    res = priority_add(&pq, (void *)(char)i, realDic->counts[i]);
    if (res) {
      free(node);
      node = NULL;
      goto out2;
    }
  }

  /* Build the tree by repeatedly pairing the lowest-weight nodes */
  while (priority_size(&pq) > 1) {
    int res;
    struct huff_node *left;
    struct huff_node *right;
    struct huff_node *joiner = malloc(sizeof(*joiner));
    if (joiner == NULL)
      goto out2;

    joiner->isLeaf = 0;
    
    /* Get the lowest-weight nodes */
    left = priority_dequeue(&pq);
    right = priority_dequeue(&pq);

    if (left == NULL || right == NULL) {
      free(joiner);
      joiner = NULL;
      goto out2;
    }

    joiner->left = left;
    joiner->right = right;
    joiner->weight = left->weight + right->weight;

    if (joiner->weight < left->weight || joiner->weight < right->weight) {
      /* We overflowed */
      free(joiner);
      joiner = NULL;
      goto out2;
    }

    res = priority_add(&pq, joiner, joiner->weight);
    if (res) {
      free(joiner);
      joiner = NULL;
      goto out2;
    }
  }

  /* The last node will be the root of the tree */
  struct huff_node *lastNode = priority_dequeue(&pq);
  if (huff_node == NULL)
    goto out2;

  realTree->root = lastNode;

  priority_destroy(&pq);
  *tree = realTree;
  return 0;
out2:
  /* Dequeue and free all nodes */
  while (priority_size(&pq) > 0) {
    struct huff_node *node = priority_dequeue(&pq);
    huff_node_free_(node);
  }
  priority_destroy(&pq);
out1:
  free(realTree);
  realTree = NULL;
out:
  return -1;
}
static int huff_tree_destroy(huff_tree* tree)
{
}
static int huff_tree_encode(huff_tree *tree, uint8_t in, const uint8_t **outBits, int nBits)
{
}
static int huff_tree_decode(huff_tree *tree, int bit, uint8_t *out)
{
}

static void huff_node_free_(struct huff_node *node)
{
  if (node == NULL)
    return;

  if (!node->isLeaf) {
    huff_node_free_(node->left);
    huff_node_free_(node->right);
  }
  free(node);
}

/* encoder */
int huff_encoder_init(huff_encoder *encoder, huff_freqdic *dic)
{
  return 0;
}

int huff_encoder_destroy(huff_encoder *encoder)
{
  return 0;
}

int huff_encoder_feeddata(huff_encoder *encoder, const uint8_t* data, int length)
{
    return 0;
}

int huff_encoder_enddata(huff_encoder* encoder)
{
  return 0;
}

int huff_encoder_bytestowrite(huff_encoder *encoder)
{
  return 0;
}

int huff_encoder_writebytes(huff_encoder *encoder, uint8_t* buf, int length)
{
  return 0;
}

/* decoder */
int huff_decoder_init(huff_decoder *decoder)
{
  return 0;
}

int huff_decoder_destroy(huff_decoder *decoder)
{
  return 0;
}

int huff_decoder_feeddata(huff_decoder *decoder, const uint8_t* data, int length)
{
  return 0;
}

int huff_decoder_bytestowrite(huff_decoder *decoder)
{
  return 0;
}

int huff_decoder_writebytes(huff_decoder *decoder, uint8_t* buf, int length)
{
  return 0;
}
