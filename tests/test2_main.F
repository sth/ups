C	Testing handling of underscores in Fortran names; also C subroutines
C	Contributed by Satish Balay <balay@mcs.anl.gov>
C	Comprises : test2_main.F test2_sub.F test2_foo.c

	program main
	integer i_i

	i_i = 10
	call Foo(i_i)
	call B_r(i_i)

	end

