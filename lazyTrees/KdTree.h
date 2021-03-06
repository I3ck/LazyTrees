/*
    Copyright (c) 2016 Martin Buck
    Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation the rights to
    use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
    and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
    The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
    DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
    OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef KDTREE_H
#define KDTREE_H

#include <memory>
#include <vector>

namespace lazyTrees {

//------------------------------------------------------------------------------


// P must implement static size_t dimensions() returning number of dimensions
// P also must be const random-accessable for up to [dimensions() - 1] returning
// the X / Y / Z / ... coordinate of the point
template <typename P>
class LazyKdTree {
private:
    enum Compare {
        NEGATIVE,
        POSITIVE
    };

    std::unique_ptr<std::vector<P> > inputData;

    std::unique_ptr<LazyKdTree> childNegative, childPositive;

    std::unique_ptr<P> data;

    const size_t dim;

//------------------------------------------------------------------------------

public:
    LazyKdTree(std::vector<P>&& in, int dimension = 0)
        : inputData(new std::vector<P>(std::move(in)))
        , childNegative(nullptr)
        , childPositive(nullptr)
        , data(nullptr)
        , dim(dimension % P::dimensions())
    {
      throw_if_input_empty();
    }

    LazyKdTree(std::vector<P> const& in, int dimension = 0)
        : inputData(new std::vector<P>(in))
        , childNegative(nullptr)
        , childPositive(nullptr)
        , data(nullptr)
        , dim(dimension % P::dimensions())
    {
      throw_if_input_empty();
    }

    LazyKdTree(LazyKdTree&&) = default;

    ///@todo maybe write impl in the future (also implement for strict version then)
    LazyKdTree(LazyKdTree const&) = delete;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

private:
    inline void throw_if_input_empty() const
    {
      if (!inputData || inputData->size() == 0)
        throw std::logic_error("LazyKdTree can't be constructed from empty inputs");
    }

    inline bool is_leaf() const
    {
        return !childNegative && !childPositive;
    }

    inline bool is_evaluated() const
    {
        return inputData == nullptr;
    }

    void ensure_evaluated()
    {
        if (is_evaluated())
            return;

        if (inputData->size() == 1) {
            data = std::unique_ptr<P>(new P(std::move((*inputData.get())[0])));
            inputData.reset();
        }

        else if (inputData->size() > 1) {
            const size_t median = inputData->size() / 2;
            std::vector<P> inputNegative, inputPositive;

            inputNegative.reserve(median - 1);
            inputPositive.reserve(median - 1);

            median_dimension_sort(*inputData.get(), dim);

            for (size_t i = 0; i < inputData->size(); ++i) {
                if (i < median)
                    inputNegative.push_back(std::move((*(inputData.get()))[i]));
                else if (i > median)
                    inputPositive.push_back(std::move((*(inputData.get()))[i]));
            }

            data = std::unique_ptr<P>(new P(std::move((*inputData.get())[median])));

            inputData.reset();

            if (inputNegative.size() > 0)
                childNegative = std::unique_ptr<LazyKdTree>(
                    new LazyKdTree(std::move(inputNegative), dim + 1));
            if (inputPositive.size() > 0)
                childPositive = std::unique_ptr<LazyKdTree>(
                    new LazyKdTree(std::move(inputPositive), dim + 1));
        }
    }

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

public:

    void ensure_evaluated_fully()
    {
        ensure_evaluated();
        if (childNegative)
            childNegative->ensure_evaluated_fully();
        if (childPositive)
            childPositive->ensure_evaluated_fully();
    }

//------------------------------------------------------------------------------

    P nearest(P const& search)
    {
        ensure_evaluated();

        if (is_leaf())
            return *data.get(); // reached the end, return current value

        const auto comp = dimension_compare(search, *data.get(), dim);

        P best; // nearest neighbor of search

        if (comp == NEGATIVE && childNegative)
            best = childNegative->nearest(search);
        else if (comp == POSITIVE && childPositive)
            best = childPositive->nearest(search);
        else
            return *data.get();

        double sqrDistanceBest = square_dist(search, best);
        const double sqrDistanceThis = square_dist(search, *data.get());

        if (sqrDistanceThis < sqrDistanceBest) {
            sqrDistanceBest = sqrDistanceThis;
            best = *data.get(); // make this value the best if it is closer than the
            // checked side
            // type
        }

        // check whether other side might have candidates as well
        const double distanceBest   = std::sqrt(sqrDistanceBest);
        const double borderNegative = search[dim] - distanceBest;
        const double borderPositive = search[dim] + distanceBest;

        // check whether distances to other side are smaller than currently best
        // and recurse into the "wrong" direction, to check for possibly additional
        // candidates
        if (comp == NEGATIVE && childPositive) {
            if (borderPositive >= (*data.get())[dim]) {
                const auto otherBest = childPositive->nearest(search);
                if (square_dist(search, otherBest) < square_dist(search, best))
                    best = otherBest;
            }
        } else if (comp == POSITIVE && childNegative) {
            if (borderNegative <= (*data.get())[dim]) {
                const auto otherBest = childNegative->nearest(search);
                if (square_dist(search, otherBest) < square_dist(search, best))
                    best = otherBest;
            }
        }

        return best;
    }

//------------------------------------------------------------------------------

    std::vector<P> k_nearest(P const& search, size_t n)
    {
        ensure_evaluated();

        if (n < 1)     return std::vector<P>(); // no real search if n < 1
        if (is_leaf()) return std::vector<P>{ *(data.get()) }; // no further recursion, return current value

        auto res = std::vector<P>();

        // decide which side to check and recurse into it
        const auto comp = dimension_compare(search, *(data.get()), dim);

        if (comp == NEGATIVE) {
            if (childNegative) {
                move_append(childNegative->k_nearest(search, n), res);
                sort_and_limit(res, search, n);
            }
        } else if (childPositive) {
            move_append(childPositive->k_nearest(search, n), res);
            sort_and_limit(res, search, n);
        }

        // check whether other side might have candidates as well
        const double distanceBest   = res.size() > 0
                                    ? std::sqrt(square_dist(search, res.back()))
                                    : 0;
        const double borderNegative = search[dim] - distanceBest;
        const double borderPositive = search[dim] + distanceBest;

        // check whether distances to other side are smaller than currently worst
        // candidate
        // and recurse into the "wrong" direction, to check for possibly additional
        // candidates
        if (comp == NEGATIVE && childPositive) {
            if (res.size() < n || borderPositive >= (*data.get())[dim]) {
                move_append(childPositive->k_nearest(search, n), res);
                sort_and_limit(res, search, n);
            }
        } else if (comp == POSITIVE && childNegative) {
            if (res.size() < n || borderNegative <= (*data.get())[dim]) {
                move_append(childNegative->k_nearest(search, n), res);
                sort_and_limit(res, search, n);
            }
        }

        if (res.size() < n || square_dist(search, *(data.get())) < square_dist(search, res.back())) {
            res.push_back(*(data.get())); // add current node if there is still room
            sort_and_limit(res, search, n);
        }

        return res;
    }

//------------------------------------------------------------------------------

    std::vector<P> in_hypersphere(P const& search, double radius)
    {
        ensure_evaluated();

        if (radius <= 0.0) return std::vector<P>(); // no real search if radius <= 0

        std::vector<P> res; // all points within the sphere
        if (sqrt(square_dist(search, *(data.get()))) <= radius)
            res.push_back(*(data.get())); // add current node if it is within the search radius

        if (is_leaf()) return res; // no children, return result

        // decide which side to check and recurse into it
        const auto comp = dimension_compare(search, *(data.get()), dim);
        if (comp == NEGATIVE) {
            if (childNegative)
                move_append(childNegative->in_hypersphere(search, radius), res);
        } else if (childPositive) {
            move_append(childPositive->in_hypersphere(search, radius), res);
        }

        const double borderNegative = search[dim] - radius;
        const double borderPositive = search[dim] + radius;

        // check whether distances to other side are smaller than radius
        // and recurse into the "wrong" direction, to check for possibly aditional
        // candidates
        if (comp == NEGATIVE && childPositive) {
            if (borderPositive >= (*data.get())[dim])
                move_append(childPositive->in_hypersphere(search, radius), res);
        } else if (comp == POSITIVE && childNegative) {
            if (borderNegative <= (*data.get())[dim])
                move_append(childNegative->in_hypersphere(search, radius), res);
        }

        return res;
    }

//------------------------------------------------------------------------------

    std::vector<P> in_box(P const& search, P const& sizes)
    {
        ensure_evaluated();

        for (size_t i = 0; i < P::dimensions(); ++i) {
            if (sizes[i] <= 0.0)
                return std::vector<P>(); // no real search if one dimension size is <= 0
        }

        std::vector<P> res; // all points within the box

        bool inBox = true;
        for (size_t i = 0; i < P::dimensions() && inBox; ++i)
            inBox = dimension_dist(search, *(data.get()), i) <= 0.5 * sizes[i];

        if (inBox) res.push_back(*(data.get()));

        if (is_leaf()) return res; // no children, return result

        // decide which side to check and recurse into it
        const auto comp = dimension_compare(search, *(data.get()), dim);
        if (comp == NEGATIVE) {
            if (childNegative)
                move_append(childNegative->in_box(search, sizes), res);
        } else if (childPositive) {
            move_append(childPositive->in_box(search, sizes), res);
        }

        const double borderNegative = search[dim] - 0.5 * sizes[dim];
        const double borderPositive = search[dim] + 0.5 * sizes[dim];

        // check whether distances to other side are smaller than radius
        // and recurse into the "wrong" direction, to check for possibly additional
        // candidates
        if (comp == NEGATIVE && childPositive) {
            if (borderPositive >= (*data.get())[dim])
                move_append(childPositive->in_box(search, sizes), res);
        } else if (comp == POSITIVE && childNegative) {
            if (borderNegative <= (*data.get())[dim])
                move_append(childNegative->in_box(search, sizes), res);
        }

        return res;
    }

//------------------------------------------------------------------------------

    size_t size() const
    {
        if (!is_evaluated())
            return inputData->size();

        size_t result = 1;
        if (childNegative)
            result += childNegative->size();
        if (childPositive)
            result += childPositive->size();
        return result;
    }

private:

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

    static inline double square_dist(P const& p1, P const& p2)
    {
        double sqrDist(0);
        const auto nDims = P::dimensions();

        for (size_t i = 0; i < nDims; ++i)
            sqrDist += pow(p1[i] - p2[i], 2);

        return sqrDist;
    }

    static inline double dimension_dist(P const& p1, P const& p2, size_t dim)
    {
        return std::fabs(p1[dim] - p2[dim]);
    }

    static inline void median_dimension_sort(std::vector<P>& pts, size_t dim)
    {
        std::nth_element(pts.begin(), pts.begin() + pts.size() / 2, pts.end(),
            [dim](P const& lhs, P const& rhs) { return lhs[dim] < rhs[dim]; });
    }

    static inline Compare dimension_compare(P const& lhs, P const& rhs,
        size_t dim)
    {
        if (lhs[dim] <= rhs[dim])
            return NEGATIVE;

        return POSITIVE;
    }

    static inline void sort_and_limit(std::vector<P>& target, P const& search,
        size_t maxSize)
    {
        if (target.size() > maxSize) {
            std::partial_sort(target.begin(), target.begin() + maxSize, target.end(),
                [&search](P const& a, P const& b) {
                    return square_dist(search, a) < square_dist(search, b);
                });
            remove_from(target, maxSize);
        }
    }

    static inline void move_append(std::vector<P>&& from, std::vector<P>& to)
    {
        if (to.empty()) {
            to = std::move(from);
            return;
        }

        to.reserve(to.size() + from.size());
        to.insert(
            std::end(to),
            std::make_move_iterator(std::begin(from)),
            std::make_move_iterator(std::end(from)));
    }

    static inline void remove_from(std::vector<P>& x, size_t index)
    {
        if (x.size() < index)
            return;
        x.erase(x.begin() + index, x.end());
    }
};

///@todo maybe own file (or rename this file to KdTree)
template <typename P>
class StrictKdTree {
private:
    mutable LazyKdTree<P> lkd;

public:
    StrictKdTree(std::vector<P>&& in)
        : lkd(std::move(in), 0)
    {
        lkd.ensure_evaluated_fully();
    }

    StrictKdTree(std::vector<P> const& in)
        : lkd(in, 0)
    {
        lkd.ensure_evaluated_fully();
    }


    StrictKdTree(LazyKdTree<P>&& in)
        : lkd(std::move(in))
    {
        lkd.ensure_evaluated_fully();
    }

    inline P nearest(P const& search) const
    {
        return lkd.nearest(search);
    }

    inline std::vector<P> k_nearest(P const& search, size_t n) const
    {
        return lkd.k_nearest(search, n);
    }

    inline std::vector<P> in_hypersphere(P const& search, double radius) const
    {
        return lkd.in_hypersphere(search, radius);
    }

    inline std::vector<P> in_box(P const& search, P const& sizes) const
    {
        return lkd.in_box(search, sizes);
    }

    inline size_t size() const
    {
        return lkd.size();
    }
};

}

#endif // KDTREE_H
