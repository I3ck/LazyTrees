///@todo copyright

#ifndef LAZYKDTREE_H
#define LAZYKDTREE_H

#include <vector>
#include <memory>

namespace lazyTrees {

//------------------------------------------------------------------------------

//P must implement static size_t dimensions() returning number of dimensions
//P also must be const random-accessable for up to [dimensions() - 1] returning the X / Y / Z / ... coordinate of the point

///@todo own project
///@todo possibility to transform to a strict tree, to make reading access const
///@todo fix intendation etc. (editor hates me)
///@todo base class which basically is the lazy version but assumes that the tree is already evaluated (everything protected)
///@todo the lazy version dann then use these implementations calling evaluate beforehand, while the strict one can directly use them and just offer public wrappers
template<typename P>
class LazyKdTree
{
private:

    enum Compare {LEFT, RIGHT}; ///@todo positive negative (remove all left/right code)

    std::unique_ptr<std::vector<P>>
        inputData;

    std::unique_ptr<LazyKdTree>
        childNegative,
        childPositive;

    std::unique_ptr<P>
        data;

    const size_t dim;

public:
    LazyKdTree(std::vector<P>&& in, int dimension = 0) : ///@todo maybe should throw if in.size() < 1
        inputData(new std::vector<P>(std::move(in))),
        childNegative(nullptr),
        childPositive(nullptr),
        data(nullptr),
        dim(dimension % P::dimensions())
    {}

    ///@todo enable some of these depending on what's required
    LazyKdTree(LazyKdTree const&) = delete;
    LazyKdTree(LazyKdTree&&)      = delete;

private:

    bool is_evaluated() const ///@todo rename to build or similar?
    {
        return inputData == nullptr;
    }

    void evaluate()
    {
        if (is_evaluated()) return;
        //consume inputData
        //assign values to children etc.

        if(inputData->size() == 1)
        {
            data = std::unique_ptr<P>(new P(std::move((*inputData.get())[0])));
            inputData.reset();
        }

        else if(inputData->size() > 1)
        {
            size_t median = inputData->size() / 2;
            std::vector<P> left, right; ///@todo rename

            left.reserve(median - 1);
            right.reserve(median - 1);

            median_dimension_sort(*inputData.get(), dim);

            for(size_t i = 0; i < inputData->size(); ++i)
            {
                if(i < median)
                    left.push_back(std::move((*(inputData.get()))[i]));
                else if(i > median)
                    right.push_back(std::move((*(inputData.get()))[i]));
            }

            data = std::unique_ptr<P>(new P(std::move((*inputData.get())[median])));

            inputData.reset();

            if(left.size() > 0)  childNegative = std::unique_ptr<LazyKdTree>(new LazyKdTree(std::move(left),  dim+1));
            if(right.size() > 0) childPositive = std::unique_ptr<LazyKdTree>(new LazyKdTree(std::move(right), dim+1));
        }
    }
public: ///@todo let evaluate recursive be public?
    void evaluate_recursive() ///@todo ly
    {
        evaluate();
        if (childNegative) childNegative->evaluate_recursive();
        if (childPositive) childPositive->evaluate_recursive();
    }


//------------------------------------------------------------------------------

public:
    P nearest(P const& search) ///@todo error case when data == nullptr
    {
        evaluate(); ///@todo rename, since this only evaluates if not yet

    if(is_leaf())
        return *data.get(); //reached the end, return current value

    auto comp = dimension_compare(search, *data.get() , dim);

    P best; //nearest neighbor of search ///@todo could be reference

    if(comp == LEFT && childNegative)
        best = childNegative->nearest(search);
    else if(comp == RIGHT && childPositive)
        best = childPositive->nearest(search);
    else
        return *data.get();

    double sqrDistanceBest = square_dist(search, best);
    double sqrDistanceThis = square_dist(search, *data.get());

    if(sqrDistanceThis < sqrDistanceBest)
    {
        sqrDistanceBest = sqrDistanceThis;
        best = *data.get(); //make this value the best if it is closer than the checked side ///@todo best could be a reference type
    }

    //check whether other side might have candidates as well
    ///@todo can stay squared?
    double borderLeft 	= search[dim] - sqrt(sqrDistanceBest);
    double borderRight 	= search[dim] + sqrt(sqrDistanceBest);
    P otherBest;

    //check whether distances to other side are smaller than currently best
    //and recurse into the "wrong" direction, to check for possibly additional candidates
    if(comp == LEFT && childPositive)
    {
        if(borderRight >= (*data.get())[dim])
        {
            otherBest = childPositive->nearest(search);
            if(square_dist(search, otherBest) < square_dist(search, best))
                best = otherBest;
        }
    }
    else if (comp == RIGHT && childNegative)
    {
        if(borderLeft <= (*data.get())[dim])
        {
            otherBest = childNegative->nearest(search);
            if(square_dist(search, otherBest) < square_dist(search, best))
                best = otherBest;
        }
    }

    return best;
}




    std::vector<P> k_nearest(P const& search, size_t n)
    {
        evaluate(); ///@todo rename, since this only evaluates if not yet

        if(n < 1) return std::vector<P>(); //no real search if n < 1
        if(is_leaf()) return std::vector<P>{*(data.get())}; //no further recursion, return current value

        auto res = std::vector<P>();
        if(res.size() < n || square_dist(search, *(data.get())) < square_dist(search, res.back()))
            res.push_back(*(data.get())); //add current node if there is still room or if it is closer than the currently worst candidate

        //decide which side to check and recurse into it
        auto comp = dimension_compare(search, *(data.get()), dim);

        if(comp == LEFT)
        {
            if(childNegative)
                move_append(childNegative->k_nearest(search, n), res);
        }
        else if(childPositive)
        {
            move_append(childPositive->k_nearest(search, n), res);
        }

        //only keep the required number of candidates and sort them by distance
        sort_and_limit(res, search, n);

        //check whether other side might have candidates aswell
        double distanceBest = square_dist(search, res.back());
        double borderLeft 	= search[dim] - distanceBest;
        double borderRight 	= search[dim] + distanceBest;

        //check whether distances to other side are smaller than currently worst candidate
        //and recurse into the "wrong" direction, to check for possibly additional candidates
        if(comp == LEFT && childPositive)
        {
            if(res.size() < n || borderRight >= (*data.get())[dim])
                move_append(childPositive->k_nearest(search, n), res);
        }
        else if (comp == RIGHT && childNegative)
        {
            if(res.size() < n || borderLeft <= (*data.get())[dim])
                move_append(childNegative->k_nearest(search, n), res);
        }

        sort_and_limit(res, search, n);
        return res;
    }

    std::vector<P> in_circle(P const& search, double radius) ///@todo rename
    {
        evaluate(); ///@todo rename, since this only evaluates if not yet

        if(radius <= 0.0) return std::vector<P>(); //no real search if radius <= 0

        std::vector<P> res; //all points within the sphere
        if(sqrt(square_dist(search, *(data.get()))) <= radius)
            res.push_back(*(data.get())); //add current node if it is within the search radius

        if(is_leaf()) return res; //no children, return result

        //decide which side to check and recurse into it
        auto comp = dimension_compare(search, *(data.get()), dim);
        if(comp == LEFT) {
            if(childNegative) move_append(childNegative->in_circle(search, radius), res);
        } else if(childPositive) {
            move_append(childPositive->in_circle(search, radius), res);
        }

        double borderLeft  = search[dim] - radius;
        double borderRight = search[dim] + radius;

        //check whether distances to other side are smaller than radius
        //and recurse into the "wrong" direction, to check for possibly additional candidates
        if(comp == LEFT && childPositive) {
            if(borderRight >= (*data.get())[dim])
            move_append(childPositive->in_circle(search, radius), res);
        }
        else if (comp == RIGHT && childNegative) {
            if(borderLeft <= (*data.get())[dim])
                move_append(childNegative->in_circle(search, radius), res);
        }

        return res;
    }

    std::vector<P> in_box(P const& search, P const& sizes) ///@todo rename
    {
        evaluate(); ///@todo rename, since this only evaluates if not yet

        for (size_t i = 0; i < P::dimensions(); ++i)
        {
            if (sizes[i] <= 0.0)
                return std::vector<P>(); //no real search if one dimension size is <= 0
        }

        std::vector<P> res; //all points within the box

        bool inBox = true;
        for (size_t i = 0; i < P::dimensions() && inBox; ++i)
            inBox = dimension_dist(search, *(data.get()), i) <= 0.5 * sizes[i];

        if (inBox) res.push_back(*(data.get()));

        if(is_leaf()) return res; //no children, return result

        //decide which side to check and recurse into it
        auto comp = dimension_compare(search, *(data.get()), dim);
        if(comp == LEFT) {
            if(childNegative) move_append(childNegative->in_box(search, sizes), res);
        } else if(childPositive) {
            move_append(childPositive->in_box(search, sizes), res);
        }

        double borderLeft 	= search[dim] - 0.5 * sizes[dim];
        double borderRight 	= search[dim] + 0.5 * sizes[dim];

        //check whether distances to other side are smaller than radius
        //and recurse into the "wrong" direction, to check for possibly additional candidates
        if(comp == LEFT && childPositive) {
            if(borderRight >= (*data.get())[dim])
                move_append(childPositive->in_box(search, sizes), res);
        }
        else if (comp == RIGHT && childNegative) {
            if(borderLeft <= (*data.get())[dim])
                move_append(childNegative->in_box(search, sizes), res);
        }

        return res;
    }

    size_t size() const
    {
        if (!is_evaluated())
            return inputData->size();

        size_t result = 1;
        if (childNegative) result += childNegative->size();
        if (childPositive) result += childPositive->size();
        return result;
    }






private: ///@todo many public/ private switches, cleanup!

    inline bool is_leaf() const {
        return !childNegative && !childPositive;
    }


//------------------------------------------------------------------------------

///@todo many of these can be moved to helper later on

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
        std::nth_element(pts.begin(), pts.begin() + pts.size()/2, pts.end(),
                         [&pts, dim] (P lhs, P rhs){return lhs[dim] < rhs[dim]; });
    }

    static inline Compare dimension_compare(P const& lhs, P const& rhs, size_t dim)
    {
        if(lhs[dim] <= rhs[dim]) return LEFT;
        else return RIGHT;
    }

    static inline void sort_and_limit(std::vector<P> &target,P const& search, size_t maxSize)
    {
        if(target.size() > maxSize)
        {
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
        else
        {
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

}


#endif // LAZYKDTREE_H
