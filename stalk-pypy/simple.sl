# a = 2 + 8
# ("test" + (a string)) println
# 
# b = [1, 2, [3, 4, [5, 6]]]
# b println

def: a: b c: d {
  return: b + d
}
def: a: b { return: b }

a: 1 c: 1 println
a: 1 println

# def: plus_two: a {
#   if: 1 then: {
#     return: 1
#   }
#   return: a + 2
# }
# 
10 times: {|v| v println } println
# 
if: 1 then: {
  "Hi!" println
}
