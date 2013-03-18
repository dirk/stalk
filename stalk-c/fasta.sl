last = 42
A    = 3877
C    = 29573
M    = 139968

def: rand: max {
  closure last = (((last * A) + C) % M)
  return: ((max * last) / M)
}

ALU = (
  "GGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTGG" + \
  "GAGGCCGAGGCGGGCGGATCACCTGAGGTCAGGAGTTCGAGA" + \
  "CCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACTAAAAAT" + \
  "ACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAATCCCA" + \
  "GCTACTCGGGAGGCTGAGGCAGGAGAATCGCTTGAACCCGGG" + \
  "AGGCGGAGGTTGCAGTGAGCCGAGATCGCGCCACTGCACTCC" + \
  "AGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAA")

IUB = new
IUB a = 0.27
IUB B = 0.02
IUB M = 0.02
IUB V = 0.02
IUB c = 0.12
IUB g = 0.12
IUB t = 0.27
IUB D = 0.02
IUB H = 0.02
IUB K = 0.02
IUB N = 0.02
IUB R = 0.02
IUB S = 0.02
IUB W = 0.02
IUB Y = 0.02

HomoSap = new
HomoSap a = 0.3029549426680
HomoSap c = 0.1979883004921
HomoSap g = 0.1975473066391
HomoSap t = 0.3015094502008

def: makeCumulative: table {
  syms   = table keys
  length = syms length
  i = 1
  prev = table get: (syms first)
  while: { i < length } do: {
    curr = closure table get: (closure syms index: (closure i))
    closure table set: curr to: (curr + (closure prev))
    closure prev = curr
    closure i = (closure i + 1)
  }
}

LineLength = 60

def: fastaRepeatTwo: length seq: alu {
  n = length
  k = 0
  kn = (alu length)
  
  while: { n > 0 } do: {
    if: (n < (closure LineLength)) then: {
      m = n
    } else: {
      m = (closure LineLength)
    }
    i = 0
    while: { i < m } do: {
      if: (k == kn) then: {
        k = 0
      }
      b = (alu from: k to: (k + 1))
      b print
      k = (k + 1)
      i = (i + 1)
    }
    "" println
    n = (n - (closure LineLength))
  }
}

def: fastaRepeat: n seq: seq {
  seqi = 0
  lenOut = 60
  while: { n > 0 } do: {
    if: (n < lenOut) then: {
      closure lenOut = n
    }
    length = (seq length)
    if: ((seqi + lenOut) < length) then: {
      newLen = (seqi + lenOut)
      seq from: seqi to: newLen println
      seqi = newLen
    } else: {
      s = (seq from: seqi)
      seqi = (lenOut - length)
      s + (seq from: 0 to: seqi) println
    }
    n = (n - lenOut)
  }
}

def: fastaRandom: n table: table {
  line = (closure array: 60)
  closure makeCumulative: table
  while: { closure n > 0 } do: {
    if: (closure n < (closure line length)) then: {
      closure closure line = (array: (closure closure n))
    }
    
    i = 0
    while: { closure i < (closure closure closure line length) } do: {
      r = (rand: i)
      keys = (closure closure table keys)
      ki = 0
      kl = (keys length)
      while: { closure ki < (closure kl) } do: {
        if: (closure r < (closure closure closure table get: \
          (closure keys index: (closure ki)))
        ) then: {
          ki = closure closure ki
          closure closure closure closure line index: k to: (closure closure keys index: ki)
          # TODO: closure break
        }
      }
    }
    closure line join: "" println
    closure n = (closure n - (closure line length))
  }
}


n = 25000000

">ONE Homo sapiens alu" println
# fastaRepeat: (2 * n) seq: ALU
fastaRepeatTwo: (2 * n) seq: ALU


">TWO IUB ambiguity codes" println
fastaRandom: (3 * n) seq: IUB

">THREE Homo sapiens frequency" println
fastaRandom: (5 * n) seq: HomoSap
