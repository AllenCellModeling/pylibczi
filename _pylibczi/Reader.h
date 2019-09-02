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
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include "inc_libCZI.h"
#include "Python.h"
#include "IndexMap.h"
#include "Image.h"

using namespace std;

namespace py = pybind11;

namespace pylibczi {

/// <summary>	A wrapper that takes a FILE * and creates an libCZI::IStream object out of it

    class CSimpleStreamImplFromFP : public libCZI::IStream {
    private:
      FILE *fp;
    public:
      CSimpleStreamImplFromFP() = delete;
      explicit CSimpleStreamImplFromFP(FILE *file_pointer) : fp(file_pointer) {}
      ~CSimpleStreamImplFromFP() override { fclose(this->fp); };
      void Read(std::uint64_t offset, void *pv, std::uint64_t size, std::uint64_t *ptrBytesRead) override;
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
      unique_ptr<CCZIReader> m_czireader;
      libCZI::SubBlockStatistics m_statistics;
    public:
      typedef std::map<libCZI::DimensionIndex, std::pair<int, int> > mapDiP;
      typedef std::tuple<py::list, py::array_t<int32_t>, std::vector<IndexMap> > tuple_ans;

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
      explicit Reader(FILE *f_in);

      /*!
       * @brief Check if the file is a mosaic file.
       *
       * This test is done by checking the maximum mIndex, mosaic files will have mIndex > 0
       *
       * @return True if the file is a mosaic file, false if it is not.
       */
      bool isMosaicFile();

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
       *    auto dims = CDimCoordinate{ { DimensionIndex::Z,8 } , { DimensionIndex::T,0 }, { DimensionIndex::C,1 } };
       *    auto img_coords_dims = czi.cziread_selected(dims)
       * @endcode
       *
       * @param planeCoord A structure containing the Dimension constraints
       * @param mIndex Is only relevant for mosaic files, if you wish to select one frame.
       */
      tuple_ans read_selected(libCZI::CDimCoordinate &planeCoord, int mIndex = -1);

      std::shared_ptr<ImageBC>
      read_mosaic(libCZI::IntRect imBox, const libCZI::CDimCoordinate &planeCoord, float scaleFactor);


      virtual ~Reader(){
          m_czireader->Close();
      }

    private:

      static bool dimsMatch(const libCZI::CDimCoordinate &targetDims, const libCZI::CDimCoordinate &cziDims);

      static void add_sort_order_index(vector<IndexMap> &vec);

      static bool isPyramid0(const libCZI::SubBlockInfo &info) {
          return (info.logicalRect.w == info.physicalSize.w && info.logicalRect.h == info.physicalSize.h);
      }

      static bool isValidRegion(const libCZI::IntRect &inBox, const libCZI::IntRect &cziBox);
    };

}

#endif //PYLIBCZI_AICS_ADDED_HPP
