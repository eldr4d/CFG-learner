Authors: Nikolaos Kofinas

# Unsupervised Learn of Probabilistic Context Free Gramamrs

## Compile:

* In order to run this code you must make the main code and the code inside the folder "Inside-Outside"

## Input

1. Dirpath: the path to the directory that contains the training file
2. Filename: the filename of the training file

The training file must follow this pattern:
positive data (1 for positive or 0 for negative) symbol1 symbol2 symbol3 symbol4 symbol5 (Symbols must not be numbers)
Example:
```
1 a a a b b b
1 a a a a a b b b b b
1 a a b b
1 a a a a b b b b
```

## Credits
1. The original code for the inside-outside algorithm can be found [here](http://web.mit.edu/course/6/6.863/python/nltk_contrib_backup/mit/six863/rr4/inside-outside/)
