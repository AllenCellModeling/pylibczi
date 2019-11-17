#ifndef _PYLIBCZI_SUBBLOCKSORTER_H
#define _PYLIBCZI_SUBBLOCKSORTER_H

#include <utility>
#include <vector>

#include "inc_libCZI.h"
#include "constants.h"

namespace pylibczi{

  class SubblockSorter{
  protected:
    libCZI::CDimCoordinate m_planeCoordinate;
    int m_indexM;
    bool m_isMosaic;
  public:
      SubblockSorter(const libCZI::CDimCoordinate  *plane_, int index_m_, bool is_mosaic_=false)
      : m_planeCoordinate(plane_), m_indexM(index_m_), m_isMosaic(is_mosaic_) {}

      const libCZI::CDimCoordinate* coordinatePtr() const { return &m_planeCoordinate; }

      int mIndex() const { return m_indexM; }

      std::map<char, int>  getDimsAsChars() const {
          return SubblockSorter::getValidIndexes(m_planeCoordinate, m_indexM, m_isMosaic);
      }

      static std::map<char, int> getValidIndexes(const libCZI::CDimCoordinate& planecoord_, int index_m_, bool is_mosaic_=false)
      {
          std::map<char, int> ans;
          for (auto di : Constants::s_sortOrder) {
              int value;
              if (planecoord_.TryGetPosition(di, &value)) ans.emplace(libCZI::Utils::DimensionToChar(di), value);
          }
          if (is_mosaic_) ans.emplace('M', index_m_);
          return ans;
      }

      std::map<char, int> getValidIndexes(bool is_mosaic_ = false) const {
          return SubblockSorter::getValidIndexes(m_planeCoordinate, m_indexM, is_mosaic_);
      }

      bool operator<(const SubblockSorter& other_) const {
          if(!m_isMosaic) return SubblockSorter::aLessThanB(m_planeCoordinate, other_.m_planeCoordinate);
          return SubblockSorter::aLessThanB(m_planeCoordinate, m_indexM, other_.m_planeCoordinate, other_.m_indexM);
      }

      bool operator==(const SubblockSorter &other_) const {
          return !(*this < other_) && !(other_ < *this);
      }

      static bool aLessThanB(const libCZI::CDimCoordinate &a_, const libCZI::CDimCoordinate &b_){
          for(auto di : Constants::s_sortOrder){
              int aValue, bValue;
              if(a_.TryGetPosition(di, &aValue) && b_.TryGetPosition(di, &bValue) &&  aValue != bValue)
                  return aValue < bValue;
          }
          return false;
      }

      static bool aLessThanB(const libCZI::CDimCoordinate &a_, const int a_index_m_, const libCZI::CDimCoordinate &b_, const int b_index_m_){
          if( !aLessThanB(a_, b_) && !aLessThanB(b_, a_) )
              return a_index_m_ < b_index_m_;
          return aLessThanB(a_, b_);
      }
  };
}

#endif //_PYLIBCZI_SUBBLOCKSORTER_H
