
#include "redblacktree.h"

/* The constants below must be defined in the file which includes
   the redblacktree.c code. */

#if 0

#define ROOT     Root                /* The name of the root variable */
#define RBNODE   struct RBTreeNode   /* The name of the structure containing the left,
                                        right, parent, nodecolor & data fields. */
#define SENTINEL Sentinel            /* The name of the sentinel node variable (must be a RBNODE) */
#define DATA     data                /* The name of the data field (example: data, sub.mydata) */

#define CompLT(a,b) (a < b)      /* modify these lines to establish data type */
#define CompEQ(a,b) (a == b)

#endif


/* Example RBTreeNode structure:

struct RBTreeNode {
  struct RBTreeNode *left;
  struct RBTreeNode *right;
  struct RBTreeNode *parent;
  UWORD magicvalue;
  NodeColor color;
  unsigned long data;
};

*/

/* prototypes */

void InsertNode(RBNODE *X);
static void DeleteNode(RBNODE *X);
void InsertFixup(RBNODE *X);
static void DeleteFixup(RBNODE *X);
void RotateLeft(RBNODE *X);
void RotateRight(RBNODE *X);
RBNODE *FindNode(unsigned long data);

#if 0
#define NIL &SENTINEL           /* all leafs are sentinels */
RBNODE Sentinel = { NULL, NULL, 0, BLACK, 0};

RBNODE *Root = NULL;               /* root of red-black tree */
#endif



void InitRedBlackTree(void) {
  ROOT=&SENTINEL;
  SENTINEL.left=&SENTINEL;
  SENTINEL.right=&SENTINEL;
  SENTINEL.parent=0;
  SENTINEL.color=BLACK;
//  SENTINEL.magicvalue=0x4a4f;
  SENTINEL.DATA=0;
}



void InsertNode(RBNODE *X) {
  RBNODE *current, *parent;

 /***********************************************
  *  allocate node for Data and insert in tree  *
  ***********************************************/

  /* find where node belongs */
  current = ROOT;
  parent = 0;
  while (current != &SENTINEL) {
    // if(CompEQ(X->DATA, current->DATA)) return (current);
    parent = current;
    current = CompLT(X->DATA, current->DATA) ? current->left : current->right;
  }

  X->parent = parent;
  X->left = &SENTINEL;
  X->right = &SENTINEL;
  X->color = RED;

  /* insert node in tree */
  if(parent) {
    if(CompLT(X->DATA, parent->DATA)) {
      parent->left = X;
    }
    else {
      parent->right = X;
    }
  }
  else {
    ROOT = X;
  }

  InsertFixup(X);
}



void InsertFixup(RBNODE *X) {

 /*************************************
  *  maintain red-black tree balance  *
  *  after inserting node X           *
  *************************************/

  /* check red-black properties */
  while (X != ROOT && X->parent->color == RED) {
    /* we have a violation */
    if(X->parent == X->parent->parent->left) {
      RBNODE *Y = X->parent->parent->right;
      if(Y->color == RED) {
        /* uncle is red */
        X->parent->color = BLACK;
        Y->color = BLACK;
        X->parent->parent->color = RED;
        X = X->parent->parent;
      }
      else {
        /* uncle is black */
        if(X == X->parent->right) {
            /* make X a left child */
            X = X->parent;
            RotateLeft(X);
        }

        /* recolor and rotate */
        X->parent->color = BLACK;
        X->parent->parent->color = RED;
        RotateRight(X->parent->parent);
      }
    }
    else {
      /* mirror image of above code */
      RBNODE *Y = X->parent->parent->left;
      if(Y->color == RED) {

        /* uncle is red */
        X->parent->color = BLACK;
        Y->color = BLACK;
        X->parent->parent->color = RED;
        X = X->parent->parent;
      }
      else {
        /* uncle is black */
        if(X == X->parent->left) {
            X = X->parent;
            RotateRight(X);
        }
        X->parent->color = BLACK;
        X->parent->parent->color = RED;
        RotateLeft(X->parent->parent);
      }
    }
  }
  ROOT->color = BLACK;
}

void RotateLeft(RBNODE *X) {

 /**************************
  *  rotate Node X to left *
  **************************/

  RBNODE *Y = X->right;

  /* establish X->right link */
  X->right = Y->left;
  if(Y->left != &SENTINEL) {
    Y->left->parent = X;
  }

  /* establish Y->parent link */
  if(Y != &SENTINEL) {
    Y->parent = X->parent;
  }
  if(X->parent) {
    if(X == X->parent->left) {
      X->parent->left = Y;
    }
    else {
      X->parent->right = Y;
    }
  }
  else {
    ROOT = Y;
  }

  /* link X and Y */
  Y->left = X;
  if(X != &SENTINEL) {
    X->parent = Y;
  }
}

void RotateRight(RBNODE *X) {

 /****************************
  *  rotate Node X to right  *
  ****************************/

  RBNODE *Y = X->left;

  /* establish X->left link */
  X->left = Y->right;
  if(Y->right != &SENTINEL) {
    Y->right->parent = X;
  }

  /* establish Y->parent link */
  if(Y != &SENTINEL) {
    Y->parent = X->parent;
  }
  if(X->parent) {
    if(X == X->parent->right) {
      X->parent->right = Y;
    }
    else {
      X->parent->left = Y;
    }
  }
  else {
    ROOT = Y;
  }

  /* link X and Y */
  Y->right = X;
  if(X != &SENTINEL) {
    X->parent = Y;
  }
}

static void DeleteNode(RBNODE *Z) {
  RBNODE *X, *Y;
  NodeColor c;

 /*****************************
  *  delete node Z from tree  *
  *****************************/

  if(!Z || Z == &SENTINEL) {
    return;
  }

  if(Z->left == &SENTINEL || Z->right == &SENTINEL) {
    /* Y has a &SENTINEL node as a child */
    Y = Z;
  }
  else {
    /* find tree successor with a &SENTINEL node as a child */
    Y = Z->right;
    while(Y->left != &SENTINEL) {
      Y = Y->left;
    }
  }

  /* X is Y's only child */
  if(Y->left != &SENTINEL) {
    X = Y->left;
  }
  else {
    X = Y->right;
  }

  /* remove Y from the parent chain */
  X->parent = Y->parent;
  if(Y->parent) {
    if(Y == Y->parent->left) {
      Y->parent->left = X;
    }
    else {
      Y->parent->right = X;
    }
  }
  else {
    ROOT = X;
  }

  c=Y->color;

  if(Y != Z) {
    Y->parent=Z->parent;
    if(Z->parent!=0) {
      if(Z == Z->parent->left) {
        Z->parent->left=Y;
      }
      else {
        Z->parent->right=Y;
      }
    }
    else {
      ROOT=Y;
    }

    Y->left=Z->left;
    Y->right=Z->right;
    Y->left->parent=Y;
    Y->right->parent=Y;
    Y->color=Z->color;
  }
  if(c == BLACK) {
    DeleteFixup(X);
  }
}

void DeleteFixup(RBNODE *X) {

   /*************************************
    *  maintain red-black tree balance  *
    *  after deleting node X            *
    *************************************/

  while (X != ROOT && X->color == BLACK) {
    if(X == X->parent->left) {
      RBNODE *W = X->parent->right;
      if(W->color == RED) {
        W->color = BLACK;
        X->parent->color = RED;
        RotateLeft (X->parent);
        W = X->parent->right;
      }
      if(W->left->color == BLACK && W->right->color == BLACK) {
        W->color = RED;
        X = X->parent;
      }
      else {
        if(W->right->color == BLACK) {
          W->left->color = BLACK;
          W->color = RED;
          RotateRight (W);
          W = X->parent->right;
        }
        W->color = X->parent->color;
        X->parent->color = BLACK;
        W->right->color = BLACK;
        RotateLeft (X->parent);
        X = ROOT;
      }
    }
    else {
      RBNODE *W = X->parent->left;
      if(W->color == RED) {
        W->color = BLACK;
        X->parent->color = RED;
        RotateRight (X->parent);
        W = X->parent->left;
      }
      if(W->right->color == BLACK && W->left->color == BLACK) {
        W->color = RED;
        X = X->parent;
      }
      else {
        if(W->left->color == BLACK) {
          W->right->color = BLACK;
          W->color = RED;
          RotateLeft (W);
          W = X->parent->left;
        }
        W->color = X->parent->color;
        X->parent->color = BLACK;
        W->left->color = BLACK;
        RotateRight (X->parent);
        X = ROOT;
      }
    }
  }
  X->color = BLACK;
}

static RBNODE *FindClosestNode(unsigned long data) {
  RBNODE *closest=0;
  RBNODE *current = ROOT;

  /* Finds the node containing data, or the next higher one.  Always
     check the resulting node.  It can be a Sentinel node, or if there
     is no node higher than or equal to data then the previous node
     will be returned. */

  while(current != &SENTINEL) {
    closest=current;
    if(CompEQ(data, current->DATA)) {
      return (current);
    }
    else {
      current = CompLT(data, current->DATA) ? current->left : current->right;
    }
  }
  return(closest);
}

RBNODE *FindNode(unsigned long data) {
  RBNODE *current = ROOT;

 /*******************************
  *  find node containing data  *
  *******************************/

  while(current != &SENTINEL) {
    if(CompEQ(data, current->DATA)) {
      return (current);
    }
    else {
      current = CompLT(data, current->DATA) ? current->left : current->right;
    }
  }
  return(0);
}



RBNODE *FindNodeFrom(RBNODE *current,unsigned long data) {

 /*******************************
  *  find node containing data  *
  *******************************/

  while(current != &SENTINEL) {
    if(CompEQ(data, current->DATA)) {
      return (current);
    }
    else {
      current = CompLT(data, current->DATA) ? current->left : current->right;
    }
  }
  return(0);
}





/* Call with FindAllNodes(ROOT,...); */

static void FindAllNodes(RBNODE *current,unsigned long data) {

 /************************************
  *  find all nodes containing Data  *
  ************************************/

  if(current != &SENTINEL) {
    if(CompEQ(data, current->DATA)) {

      // do something with Current here.

      FindAllNodes(current->left,data);
      FindAllNodes(current->right,data);
    }
    else {
      CompLT(data, current->DATA) ? FindAllNodes(current->left,data) : FindAllNodes(current->right,data);
    }
  }
}



void WalkTree(RBNODE *current) {
  if(current!=&SENTINEL) {
    WalkTree(current->left);
    WalkTree(current->right);
  }
}



RBNODE *FirstNode(void) {
  RBNODE *n=ROOT;

  /* This function returns the first node, or 0 if there are no nodes at all. */

  if(n==&SENTINEL) {
    return(0);
  }

  while(n->left != &SENTINEL) {
    n=n->left;
  }

  return(n);
}


RBNODE *NextNode(RBNODE *n) {
//  cop(n, "nextnode 1");

  if(n->right != &SENTINEL) {
    n=n->right;
    while(n->left != &SENTINEL) {
      n=n->left;
//      cop(n, "nextnode 2");
    }
    return(n);
  }

  while(n->parent!=0) {
    if(n->parent->left == n) {
      break;
    }

    n=n->parent;
//    cop(n, "nextnode 3");
  }

  return(n->parent);
}
