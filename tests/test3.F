C	Trivial test program

	external lenstr
	integer lenstr

	write(*,*) 'It will work?'
	i=12
	I_2=i+1
	write(*,*) I_2

	end

* Return length of STR not including trailing blanks, but always > 0.
	integer function lenstr (str)
	character*(*) str
	if (str.eq.' ') then
	  lenstr=1
	else
	  lenstr = lnblnk (str)
	end if
	end


