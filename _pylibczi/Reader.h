//
// Created by Jamie Sherman on 7/11/19.
//

#ifndef PYLIBCZI_AICS_ADDED_HPP
#define PYLIBCZI_AICS_ADDED_HPP

#include <cstdio>
#include <iostream>
#include <typeinfo>
#include <functional>
#include <vector>

#include "inc_libCZI.h"
#include "IndexMap.h"
#include "Image.h"

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

  class FileHolder {
      FILE* m_fp;
  public:
      FileHolder() { m_fp = nullptr; }
      FileHolder(FILE* p) { m_fp = p; }
      FILE* get() { return m_fp; }
      FileHolder& operator=(FILE* p)
      {
          m_fp = p;
          return *this;
      }
  };

/// <summary>	A wrapper that takes a FILE * and creates an libCZI::IStream object out of it

  class CSimpleStreamImplFromFP: public libCZI::IStream {
  private:
      FILE* fp;
  public:
      CSimpleStreamImplFromFP() = delete;
      CSimpleStreamImplFromFP(FILE* file_pointer) { fp = file_pointer; }
      ~CSimpleStreamImplFromFP() override { fclose(fp); };
      void Read(std::uint64_t offset, void* pv, std::uint64_t size, std::uint64_t* ptrBytesRead) override;
  };

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
  public:
      typedef std::map<libCZI::DimensionIndex, std::pair<int, int> > mapDiP;
      using Shape = std::vector<std::pair<char, int> >;
      /*!
       * @brief Construct the Reader and load the file statistics (dimensions etc)
       *
       * Example:
       * @code
       *    fp = std::fopen("myczifile.czi", "rb");      // #include <cstdio>
       *    czi = pylibczi::Reader(fp);
       * @endcode
       *
       * @param f_in A C-style FILE pointer to the CZI file
       */
      explicit Reader(FileHolder f_in);

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
      Reader::mapDiP read_dims();

      /*!
       * @brief Get the metadata from the CZI file.
       * @return A string containing the xml metadata from the file
       */
      std::string read_meta();

      /*!
       * @brief Given a CDimCoordinate, even an empty one, return the planes that match as a numpy ndarray
       *
       * The plane coordinate acts as a constraint, in the example below the dims are a set of constraints.
       * What this means is that every image taken that has Z=8, T=0, C=1 will be returned.
       * @code
       *    FILE *fp = std::fopen("mosaicfile.czi", "rb");
       *    czi = pylibczi::Reader(fp);
       *    auto dims = CDimCoordinate{ { DimensionIndex::Z,8 } , { DimensionIndex::T,0 }, { DimensionIndex::C,1 } };
       *    auto img_coords_dims = czi.cziread_selected(dims)
       * @endcode
       *
       * @param planeCoord A structure containing the Dimension constraints
       * @param flatten if false this won't flatten 3 channel images, from python this should always be true.
       * @param mIndex Is only relevant for mosaic files, if you wish to select one frame.
       */
      std::pair<ImageVector, Shape> read_selected(libCZI::CDimCoordinate& planeCoord, bool flatten = true, int mIndex = -1);

      /*!
       * @brief If the czi file is a mosaic tiled image this function can be used to reconstruct it into an image.
       * @param planeCoord A class constraining the data to an individual plane.
       * @param scaleFactor (optional) The native size for mosaic files can be huge the scale factor allows one to get something back of a more reasonable size. The default is 0.1 meaning 10% of the native image.
       * @param imBox (optional) The {x0, y0, width, height} of a sub-region, the default is the whole image.
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
      read_mosaic(const libCZI::CDimCoordinate planeCoord, float scaleFactor = 0.1, libCZI::IntRect imBox = {0, 0, -1, -1});
        // changed from {.w=-1, .h=-1} to above to support MSVC and GCC - lagging on C++14 std

      /*!
       * Convert the libCZI::DimensionIndex to a character
       * @param di, The libCZI::DimensionIndex to be converted
       * @return The character representing the DimensionIndex
       */
      char dim_to_char(libCZI::DimensionIndex di) { return libCZI::Utils::DimensionToChar(di); }

      virtual ~Reader()
      {
          m_czireader->Close();
      }

      /*!
       * Get the full size of the mosaic image without scaling. If you're selecting a subregion it must be within the box returned.
       * @return an IntRect {x, y, w, h}
       */
      libCZI::IntRect mosaic_shape() { return m_statistics.boundingBoxLayer0Only; }

      static Shape get_shape(pylibczi::ImageVector& imgs, bool is_mosaic);

  private:

      static bool dimsMatch(const libCZI::CDimCoordinate& targetDims, const libCZI::CDimCoordinate& cziDims);

      static void add_sort_order_index(vector<IndexMap>& vec);

      static bool isPyramid0(const libCZI::SubBlockInfo& info)
      {
          return (info.logicalRect.w==info.physicalSize.w && info.logicalRect.h==info.physicalSize.h);
      }

      static bool isValidRegion(const libCZI::IntRect& inBox, const libCZI::IntRect& cziBox);
  };

}

#endif //PYLIBCZI_AICS_ADDED_HPP
