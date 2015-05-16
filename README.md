Authors: Nikolaos Kofinas

# Unsupervised Learn of Probabilistic Context Free Grammars

## Compile:

* In order to run this code you must run make into the main folder and also make inside the folder "Inside-Outside"

## Run:
./CFG-learner -d DIR_PATH [-l FILE1.csv]+ [-t FILE2.csv]+ -e [FILE3.csv]+ -f FILE4 -p SPLIT -w WORD_SIZE -s SYMBOLS -L LAMBDA


## Input

1. DIR_PATH: the path to the directory that contains the all the files and where the output will be written
2. FILE1.csv: A csv file containing the learning data
3. FILE2.csv: A csv file containing the test data
4. FILE3.csv: A csv file containing the eval data5
5. SPLIT: This value specifies the split method that the programm will use:
		a. 0 -> Use the standard split method
		b. 0 -> Use the uniform split method
		c. 0 -> Use the normal split method 
6. WORD_SIZE: This defines the mean size of the generated words (comming from the quantizer)
7. SYMBOLS: How many symbols our quantizer will use
8. LAMBDA: This defines the lambda value that our learning algorithm will use (typical 0.005)

All the above inputs are necessary!

Also, if you need to change the size of the BeamSearch you need to go to the file "BeamSearch/BeamSearch.hpp" and change the value of the "\_beamDepth_" to your desired value.

## Credits
1. The original code for the inside-outside algorithm can be found [here](http://web.mit.edu/course/6/6.863/python/nltk_contrib_backup/mit/six863/rr4/inside-outside/)
