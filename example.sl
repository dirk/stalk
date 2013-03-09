
int = 2
array = [1, 2]
# @ = "as a"
# So this preface says "construct this block with closed variable scoping
# and inject the variable a as cloned integer with the name b. Cloned
# hints to the machine that b should exist as a clone of a (ie. the block
# cannot change the value of a in the original scope).
array map: <closed,int@clone: a>{|v| v * a } # returns [2, 4]

10 times: <closed,int>{|v| int = int + 10 }
int string println # Would print "102"

10 times: {|v| int = int - 10 }
# The above is the same as: 10 times <open>{|v| a = a - 10 }
# All blocks are by default open.
int string println # Would print "2"

string = "my string" uppercase

# Reassignment
string = "other string"

# The auto preface tag tells it to set up the block parameters automatically.
# ie. in this case auto will inject "|one, two|".
my_method: one other: two <closed,auto>{
  b = one * two
  return: b
}
four = my_method: one other: two

# def and clone automatically preface their blocks with <auto>.
my_method2: one two { return: (one * two) + four }

eight = my_method2: 2 2
