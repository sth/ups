C	Testing handling of underscores in Fortran names; also C subroutines
C	Contributed by Satish Balay <balay@mcs.anl.gov>
C	Comprises : test2_main.F test2_sub.F test2_foo.c

        subroutine b_r(i)
        integer i

        write(*,*) 'print from fortran = ',i
        end

