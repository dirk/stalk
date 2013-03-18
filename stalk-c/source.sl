"one" println

def: foo: baz bar: baz2 {
  return: ((baz string) + baz2)
}
i = 0
while: { i < 5 } do: {
  foo: i bar: (i string) println
  i = (i + 1)
}
