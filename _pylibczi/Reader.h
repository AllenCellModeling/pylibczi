#ifndef _PYLIBCZI_READER_H
#define _PYLIBCZI_READER_H

#include <cstdio>
#include <iostream>
#include <typeinfo>
#include <functional>
#include <vector>

#include "inc_libCZI.h"
#include "IndexMap.h"
#include "Image.h"
#include "SubblockSorter.h"
#include "SubblockMetaVec.h"

/*! \mainpage libCZI_c++_extension
 *
 * \section intro_sec Introduction
 *
 * This C++ extension wraps Zeiss's libCZI providing a minimal interface for working with CZI files.
 * There main class is Reader. Reader is constructed with a FileHolder class which is simply a class
 * that contains a FILE *.
 *
 * This is the introduction.
 *
 * \section install_sec Installation
 *
 * To install as a C++ package follow these steps
 * \subsection step1 Step 1:  git clone --recurse-submodules <github repo URL>
 * \subsection step2 Step 2:  cd cmake-build-debug
 * \subsection step3 Step 3:  cmake ../
 * \subsection step4 Step 4:  make
 * \subsection step5 Step 5:  include the headers and link against the library
 *
 * \code{.cpp}
 * #include <cstdio>
 * #include "Reader.h"
 *
 * using namespace pylibczi;
 *
 * int main(){
 *     std::string filename("yourczifile.czi");
 *     FILE *fp = fopen(filename.c_str(), "rb");
 *     FileHolder fholder(fp);
 *     auto czi = Reader(fholder);
 *     auto dims = czi.read_dims();
 *     // select some subset of dims
 *     auto c_dims = libCZI::CDimCoordinate{ { libCZI::DimensionIndex::B,0 },
 *                                           { libCZI::DimensionIndex::C,0 } };
 *     std::string xml = czi.read_meta(); // grab all the metadata as a string.
 *     if( !czi.isMosaic() )
 *
 *         auto ans = czi.read_selected(c_dims);
 *         auto imgs = ans.first;
 *         auto shape = ans.second;
 *         // imgs is a vector of ImageBC* which reference Image<T> objects;
 *     } else {
 *         auto img = czi.read_mosaic(c_dims);
 *         // img is a vector containing one ImageBC* pointing to an Image<T> which contains the whole mosaic image
 *     }
 * }
 *
 * \endcode
 */


using namespace std;

namespace pylibczi {

  /*!
   * @brief Reader class for ZISRAW / CZI files
   *
   * This class opens has the functionality to read mosaic files as well as single and multi-scene files.
   * It relies on libCZI from Zeiss and our intent is that this code will enable the reading of CZI files
   * generated by Zeiss systems. If you have a file that causes problems please contact us and share the
   * file if possible and we will do our best to enable support. Ideally make an issue on the github repo
   * https://github.com/elhuhdron/pylibczi
   */
  class Reader {
      std::shared_ptr<CCZIReader> m_czireader; // required for cast in libCZI
      libCZI::SubBlockStatistics m_statistics;
      std::vector< std::pair<SubblockSorter, int> > m_orderMapping;

  public:
      using SubblockIndexVec = std::vector< std::pair<SubblockSorter, int> >;
      using MapDiP = std::map<libCZI::DimensionIndex, std::pair<int, int> >;
      using Shape = std::vector<std::pair<char, int> >;
      /*!
       * @brief Construct the Reader and load the file statistics (dimensions etc)
       *
       * Example:
       * @code
       *    fp = std::fopen("my_czi_file.czi", "rb");      // #include <cstdio>
       *    czi = pylibczi::Reader(fp);
       * @endcode
       *
       * @param f_in_ A C-style FILE pointer to the CZI file
       */
      explicit Reader(std::shared_ptr<libCZI::IStream> istream_);

      /*!
       * @brief A convenience function for testing or use by C++ developers
       * @param file_name_ a wide character string such as L"my_filename.czi"
       */
      explicit Reader(const wchar_t* file_name_);

      /*!
       * @brief Check if the file is a mosaic file.
       *
       * This test is done by checking the maximum mIndex, mosaic files will have mIndex > 0
       *
       * @return True if the file is a mosaic file, false if it is not.
       */
      bool isMosaic();

      /*!
       * @brief Get the dimensions (shape) of the file.
       *
       * Z = The Z-dimension.
       * C = The C-dimension ("channel").
       * T = The T-dimension ("time").
       * R = The R-dimension ("rotation").
       * S = The S-dimension ("scene").
       * I = The I-dimension ("illumination").
       * H = The H-dimension ("phase").
       * V = The V-dimension ("view").
       * B = The B-dimension ("block") - its use is deprecated, but it still gets returned 🙃
       *
       * The internal structure suggests that a given dimension could start at a value other than zero.
       * Although I have yet to observe this we return a pair containing both the start and the size for the index.
       *
       * @return A map< DimensionIndex, pair<int(start), int(size)> >
       */
      Reader::MapDiP readDims();

      /*!
       * @brief Get the Dimensions in the order that they appear.
       * @return a string containing the dimensions
       */
      std::string dimsString();

      /*!
       * @brief Get the metadata from the CZI file.
       * @return A string containing the xml metadata from the file
       */
      std::string readMeta();

      /*!
       * @brief Given a CDimCoordinate, even an empty one, return the planes that match as a numpy ndarray
       *
       * The plane coordinate acts as a constraint, in the example below the dims are a set of constraints.
       * What this means is that every image taken that has Z=8, T=0, C=1 will be returned.
       * @code
       *    FILE *fp = std::fopen("mosaic_file.czi", "rb");
       *    czi = pylibczi::Reader(fp);
       *    auto dims = CDimCoordinate{ { DimensionIndex::Z,8 } , { DimensionIndex::T,0 }, { DimensionIndex::C,1 } };
       *    auto img_coords_dims = czi.cziread_selected(dims)
       * @endcode
       *
       * @param plane_coord_ A structure containing the Dimension constraints
       * @param flatten_ if false this won't flatten 3 channel images, from python this should always be true.
       * @param index_m_ Is only relevant for mosaic files, if you wish to select one frame.
       */
      std::pair<ImageVector, Shape> readSelected(libCZI::CDimCoordinate& plane_coord_, bool flatten_ = true, int index_m_ = -1);

      /*!
       * @brief provide the subblock metadata in index order consistent with readSelected.
       * @param plane_coord_ A structure containing the Dimension constraints
       * @param index_m_ Is only relevant for mosaic files, if you wish to select one frame.
       * @return a vector of metadata string blocks
       */
      SubblockMetaVec readSubblockMeta(libCZI::CDimCoordinate& plane_coord_, int index_m_ = -1);

      /*!
       * @brief If the czi file is a mosaic tiled image this function can be used to reconstruct it into an image.
       * @param plane_coord_ A class constraining the data to an individual plane.
       * @param scale_factor_ (optional) The native size for mosaic files can be huge the scale factor allows one to get something back of a more reasonable size. The default is 0.1 meaning 10% of the native image.
       * @param im_box_ (optional) The {x0, y0, width, height} of a sub-region, the default is the whole image.
       * @return an ImageVector of shared pointers containing an ImageBC*. See description of Image<> and ImageBC.
       *
       * @code
       *    FILE *fp = std::fopen("mosaicfile.czi", "rb");
       *    czi = pylibczi::Reader(fp);
       *    auto dims = czi.read_dims();  // dims = {{DimensionIndex::T, {0, 10}}, {DimensionIndex::C, {0,3}}
       *    // the dims above mean that T and C need to be constrained to single values
       *
       *    auto c_dims = CDimCoordinate{ { DimensionIndex::T,0 }, { DimensionIndex::C,1 } };
       *    auto img_p = czi.read_mosaic( c_dims, scaleFactor=0.15 );
       *    if(img_p->is_type_match<uint16_t>(){ // supported types are uint8_t, uint16_t, and float
       *        std::shared_ptr<uint16_t> img = img_p->get_derived<uint16_t>(); // cast the image to the memory size it was acquired as
       *        // do what you like with the Image<uint16_t>
       *    }
       * @endcode
       */
      ImageVector
      readMosaic(libCZI::CDimCoordinate plane_coord_, float scale_factor_ = 0.1, libCZI::IntRect im_box_ = {0, 0, -1, -1});
      // changed from {.w=-1, .h=-1} to above to support MSVC and GCC - lagging on C++14 std

      /*!
       * Convert the libCZI::DimensionIndex to a character
       * @param di_, The libCZI::DimensionIndex to be converted
       * @return The character representing the DimensionIndex
       */
      static char dimToChar(libCZI::DimensionIndex di_) { return libCZI::Utils::DimensionToChar(di_); }

      virtual ~Reader()
      {
          m_czireader->Close();
      }

      /*!
       * Get the full size of the mosaic image without scaling. If you're selecting a sub-region it must be within the box returned.
       * @return an IntRect {x, y, w, h}
       */
      libCZI::IntRect mosaicShape() { return m_statistics.boundingBoxLayer0Only; }

      static Shape getShape(pylibczi::ImageVector& images_, bool is_mosaic_) { return images_.getShape(); }

  private:

      Reader::SubblockIndexVec getMatches( SubblockSorter &match_ );

      static void addSortOrderIndex(vector<IndexMap>& vector_of_index_maps_);

      static bool isPyramid0(const libCZI::SubBlockInfo& info_)
      {
          return (info_.logicalRect.w==info_.physicalSize.w && info_.logicalRect.h==info_.physicalSize.h);
      }

      static bool isValidRegion(const libCZI::IntRect& in_box_, const libCZI::IntRect& czi_box_);
  };

}

#endif //_PYLIBCZI_READER_H
