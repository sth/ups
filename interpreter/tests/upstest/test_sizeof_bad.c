extern int extfunc(void);

int main()
{
	int arry[];
	struct s { int bit: 1; } s;
	(void)(sizeof (extfunc));
	(void)(sizeof extfunc);
	(void)(sizeof arry);
	(void)(sizeof (s.bit));
}
