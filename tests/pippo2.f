C	Test program provided by Michele Ferlauto <ferlauto@athena.polito.it>

         Program pippo2

         implicit complex (j,t)
         common /pippo01/xpippo,ypippo
         common /pippo02/tom,jerry
         common/ pippo3/jerry2,ierr,xpippo2,ypippo2 

         complex ci,za,zb
         real*8  cip,ciop
         complex*16 cdouble
 


         write(*,*)'xdum =' 
         read(*,*)xdum       
      
         pi=4.*atan(1.)
             
         ci=cmplx(0.,1.)

         za=ci*xdum
         xpippo=xdum
         ypippo=xdum**2

         tom=xpippo+ci*ypippo
         jerry=conjg(tom)
         joe=tom*2-jerry  

         call dummy(xdum)

         cip=dble(xpippo)
         ciop=dble(ypippo)
         cdouble=cip+ci*ciop
 
         ierr=int(xdum)
         jerry2=tom*3.
#if (defined f77) && (defined __FreeBSD__) && __FreeBSD__ < 3
#else
         xpippo2=sngl(dreal(cdouble)) 
#endif

         end

         subroutine dummy(xd)
         implicit complex (j,t)
         common /pippo01/xpippo,ypippo
         common /pippo02/tom,jerry


         write(*,*)'xpippo =',xpippo
         write(*,*)'ypippo =',ypippo 
         write(*,*)'tom    =',tom
         write(*,*)'jerry  =',jerry

         
         return
         end 
