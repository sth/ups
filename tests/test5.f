C test5.f Contributed by Ken DeHart

      program kentest2

      implicit none

      integer i, N

      parameter (N=9)

      double precision array1(N)

      do i = 1, N
         array1(i) = -1.0 * i
         write(*,*)i, array1(i)
      enddo

      call test(array1)

      end

      subroutine test (array1)

      implicit none
 
      integer i, N

      parameter (N=9)

      double precision array1(N), array2(N), nonarray

      do i = 1, N
         array1(i) = 1.0 * i
         array2(i) = array1(i)
         nonarray = array2(i)
         write(*,*)i, array1(i), array2(i), nonarray
      enddo
      call test2 (array1)

      return

      end

      subroutine test2 (array1)

      implicit none
 
      integer i, N

      parameter (N=9)

      double precision array1(N), array2(N), nonarray

      do i = 1, N
         array1(i) = 2.0 * i
         array2(i) = array1(i)
         nonarray = array2(i)
         write(*,*)i, array1(i), array2(i), nonarray
      enddo

      return

      end

