sr = 48000
ksmps = 10
nchnls = 2
0dbfs = 1

giSine ftgen 0, 0, 8192, 10, 1
alwayson 999

instr 1
  ; setup
  iamp = 0.2
  ifreq = cpspch(p4)

  ; envelopes
  kampenv madsr 0.001, 0.1, 0.2, 1
  kampenv = kampenv * iamp
  ; tone generators
  aout foscil kampenv, ifreq, 1, 1, 0, giSine

  ; output
  MixerSetLevel   1, 1, 1
  MixerSend aout, 1, 1, 0
  MixerSend aout, 1, 1, 1
endin

; master bus
instr 999
  a1L MixerReceive 1, 0
  a1R MixerReceive 1, 1

  aoutL = a1L
  aoutR = a1R
  ;; use tanh as a simplistic limiter
  outs tanh(aoutL), tanh(aoutR)
  MixerClear
endin
