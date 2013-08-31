#include "huff.h"

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

#define HUFF_EOF 256
#define HUFF_ENCODER_BUFFER_START 1024

typedef int32_t ctr;
#define CTR_MAX INT32_MAX

/* Generic internal data structures */

/* Linked list */
struct Node
{
  void *data;
  struct Node *next;
  struct Node *prev;
};
struct List_
{
  /* Points to a dummy starting node */
  struct Node *first;
  /* Points to a dummy ending node */
  struct Node *last;
  int size;
};
typedef struct List_ *List;

static List ListInit(void);
static void ListDestroy(List l);
static int ListSize(List l);
static void *ListData(List l, int index);
static int ListInsert(List l, void *data, int index);
/* Returns the removed item */
static void *ListRemove(List l, int index);
/* Pass -1 as |index| to get the dummy first node
   Pass size as |index| to get the dummy last node */
static struct Node *ListNode_(List l, int index);

static List ListInit(void)
{
  List l;

  l = calloc(1, sizeof(*l));
  if (l == NULL)
    goto out;

  l->first = calloc(1, sizeof(*(l->first)));
  if (l->first == NULL)
    goto out1;

  l->last = calloc(1, sizeof(*(l->last)));
  if (l->last == NULL)
    goto out2;
  
  l->first->next = l->last;
  l->last->prev = l->first;
  return l;

out2:
  free(l->first);
  l->first = NULL;
out1:
  free(l);
  l = NULL;
out:
  return NULL;
}
static void ListDestroy(List l)
{
  struct Node *last;
  struct Node *cur;
  assert(l != NULL);

  /* Walk the list, freeing the nodes */
  cur = l->first;
  
  while (cur != NULL) {
    last = cur;
    cur = cur->next;
    free(last);
    last = NULL;
  }

  free(l);
  l = NULL;
}
static int ListSize(List l)
{
  assert(l != NULL);

  return l->size;
}
static void *ListData(List l, int index)
{
  struct Node *n;
  assert(l != NULL);
  assert(index >= 0);
  assert(index < l->size);

  n = ListNode_(l, index);
  assert(n != NULL);

  return n->data;
}
static int ListInsert(List l, void *data, int index)
{
  struct Node *before;
  struct Node *at;
  struct Node *after;
  assert(l != NULL);
  assert(index >= 0);
  assert(index <= l->size);
  /* Shouldn't ever happen for this project, but still... undefined behavior */
  assert(l->size != INT_MAX);
  
  before = ListNode_(l, index-1);
  assert(before != NULL);

  after = before->next;
  assert(after != NULL);

  at = calloc(1, sizeof(*at));
  if (at == NULL)
    return -1;
  
  at->data = data;
  at->prev = before;
  at->next = after;

  before ->next = at;
  after->prev = at;

  l->size++;
  return 0;
}
static void *ListRemove(List l, int index)
{
  struct Node *before;
  struct Node *at;
  struct Node *after;
  void *ret;
  assert(l != NULL);
  assert(index >= 0);
  assert(index < l->size);

  at = ListNode_(l, index);
  assert(at != NULL);

  before = at->prev;
  after = at->next;
  assert(before != NULL);
  assert(after != NULL);

  ret = at->data;
  free(at);
  at = NULL;

  before->next = after;
  after->prev = before;

  l->size--;
  return ret;
}
static struct Node *ListNode_(List l, int index)
{
  int useFirst;
  struct Node *cur;
  assert(l != NULL);
  assert(index >= -1);
  assert(index <= l->size);

  useFirst = (index < l->size/2);
  if (useFirst) {
    int thisIdx = -1;
    cur = l->first;
    while (thisIdx < index) {
      cur = cur->next;
      thisIdx++;
      assert(cur != NULL);
    }
  } else {
    int thisIdx = l->size;
    cur = l->last;
    while (thisIdx > index) {
      cur = cur->prev;
      thisIdx--;
      assert(cur != NULL);
    }
  }

  return cur;
}

/* Priority queue wrapper interface for above list */
struct PriorityQueueItem
{
  void *data;
  ctr priority;
};
struct PriorityQueue_
{
  List l;
};
typedef struct PriorityQueue_ *PriorityQueue;

static PriorityQueue PriorityQueueInit(void);
static void PriorityQueueDestroy(PriorityQueue pq);
/* Returns -1 if there isn't enough memory
   0 otherwise */
static int PriorityQueueInsert(PriorityQueue pq, void *data, ctr priority);
static void *PriorityQueueRemoveMin(PriorityQueue pq);
static int PriorityQueueSize(PriorityQueue pq);

static PriorityQueue PriorityQueueInit(void)
{
  PriorityQueue pq;

  pq = calloc(1, sizeof(*pq));
  if (pq == NULL)
    return NULL;

  pq->l = ListInit();
  if (pq->l == NULL) {
    free(pq);
    pq = NULL;
  }
  return pq;
}
static void PriorityQueueDestroy(PriorityQueue pq)
{
  assert(pq != NULL);

  /* This frees all the PriorityQueueItem structs we calloc'd */
  while (PriorityQueueSize(pq) > 0)
    PriorityQueueRemoveMin(pq);

  ListDestroy(pq->l);

  free(pq);
}
static int PriorityQueueInsert(PriorityQueue pq, void *data, ctr priority)
{
  struct PriorityQueueItem *thisItem;
  int size;
  int insertIdx;
  List l;
  int res;
  assert(pq != NULL);

  thisItem = calloc(1, sizeof(*thisItem));
  if (thisItem == NULL)
    goto out;
  
  thisItem->data = data;
  thisItem->priority = priority;

  l = pq->l;

  size = ListSize(l);

  if (size == 0) {
    insertIdx = 0;
  } else {
    ctr lastPri;
    int i = 0;

    /* This may be unreasonably slow because of the repeated linked list random access
       who knows though, haven't profiled */
    do {
      struct PriorityQueueItem *item = ListData(l, i);
      assert(item != NULL);

      lastPri = item->priority;
      i++;
    } while (lastPri < priority && i < size);

    insertIdx = i-1;
  }

  res = ListInsert(l, thisItem, insertIdx);
  if (res == -1)
    goto out1;

  return 0;
out1:
  free(thisItem);
  thisItem = NULL;
out:
  return -1;
}
static void *PriorityQueueRemoveMin(PriorityQueue pq)
{
  struct PriorityQueueItem *item;
  assert(pq != NULL);

  item = ListRemove(pq->l, 0);
  assert(item != NULL);

  return item->data;
}
static int PriorityQueueSize(PriorityQueue pq)
{
  assert(pq != NULL);

  return ListSize(pq->l);
}

/* Main huff code */

/* HuffCounter */

struct HuffCounter_
{
  ctr counts[256];
  ctr totalCount;
};

static ctr HuffCounterCount(HuffCounter counter, int c);

HuffCounter HuffCounterInit(void)
{
  HuffCounter counter;

  counter = calloc(1, sizeof(*counter));
  if (counter == NULL)
    return NULL;

  /* One EOF will never be accounted for otherwise, so we put it here */
  counter->totalCount = 1;

  return counter;
}
HuffCounter HuffCounterCopy(HuffCounter from)
{
  HuffCounter into;
  assert(from != NULL);

  into = calloc(1, sizeof(*into));
  if (into == NULL)
    return NULL;


  memcpy(into, from, sizeof(*into));

  return into;
}
void HuffCounterDestroy(HuffCounter counter)
{
  assert(counter != NULL);

  free(counter);
}
int HuffCounterFeedData(HuffCounter counter, const uint8_t* data, int length)
{
  struct HuffCounter_ workingCounter;
  int i;
  assert(counter != NULL);
  assert(length >= 0);

  /* Copy the freq counts so we can return the originals if they overflow */
  memcpy(&workingCounter, counter, sizeof(workingCounter));

  for (i = 0; i < length; i++) {
    ctr before = workingCounter.totalCount;
    if (before == CTR_MAX) {
      /* We would overflow */
      return HUFF_TOOMUCHDATA;
    } else {
      workingCounter.counts[data[i]]++;
      workingCounter.totalCount++;
    }
  }

  memcpy(counter, &workingCounter, sizeof(workingCounter));
  return HUFF_SUCCESS;
}
static ctr HuffCounterCount(HuffCounter counter, int c)
{
  assert(counter != NULL);
  assert(c >= 0);
  assert(c <= 256);

  return counter->counts[c];
}

/* Huffman tree */
struct HuffTreeNode
{
  ctr weight;
  int isLeaf;
  struct HuffTreeNode *parent;
  union {
    struct {
      struct HuffTreeNode *left;
      struct HuffTreeNode *right;
    };
    int c;
  };
};
struct HuffTree_
{
  struct HuffTreeNode *root;
  struct HuffTreeNode *leafs[257];

  /* Used for storing encoding information */
  /* Initially all NULL */
  uint8_t *leafBits[257];
  /* Initially all 0 */
  int leafBitLengths[257];

  /* Used for storing decoding information */
  /* Current decode position */
  struct HuffTreeNode *decodeNode;
};
typedef struct HuffTree_ *HuffTree;

static HuffTree HuffTreeInit(HuffCounter counter);
static void HuffTreeDestroy(HuffTree tree);
/* Returns -1 if there isn't enough memory
   0 otherwise */
static int HuffTreeEncode(HuffTree tree, int in, const uint8_t **outBits, int *outBitCount);
/* Returns 0 if no output chars are finished yet,
   1 if an output char was finished - the output char is written to the |out| argument
   |out| is an int so that it can hold HUFFMAN_EOF, in addition to uint8_t values */
static int HuffTreeDecode(HuffTree tree, int bit, int *out);
static void HuffTreeNodeFree_(struct HuffTreeNode *node);

static HuffTree HuffTreeInit(HuffCounter counter)
{
  HuffTree tree;
  PriorityQueue pq;
  int res;
  int i;
  struct HuffTreeNode *lastNode;

  tree = calloc(1, sizeof(*tree));
  if (tree == NULL)
    goto out;

  pq = PriorityQueueInit();
  if (pq == NULL)
    goto out1;

  /* Add all the characters (plus EOF) to the priority queue with their counts as priorities */
  for (i = 0; i < 257; i++) {
    struct HuffTreeNode *node = malloc(sizeof(*node));
    if (node == NULL)
      goto out2;

    tree->leafs[i] = node;
    node->isLeaf = 1;
    node->c = i;
    node->weight = HuffCounterCount(counter, i);
    node->parent = NULL;

    res = PriorityQueueInsert(pq, (void *)(char)i, HuffCounterCount(counter, i));
    if (res) {
      free(node);
      node = NULL;
      goto out2;
    }
  }

  /* Build the tree by repeatedly pairing the lowest-weight nodes */
  while (PriorityQueueSize(pq) > 1) {
    int res;
    struct HuffTreeNode *left;
    struct HuffTreeNode *right;
    struct HuffTreeNode *joiner = malloc(sizeof(*joiner));
    if (joiner == NULL)
      goto out2;

    joiner->isLeaf = 0;
    
    /* Get the lowest-weight nodes */
    left = PriorityQueueRemoveMin(pq);
    right = PriorityQueueRemoveMin(pq);
    assert(left != NULL);
    assert(right != NULL);

    joiner->left = left;
    joiner->right = right;
    joiner->parent = NULL;
    left->parent = joiner;
    right->parent = joiner;

    /* Weights should always be non-negative */
    assert(left->weight >= 0);
    assert(right->weight >= 0);
    /* We should never overflow, because we keep track of counts in the counter and limit the number of total counts to be <= CTR_MAX */
    assert(left->weight <= CTR_MAX - right->weight);

    joiner->weight = left->weight + right->weight;

    res = PriorityQueueInsert(pq, joiner, joiner->weight);
    if (res) {
      free(joiner);
      joiner = NULL;
      goto out2;
    }
  }

  /* The last node will be the root of the tree */
  lastNode = PriorityQueueRemoveMin(pq);
  assert(lastNode != NULL);

  tree->root = lastNode;
  tree->decodeNode = tree->root;

  PriorityQueueDestroy(pq);

  return tree;
out2:
  /* Dequeue and free all nodes */
  while (PriorityQueueSize(pq) > 0) {
    struct HuffTreeNode *node = PriorityQueueRemoveMin(pq);
    assert(node != NULL);
    HuffTreeNodeFree_(node);
  }
  PriorityQueueDestroy(pq);
out1:
  free(tree);
  tree = NULL;
out:
  return NULL;
}
static void HuffTreeDestroy(HuffTree tree)
{
  int i;
  assert(tree != NULL);

  for (i = 0; i < 257; i++) {
    /* Not used ones are left as NULL, so it's okay to free all of them */
    free(tree->leafBits[i]);
  }

  /* We must free the nodes this way and not through tree->leafs */
  HuffTreeNodeFree_(tree->root);

  free(tree);
}
static int HuffTreeEncode(HuffTree tree, int in, const uint8_t **outBits, int *outBitCount)
{
  uint8_t *bits;
  assert(tree != NULL);
  assert(in >= 0);
  assert(in <= 256);
  assert(outBits != NULL);
  assert(outBitCount != NULL);

  bits = tree->leafBits[in];
  if (bits == NULL) {
    int length = 0;
    struct HuffTreeNode *curNode = tree->leafs[in];
    int byteIdx;
    int bitIdx;
    uint8_t *bitBase;
    assert(tree->leafBitLengths[in] == 0);

    /* Calculate length of bit pattern */
    while (curNode->parent != NULL) {
      /* The maximum depth is certainly less than 260 < INT_MAX, so the condition should always hold
         Don't do undefined behavior, kids */
      assert(length != INT_MAX);
      length++;
      curNode = curNode->parent;
    }
    assert(length > 0);
    assert(length <= INT_MAX - 7);

    tree->leafBits[in] = calloc(1, (length + 7)/8);
    if (tree->leafBits[in] != NULL)
      return -1;

    bitBase = tree->leafBits[in];
    bitIdx = (length-1) % 8;
    byteIdx = (length-1) / 8;

    curNode = tree->leafs[in];
    while (curNode->parent != NULL) {
      int bit;
      int isLeft = curNode->parent->left == curNode;
      int isRight = curNode->parent->right == curNode;
      assert(isLeft || isRight);

      if (isLeft)
        bit = 0;
      else /* if (isRight) */
        bit = 1;

      assert(byteIdx >= 0);
      assert(bitIdx >= 0);

      /* Where we're going, we don't need temporaries */
      *(bitBase+byteIdx) = (uint8_t)(((*(bitBase+byteIdx)) & (~(1<<bitIdx))) | (bit<<bitIdx));

      bitIdx--;
      if (bitIdx < 0) {
        bitIdx = 7;
        byteIdx--;
      }

      curNode = curNode->parent;
    }

    /* Make sure that the length matched up exactly with the number of expected bits */
    assert(bitIdx == 7);
    assert(byteIdx == -1);

    /* The bits have been written, now we just need to update the length */
    tree->leafBitLengths[in] = length;
  }

  *outBits = tree->leafBits[in];
  *outBitCount = tree->leafBitLengths[in];
  return 0;
}
static int HuffTreeDecode(HuffTree tree, int bit, int *out)
{
  assert(tree != NULL);
  assert(bit == 0 || bit == 1);
  assert(out != NULL);
  assert(tree->decodeNode != NULL);
  assert(!(tree->decodeNode->isLeaf));

  if (bit == 0)
    tree->decodeNode = tree->decodeNode->left;
  else /* if (bit == 1) */
    tree->decodeNode = tree->decodeNode->right;

  assert(tree->decodeNode != NULL);

  if (tree->decodeNode->isLeaf) {
    *out = tree->decodeNode->c;
    tree->decodeNode = tree->root;
    return 1;
  }
  else {
    return 0;
  }
}
static void HuffTreeNodeFree_(struct HuffTreeNode *node)
{
  assert(node != NULL);

  if (!node->isLeaf) {
    assert(node->left != NULL);
    assert(node->right != NULL);

    HuffTreeNodeFree_(node->left);
    HuffTreeNodeFree_(node->right);
  }
  free(node);
}

/* encoder */
struct HuffEncoder_
{
  HuffCounter counter;
  int counterBytesToWrite;

  HuffTree tree;

  uint8_t* buffer;
  int bufferSize;
  int byteIdx;
  int bitIdx;
};

int HuffEncoderExpandBufferToFit_(HuffEncoder encoder, int byteCount, int bitCount);
int HuffEncoderFeedSingle_(HuffEncoder encoder, int data);
int HuffEncoderWriteHeaderBytes_(HuffEncoder encoder, uint8_t *buf, int length);

HuffEncoder HuffEncoderInit(HuffCounter counter, int initialBufferSize)
{
  HuffEncoder enc;
  assert(initialBufferSize >= 0);

  enc = calloc(1, sizeof(*enc));
  if (enc == NULL)
    goto out;

  enc->counter = HuffCounterCopy(counter);
  if (enc->counter == NULL)
    goto out1;

  /* I know you might want a big data type, but... really? You don't need INT_MAX / 256 bytes */
  assert(sizeof(ctr) <= INT_MAX / 256);

  enc->counterBytesToWrite = 256*sizeof(ctr);

  enc->tree = HuffTreeInit(enc->counter);
  if (enc->tree == NULL)
    goto out2;

  if (initialBufferSize == 0)
    initialBufferSize = HUFF_ENCODER_BUFFER_START;

  enc->buffer = malloc(initialBufferSize);
  if (enc->buffer == NULL)
    goto out3;

  enc->bufferSize = initialBufferSize;
  enc->byteIdx = 0;
  enc->bitIdx = 0;

  return enc;
out3:
  HuffTreeDestroy(enc->tree);
out2:
  HuffCounterDestroy(enc->counter);
out1:
  free(enc);
  enc = NULL;
out:
  return NULL;
}
void HuffEncoderDestroy(HuffEncoder encoder)
{
  assert(encoder != NULL);

  HuffCounterDestroy(encoder->counter);
  HuffTreeDestroy(encoder->tree);
  free(encoder->buffer);
  free(encoder);
}
int HuffEncoderFeedData(HuffEncoder encoder, const uint8_t *data, int length, int *processed)
{
  int i;
  int ret = HUFF_NOMEM;
  assert(encoder != NULL);
  assert(length >= 0);
  assert(length == 0 || data != NULL);
  assert(length == 0 || processed != NULL);

  for (i = 0; i < length; i++) {
    ret = HuffEncoderFeedSingle_(encoder, data[i]);
    if (ret != HUFF_SUCCESS)
      break;
  }

  *processed = i;
  return ret;
}
int HuffEncoderEndData(HuffEncoder encoder)
{
  assert(encoder != NULL);
  return HuffEncoderFeedSingle_(encoder, HUFF_EOF);
}
int HuffEncoderByteCount(HuffEncoder encoder)
{
  assert(encoder != NULL);
  return encoder->byteIdx + encoder->counterBytesToWrite;
}
int HuffEncoderWriteBytes(HuffEncoder encoder, uint8_t *buf, int length)
{
  int headerWriteCount;
  int toWrite;
  int byteCount;
  int toShift;
  int i;
  assert(encoder != NULL);
  assert(length >= 0);
  assert(length == 0 || buf != NULL);

  headerWriteCount = HuffEncoderWriteHeaderBytes_(encoder, buf, length);

  length -= headerWriteCount;

  byteCount = HuffEncoderByteCount(encoder);
  toWrite = (length < byteCount) ? length : byteCount;

  if (toWrite > 0) {
    memcpy(buf+headerWriteCount, encoder->buffer, toWrite);
    
    /* Shift data down in the internal buffer */
    toShift = encoder->bufferSize - toWrite;
    for (i = 0; i < toShift; i++)
      encoder->buffer[i] = encoder->buffer[i+toWrite];

    encoder->byteIdx -= toWrite;
  }

  return headerWriteCount + toWrite;
}
int HuffEncoderFeedSingle_(HuffEncoder encoder, int data)
{
  const uint8_t *bits;
  int totalBitCount;
  int bitCount;
  int byteCount;
  int byteLookup;
  int bitLookup;
  int j;
  int res;
 
  assert(data >= 0);
  assert(data <= 256);
  res = HuffTreeEncode(encoder->tree, data, &bits, &totalBitCount);
  if (res)
    return HUFF_NOMEM;
  
  byteCount = totalBitCount / 8;
  bitCount = totalBitCount %  8;
  res = HuffEncoderExpandBufferToFit_(encoder, byteCount, bitCount);
  if (res)
    return HUFF_TOOMUCHDATA;

  /* Actually add the encoded bits to the buffer */
  byteLookup = 0;
  bitLookup = 0;
  for (j = 0; j < totalBitCount; j++) {
    int bit;
    assert(bitLookup >= 0);
    assert(bitLookup < 8);
    assert(byteLookup >= 0);
    bit = (bits[byteLookup] & (1<<bitLookup)) >> bitLookup;
    
    assert(encoder->bitIdx >= 0);
    assert(encoder->bitIdx < 8);
    assert(encoder->byteIdx >= 0);
    assert(encoder->byteIdx < encoder->bufferSize);
    /* If you can't tell what this does at a glance, you're not a real a C programmer */
    *(encoder->buffer+encoder->byteIdx) = (uint8_t)(((*(encoder->buffer+encoder->byteIdx)) & (~(1<<encoder->bitIdx))) | (bit<<encoder->bitIdx));

    bitLookup++;
    if (bitLookup == 8) {
      bitLookup = 0;
      byteLookup++;
    }

    encoder->bitIdx++;
    if (encoder->bitIdx == 8) {
      encoder->bitIdx = 0;
      encoder->byteIdx++;
    }
  }

  return HUFF_SUCCESS;
}
int HuffEncoderExpandBufferToFit_(HuffEncoder encoder, int byteCount, int bitCount)
{
  int freeBytes;
  int freeBits;
  int notEnoughSpace;
  assert(encoder != NULL);
  assert(encoder->byteIdx >= 0);
  assert(encoder->bitIdx >= 0);
  /* Check if we already have enough space in the buffer */
  freeBytes = encoder->bufferSize - encoder->byteIdx + (encoder->bitIdx + 7)/8;
  freeBits = (8 - encoder->bitIdx) % 8;
  notEnoughSpace = freeBytes < byteCount || (freeBytes == byteCount && freeBits < bitCount);
  while (notEnoughSpace) {
    uint8_t *newBuffer;
    /* There isn't enough space - attempt to get bigger */
    if (encoder->bufferSize > INT_MAX / 2)
      /* Can't get bigger due to overflow */
      break;

    newBuffer = realloc(encoder->buffer, encoder->bufferSize*2);
    if (newBuffer == NULL)
      /* Couldn't get bigger due to lack of memory */
      break;
    
    freeBytes += encoder->bufferSize;
    encoder->buffer = newBuffer;
    encoder->bufferSize *= 2;
  }

  notEnoughSpace = freeBytes < byteCount || (freeBytes == byteCount && freeBits < bitCount);
  if (notEnoughSpace)
    return -1;
  else
    return 0;
}
int HuffEncoderWriteHeaderBytes_(HuffEncoder encoder, uint8_t *buf, int length)
{
  int toWrite;
  int byteIdx;
  int countLookup;
  int byteLookup;
  int i;
  assert(encoder != NULL);
  assert(length == 0 || buf != NULL);

  toWrite = (length < encoder->counterBytesToWrite) ? length : encoder->counterBytesToWrite;

  byteIdx = (256*sizeof(ctr) - encoder->counterBytesToWrite);
  countLookup = byteIdx / sizeof(ctr);
  byteLookup = byteIdx % sizeof(ctr);

  for (i = 0; i < toWrite; i++) {
    /* This expression is simpler than the other ones, at least
       It's a little-endian serializationm by the way */
    uint8_t byte = (uint8_t)((HuffCounterCount(encoder->counter, countLookup) & ((ctr)0xFF) << ((ctr)(byteLookup*8))) >> ((ctr)(byteLookup*8)));
    buf[i] = byte;

    byteLookup++;
    if (byteLookup == sizeof(ctr)) {
      byteLookup = 0;
      countLookup++;
    }
  }

  encoder->counterBytesToWrite -= toWrite;
  return toWrite;
}

/* decoder */
HuffDecoder HuffDecoderInit(void)
{
  /* TODO - implement */
  return NULL;
}

void HuffDecoderDestroy(HuffDecoder decoder)
{
  /* TODO - implement */
}

int HuffDecoderFeedData(HuffDecoder decoder, const uint8_t *data, int length)
{
  /* TODO - implement */
  return 0;
}

int HuffDecoderByteCount(HuffDecoder decoder)
{
  /* TODO - implement */
  return 0;
}

int HuffDecoderWriteBytes(HuffDecoder decoder, uint8_t *buf, int length)
{
  /* TODO - implement */
  return 0;
}
