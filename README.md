# Calculate a number

Given a target number *t* and a set of *n* numbers *x<sub>1</sub>, ... , x<sub>n</sub>*, the goal is to calculate the target *t* from the numbers *x<sub>1</sub>, ... , x<sub>n</sub>* using the arithmetic operators +, -, *, /, and unrestricted parentheses.

## Calculations are trees
Every calculation can be represented as an expression tree (think of Abstract Syntax Trees). For example, 3 * (4 + 1) is this tree:
```
  *
 / \
3   +
   / \
  4   1
```

In the next section we need the notion of a partial calculation. That is, a calculation where not all numbers are filled in. For example 3 * (. + 1), where the dot denotes an open position. This can be represented in an expression tree as follows:
```
  *
 / \
3   +
   / \
  .   1
```

The 'period' or open node can be filled by either an operator or a number. If filled by an operator that expands the tree and creates two new open nodes, for example:
```
  *
 / \
3   +
   / \
  *   1
 / \
.   .
```

We say an expression tree is *evaluable* when there are no more open nodes.

## Search space is a tree of trees
The search space can be modeled as a tree where each node of the tree contains a (partial) calculation, i.e. an expression tree. The root is an expression tree that has a single open node as root.

Children of a node in this tree are produced by filling in the left-most open node with all possible remaining nodes, i.e. all operators and all unused numbers in *x<sub>1</sub>, ... , x<sub>n</sub>*. 

We say a node is *valid* when it is evaluable and all numbers *x<sub>1</sub>, ... , x<sub>n</sub>* are used exactly once.

## Strategies

### Naive
First strategy is to simply do an exhaustive search in this space. The search is depth first, so it can be programmed recursively. 

### DFS with memoization
While searching we encounter partial evaluable calculations. We can cache these and whenever a tree has 1 open node remaining check if we have already encountered the value needed to compute the exact number. 

Interestingly this does not lead to any improvement. Probably because the search order prevents caching any set of remaining numbers that are left in the search tree. This could be an artefact of DFS from left to right. Let's explore more later.

### A*


## Metrics
We can use wall clock time as general metric. When doing further optimizations we can use the count of explored nodes in the search tree as metric. 

## Some more observations

### Uniqueness of expression trees
There can be more expression trees that yield the same calculated number. This is not a problem if the calculation is different in a non-trivial way. For example: 1+2+3+4 = 2*4+3-1.
```
       +             +
      / \           / \
    +     +       *     -
   / \   / \     / \   / \
  1   2 3   4   2   4 3   1
```

The trivial differences complicate searching for a solution and could be avoided, as they only clutter the search space. An example of two trivially different expression trees is (1+2)+(3+4) and (3+4)+(1+2)
```
       +             +
      / \           / \
    +     +       +     +
   / \   / \     / \   / \
  1   2 3   4   3   4 1   2
```

The cause of this difference is the commutativity of the + and * operators. 

NOTE possible solution is to sort children 'alphabetically'?