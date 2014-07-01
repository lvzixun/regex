regex
=====

a simple regular express engine

## pattern
only support 4 pattern.

### `and` pattern
```
  pattern1 pattern2

  the nfa map:
    <start> -- pattern1 --> <s1> -- pattern2 --> <end>
```


### `or` pattern
```
pattern1 | pattern2

  the nfa map:
    |----pattern1----> <s1> ---ε-------|
    |                                  ∨
  <start> --pattern2--> <s2> ---ε---> <end>
```


### `repeated` pattern
```
  pattern *

  the nfa map:
    |----------ε---------|
    ∨                    |
  <start> --pattern--> <end> 
```  


### `range` pattern
```
  [a-b]
  
  the nfa map:
  <start> --[a-b]--> <end>
```

## detail
```
  TODO IT....
```
