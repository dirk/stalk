# a = 2 + 8
# ("test" + (a string)) println
# 
# b = [1, 2, [3, 4, [5, 6]]]
# b println

def: a: b c: d {
  return: b + d
}
def: a: b { return: b }

a: 1 c: 2 println
a: 3 println
