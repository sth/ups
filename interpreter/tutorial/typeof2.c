#include "../../ups/develhdrs/mtrprog/ifdefs.h"
#include "../../ups/develhdrs/local/ukcprog.h"

#include "../../ups/ups.h"
#include "../../ups/symtab.h"

int printf(const char *, ...);
type_t *$typeof();

struct name {
	char firstname[30];
	char lastname[30];
};	

char *
typename(type_t *type)
{
	switch (type->ty_code) {
	case TY_CHAR: return "char"; break;
	case TY_UCHAR: return "uchar"; break;
	case TY_SHORT: return "short"; break;
	case TY_USHORT: return "ushort"; break;
	case TY_INT: return "int"; break;
	case TY_UINT: return "uint"; break;
	case TY_LONG: return "long"; break;
	case TY_ULONG: return "ulong"; break;
	case TY_FLOAT: return "float"; break;
	case TY_DOUBLE: return "double"; break;
	case DT_PTR_TO: return "<ptr>"; break;
	case TY_ENUM: return "<enum>"; break;
	case TY_U_ENUM: return "<undef enum>"; break;
	case TY_STRUCT: return "<struct>"; break;
	case TY_U_STRUCT: return "<undef struct>"; break;
	case TY_UNION: return "<union>"; break;
	case TY_U_UNION: return "<undef union>"; break;
	case DT_ARRAY_OF: return "<array>"; break;
	default: return "<unknown>"; break;
	}
}
int main()
{
	int i = 5;
	long l = 6;
	struct name n;
	type_t *type;

	type = $typeof(i);
	printf("$typeof(i) returned %s\n", typename(type));
	printf("$typeof(l) returned %s\n", typename($typeof(l)));
	printf("$typeof(&l) returned %s\n", typename($typeof(&l)));
	printf("$typeof(n) returned %s\n", typename($typeof(n)));
	printf("$typeof(&n) returned %s\n", typename($typeof(&n)));
	return 0;
}
