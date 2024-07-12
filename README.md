# About

A simple scripting language designed following the book "Crafting Interpreters" to learn aboout compiler design.

# Build

Build using CMAKE to generate project files.

```
cmake -S . -B build/ -D CMAKE_BUILD_TYPE=Release
```

# Syntax

```
fun fib(n){
    var x=0;
    if(n<2){
        x =n;
    }
    else{
        x = fib(n-1)+fib(n-2);
    }
    return x;

}
var start=clock();
print fib(35);
print clock()-start;
```

```
var start = clock();
var x =0;
while(x<100000000){
    x=x+1;
}
print x;
print clock()-start;
```

```
for(x = 0; x<1000;x=x+1){
    print "hello";
}
```

# Run

<b>Run executable passing file name of code as argument</b> <br>
<b> Note : Every function must have a return at end.
use
`return;` to return no value <br>
`clock()` returns time elapsed from start of execution in ms <br>
`len()` returns length of string
