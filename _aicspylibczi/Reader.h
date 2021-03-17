#ifndef _PYLIBCZI_READER_H
#define _PYLIBCZI_READER_H

#include <cstdio>
#include <functional>
#include <iostream>
#include <typeinfo>
#include <vector>

#include "inc_libCZI.h"

#include "DimIndex.h"
#include "Image.h"
#include "ImagesContainer.h"
#include "IndexMap.h"
#include "SubblockMetaVec.h"
#include "SubblockSortable.h"

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
class Reader
{

  std::shared_ptr<CCZIReader> m_czireader; // required for cast in libCZI
  libCZI::SubBlockStatistics m_statistics;
  libCZI::PixelType m_pixelType;
  bool m_specifyScene;

public:
  // was vector
  using TileBBoxMap = std::map<SubblockSortable, libCZI::IntRect>;
  using TilePair = std::pair<const SubblockSortable, libCZI::IntRect>;
  using SceneBBoxMap = std::map<unsigned int, libCZI::IntRect>;
  using ScenePair = std::pair<const unsigned int, libCZI::IntRect>;

  using SubblockIndexVec = std::set<std::pair<SubblockSortable, int>>;
  using DimIndexRangeMap = std::map<DimIndex, std::pair<int, int>>;
  using Shape = std::vector<std::pair<char, size_t>>;
  using DimsShape = std::vector<DimIndexRangeMap>;

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
  bool isMosaic() const;

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
   * @return A vector< map< DimensionIndex, pair<int(start), int(end)> > > which conforms to [start, end) meaning
   *    for( int i = start; i < end ; i++) is valid. If the shape of the scenes is consistent there will be one
   *    element in the vector. If the shape is not consistent each Scene will be present in the vector with it's
   *    explicit dimensions.
   */
  Reader::DimsShape readDimsRange();

  /*!
   * @brief Get the Dimensions in the order that they appear.
   * @return a string containing the dimensions. Y & X are included for completeness despite not being DimensionIndexes.
   */
  std::string dimsString();

  /*!
   * @brief Get the size of the dimensions in the same order as dimsString()
   * @return a vector containing the dimension sizes. Y & X are included for completeness.
   *    If the dims are not consistent a vector of -1 values is returned. The used should then use readDimsRange()
   */
  std::vector<int> dimSizes();

  /*!
   * @brief check if the dims are consistent across scenes
   * @param dShape_ A vector of scene specific dimensions
   * @return true if the shape is the same across scene false if it is not
   */
  static bool consistentShape(DimsShape& dShape_);

  /*!
   * @brief Enable the user to query the Scene shape directly
   * @return a tuple, the first index is if S is defined the second is the start index and the third is the number of S
   * indexs
   */
  std::tuple<bool, int, int> scenesStartSize() const;

  /*!
   * @brief get the shape of the specified Scene
   * @param scene_index_ the integer index of the Scene
   * @return A map containing the DimIndex with a pair of integers start, end such that for(int i = start; i < end ;
   * i++) is valid.
   */
  Reader::DimIndexRangeMap sceneShape(int scene_index_);

  /*!
   * @brief Get the metadata from the CZI file.
   * @return A string containing the xml metadata from the file. Y & X are included for completeness despite not being
   * DimensionIndexes.
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
   * @param index_m_ Is only relevant for mosaic files, if you wish to select one frame.
   */
  std::pair<ImagesContainerBase::ImagesContainerBasePtr, std::vector<std::pair<char, size_t>>>
  readSelected(libCZI::CDimCoordinate& plane_coord_, int index_m_ = -1, unsigned int cores_ = 3);

  /*!
   * @brief provide the subblock metadata in index order consistent with readSelected.
   * @param plane_coord_ A structure containing the Dimension constraints
   * @param index_m_ Is only relevant for mosaic files, if you wish to select one frame.
   * @param cores_ The number of cores to use to process threads
   * @return a vector of metadata string blocks
   */
  SubblockMetaVec readSubblockMeta(libCZI::CDimCoordinate& plane_coord_, int index_m_ = -1);

  /*!
   * @brief If the czi file is a mosaic tiled image this function can be used to reconstruct it into an image.
   * @param plane_coord_ A class constraining the data to an individual plane.
   * @param scale_factor_ (optional) The native size for mosaic files can be huge the scale factor allows one to get
   * something back of a more reasonable size. The default is 0.1 meaning 10% of the native image.
   * @param im_box_ (optional) The {x0, y0, width, height} of a sub-region, the default is the whole image.
   * @return an ImagesContainerBasePtr containing the raw memory, a list of images, and a list of corresponding
   * dimensions
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
   *        std::shared_ptr<uint16_t> img = img_p->get_derived<uint16_t>(); // cast the image to the memory size it was
   * acquired as
   *        // do what you like with the Image<uint16_t>
   *    }
   * @endcode
   */
  ImagesContainerBase::ImagesContainerBasePtr readMosaic(libCZI::CDimCoordinate plane_coord_,
                                                         float scale_factor_ = 1.0,
                                                         libCZI::IntRect im_box_ = { 0, 0, -1, -1 });

  /*!
   * Convert the libCZI::DimensionIndex to a character
   * @param di_, The libCZI::DimensionIndex to be converted
   * @return The character representing the DimensionIndex
   */
  static char dimToChar(libCZI::DimensionIndex di_) { return libCZI::Utils::DimensionToChar(di_); }

  bool shapeIsConsistent() const { return !m_specifyScene; }

  virtual ~Reader() { m_czireader->Close(); }


  /*!
   * @brief get the shape of the loaded images
   * @param images_ the ImageVector of images to get the shape of
   * @param is_mosaic_ a boolean telling the function if it's a mosaic file
   * @return a vector of (Dimension letter, Dimension size)
   */
  static Shape getShape(pylibczi::ImageVector& images_, bool is_mosaic_) { return images_.getShape(); }

  /*!
   * @brief get the bounding box of the
   * @return
   */
  TilePair tileBoundingBox(libCZI::CDimCoordinate& plane_coord_);

  TileBBoxMap tileBoundingBoxes(libCZI::CDimCoordinate& plane_coord_);

  /*!
   * @brief get the scene bounding box
   *
   */
  libCZI::IntRect sceneBoundingBox(unsigned int scene_index_ = 0);

  SceneBBoxMap allSceneBoundingBoxes();

  libCZI::IntRect mosaicBoundingBox() const;

  TilePair mosaicTileBoundingBox(libCZI::CDimCoordinate& plane_coord_, int index_m_);

  TileBBoxMap mosaicTileBoundingBoxes(libCZI::CDimCoordinate& plane_coord_);

  libCZI::IntRect mosaicSceneBoundingBox(unsigned int scene_index_);

  SceneBBoxMap allMosaicSceneBoundingBoxes();

  std::string pixelType()
  {
    // each subblock can apparently have a different pixelType 🙄
    if (m_pixelType == libCZI::PixelType::Invalid)
      m_pixelType = getFirstPixelType();
    return libCZI::Utils::PixelTypeToInformalString(m_pixelType);
  }

private:
  Reader::SubblockIndexVec getMatches(SubblockSortable& match_);

  static bool isPyramid0(const libCZI::SubBlockInfo& info_)
  {
    return (info_.logicalRect.w == info_.physicalSize.w && info_.logicalRect.h == info_.physicalSize.h);
  }

  static bool isValidRegion(const libCZI::IntRect& in_box_, const libCZI::IntRect& czi_box_);

  TileBBoxMap tileBoundingBoxesWith(SubblockSortable& subblocksToFind_);

  void checkSceneShapes();

  libCZI::PixelType getFirstPixelType();

  /*!
   * @brief get the pyramid 0 (acquired data) shape
   * @param scene_index_ specifies scene but defaults to the first scene,
   * Scenes can have different sizes
   * @param get_all_matches_ if true return all matching bounding boxes
   * @return std::vector<libCZI::IntRect> containing (x0, y0, w, h)
   */
  std::vector<libCZI::IntRect> getAllSceneYXSize(int scene_index_ = -1, bool get_all_matches_ = false);

  /*
   * This is a remnant of 2.x I would like to factor it out but will do so at a later point.
   * It should be replaceable with the tile function but may require some munging.
   */
  libCZI::IntRect getSceneYXSize(int scene_index_ = -1)
  {
    std::vector<libCZI::IntRect> matches = getAllSceneYXSize(scene_index_);
    return matches.front();
  }
};

}

#endif //_PYLIBCZI_READER_H
