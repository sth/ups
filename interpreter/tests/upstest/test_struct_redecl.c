struct s { int x[5]; };

int main()
{
	struct t { struct s *sptr; int i; };
	struct s { struct s *sptr; int y; };

	struct s S;
	struct t T;

	(void)(T.sptr->x[0] == 0);
	(void)(S.sptr->y == 0);

	return 0;
}
