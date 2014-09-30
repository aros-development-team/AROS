/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/lists.h>
#include <proto/exec.h>

#include <stdio.h>

struct Node node1,node2,node3,node4;
struct List list;

int main(void)
{
    struct Node *n;
    
    NEWLIST(&list);

    node1.ln_Pri = 3;
    node1.ln_Name = "Eins";

    node2.ln_Pri = 2;
    node2.ln_Name = "Zwei";

    node3.ln_Pri = 1;
    node3.ln_Name = "Drei";

    node4.ln_Pri = 3;
    node4.ln_Name = "Vier";
    
    Enqueue(&list, &node1);
    Enqueue(&list, &node2);
    Enqueue(&list, &node3);
    Enqueue(&list, &node4);
    
    ForeachNode(&list, n)
    {
        printf("%s %d\n", n->ln_Name, n->ln_Pri);
    }
    
    return 0;
}
