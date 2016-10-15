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
        return inputData != nullptr;
    }

    void evaluate()
    {
        if (is_evaluated()) return;
        //consume inputData
        //assign values to children etc.

        if(inputData->size() == 1)
        {
            data = std::unique_ptr<P>(std::move(inputData[0]));
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
                    left.push_back(std::move(*inputData.get()[i]));
                else if(i > median)
                    right.push_back(std::move(*inputData.get()[i]));
            }

            data = std::unique_ptr<P>(std::move(*inputData.get()[median]));

            inputData.reset();

            if(left.size() > 0)  childNegative = std::unique_ptr<LazyKdTree>(new LazyKdTree(std::move(left),  dim+1));
            if(right.size() > 0) childPositive = std::unique_ptr<LazyKdTree>(new LazyKdTree(std::move(right), dim+1));
        }
    }

    void evaluate_recursive()
    {
        evaluate();
        if (childNegative) childNegative->evaluate_recursive();
        if (childPositive) childPositive->evaluate_recursive();
    }


//------------------------------------------------------------------------------

public:
    P nearest(P const& search) const ///@todo must evaluate ///@todo error case when data == nullptr
    {

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




    std::vector<P> k_nearest(P const& search, size_t n) const ///@todo must evaluate
    {
        if(n < 1) return std::vector<P>(); //no real search if n < 1
        if(is_leaf()) return std::vector<P>{data}; //no further recursion, return current value

        std::vector<P> res();
        if(res.size() < n || square_dist(search, data) < square_dist(search, res.last()))
            res.push_back(data); //add current node if there is still room or if it is closer than the currently worst candidate

        //decide which side to check and recurse into it
        auto comp = dimension_compare(search, data, dim);

        if(comp == LEFT)
        {
            if(childNegative)
                res.push_back(childNegative->k_nearest(search, n)); ///@todo merging of two vectors
        }
        else if(childPositive)
        {
            res.push_back(childPositive->k_nearest(search, n)); ///@todo merging of two vectors
        }

        //only keep the required number of candidates and sort them by distance
        sort_and_limit(res, search, n);

        //check whether other side might have candidates aswell
        double distanceBest = square_dist(search, res.last());
        double borderLeft 	= search[dim] - distanceBest;
        double borderRight 	= search[dim] + distanceBest;

        //check whether distances to other side are smaller than currently worst candidate
        //and recurse into the "wrong" direction, to check for possibly additional candidates
        if(comp == LEFT && childPositive)
        {
            if(res.size() < n || borderRight >= (*data.get())[dim])
                res.push_back(childPositive->k_nearest(search, n)); ///@todo merging of vectors
        }
        else if (comp == RIGHT && childNegative)
        {
            if(res.size() < n || borderLeft <= (*data.get())[dim])
                res.push_back(childNegative->k_nearest(search, n)); ///@todo merging of vectors
        }

        sort_and_limit(res, search, n);
        return res;
    }

    std::vector<P> in_circle(P const& search, double radius) const ///@todo rename, todo must evaluate beforehand
    {
        if(radius <= 0.0) return std::vector<P>(); //no real search if radius <= 0

        std::vector<P> res; //all points within the sphere
        if(sqrt(square_dist(search, data)) <= radius)
            res.push_back(data); //add current node if it is within the search radius

        if(is_leaf()) return res; //no children, return result

        //decide which side to check and recurse into it
        auto comp = dimension_compare(search, data, dim);
        if(comp == LEFT) {
            if(childNegative) res.push_back(childNegative->in_circle(search, radius)); ///@todo merging of vectors
        } else if(childPositive) {
            res.push_back(childPositive->in_circle(search, radius)); ///@todo merging of vectors
        }

        double borderLeft  = search[dim] - radius;
        double borderRight = search[dim] + radius;

        //check whether distances to other side are smaller than radius
        //and recurse into the "wrong" direction, to check for possibly additional candidates
        if(comp == LEFT && childPositive) {
            if(borderRight >= (*data.get())[dim])
                res.push_back(childPositive->in_circle(search, radius)); ///@todo merging of vectors
        }
        else if (comp == RIGHT && childNegative) {
            if(borderLeft <= (*data.get())[dim])
                res.push_back(childNegative->in_circle(search, radius)); ///@todo merging of vectors
        }

        return res;
    }

    std::vector<P> in_box(P const& search, P const& sizes) const ///@todo rename, @todo must evaluate beforehand
    {
        for (size_t i = 0; i < P::dimensions(); ++i)
        {
            if (sizes[i] <= 0.0)
                return std::vector<P>(); //no real search if one dimension size is <= 0
        }

        std::vector<P> res; //all points within the box

        bool inBox = true;
        for (size_t i = 0; i < P::dimensions() && inBox; ++i)
            inBox = dimension_dist(search, data, i) <= 0.5 * sizes[i];

        if (inBox) res.push_back(data);

        if(is_leaf()) return res; //no children, return result

        //decide which side to check and recurse into it
        auto comp = dimension_compare(search, data, dim);
        if(comp == LEFT) {
            if(childNegative) res.push_back(childNegative->in_box(search, sizes)); ///@todo merging of vectors
        } else if(childPositive) {
            res.push_back(childPositive->in_box(search, sizes)); ///@todo merging of vectors
        }

        double borderLeft 	= search[dim] - 0.5 * sizes[dim];
        double borderRight 	= search[dim] + 0.5 * sizes[dim];

        //check whether distances to other side are smaller than radius
        //and recurse into the "wrong" direction, to check for possibly additional candidates
        if(comp == LEFT && childPositive) {
            if(borderRight >= (*data.get())[dim])
                res.push_back(childPositive->in_box(search, sizes)); ///@todo merging of vectors
        }
        else if (comp == RIGHT && childNegative) {
            if(borderLeft <= (*data.get())[dim])
                res.push_back(childNegative->in_box(search, sizes)); ///@todo merging of vectors
        }

        return res;
    }






private: ///@todo many public/ private switches, cleanup!

    inline bool is_leaf() const {
        return !childNegative && !childPositive;
    }


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
        std::nth_element(pts->begin(), pts->begin() + pts.size()/2, pts.end(),
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
            target.remove_from(maxSize);
        }
    }


};

}


#endif // LAZYKDTREE_H
