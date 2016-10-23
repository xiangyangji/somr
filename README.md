# SOM++ based on 32bit omr. Let's call this project as SOMR( SOM+OMR).

To make it work , following instructions could do help. 

	#build the omr firstly.
	make omr
	
	#clean the omr
	make omrclean
	
	#build the som++
	make 
	
	#clean the som++
	make clean
	
	#build the omr and som++ together. 
	make somrall
	
	#Clean the omr and som++
	make somrallclean

	#Run the SOM
	export LD_LIBRARY_PATH=.
	./somr -cp Smalltalk Examples/Hello/Hello.som
	./somr -cp Smalltalk Examples/Benchmarks/Sum.som
