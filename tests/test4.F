C	Trivial test program

	dimension A(10,10)
	do 2 i = 1,10
	do 3 j = 1,10
	A(i,j) = i * j;
3	continue
2	continue

	call large (A, 10, 6, BIGGST, m)
	write(*,*) BIGGST

	end

	subroutine large(array,n,i,big,j)
	dimension array(n, n)
	big = abs(array(i, 1))
	j = 1
	do 99 k = 2, n
	if (abs(array(i,k)) .LT. big)go to 99
	big = abs(array(i,k))
	j = k
99	continue
	return
	end

