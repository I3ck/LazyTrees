LazyTrees
=========
A C++ library for lazy tree structures.  
Trees are usually used for fast searches. For example to find the nearest neighbour of a point. Building such a tree can be time consuming, which is why `LazyTrees` are only built / evaluated as little as possible.  
Since the trees are built when accessing, `read` operations can't be declared `const`. Which is why strict versions of the trees are also offered, which can be built from their lazy versions.  

LazyKdTree<P>
-------------
The lazy version of a k-d tree (https://en.wikipedia.org/wiki/K-d_tree).  
`P` must implement :  
```cpp
static size_t dimensions(); //returning the number of dimensions (2D Point => return 2)
const double& operator[](size_t) const; //overloading the random access operator. [0] => x-coordinate, [1] => y-coordinate ... [dimensions() - 1]
```
The tree can be constructed with `vector<P>`.

StrictKdTree<P>
---------------
Analog to its lazy version, already being fully evaluated and offering `const` read access, making it thread-safe.  
Can be constructed with `vector<P>`, but also from a `LazyKdTree<P>`.

### Performance (Lazy)KdTree
1 million 2D points  
The second queries are identical to the first, but the required parts of the tree will already be evaluated.
The performance tests can be found in `tests/`.

| Action                  | First / Second | Time     | Comment                        |
| ----------------------- | -------------- | -------- | ------------------------------ |
| creation input vector   |                | 4.8 ms   | not part of the tree creation  |
| move creation lazy tree |                | 0.89 μs  |                                |
| nearest()               | first          | 32.59 ms |                                |
|                         | second         | 2.717 μs |                                |
| knearest(1000)          | first          | 22.47 ms |                                |
|                         | second         | 9.166 ms |                                |
| hypersphere(1000)       | first          | 7.08 ms  |                                |
|                         | second         | 1.43 ms  |                                |
| box(10000,10000)        | first          | 1.83 ms  |                                |
|                         | second         | 0.85 μs  |                                |
| fully evaluate          | first          | 294.3 ms | same as making it a StrictTree |
|                         | second         | 10.87 ms |                                |  

Creating a strict version of this tree would take `> 294.3ms`, while the overhead of the first nearest call is roughly `32 ms`. If all further queries would roughly occur in the same area, the creation would be about `10` times faster.  
Creation and `1000 * nearest` would only take about `39 ms`.


Examples
--------
See `tests/*` for usage examples and tests.



Version
-------
0.1.0

License
------
MIT (See LICENSE)
