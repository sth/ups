
#include "dw_node.h"

int main()
{
    int j;
    double k;
    node n1, n2;

    n1.get_node_data(j, k);
    n1.assign(j, k);

    n2.get_node_data(j, k);
    n2.assign(j, k);

    n1.print();
    n2.print();

    return 0;
}

