///@todo copyright

#ifndef LAZYKDTREE_H
#define LAZYKDTREE_H

#include <memory>
#include <vector>

namespace lazyTrees {

//------------------------------------------------------------------------------


// P must implement static size_t dimensions() returning number of dimensions
// P also must be const random-accessable for up to [dimensions() - 1] returning
// the X / Y / Z / ... coordinate of the point

///@todo possibility to transform to a strict tree, to make reading access const
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
        if (inputData->size() == 0)
            throw std::logic_error(
                "LazyKdTree can't be constructed from empty inputs");
    }

    LazyKdTree(std::vector<P> const& in, int dimension = 0)
        : inputData(new std::vector<P>(in))
        , childNegative(nullptr)
        , childPositive(nullptr)
        , data(nullptr)
        , dim(dimension % P::dimensions())
    {
        if (inputData->size() == 0)
            throw std::logic_error(
                "LazyKdTree can't be constructed from empty inputs");
    }

    ///@todo enable some of these depending on what's required
    LazyKdTree(LazyKdTree const&) = delete;
    LazyKdTree(LazyKdTree&&) = delete;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

private:
    inline bool is_leaf() const
    {
        return !childNegative && !childPositive;
    }

    inline bool is_evaluated() const ///@todo rename to build or similar?
    {
        return inputData == nullptr;
    }

    void evaluate()
    {
        if (is_evaluated())
            return;
        // consume inputData
        // assign values to children etc.

        if (inputData->size() == 1) {
            data = std::unique_ptr<P>(new P(std::move((*inputData.get())[0])));
            inputData.reset();
        }

        else if (inputData->size() > 1) {
            size_t median = inputData->size() / 2;
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

    void evaluate_recursive() ///@todo ly
    {
        evaluate();
        if (childNegative)
            childNegative->evaluate_recursive();
        if (childPositive)
            childPositive->evaluate_recursive();
    }

//------------------------------------------------------------------------------

    P nearest(P const& search)
    {
        evaluate(); ///@todo rename, since this only evaluates if not yet

        if (is_leaf())
            return *data.get(); // reached the end, return current value

        auto comp = dimension_compare(search, *data.get(), dim);

        P best; // nearest neighbor of search ///@todo could be reference

        if (comp == NEGATIVE && childNegative)
            best = childNegative->nearest(search);
        else if (comp == POSITIVE && childPositive)
            best = childPositive->nearest(search);
        else
            return *data.get();

        double sqrDistanceBest = square_dist(search, best);
        double sqrDistanceThis = square_dist(search, *data.get());

        if (sqrDistanceThis < sqrDistanceBest) {
            sqrDistanceBest = sqrDistanceThis;
            best = *data.get(); // make this value the best if it is closer than the
            // checked side ///@todo best could be a reference
            // type
        }

        // check whether other side might have candidates as well
        double borderNegative = search[dim] - sqrt(sqrDistanceBest);
        double borderPositive = search[dim] + sqrt(sqrDistanceBest);
        P otherBest;

        // check whether distances to other side are smaller than currently best
        // and recurse into the "wrong" direction, to check for possibly additional
        // candidates
        if (comp == NEGATIVE && childPositive) {
            if (borderPositive >= (*data.get())[dim]) {
                otherBest = childPositive->nearest(search);
                if (square_dist(search, otherBest) < square_dist(search, best))
                    best = otherBest;
            }
        } else if (comp == POSITIVE && childNegative) {
            if (borderNegative <= (*data.get())[dim]) {
                otherBest = childNegative->nearest(search);
                if (square_dist(search, otherBest) < square_dist(search, best))
                    best = otherBest;
            }
        }

        return best;
    }

//------------------------------------------------------------------------------

    std::vector<P> k_nearest(P const& search, size_t n)
    {
        evaluate(); ///@todo rename, since this only evaluates if not yet

        if (n < 1)
            return std::vector<P>(); // no real search if n < 1
        if (is_leaf())
            return std::vector<P>{ *(
                data.get()) }; // no further recursion, return current value

        auto res = std::vector<P>();
        if (res.size() < n || square_dist(search, *(data.get())) < square_dist(search, res.back()))
            res.push_back(*(data.get())); // add current node if there is still room
        // or if it is closer than the currently
        // worst candidate

        // decide which side to check and recurse into it
        auto comp = dimension_compare(search, *(data.get()), dim);

        if (comp == NEGATIVE) {
            if (childNegative)
                move_append(childNegative->k_nearest(search, n), res);
        } else if (childPositive) {
            move_append(childPositive->k_nearest(search, n), res);
        }

        // only keep the required number of candidates and sort them by distance
        sort_and_limit(res, search, n);

        // check whether other side might have candidates aswell
        double distanceBest = square_dist(search, res.back());
        double borderNegative = search[dim] - distanceBest;
        double borderPositive = search[dim] + distanceBest;

        // check whether distances to other side are smaller than currently worst
        // candidate
        // and recurse into the "wrong" direction, to check for possibly additional
        // candidates
        if (comp == NEGATIVE && childPositive) {
            if (res.size() < n || borderPositive >= (*data.get())[dim])
                move_append(childPositive->k_nearest(search, n), res);
        } else if (comp == POSITIVE && childNegative) {
            if (res.size() < n || borderNegative <= (*data.get())[dim])
                move_append(childNegative->k_nearest(search, n), res);
        }

        sort_and_limit(res, search, n);
        return res;
    }

//------------------------------------------------------------------------------

    std::vector<P> in_hypersphere(P const& search, double radius)
    {
        evaluate(); ///@todo rename, since this only evaluates if not yet

        if (radius <= 0.0)
            return std::vector<P>(); // no real search if radius <= 0

        std::vector<P> res; // all points within the sphere
        if (sqrt(square_dist(search, *(data.get()))) <= radius)
            res.push_back(
                *(data.get())); // add current node if it is within the search radius

        if (is_leaf())
            return res; // no children, return result

        // decide which side to check and recurse into it
        auto comp = dimension_compare(search, *(data.get()), dim);
        if (comp == NEGATIVE) {
            if (childNegative)
                move_append(childNegative->in_hypersphere(search, radius), res);
        } else if (childPositive) {
            move_append(childPositive->in_hypersphere(search, radius), res);
        }

        double borderNegative = search[dim] - radius;
        double borderPositive = search[dim] + radius;

        // check whether distances to other side are smaller than radius
        // and recurse into the "wrong" direction, to check for possibly additional
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
        evaluate(); ///@todo rename, since this only evaluates if not yet

        for (size_t i = 0; i < P::dimensions(); ++i) {
            if (sizes[i] <= 0.0)
                return std::vector<P>(); // no real search if one dimension size is <=
            // 0
        }

        std::vector<P> res; // all points within the box

        bool inBox = true;
        for (size_t i = 0; i < P::dimensions() && inBox; ++i)
            inBox = dimension_dist(search, *(data.get()), i) <= 0.5 * sizes[i];

        if (inBox)
            res.push_back(*(data.get()));

        if (is_leaf())
            return res; // no children, return result

        // decide which side to check and recurse into it
        auto comp = dimension_compare(search, *(data.get()), dim);
        if (comp == NEGATIVE) {
            if (childNegative)
                move_append(childNegative->in_box(search, sizes), res);
        } else if (childPositive) {
            move_append(childPositive->in_box(search, sizes), res);
        }

        double borderNegative = search[dim] - 0.5 * sizes[dim];
        double borderPositive = search[dim] + 0.5 * sizes[dim];

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
        double sqrDist = 0;
        auto nDims = P::dimensions();

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
            [&pts, dim](P lhs, P rhs) { return lhs[dim] < rhs[dim]; });
    }

    static inline Compare dimension_compare(P const& lhs, P const& rhs,
        size_t dim)
    {
        if (lhs[dim] <= rhs[dim])
            return NEGATIVE;
        else
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
        if (to.empty())
            to = std::move(from);
        else {
            to.reserve(to.size() + from.size());
            std::move(std::begin(from), std::end(from), std::back_inserter(to));
        }
    }

    static inline void remove_from(std::vector<P>& x, size_t index)
    {
        if (x.size() < index)
            return;
        x.erase(x.begin() + index, x.end());
    }
};

///@todo utest and maybe own file (or rename this file to KdTree)
template <typename P>
class StrictKdTree {
private:
    mutable LazyKdTree<P> lkd;

public:
    StrictKdTree(std::vector<P>&& in)
        : lkd(std::move(in), 0)
    {
        lkd.evaluate_recursive();
    }

    StrictKdTree(std::vector<P> const& in)
        : lkd(in, 0)
    {
        lkd.evaluate_recursive();
    }


    StrictKdTree(LazyKdTree<P>&& in)
        : lkd(std::move(in))
    {
        lkd.evaluate_recursive();
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

#endif // LAZYKDTREE_H
