
#define NULL ((void *)0)

class node
{
private:
    struct tnode {
	int x;
	double y;
	tnode *next;
    };
    tnode *instance;

public:
    node() { instance = new tnode; }
    void get_node_data (int&, double&);
    void assign (int a, double b)
    {
	instance = new tnode;
	instance->x = a;
	instance->y = b;
	instance->next = (tnode *)NULL;
    }
    void print();
};

