/**
***************************************************************************
* @file brick/computerVision/keypointMatcherFast.cc
*
* Source file defining a class template for matching keypoints using
* Rosten's 2005 FAST keypoint matching algorithm.
*
* Copyright (C) 2011 David LaRose, dlr@cs.cmu.edu
* See accompanying file, LICENSE.TXT, for details.
*
***************************************************************************
*/

#include <brick/computerVision/keypointMatcherFast.hh>

namespace brick {

  namespace computerVision {

    // Default constructor.
    KeypointMatcherFast::
    KeypointMatcherFast()
      : m_keypointMap()
    {
      // Empty.
    }

      
    // Destructor.
    virtual
    KeypointMatcherFast::
    ~KeypointMatcherFast()
    {
      // Empty.
    }


    bool
    matchKeypoint(KeypointFast const& query, KeypointFast& bestMatch) const
    {
      // Start by finding the keypoint who's feature vector mean is
      // closest to that of the query point.  This is a good starting
      // point for a linear search.
      typedef std::map<double, KeypointFast>::const_iterator MapIterator;
      featureVectorMean = this->computeFeatureVectorMean(query);
      MapIterator startIter = m_keypointMap.lower_bound(featureVectorMean);
      
      // Now search forward until we know for sure we're not going to
      // find a better match.  We'll know we've gone far enough when
      // the difference in feature vector means is big enough to
      // guarantee that the new SSD between feature vectors is larger
      // than our best so far.
      common::Int64 bestSSD = std::numeric_limits<common::Int64>::max();
      MapIterator currentIter = startIter;
      while(currentIter != m_keypointMap.end()) {
        double differenceInMeans = featureVectorMean - currentIter->first;
        double boundOnNewSSD = (differenceInMeans * differenceInMeans
                                * KeypointFast::numberOfFeatures);
        if(boundOnNewSSD >= bestSSDSoFar) {
          break;
        }
        double newSSD = this->computeSSD(query, *currentIter);
        if(newSSD < bestSSDSoFar) {
          bestSSDSoFar = newSSD;
          bestMatch = *currentIter;
        }
        ++currentIter;
      }

      // Do the search again, but this time go backward, toward the
      // beginning of the map.
      currentIter = startIter;
      while(currentIter != m_keypointMap.begin()) {
        double differenceInMeans = featureVectorMean - currentIter->first;
        double boundOnNewSSD = (differenceInMeans * differenceInMeans
                                * KeypointFast::numberOfFeatures);
        if(boundOnNewSSD >= bestSSDSoFar) {
          break;
        }
        double newSSD = this->computeSSD(query, *currentIter);
        if(newSSD < bestSSDSoFar) {
          bestSSDSoFar = newSSD;
          bestMatch = *currentIter;
        }
        --currentIter;
      }
      return true;
    }


    template <class Iter>
    void
    KeypointMatcherFast::
    setKeypoints(Iter sequenceBegin, Iter sequenceEnd)
    {
      m_keypointMap.clear();
      while(sequenceBegin != sequenceEnd) {
        double featureVectorMean = this->computerFeatureVectorMean(
          *sequenceBegin);
        m_keypointMap.insert(std::make_pair(featureVectorMean, *sequenceBegin));
      }
    }


    double
    KeypointMatcherFast::
    computeFeatureVectorMean(KeypointFast const& keypoint)
    {
      return std::accumulate(
        &(sequenceBegin->featureVector[0]),
        &(sequenceBegin->featureVector[0]) + KeypointFast::numberOfFeatures,
        double(0.0)) / KeypointFast::numberOfFeatures;
    }

    
    double
    KeypointMatcherFast::
    computeSSD(KeypointFast const& keypoint0, KeypointFast const& keypoint1)
    {
      double ssd = 0.0;
      for(unsigned int ii = 0; ii < KeypointFast::numberOfFeatures; ++ii) {
        double difference = (keypoint1.featureVector[ii]
                             - keypoint0.featureVector[ii]);
        ssd += difference * difference;
      }
      return ssd;
    }
    
  } // namespace computerVision
  
} // namespace brick

#endif /* #ifndef BRICK_COMPUTERVISION_KEYPOINTMATCHERFAST_HH */
